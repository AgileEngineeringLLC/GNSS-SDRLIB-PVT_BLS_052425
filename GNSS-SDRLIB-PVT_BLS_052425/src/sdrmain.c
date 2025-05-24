//-----------------------------------------------------------------------------
// sdrmain.c : SDR main functions
//
// Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
// Edits from Don Kelly, don.kelly@mac.com, 2025
//-----------------------------------------------------------------------------
#include "sdr.h"

// Thread handles and mutex
thread_t hmainthread;
thread_t hsyncthread;
thread_t hkeythread;
thread_t hdatathread;
thread_t hguithread;

mlock_t hbuffmtx;
mlock_t hreadmtx;
mlock_t hfftmtx;
mlock_t hobsmtx;
mlock_t hresetmtx;
mlock_t hobsvecmtx;
mlock_t hmsgmtx;

// SDR structs
sdrini_t sdrini={0};
sdrstat_t sdrstat={0};
sdrch_t sdrch[MAXSAT]={{0}};
sdrekf_t sdrekf={0};
sdrgui_t sdrgui={0};

// Keyboard thread ------------------------------------------------------------
// keyboard thread for program termination
// args   : void   *arg      I   not used
// return : none
//-----------------------------------------------------------------------------
extern void *keythread(void * arg)
{
  do {
    switch(getchar()) {
      case 'q':
      case 'Q':
        sdrstat.stopflag=1;
        break;
      default:
        SDRPRINTF("press 'q' to exit...\n");
        break;
      }
    } while (!sdrstat.stopflag);

  return THRETVAL;
}

// main function --------------------------------------------------------------
// main entry point in CLI application
// args   : none
// return : none
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  // Set processor to Performance mode
  // (Might add this as system command)

  // Read ini file
  if (readinifile(&sdrini)<0) {
    return -1;
  }

  // Declare CPU affinity variables
  int num_cpus;
  cpu_set_t cpu_set;

  // Find the number of available processor CPUs with sysconf call.
  num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

  // We will leave the last four CPUs for OS tasks, then enable the rest for
  // real-time use by main. Select CPUs to use with CPU_SET.
  for (int n=0;n<(num_cpus-4);n++) {
    CPU_SET(n, &cpu_set);
  }

  // Schedule the cores for use by main using sched_setaffinity
  if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set) == -1) {
    perror("error: sched_setaffinity\n");
  }

  // Start SDR and threads
  startsdr();

  return 0;
}

