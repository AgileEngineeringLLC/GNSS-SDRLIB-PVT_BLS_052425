//-----------------------------------------------------------------------------
//
// sdrpvt.c.c
//
// Function for computing PVT from a set of observations
// Copyright 2025, Don Kelly, don.kelly@mac.com
//-----------------------------------------------------------------------------

// Include libraries
#include "nml.h"
#include "sdr.h"

//-----------------------------------------------------------------------------
// PVT functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function that drives PVT calculations
//-----------------------------------------------------------------------------
extern int pvtProcessor(void)
{
  mlock(hobsvecmtx);
  int numSat = sdrstat.nsatValid;
  unmlock(hobsvecmtx);

  double *pr_v = (double *)malloc(numSat * sizeof(double));
  double *Xs_v = (double *)malloc(numSat * 3 * sizeof(double));
  //double *G_v = (double *)malloc(numSat * 3 * sizeof(double));

  // Create dynamic variables to store obs input data
  //int *satList_v = (int *)malloc(numSat * sizeof(int));
  double *rcvr_tow_v = (double *)malloc(numSat * sizeof(double));
  double *prRaw_v = (double *)malloc(numSat * sizeof(double));
  double *prSvClkCorr_v = (double *)malloc(numSat * sizeof(double));
  double *snr_v = (double *)malloc(numSat * sizeof(double));

  double tau;
  double lambda;
  double phi;
  double height;
  double rcvr_tow;
  double lat, lon;

  // Initialize obs input data
  mlock(hobsvecmtx);
  for (int i=0; i<numSat; i++) {
    int prn = sdrstat.obsValidList[i];
    prRaw_v[i]         = sdrstat.obs_v[(prn-1)*11+5];
    rcvr_tow_v[i]      = sdrstat.obs_v[(prn-1)*11+6];
    snr_v[i]           = sdrstat.obs_v[(prn-1)*11+8];
  }
  unmlock(hobsvecmtx);

  // Set rcvr time
  rcvr_tow = rcvr_tow_v[0];

  // Initialize parameters
  double xyzdt_v[] = {0,0,0,0};
  double xs_v[3];
  double svClkCorr;
  double transmitTime;
  double gdop = 0.0;
  int ret = 0;

  //---------------------------------------------------------------------------
  // Calculate SV positions
  //---------------------------------------------------------------------------
  for (int i=0; i<numSat; i++) {
    // Correct PR for user clock bias

    mlock(hobsvecmtx);
    pr_v[i] = prRaw_v[i] - sdrstat.xyzdt[3];
    unmlock(hobsvecmtx);

    // Calculate transit time
    tau = pr_v[i] / CTIME;

    // Update transmit time and get satellite position
    transmitTime = rcvr_tow - tau;
    mlock(hobsvecmtx);
    ret = satPos(&sdrch[sdrstat.obsValidList[i]-1].nav.sdreph, transmitTime, xs_v,
             &svClkCorr);
    unmlock(hobsvecmtx);
    if (ret != 0) {
      printf("Function satPos has xs NaN for G%02d, exiting pvtProcessor\n",
             sdrstat.obsValidList[i]);
      goto errorDetected;
    }

    // If xs calculated as NaN, exit pvtProcessor
    if (isnan(xs_v[0]) || isnan(xs_v[0]) || isnan(xs_v[0])) {
      //printf("Function satPos has xs NaN for i=%02d, exiting pvtProcessor\n",
      //       i);
      goto errorDetected;
    }

    // Correct raw PR for satellite clock bias
    prSvClkCorr_v[i] = prRaw_v[i] + (CTIME * svClkCorr);

    // Build Xs_v (vector of sat positions) from each individual sat
    // posn xs_v
    Xs_v[(i*3)+0] = xs_v[0];
    Xs_v[(i*3)+1] = xs_v[1];
    Xs_v[(i*3)+2] = xs_v[2];

    // Load SV positions into obs_v
    mlock(hobsvecmtx);
    int prn = sdrstat.obsValidList[i];
    sdrstat.obs_v[(prn-1)*11+2] = xs_v[0];
    sdrstat.obs_v[(prn-1)*11+3] = xs_v[1];
    sdrstat.obs_v[(prn-1)*11+4] = xs_v[2];
    unmlock(hobsvecmtx);

  } // end for numSat

  // Continue if we still have at least 4 SVs
  if (sdrstat.nsatValid<4) {
    goto errorDetected;
  }

  // Run least squares to calculate new receiver position and receiver
  // clock bias
  if (sdrini.ekfFilterOn==0) {
    ret = blsFilter(Xs_v, prSvClkCorr_v, numSat, xyzdt_v, &gdop);
  } else {
    //ret = ekfFilter(Xs_v, prSvClkCorr_v, numSat, xyzdt_v, &gdop);
  }

  // If x calculated as NaN, exit pvtProcessor
  if (isnan(xyzdt_v[0]) || isnan(xyzdt_v[1]) || isnan(xyzdt_v[2])) {
    printf("Function estRcvrPosn gets NaN for xu, exiting pvtProcessor\n");
    goto errorDetected;
  }

  // Convert ECEF to LLA
  ecef2lla(xyzdt_v[0], xyzdt_v[1], xyzdt_v[2], &lambda, &phi, &height);

  // Convert radians to degrees
  lat = phi * 180.0/M_PI;
  lon = lambda * 180.0/M_PI;

  // Save LLA to sdrstat struct
  mlock(hobsvecmtx);
  sdrstat.lat = lat;
  sdrstat.lon = lon;
  sdrstat.hgt = height;
  sdrstat.gdop = gdop;
  sdrstat.xyzdt[0] = xyzdt_v[0];
  sdrstat.xyzdt[1] = xyzdt_v[1];
  sdrstat.xyzdt[2] = xyzdt_v[2];
  sdrstat.xyzdt[3] = xyzdt_v[3];
  unmlock(hobsvecmtx);

  // Free memory
  free(Xs_v);
  free(pr_v);

  free(prRaw_v);
  free(prSvClkCorr_v);
  free(snr_v);
  free(rcvr_tow_v);

  return 0;

errorDetected:
  // Save LLA to sdrstat struct
  mlock(hobsvecmtx);
  sdrstat.lat = 0.0;
  sdrstat.lon = 0.0;
  sdrstat.hgt = 0.0;
  sdrstat.gdop = 0.0;
  unmlock(hobsvecmtx);

  // Free memory
  free(Xs_v);
  free(pr_v);

  free(prRaw_v);
  free(prSvClkCorr_v);
  free(snr_v);
  free(rcvr_tow_v);

  //sdrstat.pvtflag = 0; // Not good solution
  return -1;
} // end function

