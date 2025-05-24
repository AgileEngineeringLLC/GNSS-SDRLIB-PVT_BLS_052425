//------------------------------------------------------------------------------
// rtklib.h : rtklib constants, types and function prototypes
//
// This is a greatly-reduced version of rtklib.h, pulling out only what is
// needed for GNSS-SDRCLI
//-----------------------------------------------------------------------------
#ifndef RTKLIB_H
#define RTKLIB_H

#ifdef __cplusplus
extern "C" {
#endif

// constants -----------------------------------------------------------------
#define PI          3.1415926535897932  // pi   
#define D2R         (PI/180.0)          // deg to rad   
#define R2D         (180.0/PI)          // rad to deg   
#define CLIGHT      299792458.0         // speed of light (m/s)   
#define SC2RAD      3.1415926535898     // semi-circle to radian (IS-GPS)   
#define AU          149597870691.0      // 1 AU (m)   
#define AS2R        (D2R/3600.0)        // arc sec to radian
#define OMGE        7.2921151467E-5     // earth angular velocity (IS-GPS) (rad/s)
#define RE_WGS84    6378137.0           // earth semimajor axis (WGS84) (m)   
#define FE_WGS84    (1.0/298.257223563) // earth flattening (WGS84)
#define HION        350000.0            // ionosphere height (m)
#define MAXFREQ     7                   // max NFREQ
#define FREQ1       1.57542E9           // L1/E1  frequency (Hz)
#define EFACT_GPS   1.0                 // error factor: GPS   

#define SYS_NONE    0x00                // navigation system: none   
#define SYS_GPS     0x01                // navigation system: GPS   
#define SYS_SBS     0x02                // navigation system: SBAS   

#define TSYS_GPS    0                   // time system: GPS time   
#define TSYS_UTC    1                   // time system: UTC   

#ifndef NFREQ
#define NFREQ       3                   // number of carrier frequencies   
#endif

#ifndef NEXOBS
#define NEXOBS      0                   // number of extended obs codes   
#endif

#define MINPRNGPS   1                   // min satellite PRN number of GPS   
#define MAXPRNGPS   32                  // max satellite PRN number of GPS   
#define NSATGPS     (MAXPRNGPS-MINPRNGPS+1) // number of GPS satellites   
#define NSYSGPS     1

#define NSYS        NSYSGPS // number of systems

#define MINPRNSBS   120                 // min satellite PRN number of SBAS   
#define MAXPRNSBS   142                 // max satellite PRN number of SBAS   
#define NSATSBS     (MAXPRNSBS-MINPRNSBS+1) // number of SBAS satellites   

//#define MAXSAT      (NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+NSATSBS+NSATLEO)
#define MAXSAT      32  // DK set to GPS size
                                        // max satellite number (1 to MAXSAT)   
#ifndef MAXOBS
#define MAXOBS      64                  // max number of obs in an epoch   
#endif

#define MAXRCV      64                  // max receiver number (1 to MAXRCV)   
#define MAXOBSTYPE  64                  // max number of obs type in RINEX   
#define DTTOL       0.005               // tolerance of time difference (s)   

#if 0
#define MAXDTOE     10800.0             // max time difference to ephem Toe (s) for GPS   
#else
#define MAXDTOE     7200.0              // max time difference to ephem Toe (s) for GPS   
#endif

//#define MAXDTOE_GLO 1800.0              // max time difference to GLONASS Toe (s)   
#define MAXDTOE_SBS 360.0               // max time difference to SBAS Toe (s)   
#define MAXDTOE_S   86400.0             // max time difference to ephem toe (s) for other   
#define MAXGDOP     300.0               // max GDOP   

#define MAXEXFILE   100                 // max number of expanded files   
#define MAXSBSAGEF  30.0                // max age of SBAS fast correction (s)   
#define MAXSBSAGEL  1800.0              // max age of SBAS long term corr (s)   
#define MAXSBSURA   8                   // max URA of SBAS satellite   
#define MAXBAND     10                  // max SBAS band of IGP   
#define MAXNIGP     201                 // max number of IGP in SBAS band   
#define MAXNGEO     4                   // max number of GEO satellites   
#define MAXCOMMENT  10                  // max number of RINEX comments   
#define MAXSTRPATH  1024                // max length of stream path   
#define MAXSTRMSG   1024                // max length of stream message   
#define MAXSTRRTK   8                   // max number of stream in RTK server   
#define MAXSBSMSG   32                  // max number of SBAS msg in RTK server   
#define MAXSOLMSG   4096                // max length of solution message   
#define MAXRAWLEN   4096                // max length of receiver raw message   
#define MAXERRMSG   4096                // max length of error/warning message   
#define MAXANT      64                  // max length of station name/antenna type   
#define MAXSOLBUF   256                 // max number of solution buffer   
#define MAXOBSBUF   128                 // max number of observation data buffer   
#define MAXNRPOS    16                  // max number of reference positions   

#define OBSTYPE_PR  0x01                // observation type: pseudorange   
#define OBSTYPE_CP  0x02                // observation type: carrier-phase   
#define OBSTYPE_DOP 0x04                // observation type: doppler-freq   
#define OBSTYPE_SNR 0x08                // observation type: SNR   
#define OBSTYPE_ALL 0xFF                // observation type: all   

#define FREQTYPE_L1 0x01                // frequency type: L1/E1   

#define CODE_NONE   0                   // obs code: none or unknown   
#define CODE_L1C    1                   // obs code: L1C/A,G1C/A,E1C (GPS,GLO,GAL,QZS,SBS)   
#define CODE_L1P    2                   // obs code: L1P,G1P    (GPS,GLO)   
#define CODE_L1W    3                   // obs code: L1 Z-track (GPS)   
#define CODE_L1Y    4                   // obs code: L1Y        (GPS)   
#define CODE_L1M    5                   // obs code: L1M        (GPS)   
#define CODE_L1N    6                   // obs code: L1codeless (GPS)   
#define CODE_L1S    7                   // obs code: L1C(D)     (GPS,QZS)   
#define CODE_L1L    8                   // obs code: L1C(P)     (GPS,QZS)   

#define MAXCODE     48                  // max number of obs code

#define TIMES_GPST  0                   // time system: gps time   
#define TIMES_UTC   1                   // time system: utc

#define EPHOPT_BRDC 0                   // ephemeris option: broadcast ephemeris   
#define EPHOPT_PREC 1                   // ephemeris option: precise ephemeris   
#define EPHOPT_SBAS 2                   // ephemeris option: broadcast + SBAS   
#define EPHOPT_SSRAPC 3                 // ephemeris option: broadcast + SSR_APC   
#define EPHOPT_SSRCOM 4                 // ephemeris option: broadcast + SSR_COM   

#define SBSOPT_LCORR 1                  // SBAS option: long term correction   
#define SBSOPT_FCORR 2                  // SBAS option: fast correction   
#define SBSOPT_ICORR 4                  // SBAS option: ionosphere correction   
#define SBSOPT_RANGE 8                  // SBAS option: ranging   

// P2 values are used in GPS nav decode
#define P2_5        0.03125             // 2^-5   
#define P2_6        0.015625            // 2^-6   
#define P2_11       4.882812500000000E-04 // 2^-11   
#define P2_15       3.051757812500000E-05 // 2^-15   
#define P2_17       7.629394531250000E-06 // 2^-17   
#define P2_19       1.907348632812500E-06 // 2^-19   
#define P2_20       9.536743164062500E-07 // 2^-20   
#define P2_21       4.768371582031250E-07 // 2^-21   
#define P2_23       1.192092895507810E-07 // 2^-23   
#define P2_24       5.960464477539063E-08 // 2^-24   
#define P2_27       7.450580596923828E-09 // 2^-27   
#define P2_29       1.862645149230957E-09 // 2^-29   
#define P2_30       9.313225746154785E-10 // 2^-30   
#define P2_31       4.656612873077393E-10 // 2^-31   
#define P2_32       2.328306436538696E-10 // 2^-32   
#define P2_33       1.164153218269348E-10 // 2^-33   
#define P2_35       2.910383045673370E-11 // 2^-35   
#define P2_38       3.637978807091710E-12 // 2^-38   
#define P2_39       1.818989403545856E-12 // 2^-39   
#define P2_40       9.094947017729280E-13 // 2^-40   
#define P2_43       1.136868377216160E-13 // 2^-43   
#define P2_48       3.552713678800501E-15 // 2^-48   
#define P2_50       8.881784197001252E-16 // 2^-50   
#define P2_55       2.775557561562891E-17 // 2^-55

#define thread_t    pthread_t
#define lock_t      pthread_mutex_t
#define initlock(f) pthread_mutex_init(f,NULL)
#define lock(f)     pthread_mutex_lock(f)
#define unlock(f)   pthread_mutex_unlock(f)
#define FILEPATHSEP '/'

// type definitions ----------------------------------------------------------  

typedef struct {        // time struct   
    time_t time;        // time (s) expressed by standard time_t   
    double sec;         // fraction of second under 1 s   
} gtime_t;

typedef struct {        // GPS/QZS/GAL broadcast ephemeris type   
    int sat;            // satellite number   
    int iode,iodc;      // IODE,IODC   
    int sva;            // SV accuracy (URA index)   
    int svh;            // SV health (0:ok)   
    int week;           // GPS/QZS: gps week, GAL: galileo week   
    int code;           // GPS/QZS: code on L2, GAL/CMP: data sources   
    int flag;           // GPS/QZS: L2 P data flag, CMP: nav type   
    gtime_t toe,toc,ttr; // Toe,Toc,T_trans   
                        // SV orbit parameters   
    double A,e,i0,OMG0,omg,M0,deln,OMGd,idot;
    double crc,crs,cuc,cus,cic,cis;
    double toes;        // Toe (s) in week   
    double fit;         // fit interval (h)   
    double f0,f1,f2;    // SV clock parameters (af0,af1,af2)   
    double tgd[4];      // group delay parameters   
                        // GPS/QZS:tgd[0]=TGD   
                        // GAL    :tgd[0]=BGD E5a/E1,tgd[1]=BGD E5b/E1   
                        // CMP    :tgd[0]=BGD1,tgd[1]=BGD2   
} eph_t;

typedef struct {        // SBAS message type   
    int week,tow;       // receiption time   
    int prn;            // SBAS satellite PRN number   
    unsigned char msg[29]; // SBAS message (226bit) padded by 0   
} sbsmsg_t;

typedef struct {        // SBAS messages type   
    int n,nmax;         // number of SBAS messages/allocated   
    sbsmsg_t *msgs;     // SBAS messages   
} sbs_t;

typedef struct {        // SBAS fast correction type   
    gtime_t t0;         // time of applicability (TOF)   
    double prc;         // pseudorange correction (PRC) (m)   
    double rrc;         // range-rate correction (RRC) (m/s)   
    double dt;          // range-rate correction delta-time (s)   
    int iodf;           // IODF (issue of date fast corr)   
    short udre;         // UDRE+1   
    short ai;           // degradation factor indicator   
} sbsfcorr_t;

typedef struct {        // SBAS long term satellite error correction type   
    gtime_t t0;         // correction time   
    int iode;           // IODE (issue of date ephemeris)   
    double dpos[3];     // delta position (m) (ecef)   
    double dvel[3];     // delta velocity (m/s) (ecef)   
    double daf0,daf1;   // delta clock-offset/drift (s,s/s)   
} sbslcorr_t;

typedef struct {        // SBAS satellite correction type   
    int sat;            // satellite number   
    sbsfcorr_t fcorr;   // fast correction   
    sbslcorr_t lcorr;   // long term correction   
} sbssatp_t;

typedef struct {        // SBAS satellite corrections type   
    int iodp;           // IODP (issue of date mask)   
    int nsat;           // number of satellites   
    int tlat;           // system latency (s)   
    sbssatp_t sat[MAXSAT]; // satellite correction   
} sbssat_t;

typedef struct {        // SBAS ionospheric correction type   
    gtime_t t0;         // correction time   
    short lat,lon;      // latitude/longitude (deg)   
    short give;         // GIVI+1   
    float delay;        // vertical delay estimate (m)   
} sbsigp_t;

typedef struct {        // IGP band type   
    short x;            // longitude/latitude (deg)   
    const short *y;     // latitudes/longitudes (deg)   
    unsigned char bits; // IGP mask start bit   
    unsigned char bite; // IGP mask end bit   
} sbsigpband_t;

typedef struct {        // SBAS ionospheric corrections type   
    int iodi;           // IODI (issue of date ionos corr)   
    int nigp;           // number of igps   
    sbsigp_t igp[MAXNIGP]; // ionospheric correction   
} sbsion_t;

typedef struct {        // navigation data type   
    int n,nmax;         // number of broadcast ephemeris   
    int ng,ngmax;       // number of glonass ephemeris   
    int ns,nsmax;       // number of sbas ephemeris   
    int ne,nemax;       // number of precise ephemeris   
    int nc,ncmax;       // number of precise clock   
    int na,namax;       // number of almanac data   
    int nt,ntmax;       // number of tec grid data   
    int nn,nnmax;       // number of stec grid data   
    eph_t *eph;         // GPS/QZS/GAL ephemeris   
    double utc_gps[4];  // GPS delta-UTC parameters {A0,A1,T,W}
    double utc_glo[4];  // GLONASS UTC GPS time parameters   
    double utc_gal[4];  // Galileo UTC GPS time parameters   
    double utc_qzs[4];  // QZS UTC GPS time parameters   
    double utc_cmp[4];  // BeiDou UTC parameters   
    double utc_sbs[4];  // SBAS UTC parameters   
    double ion_gps[8];  // GPS iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3}   
    double ion_gal[4];  // Galileo iono model parameters {ai0,ai1,ai2,0}   
    double ion_qzs[8];  // QZSS iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3}   
    double ion_cmp[8];  // BeiDou iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3}   
    int leaps;          // leap seconds (s)   
    double lam[MAXSAT][NFREQ]; // carrier wave lengths (m)   
    double cbias[MAXSAT][3];   // code bias (0:p1-p2,1:p1-c1,2:p2-c2) (m)   
    double wlbias[MAXSAT];     // wide-lane bias (cycle)   
    sbssat_t sbssat;    // SBAS satellite corrections
    sbsion_t sbsion[MAXBAND+1]; // SBAS ionosphere corrections   
} nav_t;

// global variables ----------------------------------------------------------
extern const sbsigpband_t igpband1[][8]; // SBAS IGP band 0-8   
extern const sbsigpband_t igpband2[][5]; // SBAS IGP band 9-10   

// satellites, systems, codes functions --------------------------------------
extern int  satno   (int sys, int prn);
extern int  satsys  (int sat, int *prn);
extern int  satid2no(const char *id);
extern void satno2id(int sat, char *id);

// time and string functions -------------------------------------------------
extern gtime_t epoch2time(const double *ep);
extern gtime_t gpst2time(int week, double sec);
extern gtime_t gst2time(int week, double sec);
extern gtime_t timeadd  (gtime_t t, double sec);
extern double  timediff (gtime_t t1, gtime_t t2);
extern gtime_t utc2gpst (gtime_t t);
extern int adjgpsweek(int week);
extern void sleepms(int ms);

/*
// debug trace functions -----------------------------------------------------
extern void traceopen(const char *file);
extern void traceclose(void);
extern void tracelevel(int level);
extern void trace    (int level, const char *format, ...);
extern void tracet   (int level, const char *format, ...);
extern void tracemat (int level, const double *A, int n, int m, int p, int q);
//extern void traceobs (int level, const obsd_t *obs, int n);
extern void tracenav (int level, const nav_t *nav);
extern void tracegnav(int level, const nav_t *nav);
extern void tracehnav(int level, const nav_t *nav);
extern void tracepeph(int level, const nav_t *nav);
extern void tracepclk(int level, const nav_t *nav);
extern void traceb   (int level, const unsigned char *p, int n);
*/

// receiver raw data functions -----------------------------------------------
extern unsigned int getbitu(const unsigned char *buff, int pos, int len);
extern int          getbits(const unsigned char *buff, int pos, int len);
extern void setbitu(unsigned char *buff, int pos, int len, unsigned int data);
extern void setbits(unsigned char *buff, int pos, int len, int data);
extern unsigned int crc32  (const unsigned char *buff, int len);
extern unsigned int crc24q (const unsigned char *buff, int len);

#ifdef __cplusplus
}
#endif
#endif // RTKLIB_H   