// sdr start ------------------------------------------------------------------
// start sdr function
// args   : void   *arg      I   not used
// return : none
// note : This is called as a function in CLI application
//-----------------------------------------------------------------------------
extern void startsdr(void)
{
  // Timer struct for terminal printf
  //struct timespec start, end, orig;

  int i;
  SDRPRINTF("GNSS-SDRLIB start!\n");

  // Establish attr and param for real time threads
  struct sched_param param1;
  struct sched_param param2;
  pthread_attr_t attr1;
  pthread_attr_t attr2;
  int ret; // thread return state

  // Initialize EKF
  //ret = ekfInit();

  // Declare thread attribute attr1 and attr2
  ret = pthread_attr_init(&attr1);
  if (ret) {
    printf("Init for thread attr1 failed: %s\n", strerror(ret));
  }
  ret = pthread_attr_init(&attr2);
  if (ret) {
    printf("Init for thread attr2 failed: %s\n", strerror(ret));
  }

  // Set stack size
  ret = pthread_attr_setstacksize(&attr1, PTHREAD_STACK_MIN);
  if (ret) {
    printf("Stack size for thread attr1 failed: %s\n", strerror(ret));
  }
  ret = pthread_attr_setstacksize(&attr2, PTHREAD_STACK_MIN);
  if (ret) {
    printf("Stack size for thread attr2 failed: %s\n", strerror(ret));
  }

  // Set scheduler policy and priority
  ret = pthread_attr_setschedpolicy(&attr1, SCHED_FIFO);
  if (ret) {
    printf("Policy for thread attr1 failed: %s\n", strerror(ret));
  }
  ret = pthread_attr_setschedpolicy(&attr2, SCHED_OTHER);
  if (ret) {
    printf("Policy for thread attr2 failed: %s\n", strerror(ret));
  }

  // Set thread priority in attr and param
  param1.sched_priority = 40;
  ret = pthread_attr_setschedparam(&attr1, &param1);
  if (ret) {
    printf("Priority for thread attr1 failed: %s\n", strerror(ret));
  }
  param2.sched_priority = 0;
  ret = pthread_attr_setschedparam(&attr2, &param2);
  if (ret) {
    printf("Priority for thread attr2 failed: %s\n", strerror(ret));
  }

  // Direct attr to use the scheduling parameters we set
  ret = pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);
  if (ret) {
    printf("Inherit for thread attr1 failed: %s\n", strerror(ret));
  }
  ret = pthread_attr_setinheritsched(&attr2, PTHREAD_EXPLICIT_SCHED);
  if (ret) {
    printf("Inherit for thread attr2 failed: %s\n", strerror(ret));
  }

  // check initial value
  if (chk_initvalue(&sdrini)<0) {
    SDRPRINTF("error: chk_initvalue\n");
    quitsdr(&sdrini,1);
    return;
  }

  // receiver initialization
  if (rcvinit(&sdrini)<0) {
    SDRPRINTF("error: rcvinit\n");
    quitsdr(&sdrini,1);
    return;
  }
  // initialize sdr channel struct
  for (i=0;i<sdrini.nch;i++) {
    if (initsdrch(i+1,sdrini.sys[i],sdrini.prn[i],sdrini.ctype[i],
      sdrini.dtype[sdrini.ftype[i]-1],sdrini.ftype[i],
      sdrini.f_gain[sdrini.ftype[i]-1],sdrini.f_bias[sdrini.ftype[i]-1],
      sdrini.f_clock[sdrini.ftype[i]-1],sdrini.f_cf[sdrini.ftype[i]-1],
      sdrini.f_sf[sdrini.ftype[i]-1],sdrini.f_if[sdrini.ftype[i]-1],
      &sdrch[i])<0) {

      SDRPRINTF("error: initsdrch\n");
      quitsdr(&sdrini,2);
      return;
    }
  }

  // mutexes and events
  openhandles();

  // Create threads ---------------------------------------------------------
  // Keyboard thread
  //ret = pthread_create(&hkeythread,&attr2,keythread,NULL);
  ret = pthread_create(&hkeythread,NULL,keythread,NULL);
  if (ret) {
    printf(BRED "Create for keyboard thread failed: %s\n" reset,
         strerror(ret));
  }

  // Sync thread
  //ret = pthread_create(&hsyncthread,&attr1,syncthread,NULL);
  ret = pthread_create(&hsyncthread,NULL,syncthread,NULL);
  if (ret) {
    printf(BRED "Create for sync thread failed: %s\n" reset,
           strerror(ret));
  }

  // SDR channel threads
  for (i=0;i<sdrini.nch;i++) {
    // GPS/QZS/GLO/GAL/CMP L1
    if (sdrch[i].ctype==CTYPE_L1CA  || sdrch[i].ctype==CTYPE_L1SBAS){
      //ret=pthread_create(&sdrch[i].hsdr,&attr1,sdrthread,&sdrch[i]);
      ret=pthread_create(&sdrch[i].hsdr,NULL,sdrthread,&sdrch[i]);
      if (ret) {
        printf(BRED "Create for sdr thread failed: %s\n" reset,
               strerror(ret));
      } // if
    } // if
  } // for (sdrch threads)

  // Data grabber thread
  //ret=pthread_create(&hdatathread,&attr1,datathread,NULL);
  ret=pthread_create(&hdatathread,NULL,datathread,NULL);
  if (ret) {
    printf(BRED "Create for data thread failed: %s\n" reset, strerror(ret));
  }

  // Initialize ncurses
  initscr();
  start_color();  // Enable color functionality
  noecho();
  cbreak();
  curs_set(FALSE);
  timeout(0);  // Non-blocking input

  // Establish set of GUI colors
  init_color(PUR1,200,0,200);
  init_color(PUR2,200,0,100);
  init_pair(1,COLOR_WHITE,COLOR_BLUE);
  init_pair(2,COLOR_WHITE,COLOR_MAGENTA);
  init_pair(3,COLOR_WHITE,COLOR_CYAN);
  init_pair(4,COLOR_WHITE,COLOR_GREEN);
  init_pair(5,COLOR_WHITE,PUR1);
  init_pair(6,COLOR_WHITE,PUR2);

  // Declare window parms
  int hgt1, wid1;
  int starty1, startx1;
  int hgt2, wid2;
  int starty2, startx2;
  int scr_width, scr_height;
  int counter = 0;

  // Get size of stdscr
  getmaxyx(stdscr, scr_height, scr_width);

  // Dummy line to prevent error message at compile
  if(0) printf("%d\n",scr_height);

  // Parms needed to make both boxes
  //hgt1 = scr_height /2;
  hgt1 = 21;
  wid1 = scr_width;
  starty1 = 0;
  startx1 = 0;
  hgt2 = 12;
  wid2 = scr_width;
  starty2 = 21;
  startx2 = 0;

  // Create two windows and set window parms
  WINDOW *win1 = newwin(hgt1, wid1, starty1, startx1);
  WINDOW *win2 = newwin(hgt2, wid2, starty2, startx2);
  scrollok(win2, TRUE);
  wbkgd(win1,COLOR_PAIR(5));
  wbkgd(win2,COLOR_PAIR(6));

  // Start timer
  struct timespec start_time, current_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  sdrstat.elapsedTime = 0.0;

  // Main while loop
  while (!sdrstat.stopflag) {

    // Update elepsed time
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    long seconds = current_time.tv_sec - start_time.tv_sec;
    long ns = current_time.tv_nsec - start_time.tv_nsec;
    sdrstat.elapsedTime = ((seconds * 1000) + (ns / 1e6) ) / 1000.0;

    // Update both status windows
    updateNavStatusWin(win1, counter);
    updateProgramStatusWin(win2, hgt2);

    // Update counter (used in Nav Status win)
    counter++;

    // Update GUI at desired rate
    usleep(100000);

    // Use these lines to send test messages
    //char buffer[MSG_LENGTH];
    //snprintf(buffer, sizeof(buffer), "Message goes here %d", 1);
    //add_message(buffer);

  } // end main while

  // Cleanup threads and messages
  for (int i = 0; i < sdrgui.message_count; i++) {
    free(sdrgui.messages[i]);
  }

  // Cleanup ncurses memory
  delwin(win1);
  delwin(win2);
  endwin();

  // Wait (pthreads join) threads
  waitthread(hsyncthread);
  for (i=0;i<sdrini.nch;i++) {
    waitthread(sdrch[i].hsdr);
  }
  waitthread(hdatathread);

  // SDR termination
  quitsdr(&sdrini,0);

  SDRPRINTF("GNSS-SDRLIB is finished!\n");
}