//-----------------------------------------------------------------------------
// Estimate receiver position function with BLS filter
//-----------------------------------------------------------------------------
extern int blsFilter(double *X_v, double *pr_v, int numSat,
                 double xyzdt_v[], double *gdop)
{
  // Set up numSat
  numSat = sdrstat.nsatValid;

  // Declare matrix types
  nml_mat *x, *A, *W, *pos, *omc;
  nml_mat *At, *AtA, *AtW, *AtWA, *iAtA, *iAtWA, *iAtWAAt, *iAtWAAtW, *tempPos;

  // Set up empty matrices
  x = nml_mat_new(4,1);
  pos = nml_mat_new(4,1);
  //A = nml_mat_new(numSat,4);
  W = nml_mat_new(numSat,numSat);
  omc = nml_mat_new(numSat,1);
  At = nml_mat_new(4,numSat);
  AtA = nml_mat_new(4,4);
  AtW = nml_mat_new(4,numSat);
  AtWA = nml_mat_new(4,4);
  iAtA = nml_mat_new(4,4);
  iAtWA = nml_mat_new(4,4);
  iAtWAAt = nml_mat_new(4,numSat);
  iAtWAAtW = nml_mat_new(4,numSat);
  tempPos = nml_mat_new(4,1);

  // Dynamic and static variables
  double Rot_X_v[] = {0,0,0};
  double pos_v[] = {0,0,0,0};
  double rho2 = 0.0;
  double travelTime = 0.0;
  double omegatau = 0.0;
  double rhoSq = 0.0;
  double *A_v = (double *)calloc(numSat * 4, sizeof(double));
  double *W_v = (double *)calloc(numSat * numSat, sizeof(double));
  double det = 0.0;
  double detTol = 1e-12;
  double trop = 0.0;
  double *omc_v = (double *)calloc(numSat, sizeof(double));
  int ret = 0;
  double az, el;
  double D = 0.0;
  double X[] = {0,0,0};
  double dx[] = {0,0,0};
  double normX = 100.0;
  int iter = 0;

  // Try initializing pos_v
  pos_v[0] = (double)sdrini.xu0_v[0];
  pos_v[1] = (double)sdrini.xu0_v[1];
  pos_v[2] = (double)sdrini.xu0_v[2];
  //pos_v[0] = sdrstat.xyzdt[0];
  //pos_v[1] = sdrstat.xyzdt[1];
  //pos_v[2] = sdrstat.xyzdt[2];
  //pos_v[3] = sdrstat.xyzdt[3]; // leave out?

  // Set up the weighting vector diagonals
  mlock(hobsvecmtx);
  sdrekf.varR = 5.0 * 5.0;       // Default is 30^2
  for (int i=0; i<32; i++) {
    sdrekf.rk1_v[i] = 0.0;
  }
  for (int k=0; k<numSat; k++) {
    // Find prn value
    int prn = sdrstat.obsValidList[k];
    sdrekf.rk1_v[prn-1] = sdrekf.varR;

    double el2 = sdrstat.obs_v[(prn-1)*11+10];
    if ((sdrstat.azElCalculatedflag) && (el2 < 30)) {
      sdrekf.rk1_v[prn-1] = sdrekf.varR +
        (25 - (25/15)*(el2-15.0)) * (25 - (25/15)*(el2-15.0));
    }

    // Load default varR to diagonals
    W_v[k + (k*numSat)] = 1.0 / sdrekf.rk1_v[prn-1];
  }
  unmlock(hobsvecmtx);

  // Define number of iterations for BLS
  int nmbOfIterations = 10;

  // Loop through BLS equation 10 times
  // Note: Should revise to exit on tolerance (norm of x small)
  for (int j=0; j<nmbOfIterations; j++) {
  //while (normX > 1.0e-10) {

    // Break out of BLS loop if desired accuracy is achieved
    if (normX < 1.0e-10) {
      //printf("breaking from BLS loop early, iteration %d\n",j);
      break;
    }

    // Loop through obs and rotate xs, solve for el angle, and solve for tropo
    // correction.
    for (int i=0; i<numSat; i++) {
      // For first iteration, set Rot_X and trop
      if (iter==0) {
        for (int k=0; k<3; k++) {
          Rot_X_v[k] = X_v[i*3+k];
        }
        trop = 2.0;
      } else {

      // Solve for estimated range squared
      rho2 = ( ( (X_v[i*3+0] - pos_v[0]) * (X_v[i*3+0] - pos_v[0]) ) +
               ( (X_v[i*3+1] - pos_v[1]) * (X_v[i*3+1] - pos_v[1]) ) +
               ( (X_v[i*3+2] - pos_v[2]) * (X_v[i*3+2] - pos_v[2]) ) );

      // Solve for travelTime
      travelTime = sqrt(rho2) / CTIME;

      // Rotate SV position to account for Earth rotation during
      // travel time
      omegatau =  OMEGAEDOT * travelTime;
      Rot_X_v[0] =  (cos(omegatau) * X_v[i*3+0])  + (sin(omegatau) * X_v[i*3+1]);
      Rot_X_v[1] = (-sin(omegatau) * X_v[i*3+0]) + (cos(omegatau) * X_v[i*3+1]);
      Rot_X_v[2] = X_v[i*3+2];

      // Update SV el angles and write values in sdrch
      for (int i=0; i<3; i++) {
        X[i] = pos_v[i];
        dx[i] = Rot_X_v[i] - pos_v[i];
      }
      ret = topocent(X, dx, &az, &el, &D);
      if (ret!=0) {
        printf("topocent function: error\n");
      }
      sdrstat.azElCalculatedflag = 1;  // Set azEl flag

      // Load SV positions and angles for current SV into obs
      mlock(hobsvecmtx);
      int prn = sdrstat.obsValidList[i];
      sdrstat.obs_v[(prn-1)*11+2] = Rot_X_v[0];
      sdrstat.obs_v[(prn-1)*11+3] = Rot_X_v[1];
      sdrstat.obs_v[(prn-1)*11+4] = Rot_X_v[2];
      sdrstat.obs_v[(prn-1)*11+9] = az;
      sdrstat.obs_v[(prn-1)*11+10] = el;
      unmlock(hobsvecmtx);

      // Calculate tropo correction (do later)
      ret = tropo(sin(el*D2R), 0.0, 1013.0, 293.0, 50.0,
                               0.0, 0.0, 0.0, &trop);
      if (ret!=0) {
        printf("tropo function: error\n");
      }

      } // end of if else

      // Apply the corrections to the observations to get estimated
      // range squared
      rhoSq = ( ( (Rot_X_v[0] - pos_v[0]) * (Rot_X_v[0] - pos_v[0]) ) +
                ( (Rot_X_v[1] - pos_v[1]) * (Rot_X_v[1] - pos_v[1]) ) +
                ( (Rot_X_v[2] - pos_v[2]) * (Rot_X_v[2] - pos_v[2]) ) );

      // Calculate the measurement residuals, PR observed minus PR calc
      omc_v[i] = pr_v[i] - sqrt(rhoSq) - pos_v[3] - trop;

      // Construct the A matrix
      A_v[i*4+0] = (-(Rot_X_v[0] - pos_v[0])) / sqrt(rhoSq);
      A_v[i*4+1] = (-(Rot_X_v[1] - pos_v[1])) / sqrt(rhoSq);
      A_v[i*4+2] = (-(Rot_X_v[2] - pos_v[2])) / sqrt(rhoSq);
      A_v[i*4+3] = 1.0;

    } // end of numSat loop

    // Exit if bad rank

    // Perform BLS to solve for error states
    //   dx = inv(A' * A) * A' * dz

    // Form the W and A matrices
    W = nml_mat_from(numSat,numSat,numSat*numSat,W_v);
    A = nml_mat_from(numSat,4,numSat*4,A_v); // A: nx4
    At = nml_mat_transp(A);            // A': 4xn

    // Solve for AtA and AtWA
    AtA = nml_mat_dot(At,A);     // A'A: 4x4
    AtW = nml_mat_dot(At,W);     // A'W: 4xn
    AtWA = nml_mat_dot(AtW,A);   // A'WA: 4x4

    // Calculate the LUP (needed for det and inverse calculations)
    nml_mat_lup *lupAtA;
    lupAtA = nml_mat_lup_solve(AtA);         // lup(A'A): 4x4
    nml_mat_lup *lupAtWA;
    lupAtWA = nml_mat_lup_solve(AtWA);       // lup(A'WA): 4x4

    // Check for viable inverse (if det equals zero, rank too low)
    det = nml_mat_det(lupAtA);
    if (fabs(det)<detTol) {
      printf("Exiting estRcvrPosn, determinant=%lf\n", det);
      goto errorDetected;
    }

    // Calculate the inverse of AtA
    iAtA = nml_mat_inv(lupAtA);               // inv(A'A): 4x4
    nml_mat_lup_free(lupAtA);
    iAtWA = nml_mat_inv(lupAtWA);             // inv(A'WA): 4x4
    nml_mat_lup_free(lupAtWA);

    // Solve for x
    iAtWAAt = nml_mat_dot(iAtWA,At);      // inv(A'WA)A': 4xn
    iAtWAAtW = nml_mat_dot(iAtWAAt,W);    // inv(A'WA)A'W: 4xn
    omc = nml_mat_from(numSat,1,numSat,omc_v); // omc: nx1
    x   = nml_mat_dot(iAtWAAtW,omc);      // inv(A'WA)A'Wdz: 4x1

    // Apply error states to whole states
    tempPos = nml_mat_from(4,1,4,pos_v);             // tempPos: 4x1
    pos = nml_mat_add(tempPos,x);                   // pos: 4x1

    // Reset pos_v for next iteration
    pos_v[0] = pos->data[0][0];
    pos_v[1] = pos->data[1][0];
    pos_v[2] = pos->data[2][0];
    pos_v[3] = pos->data[3][0];

    // Calculate norm of X
    normX = sqrt( (x->data[0][0]*x->data[0][0]) +
                  (x->data[1][1]*x->data[1][1]) +
                  (x->data[2][2]*x->data[2][2]) +
                  (x->data[3][3]*x->data[3][3]) );

    // Increment iter counter
    iter = iter + 1;

  } // end the estimator loop

  // Save pos values to xyzdt_v
  xyzdt_v[0] = pos->data[0][0];
  xyzdt_v[1] = pos->data[1][0];
  xyzdt_v[2] = pos->data[2][0];
  xyzdt_v[3] = pos->data[3][0];

  // Calculate DOP values (DOP is sqrt of the trace)
  *gdop =  sqrt( iAtA->data[0][0] + iAtA->data[1][1] +
                 iAtA->data[2][2] + iAtA->data[3][3] );

  // Write residuals to sdrstat so GUI can display
  mlock(hobsvecmtx);
  for (int i=0; i<32; i++) {
    sdrstat.vk1_v[i] = 0;
  }
  for (int j=0; j<numSat; j++) {
      int prn = sdrstat.obsValidList[j];
      sdrstat.vk1_v[prn-1] = omc_v[j];
  }
  unmlock(hobsvecmtx);

  // Free matrices
  nml_mat_free(x);
  nml_mat_free(A);
  nml_mat_free(W);
  nml_mat_free(pos);
  nml_mat_free(omc);
  nml_mat_free(At);
  nml_mat_free(AtA);
  nml_mat_free(AtW);
  nml_mat_free(AtWA);
  nml_mat_free(iAtA);
  nml_mat_free(iAtWA);
  nml_mat_free(iAtWAAt);
  nml_mat_free(iAtWAAtW);
  nml_mat_free(tempPos);

  // Free vectors (arrays)
  free(A_v);
  free(W_v);
  free(omc_v);

  // Normal return with good solution
  return 0;

errorDetected:
  // Save output values as zero
  xyzdt_v[0] = 0.0;
  xyzdt_v[1] = 0.0;
  xyzdt_v[2] = 0.0;
  xyzdt_v[3] = 0.0;
  *gdop = 0.0;

  // Free matrices
  nml_mat_free(x);
  nml_mat_free(A);
  nml_mat_free(W);
  nml_mat_free(pos);
  nml_mat_free(omc);
  nml_mat_free(At);
  nml_mat_free(AtA);
  nml_mat_free(AtW);
  nml_mat_free(AtWA);
  nml_mat_free(iAtA);
  nml_mat_free(iAtWA);
  nml_mat_free(iAtWAAt);
  nml_mat_free(iAtWAAtW);
  nml_mat_free(tempPos);

  // Free vectors (arrays)
  free(A_v);
  free(W_v);
  free(omc_v);

  // Error return without good solution
  return -1;

} // end function

