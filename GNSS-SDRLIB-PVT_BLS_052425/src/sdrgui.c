//----------------------------------------------------------------------------
//
//  sdrgui.c
//
//  Copyright 2025, Don Kelly, don.kelly@mac.com
//----------------------------------------------------------------------------

#include "sdr.h"

//-----------------------------------------------------------------------------
// Supporting GUI functions
//-----------------------------------------------------------------------------

extern void add_message(const char *msg)
{
  pthread_mutex_lock(&hmsgmtx);
  if (sdrgui.message_count < MAX_MESSAGES) {
      sdrgui.messages[sdrgui.message_count++] = strdup(msg);
  } else {
    free(sdrgui.messages[0]);
    for (int i = 1; i < MAX_MESSAGES; i++) {
      sdrgui.messages[i - 1] = sdrgui.messages[i];
    }
      sdrgui.messages[MAX_MESSAGES - 1] = strdup(msg);
  }
  pthread_mutex_unlock(&hmsgmtx);
}

/*
// This function may be used to stream test messages to the GUI
extern void *message_producer(void *arg)
{
  int counter = 1;
  char buffer[MSG_LENGTH];

  while (1) {
    snprintf(buffer, sizeof(buffer), "Incoming message #%d", counter++);
    add_message(buffer);
    usleep(200000);  // Simulate external message streaming
  }

  return NULL;
}
*/

extern void updateNavStatusWin(WINDOW *win1, int counter)
{
  // Pull data from sdrstat and sdrch
  int prn[32] = {0};
  int flagacq[32] = {0};
  int flagsync[32] = {0};
  int flagdec[32] = {0};
  int nsat = 0;
  double lat = 0.0;
  double lon = 0.0;
  double hgt = 0.0;
  double gdop = 0.0;
  double clkBias = 0.0;
  double obs_v[MAXSAT*11] = {0.0};
  double vk1_v[MAXSAT] = {0.0};
  double rk1_v[MAXSAT] = {0.0};
  int gps_week;
  double gps_tow;
  char bufferNav[256];
  char str1[10];

  // Load in data to display
  mlock(hobsvecmtx);
  for (int i=0; i<32; i++) {
    prn[i] = sdrch[i].prn;
    flagacq[i] = sdrch[i].flagacq;
    flagsync[i] = sdrch[i].nav.flagsync;
    flagdec[i] = sdrch[i].nav.flagdec;
  }
  nsat = sdrstat.nsatValid;
  lat = sdrstat.lat;
  lon = sdrstat.lon;
  hgt = sdrstat.hgt;
  gdop = sdrstat.gdop;
  clkBias = sdrstat.xyzdt[3];
  int numIter = 32*11;
  for (int m=0; m<numIter; m++) {
    obs_v[m] = sdrstat.obs_v[m];
  }
  for (int n=0; n<32; n++) {
    vk1_v[n] = sdrstat.vk1_v[n];
    rk1_v[n] = sdrekf.rk1_v[n];
  }
  gps_tow = sdrstat.obs_v[(sdrstat.obsValidList[0]-1)*11+6] ;
  gps_week = (int)sdrstat.obs_v[(sdrstat.obsValidList[0]-1)*11+7];
  unmlock(hobsvecmtx);

  // Correct rcvr TOW with rcvr clock bias for precise UTC
  time_t utc_time_seconds = gps_to_utc(gps_week, gps_tow+clkBias/CTIME);
  struct tm utc_tm;
  gmtime_r(&utc_time_seconds, &utc_tm);
  sprintf(bufferNav,"UTC Time:       %04d-%02d-%02d %02d:%02d:%02d.%03d",
     utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday,
     utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec, (int)(gps_tow * 1000) % 1000);

  // Nav status window update
  // Clear and redraw box
  werase(win1);
  box(win1, 0, 0);
  wattron(win1,A_BOLD);
  mvwprintw(win1, 0, 5, " Navigation Status ");
  wattroff(win1,A_BOLD);

  // Update elapsed time and UTC
  mvwprintw(win1, 1, 2, "Elapsed Time:   %.3f", sdrstat.elapsedTime);
  mvwprintw(win1, 2, 2, "%s", bufferNav);

  // Update filter mode
  if (sdrini.ekfFilterOn) {
    mvwprintw(win1, 1, 70, "Filter Mode: Kalman (EKF)");
  } else {
    mvwprintw(win1, 1, 70, "Filter Mode: Least Squares (WLS)");
  }

  // Update acquired SVs
  sprintf(bufferNav, "Acquired SVs:   ");
  for (int i=0; i<32; i++) {
    if (flagacq[i] ==1) {
      sprintf(str1, "%02d  ", prn[i]);
      strcat(bufferNav, str1);
    }
  }
  mvwprintw(win1, 4, 2, "%s", bufferNav);

  // Update tracked SVs
  sprintf(bufferNav, "Tracked SVs:    ");
  for (int i=0; i<32; i++) {
    if (flagsync[i] ==1) {
      sprintf(str1, "%02d  ", prn[i]);
      strcat(bufferNav, str1);
    }
  }
  mvwprintw(win1, 5, 2, "%s", bufferNav);

  // Update nav decoded SVs
  sprintf(bufferNav, "NavDecoded SVs: ");
  for (int i=0; i<32; i++) {
    if (flagdec[i] ==1) {
      sprintf(str1, "%02d  ", prn[i]);
      strcat(bufferNav, str1);
    }
  }
  mvwprintw(win1, 6, 2, "%s", bufferNav);

  // Update LLA data
  sprintf(bufferNav, "Lat: %.7f  Lon: %.7f  Alt: %.1f  GDOP: %.2f  CB: %.5e  SVs: %02d",
  lat, lon, hgt, gdop, clkBias/CTIME, nsat);
  mvwprintw(win1, 8, 2, "%s", bufferNav);

  // Display Obs data for all valid SVs once it is calculated
  for (int i=0; i<nsat; i++) {
    int prn = sdrstat.obsValidList[i];
    sprintf(bufferNav, "G%02d  TOW=%.1f  Week=%d  SNR=%.1f  PR=%.1f  Az = %05.1f  EL=%04.1f  rk1=%05.1f  vk1=%7.1f",
      (int)obs_v[(prn-1)*11+0],
      obs_v[(prn-1)*11+6],
      (int)obs_v[(prn-1)*11+7],
      obs_v[(prn-1)*11+8],
      obs_v[(prn-1)*11+5],
      obs_v[(prn-1)*11+9],
      obs_v[(prn-1)*11+10],
      rk1_v[(prn-1)],
      vk1_v[(prn-1)]);
    mvwprintw(win1, 10+i, 2, "%s", bufferNav);
  }

  // Refresh win1
  wrefresh(win1);
}

extern void updateProgramStatusWin(WINDOW *win2, int hgt2)
{
  pthread_mutex_lock(&hmsgmtx);

  // Clear win, add messages, draw a boundary box, and label
  werase(win2);
  int start = (sdrgui.message_count > hgt2 - 2) ? sdrgui.message_count - (hgt2 - 2) : 0;
  int y = 1;
  for (int i = start; i < sdrgui.message_count; i++) {
    mvwprintw(win2, y++, 2, "%s", sdrgui.messages[i]);
  }
  box(win2, 0, 0);
  wattron(win2,A_BOLD);
  mvwprintw(win2, 0, 5, " Program Status ");
  wattroff(win2,A_BOLD);

  pthread_mutex_unlock(&hmsgmtx);
  wrefresh(win2);
}