// sdr termination ------------------------------------------------------------
// sdr termination process
// args   : sdrini_t *ini    I   sdr init struct
// args   : int    stop      I   stop position in function 0: run all
// return : none
//-----------------------------------------------------------------------------
extern void quitsdr(sdrini_t *ini, int stop)
{
    int i;

    if (stop==1) return;

    // SDR termination
    rcvquit(ini);
    if (stop==2) return;

    // Free memory
    for (i=0;i<ini->nch;i++) freesdrch(&sdrch[i]);
    if (stop==3) return;

    // Mutexes and events
    closehandles();
    if (stop==4) return;
}

// sdr channel thread ---------------------------------------------------------
// sdr channel thread for signal acquisition and tracking
// args   : void   *arg      I   sdr channel struct
// return : none
// note : This thread handles the acquisition and tracking of one of the signals.
//        The thread is created at startsdr function.
//-----------------------------------------------------------------------------
extern void *sdrthread(void *arg)
{
  sdrch_t *sdr=(sdrch_t*)arg;
  uint64_t buffloc=0,bufflocnow=0,cnt=0,loopcnt=0;
  double *acqpower=NULL;
  double snr, el;
  int ret = 0;
  char bufferSDR[MSG_LENGTH];

  // Establish timer parameters
  time_t current_time;//start_time_acq,start_time_nav,start_time_snr,
  time_t start_acq_timer; // Starts when flagacq equals 1
  start_acq_timer = time(NULL); // declare it here, but really assigned by acq
  double elapsed_acq_time = 0;

  // Slightly delay the start of each thread independently
  sleepms(sdr->no*500);

  //-------------------------------------------------------------------------
  // While loop for sdrch thread
  //-------------------------------------------------------------------------
  while (!sdrstat.stopflag) {

    // SDR Channel Reset Checks -------------------------------------------
    // Calculate elapsed time since flagacq was set.
    current_time = time(NULL);
    if (sdr->flagacq) {
      elapsed_acq_time = current_time - start_acq_timer;
    }

    // Every 30s check SNR to make sure it is not too low
    if (elapsed_acq_time>60) {
      mlock(hobsmtx);
      snr = sdr->trk.S[0];
      unmlock(hobsmtx);

      // If SNR is low, set resetflag for this channel
      if (snr<SNR_RESET_THRES) {

        snprintf(bufferSDR, sizeof(bufferSDR),
          "%.3f  G%02d resetting with SNR of %.1f and flagacq of %d\n",
           sdrstat.elapsedTime, sdr->prn, snr, sdr->flagacq);
        add_message(bufferSDR);

        // Reset struct terms
        int i = sdr->prn - 1;
        ret = resetStructs(&sdrch[i]);
        elapsed_acq_time = 0; // reset elapsed acq time
        if (ret==-1) { printf("resetStructs: error\n"); }

        // Continue to next iteration of while loop
        continue;

      } // end if
    } // end if

    // Check to see if tracking and nav decode is successful.
    // Reset channel if not. Make sure GPS week is near current week (and
    // thus non-zero).
    // FIX: May want to not hard-code specific week for GPS week !!
    if (elapsed_acq_time>60) {

      // Check several nav flags
      if (!sdr->nav.flagdec ||
          !sdr->nav.flagsync ||
          (sdr->nav.sdreph.week_gpst<GPS_WEEK) ) {

        snprintf(bufferSDR, sizeof(bufferSDR),
          "%.3f  G%02d resetting, flagdec:%d, flagsync:%d, Week:%d\n",
               sdrstat.elapsedTime, sdr->prn, sdr->nav.flagdec, sdr->nav.flagsync,
               sdr->nav.sdreph.week_gpst);
        add_message(bufferSDR);

        // Reset struct terms
        int i = sdr->prn - 1;
        ret = resetStructs(&sdrch[i]);
        elapsed_acq_time = 0; // reset elapsed acq time
        if (ret==-1) { printf("resetStructs: error\n"); }

        // Break out of while loop
        continue;
      }  // end if
    } // end if

    // Check SV elevation
    if (elapsed_acq_time>60) {
      // Pull values
      mlock(hobsmtx);
      int i = sdr->prn - 1;
      el = sdrstat.obs_v[i*11+10];
      unmlock(hobsmtx);

      // Check several nav flags
      if (el < SV_EL_RESET_MASK) {

        snprintf(bufferSDR, sizeof(bufferSDR),
          "%.3f  G%02d resetting, SV el: %.1f\n",
          sdrstat.elapsedTime, i+1, el);
        add_message(bufferSDR);

        // Reset struct terms
        int i = sdr->prn - 1;
        ret = resetStructs(&sdrch[i]);
        elapsed_acq_time = 0; // reset elapsed acq time
        if (ret==-1) { printf("resetStructs: error\n"); }

        // Break out of while loop
        continue;
      }  // end if
    } // end if

    // Check if mismatch between flagacq setting and obs use for pvt
    //checkObsDelay(sdr->prn);

    // Acquisition --------------------------------------------------------
    if (!sdr->flagacq) {
      // memory allocation
      if (acqpower!=NULL) free(acqpower);
      acqpower=(double*)calloc(sizeof(double),sdr->nsamp*sdr->acq.nfreq);

      // fft correlation
      buffloc=sdraqcuisition(sdr,acqpower);

      // Start timer. Note that this gets reset every time if flagacq = 0,
      // but doesn't get called when flagacq is 1.
      start_acq_timer = time(NULL);
    }

    // Tracking -----------------------------------------------------------
    if (sdr->flagacq) {
      bufflocnow=sdrtracking(sdr,buffloc,cnt);
      if (sdr->flagtrk) {

        // correlation output accumulation
        cumsumcorr(&sdr->trk,sdr->nav.ocode[sdr->nav.ocodei]);

        sdr->trk.flagloopfilter=0;
        if (!sdr->nav.flagsync) {
          pll(sdr,&sdr->trk.prm1,sdr->ctime);
          dll(sdr,&sdr->trk.prm1,sdr->ctime);
          sdr->trk.flagloopfilter=1;
        }
        else if (sdr->nav.swloop) {
          pll(sdr,&sdr->trk.prm2,(double)sdr->trk.loopms/1000);
          dll(sdr,&sdr->trk.prm2,(double)sdr->trk.loopms/1000);
          sdr->trk.flagloopfilter=2;

          mlock(hobsmtx);
          // calculate observation data
          if (loopcnt%(SNSMOOTHMS/sdr->trk.loopms)==0) {
            setobsdata(sdr,buffloc,cnt,&sdr->trk,1);
          } else {
            setobsdata(sdr,buffloc,cnt,&sdr->trk,0);
          }
          unmlock(hobsmtx);

          // Increment loop counter
          loopcnt++;

        } // end else if

        if (sdr->trk.flagloopfilter) clearcumsumcorr(&sdr->trk);
        cnt++;
        buffloc+=sdr->currnsamp;

      } // end tracking if (flagtrk)
    } // end tracking if (flagacq)

    sdr->trk.buffloc=buffloc;
  } // end while

  // Thread finished
  if (sdr->flagacq) {
    SDRPRINTF("SDR channel %s thread finished! Delay=%d [ms]\n",
               sdr->satstr,(int)(bufflocnow-buffloc)/sdr->nsamp);
  } else {
    SDRPRINTF("SDR channel %s thread finished!\n",sdr->satstr);
  }

  return THRETVAL;
}