//-----------------------------------------------------------------------------
// Function to account for week crossover
//-----------------------------------------------------------------------------
extern void check_t(double time, double *corrTime) {

 // Initial guess for latitude
 double halfweek = 302400.0;

 // Initialize corrTime
 *corrTime = time;

 // Iterative process to find the geodetic latitude
 if (time > halfweek) {
  *corrTime = time - (2 * halfweek);
 }  else if (time < -halfweek) {
  *corrTime = time + (2 * halfweek);
 }
} // end function

//-----------------------------------------------------------------------------
// Function to convert ECEF to geodetic coordinates
//-----------------------------------------------------------------------------
extern void ecef2lla(double x, double y, double z,
                    double *lambda, double *phi, double *height)
{
 // WGS-84 ellipsoid parameters
 double A = 6378137.0;  // Semi-major axis (meters)
 double F = 1.0 / 298.257223563;  // Flattening
 double E = sqrt( (2*F) - (F*F) );

 // Initial guess for latitude
 *lambda = atan2(y,x);
 double p = sqrt(x * x + y * y);

 // Initial value of phi assuming h = 0
 *height = 0.0;
 *phi = atan2(z, p*(1.0 - E*E));
 double N = A / sqrt( (1.0 - sin(*phi)) * (1.0 - sin(*phi)) );
 double delta_h = 1000000;

 // Iterative process to find the geodetic latitude
 while (delta_h > 0.01) {
   double prev_h = *height;
   *phi = atan2(z, p*(1-(E*E) * (N / (N + *height) ) ) );
   N = A / sqrt( (1 - ( (E * sin(*phi)) * (E * sin(*phi)) ) ) );
   *height = (p / cos(*phi)) - N;
   delta_h = fabs(*height - prev_h);
 }
} // end function

//-----------------------------------------------------------------------------
// Function to calculate satellite position
//-----------------------------------------------------------------------------
extern int satPos(sdreph_t *sdreph, double transmitTime, double svPos[3],
            double *svClkCorr)
{
  // Sample ephemeris parameters (in order of 1 to 21). These values are
  // from the Thompson paper "Computing GPS Velocity and Acceleration from
  // the Broadcast Navigation Message."
  //int sat                         = sdreph->eph.sat; // not used
  double toe                      = sdreph->eph.toes;
  double toc						= toe;
  //double sqrta                    = sdreph->eph.A;
  double sqrta                    = sqrt(sdreph->eph.A);
  double e                        = sdreph->eph.e;
  double M0                       = sdreph->eph.M0;
  double omega                    = sdreph->eph.omg;
  double i0                       = sdreph->eph.i0;
  double Omega0                   = sdreph->eph.OMG0;
  double deltan                   = sdreph->eph.deln;
  double idot                     = sdreph->eph.idot;
  double Omegadot                 = sdreph->eph.OMGd;
  double cuc                      = sdreph->eph.cuc;
  double cus                      = sdreph->eph.cus;
  double crc                      = sdreph->eph.crc;
  double crs                      = sdreph->eph.crs;
  double cic                      = sdreph->eph.cic;
  double cis                      = sdreph->eph.cis;
  double af0                      = sdreph->eph.f0;
  double af1                      = sdreph->eph.f1;
  double af2                      = sdreph->eph.f2;
  double tgd                      = sdreph->eph.tgd[0];

  /* // Test code
  printf("sat: %d\n", sat);
  printf("transmitTime: %e, toe: %e, toc: %e\n", transmitTime, toe, toc);
  printf("sqrtA: %e, e: %e, m0: %e, omega: %e\n", sqrta, e, M0, omega);
  printf("i0: %e, Omega0: %e, deltan: %e, idot: %e\n", i0, Omega0, deltan, idot);
  printf("Omegadot: %e, cuc: %e, cus: %e, crc: %e\n", Omegadot, cuc, cus, crc);
  printf("crs: %e, cic: %e, cis: %e, af0: %e\n", crs, cic, cis, af0);
  printf("af1: %e, af2: %e, tgd: %e\n", af1,af2,tgd);
  //*/

  // Local parameters
  double t = transmitTime; // rcvr_tow
  double A;
  double n0, n;
  double tk, tc;
  double Mk, vk, ik, phik, u_k, rk, Omega_k;
  double delta_uk, delta_rk, delta_ik;
  double x_kp, y_kp;
  double xk, yk, zk;
  double E0, Ek, dtr, dt;
  int ii;

  // Initialize svPos to zero
  svPos[0] = 0.0; svPos[1] = 0.0; svPos[2] = 0.0;

  // Velocity terms (if velocity calculated)
  //long double ekdot, vkdot, ukdot, ikdot, rkdot, omegakdot;
  //long double xpkdot, ypkdot;
  //long double xkdot, ykdot, zkdot;

  // Semi-major axis
  A = sqrta * sqrta;           //sqrta is the square root of A

  // Computed mean motion
  n0 = sqrt( MU / (A*A*A) );

  // Time from ephemeris, reference clock
  tk = t - toe;              // t is the time of the pos. & vel. request.
  if (tk > 302400) {
    tk = tk - (2 * 302400);
  }
  else if (tk < -302400) {
  	tk = tk + (604800);
  }

  // Corrected mean motion
  n = n0 + deltan;

  // Mean anomaly at t
  Mk = M0 + n*tk;

  // Kepler equation for eccentric anomaly
  E0 = Mk;

  for (ii=0; ii<3; ii++){
    Ek = E0 + (Mk - E0 + e * sin(E0)) / (1 - e * cos(E0));
    E0 = Ek;
  }

  //In the line, below, tak is the true anomaly (which is nu in the ICD-200).
  vk = 2 * atan( sqrt( (1.0 + e) / (1.0 - e) ) * tan(Ek / 2) );

  // Argument of latitude
  phik = omega + vk;

  // Second harmonic perturbations
  delta_uk = cus * sin(2.0 * phik) + cuc * cos(2.0 * phik);
  delta_rk = crs * sin(2.0 * phik) + crc * cos(2.0 * phik);
  delta_ik = cis * sin(2.0 * phik) + cic * cos(2.0 * phik);

  // Corrected argument of latitude
  u_k = phik + delta_uk;

  // Corrected radius
  rk = A * (1.0 - (e * cos(Ek))) + delta_rk;

  // Corrected inclination
  ik = i0 + (idot * tk) + delta_ik;

  // Cos and sin values for rk
  x_kp = rk * cos(u_k);
  y_kp = rk * sin(u_k);

  // Longitude of ascending node
  Omega_k = Omega0 + (Omegadot - OMEGAEDOT) * tk - (OMEGAEDOT * toe);

  // SV position in ECEF
  xk = (x_kp * cos(Omega_k)) - (y_kp * sin(Omega_k) * cos(ik));
  yk = (x_kp * sin(Omega_k)) + (y_kp * cos(Omega_k) * cos(ik));
  zk =                          y_kp * sin(ik);

  if ( isnan(xk) || isnan(yk) || isnan(zk) ) {
    goto errorDetected;
  }

  // Load svPos vector
  svPos[0] = xk; svPos[1] = yk; svPos[2] = zk;

  //-----------------------------------------------------------------------------
  // SV clock correction calculation
  //-----------------------------------------------------------------------------

  // Time from ephemeris, SV clock
  tc = t - toc;              // t is the time of the pos. & vel. request.
  if (tc > 302400) {
    tc = tc - (2 * 302400);
  }
  else if (tc < -302400) {
    tc = tc + (604800);
  }

  // Relativistic SV clock correction correction
  dtr = -2 * sqrt(MU) / (CTIME * CTIME) * e * sqrta * sin(Ek);

  // SV clock bias with group delay (tgd) removed and relativistic correction
  // (dtr) added. the tgd term is included for L1-only signals, so it would
  // be removed if dual L1/L2 signals are used.
  dt = af0 + (af1 * tc) + (af2 * (tc * tc)) - tgd + dtr;
  *svClkCorr = dt;

  // Normal return
  return 0;

// Exit with error detected
errorDetected:
  return -1;

} // end of function