//-----------------------------------------------------------------------------
// Data grabber thread
//-----------------------------------------------------------------------------
extern void *datathread(void *arg)
{
    // start grabber
    if (rcvgrabstart(&sdrini)<0) {
        quitsdr(&sdrini,4);
    }

    // data grabber loop
    while (!sdrstat.stopflag) {
        if (rcvgrabdata(&sdrini)<0) {
            sdrstat.stopflag=ON;
        }
    } // end while loop

    return THRETVAL;
}

//-----------------------------------------------------------------------------
// Reset structures for sdrch
//-----------------------------------------------------------------------------
extern int resetStructs(void *arg)
{
  // Declare channel struct to reset
  sdrch_t *sdr=(sdrch_t*)arg;

  mlock(hobsvecmtx);
  // Set prn
  int prn = sdr->prn;
  int i = prn-1;
  char bufferReset[MSG_LENGTH];

  // Reset all values in sdrch[i]
  memset(&sdrch[i], 0, sizeof(sdrch_t));

  // Reset sdrstat flags (may be better to use nav timer by channel)
  sdrstat.azElCalculatedflag = 0;

  // Initialize channel
  if (initsdrch(i+1,sdrini.sys[i],sdrini.prn[i],sdrini.ctype[i],
      sdrini.dtype[sdrini.ftype[i]-1],sdrini.ftype[i],
      sdrini.f_gain[sdrini.ftype[i]-1],sdrini.f_bias[sdrini.ftype[i]-1],
      sdrini.f_clock[sdrini.ftype[i]-1],sdrini.f_cf[sdrini.ftype[i]-1],
      sdrini.f_sf[sdrini.ftype[i]-1],sdrini.f_if[sdrini.ftype[i]-1],
      &sdrch[i])<0) {

      SDRPRINTF("error: initsdrch call in resetStructs\n");
      quitsdr(&sdrini,2);
      //return;
  }
  unmlock(hobsvecmtx);

  // Announce channel reset
  snprintf(bufferReset, sizeof(bufferReset),
     "%.3f  resetStructs: G%02d channel has been reset and will reacquire in 10s",
     sdrstat.elapsedTime, prn);
  add_message(bufferReset);

  // Pause a bit before continuing to reacquire
  sleepms(10000);

  return 0;
}