//-----------------------------------------------------------------------------
// Rotation matrix function
//-----------------------------------------------------------------------------
extern void rot(double R[9], double angle, int axis) {
 R[0] = 1.0; R[4] = 1.0; R[8] = 1.0;
 R[1] = 0.0; R[3] = 0.0; R[6] = 0.0;
 R[2] = 0.0; R[5] = 0.0; R[7] = 0.0;

 double cang, sang;
 cang = cos(angle * M_PI/180.0);
 sang = sin(angle * M_PI/180.0);

 if (axis == 1) {
  R[4] = cang;
  R[8] = cang;
  R[5] = sang;
  R[7] = -sang;
 }
 if (axis == 2) {
  R[0] = cang;
  R[8] = cang;
  R[2] = -sang;
  R[6] = sang;
 }
 if (axis == 3) {
  R[0] = cang;
  R[4] = cang;
  R[3] = -sang;
  R[1] = sang;
 }
} // end function

//-----------------------------------------------------------------------------
// precheckObs
//
// Input: index (the index of row we wish to remove, starting at 0
//-----------------------------------------------------------------------------
extern void precheckObs()
{

  // Initialize parameters
  int ret = 0;
  int updateRequired = 0;
  int prn = 0;
  int i = 0;
  double tol = 1e-15; // tolerance for checking if non-zero
  char buffer[MSG_LENGTH];

  // Set mutex, loop through all obs
  mlock(hobsvecmtx);
  for (i=0; i<sdrstat.nsatValid; i++) {
    prn = sdrstat.obsValidList[i];

    // Check that SNR level is above threshold
    if (sdrstat.obs_v[(prn-1) * 11 + 8] < SNR_PVT_THRES) {
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  preCheckObs: G%02d has SNR:%.1f\n",
        sdrstat.elapsedTime, prn,
        sdrstat.obs_v[(prn-1) * 11 + 8]);
      add_message(buffer);
      updateRequired = 1;
    } //end if

    // Check that GPS Week is non-zero
    if (sdrstat.obs_v[(prn-1) * 11 + 7] < GPS_WEEK) {
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  preCheckObs: G%02d has Week:%d\n",
        sdrstat.elapsedTime, prn,
         (int)sdrstat.obs_v[(prn-1) * 11 + 7]);
      add_message(buffer);
      updateRequired = 1;
    } // end if

    // Check that TOW is non-zero
    if (sdrstat.obs_v[(prn-1) * 11 + 6] < 1.0) {
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  preCheckObs: G%02d has ToW:%.1f\n",
        sdrstat.elapsedTime, prn,
         sdrstat.obs_v[(prn-1) * 11 + 6]);
      add_message(buffer);
      updateRequired = 1;
    } // end if

    // Check that the PR is not too low
    if (sdrstat.obs_v[(prn-1) * 11 + 5] < LOW_PR) {
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  preCheckObs: G%02d has Low PR:%.1f\n",
        sdrstat.elapsedTime, prn,
         sdrstat.obs_v[(prn-1) * 11 + 5]);
      add_message(buffer);
      updateRequired = 1;
    } // end if

    // Check that the PR is not too high
    if (sdrstat.obs_v[(prn-1) * 11 + 5] > HIGH_PR) {
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  preCheckObs: G%02d has High PR:%.1f\n",
        sdrstat.elapsedTime, prn,
         sdrstat.obs_v[(prn-1) * 11 + 5]);
      add_message(buffer);
      updateRequired = 1;
    } // end if

    // Check ephemeris values for reasonableness
    if ( (sdrch[prn-1].nav.sdreph.eph.toes<1.0) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.A)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.e)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.M0)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.omg)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.i0)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.OMG0)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.deln)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.idot)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.OMGd)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.cuc)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.cus)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.crc)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.crs)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.cic)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.cis)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.f0)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.f1)<tol) ||
         (fabs(sdrch[prn-1].nav.sdreph.eph.tgd[0])<tol) ){

      // Mark obs for removal and set updateRequired flag
      sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
      snprintf(buffer, sizeof(buffer),
        "%.3f  precheckEPH: G%02d tagged for removal for eph error\n",
        sdrstat.elapsedTime, prn);
      add_message(buffer);
      updateRequired = 1;
    } // end if

    // Check SV elevations to make sure they are above the elevation mask, but
    // wait for PVT to have calculated az and el.
    if (sdrstat.azElCalculatedflag) {
      if (sdrstat.obs_v[(prn-1) * 11 + 10] < SV_EL_PVT_MASK) {
        sdrstat.obs_v[(prn-1) * 11 + 1] = 0;
        snprintf(buffer, sizeof(buffer),
            "%.3f  precheckObs: G%02d tagged for removal with el of %.1f\n",
             sdrstat.elapsedTime, prn, sdrstat.obs_v[(prn-1) * 11 + 10]);
        add_message(buffer);
        updateRequired = 1;
      } // end if
    } // end if

  // End of looping for basic pre-checks
  } // end for

  // Unlock mutex for pre-checks
  unmlock(hobsvecmtx);

  // Update nsatValid and obsValidList
  if (updateRequired) {
    ret = updateObsList();
    if (ret==-1) { printf("updateObsList: error\n"); }
  }

  //return 0;
}