//-----------------------------------------------------------------------------
// Checks sdrch channel to see if channel has been acquired (flagacq) for more
// than 90s, but valid obs (obsValid_v) are not produced for PVT. If no
// valid obs in 90s, the channel is reset.
//-----------------------------------------------------------------------------
extern int checkObsDelay(int prn)
{
  // Initialize parameters
  int i = prn-1;
  int resetFlag = 0;
  int ret = 0;
  char bufferReset[MSG_LENGTH];

  // Check to see if there is an obs for this PRN in obs_v. If not, leave
  // resetFlag equal to 1 and reset channel.
  mlock(hobsvecmtx);
  int nsat = sdrstat.nsatValid;
  if (sdrch[i].flagacq==1) {
    if (sdrch[i].elapsed_time_nav>90) {
      resetFlag = 1;
      for (int j=0;j<nsat;j++) {
        if (prn==sdrstat.obsValidList[j]) {
          resetFlag = 0;  // Valid obs found, so don't reset
        }
      } // end for
    } // end if
  } // end if
  unmlock(hobsvecmtx);

  // Reset the channel if mismatch between
  if (resetFlag) {
    //printf("checkObsDelay: resetting G%02d due to mismatch\n", prn);
    snprintf(bufferReset, sizeof(bufferReset),
     "%.3f  checkObsDelay: resetting G%02d due to mismatch",
     sdrstat.elapsedTime, prn);
    add_message(bufferReset);

    ret = resetStructs(&sdrch[i]);
    if (ret==-1) { printf("resetStructs: error\n"); }
  }

  return 0;
}