//-----------------------------------------------------------------------------
// Estimate tropo correction
//-----------------------------------------------------------------------------
extern int tropo(double sinel, double hsta, double p, double tkel,
                 double hum, double hp, double htkel, double hhum,
                 double *ddr) {

//TROPO  Calculation of tropospheric correction.
//       The range correction ddr in m is to be subtracted from
//       pseudo-ranges and carrier phases
//
// ddr = tropo(sinel, hsta, p, tkel, hum, hp, htkel, hhum);
//
//   Inputs:
//       sinel   - sin of elevation angle of satellite
//       hsta    - height of station in km
//       p       - atmospheric pressure in mb at height hp
//       tkel    - surface temperature in degrees Kelvin at height htkel
//       hum     - humidity in % at height hhum
//       hp      - height of pressure measurement in km
//       htkel   - height of temperature measurement in km
//       hhum    - height of humidity measurement in km
//
//   Outputs:
//       ddr     - range correction (meters)
//
// Reference
// Goad, C.C. & Goodman, L. (1974) A Modified Tropospheric
// Refraction Correction Model. Paper presented at the
// American Geophysical Union Annual Fall Meeting, San
// Francisco, December 12-17

// A Matlab reimplementation of a C code from driver.
// Kai Borre 06-28-95
//
// CVS record:
// $Id: tropo.m,v 1.1.1.1.2.4 2006/08/22 13:46:00 dpl Exp $
//==========================================================================

double a_e    = 6378.137;     // semi-major axis of earth ellipsoid
double b0     = 7.839257e-5;
double tlapse = -6.5;
double tkhum  = tkel + tlapse * (hhum - htkel);
double atkel  = 7.5*(tkhum-273.15) / (237.3+tkhum-273.15);
double e0     = 0.0611 * hum * pow(10,atkel);
double tksea  = tkel - tlapse * htkel;
double em     = -978.77 / (2.8704e6*tlapse*1.0e-5);
double tkelh  = tksea + tlapse*hhum;
double e0sea  = e0 * pow((tksea/tkelh),(4*em));
double tkelp  = tksea + tlapse*hp;
double psea   = p * pow((tksea/tkelp),em);
double alpha[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
double rn[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};

if (sinel < 0) {
    sinel = 0;
}

double tropo   = 0;
int done    = 0;
double refsea  = 77.624e-6 / tksea;
double htop    = 1.1385e-5 / refsea;
refsea  = refsea * psea;
double ref     = refsea * pow(((htop-hsta)/htop),4);

while (1) {
    double rtop = pow((a_e+htop),2) - pow((a_e+hsta),2)*(1-pow(sinel,2));

    // check to see if geometry is crazy
    if (rtop < 0) {
        rtop = 0;
    }

    rtop = sqrt(rtop) - (a_e+hsta)*sinel;
    double a    = -sinel/(htop-hsta);
    double b    = -b0*(1-pow(sinel,2)) / (htop-hsta);
    //double rn   = zeros(8,1);

    for (int i=0; i<7; i++) {
        rn[i] = pow(rtop,(i+2));
    }

    alpha[0] = 2 * a;
    alpha[1] = 2 * pow(a,2) + 4 * b/3;
    alpha[2] = a * (pow(a,2) + 3 * b);
    alpha[3] = pow(a,4)/5 + 2.4 * pow(a,2) * b + 1.2 * pow(b,2);
    alpha[4] = 2 * a * b * (pow(a,2) + 3 * b) / 3;
    alpha[5] = pow(b,2) * (6 * pow(a,2) + 4 * b) * 1.428571e-1;
    alpha[6] =     0.0;
    alpha[7] =     0.0;

    if (pow(b,2) > 1.0e-35) {
        alpha[6] = a*pow(b,3)/2;
        alpha[7] = pow(b,4)/9;
    }

    double dr = rtop;
    // Calculate dr = dr + alpha * rn;
    for (int i=0; i<7; i++) {
      dr = dr + alpha[i] * rn[i];
    }

    tropo = tropo + dr * ref * 1000;

    if (done == 1) {
        *ddr = tropo;
        break;
    }

    done    = 1;
    refsea  = (371900.0e-6 / tksea - 12.92e-6) / tksea;
    htop    = 1.1385e-5 * (1255 / tksea + 0.05) / refsea;
    ref     = refsea * e0sea * pow(((htop-hsta)/htop),4);

} // end while loop

// end of function
return 0;

} // end function

//-----------------------------------------------------------------------------
// Estimate LLA given ECEF
//-----------------------------------------------------------------------------
extern int togeod(double a, double finv, double X, double Y, double Z,
                  double *dphi, double *dlambda, double *h) {
//TOGEOD   Subroutine to calculate geodetic coordinates latitude, longitude,
//         height given Cartesian coordinates X,Y,Z, and reference ellipsoid
//         values semi-major axis (a) and the inverse of flattening (finv).
//
//[dphi, dlambda, h] = togeod(a, finv, X, Y, Z);
//
//  The units of linear parameters X,Y,Z,a must all agree (m,km,mi,ft,..etc)
//  The output units of angular quantities will be in decimal degrees
//  (15.5 degrees not 15 deg 30 min). The output units of h will be the
//  same as the units of X,Y,Z,a.
//
//   Inputs:
//       a           - semi-major axis of the reference ellipsoid
//       finv        - inverse of flattening of the reference ellipsoid
//       X,Y,Z       - Cartesian coordinates
//
//   Outputs:
//       dphi        - latitude
//       dlambda     - longitude
//       h           - height above reference ellipsoid

//  Copyright (C) 1987 C. Goad, Columbus, Ohio
//  Reprinted with permission of author, 1996
//  Fortran code translated into MATLAB
//  Kai Borre 03-30-96
//
// CVS record:
// $Id: togeod.m,v 1.1.1.1.2.4 2006/08/22 13:45:59 dpl Exp $
//==========================================================================

  *h       = 0;
  double tolsq   = 1.e-10;
  double maxit   = 50;
  double esq = 0.0;
  double sinphi = 0.0;

  // compute radians-to-degree factor
  //double rtd     = 180/PI;

  // compute square of eccentricity
  if (finv < 1.e-20) {
    esq = 0;
  } else {
    esq = (2 - 1/finv) / finv;
  }

  double oneesq  = 1 - esq;

  // first guess
  // P is distance from spin axis
  double P = sqrt(X*X+Y*Y);
  // direct calculation of longitude

  if (P > 1.e-20) {
    *dlambda = atan2(Y,X) * R2D;
  } else {
    *dlambda = 0;
  }

  if (*dlambda < 0) {
    *dlambda = *dlambda + 360;
  }

  // r is distance from origin (0,0,0)
  double r = sqrt(P*P + Z*Z);

  if (r > 1.e-20) {
    sinphi = Z/r;
  } else {
    sinphi = 0;
  }

  *dphi = asin(sinphi);

  // initial value of height  =  distance from origin minus
  // approximate distance from origin to surface of ellipsoid
  if (r < 1.e-20) {
    *h = 0;
    goto errorDetected;
  }

  *h = r - a * (1 - sinphi * sinphi / finv);

  // iterate
  for (int i=0; i<maxit; i++) {
    double sinphi  = sin(*dphi);
    double cosphi  = cos(*dphi);

    // compute radius of curvature in prime vertical direction
    double N_phi   = a / sqrt(1 - esq * sinphi * sinphi);

    // compute residuals in P and Z
    double dP      = P - (N_phi + *h) * cosphi;
    double dZ      = Z - (N_phi * oneesq + *h) * sinphi;

    // update height and latitude
    *h       = *h + (sinphi*dZ + cosphi*dP);
    *dphi    = *dphi + (cosphi*dZ - sinphi*dP)/(N_phi + *h);

    // test for convergence
    if (dP*dP + dZ*dZ < tolsq) {
        break;
    }

    // Not Converged--Warn user
    if (i == maxit) {
        printf("Problem in TOGEOD, did not converge in %2d iterations\n", i);
    }
  } // for i = 1:maxit

  *dphi = *dphi * R2D;

  // end togeod.m
  return 0;

errorDetected:
    printf("togeod--- errorDetected\n");
    return -1;

} // end function

//-----------------------------------------------------------------------------
// Estimate Az and El given ECEF vector
//-----------------------------------------------------------------------------
extern int topocent(double X[], double dx[], double *Az, double *El, double *D) {
//TOPOCENT  Transformation of vector dx into topocentric coordinate
//          system with origin at X.
//          Both parameters are 3 by 1 vectors.
//
//[Az, El, D] = topocent(X, dx);
//
//   Inputs:
//       X           - vector origin corrdinates (in ECEF system [X; Y; Z;])
//       dx          - vector ([dX; dY; dZ;]).
//
//   Outputs:
//       D           - vector length. Units like units of the input
//       Az          - azimuth from north positive clockwise, degrees
//       El          - elevation angle, degrees

//Kai Borre 11-24-96
//Copyright (c) by Kai Borre
//
// CVS record:
// $Id: topocent.m,v 1.1.1.1.2.4 2006/08/22 13:45:59 dpl Exp $
//==========================================================================

int ret;
//double F_v[9] = {0};
double local_v[3] = {0};
double phi;
double lambda;
double h;

ret = togeod(6378137, 298.257223563, X[0], X[1], X[2], &phi, &lambda, &h);
if (ret!=0) {
 printf("togeod function: error\n");
}

double cl  = cos(lambda * D2R);
double sl  = sin(lambda * D2R);
double cb  = cos(phi * D2R);
double sb  = sin(phi * D2R);

// Definition of F
//double F   = [-sl -sb*cl cb*cl;
//        cl -sb*sl cb*sl;
//        0    cb   sb];

// Calculate local_v = F' * dx;
local_v[0] = -sl * dx[0] + cl * dx[1] + 0.0 * dx[2];
local_v[1] = -sb*cl * dx[0] - sb*sl * dx[1] + cb * dx[2];
local_v[2] = cb*cl * dx[0] + cb*sl * dx[1] + sb * dx[2];

double E   = local_v[0];
double N   = local_v[1];
double U   = local_v[2];

double hor_dis = sqrt(E*E + N*N);

if (hor_dis < 1.e-20) {
  *Az = 0.0;
  *El = 90;
} else {
  *Az = atan2(E, N)/D2R;
  *El = atan2(U, hor_dis)/D2R;
}

if (*Az < 0) {
  *Az = *Az + 360;
}

*D   = sqrt(dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2]);

return 0;
}  // end of function

//-----------------------------------------------------------------------------
// Updates nsatValid and obsValidList
//-----------------------------------------------------------------------------
extern int updateObsList(void) {

  mlock(hobsvecmtx);
  // Zero nsatValid
  sdrstat.nsatValid = 0;

  // Zero all elements of obsValidList
  for (int j=0; j<32; j++) {
    sdrstat.obsValidList[j] = 0;
  }

  // Now fill with valid PRNs
  for (int i=0; i<32; i++) {
     if (sdrstat.obs_v[i*11+1]==1) {
      sdrstat.obsValidList[sdrstat.nsatValid] = sdrstat.obs_v[i*11+0];
      sdrstat.nsatValid = sdrstat.nsatValid + 1;
    }  // end if
  } // end for
  unmlock(hobsvecmtx);

  return 0;
}
