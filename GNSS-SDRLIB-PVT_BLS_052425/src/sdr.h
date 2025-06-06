//-----------------------------------------------------------------------------
// sdr.h : constants, types and function prototypes
//
// Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
// Edits from Don Kelly, don.kelly@mac.com, 2025
//-----------------------------------------------------------------------------
#ifndef SDR_H
#define SDR_H

#define _CRT_SECURE_NO_WARNINGS
#define _GNU_SOURCE  // Need this before includes for CPU affinity

// Include system level libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h> // needed for usleep()

// SIMD (SSE2_ENABLE)
#if defined(SSE2_ENABLE)
  #include <emmintrin.h>
  #include <tmmintrin.h>
#endif

// SIMD (AVX_ENABLE/AVX2_ENABLE)
#if defined(AVX_ENABLE)||defined(AVX2_ENABLE)
  #include <immintrin.h>
#endif

// Include additional system level libraries
#include <sys/socket.h>
#include <stdbool.h>
#include <inttypes.h>
#include <fec.h>
#include <fftw3.h>
#include "rtklib.h"
#include <libusb-1.0/libusb.h>
#include "ansiColorCodes.h"
#include <pthread.h>
#include <ncurses.h>

// Includes for BladeRF SDR
#ifdef BLADERF
  #include <libbladeRF.h>
  #include "bladeRF.h"
#endif

// Includes for RTLSDR SDR
#ifdef RTLSDR
  #include "rtl-sdr.h"
#endif

#define SDRPRINTF printf

#ifdef __cplusplus
  extern "C" {
#endif

// constants ------------------------------------------------------------------
#define ROUND(x)      ((int)floor((x)+0.5)) // round function  
#define PI            3.1415926535897932 // pi  
#define DPI           (2.0*PI)         // 2*pi  
#define D2R           (PI/180.0)       // degree to radian  
#define R2D           (180.0/PI)       // radian to degree  
#define CLIGHT        299792458.0      // speed of light (m/s)  
#define ON            1                // flag ON  
#define OFF           0                // flag OFF  
#define MAXBITS       3000             // maximum bit length  

// sdrpvt.c constants (need to eliminate duplicates)
#define GPS_PI                  3.1415926535897932
#define MU                      3.986005e14
#define OMEGAEDOT               7.2921151467e-5
#define CTIME				    2.99792458e8

// front end setting
#define FEND_RTLSDR   3                // front end type: RTL-SDR  
#define FEND_BLADERF  4                // front end type: Nuand BladeRF  
#define FEND_FRTLSDR  8                // front end type: RTL-SDR binary file  
#define FEND_FBLADERF 9                // front end type: BladeRF binary file  
#define FEND_FILE     10               // front end type: IF file  
#define FTYPE1        1                // front end number  
#define FTYPE2        2                // front end number  
#define DTYPEI        1                // sampling type: real  
#define DTYPEIQ       2                // sampling type: real+imag  

#define MEMBUFFLEN    5000             // number of temporary buffer  

#define FILE_BUFFSIZE 65536            // buffer size for post processing  

// acquisition setting
#define NFFTTHREAD    4                // number of thread for executing FFT  
#define ACQINTG_L1CA  10               // number of non-coherent integration  
#define ACQINTG_G1    10               // number of non-coherent integration  
#define ACQINTG_E1B   4                // number of non-coherent integration  
#define ACQINTG_B1I   10               // number of non-coherent integration  
#define ACQINTG_SBAS  10               // number of non-coherent integration  
#define ACQHBAND      7000             // half width for doppler search (Hz)  
#define ACQSTEP       200              // doppler search frequency step (Hz)  
#define ACQTH         3.0              // acquisition threshold (peak ratio)  
#define ACQSLEEP      2000             // acquisition process interval (ms)  

// tracking setting  
#define LOOP_L1CA     10               // loop interval  
#define LOOP_G1       10               // loop interval  
#define LOOP_E1B      1                // loop interval  
#define LOOP_B1I      10               // loop interval  
#define LOOP_B1IG     2                // loop interval  
#define LOOP_SBAS     2                // loop interval  
#define LOOP_LEX      4                // loop interval  

// navigation parameter  
#define NAVSYNCTH       50             // navigation frame synch. threshold  

// GPS/QZSS L1CA  
#define NAVRATE_L1CA    20             // length (multiples of ranging code)  
#define NAVFLEN_L1CA    300            // navigation frame data (bits)  
#define NAVADDFLEN_L1CA 2              // additional bits of frame (bits)  
#define NAVPRELEN_L1CA  8              // preamble bits length (bits)  
#define NAVEPHCNT_L1CA  3              // number of eph. contained frame  

// L1 SBAS/QZSS L1SAIF  
#define NAVRATE_SBAS    2              // length (multiples of ranging code)  
#define NAVFLEN_SBAS    1500           // navigation frame data (bits)  
#define NAVADDFLEN_SBAS 12             // additional bits of frame (bits)  
#define NAVPRELEN_SBAS  16             // preamble bits length (bits)  
#define NAVEPHCNT_SBAS  3              // number of eph. contained frame  

// observation data generation  
#define PTIMING       68.802           // pseudo range generation timing (ms)  
#define OBSINTERPN    80               // # of obs. stock for interpolation  
#define SNSMOOTHMS    100              // SNR smoothing interval (ms)  

// code generation parameter  
#define MAXGPSSATNO   210              // max satellite number  
#define MAXGALSATNO   50               // max satellite number  
#define MAXCMPSATNO   37               // max satellite number  

// code type  
#define CTYPE_L1CA    1                // GPS/QZSS L1C/A  
#define CTYPE_L1CP    2                // GPS/QZSS L1C Pilot  
#define CTYPE_L1CD    3                // GPS/QZSS L1C Data  
#define CTYPE_L1CO    4                // GPS/QZSS L1C overlay  
#define CTYPE_L1SBAS  27               // SBAS compatible L1CA  
#define CTYPE_NH10    28               // 10 bit Neuman-Hoffman code  
#define CTYPE_NH20    29               // 20 bit Neuman-Hoffman code  

// SBAS/QZSS L1-SAIF setting  
#define LENSBASMSG    32               // SBAS message length 150/8 (byte)  
#define LENSBASNOV    80               // message length in NovAtel format  

// Pseudorange thresholds. Acceptable ranges are typically 60 to 92ms,
// or 64 to 85ms if tighter limits are desired.
#define LOW_PR        00e-3 * CTIME // default 60e-3*CTIME
#define HIGH_PR       92e-3 * CTIME

// SNR thresholds
#define SNR_RESET_THRES		15  	// Threshold to reset sdrch channel
#define SNR_PVT_THRES 		19      // Threshold to use obs for PVT

// The sdrthread function uses this as a check to make sure GPS week
// is non-zero, so just needs to be about right. Might automate this
// at some point.
#define GPS_WEEK      2360

// GUI update rate
//#define GUI_RATE	  200  // GUI update rate in ms

// GPS Epoch (1980-01-06 00:00:00 UTC)
#define GPS_EPOCH_SECONDS 315964800

// SV elevation mask parameters (used in precheckObs function)
#define ET_TIMER  	60.0
#define SV_EL_PVT_MASK    15.0  // 15 default
#define SV_EL_RESET_MASK  12.0  // 12 default

// GUI parameters
#define MAX_MESSAGES 100  // Message buffer
#define MSG_LENGTH   128  // Max message length
#define PUR1 1
#define PUR2 2

// thread functions  
#define mlock_t       pthread_mutex_t
#define initmlock(f)  pthread_mutex_init(&f,NULL)
#define mlock(f)      pthread_mutex_lock(&f)
#define unmlock(f)    pthread_mutex_unlock(&f)
#define delmlock(f)   pthread_mutex_destroy(&f)
#define event_t       pthread_cond_t
#define initevent(f)  pthread_cond_init(&f,NULL)
#define setevent(f)   pthread_cond_signal(&f)
#define waitevent(f,m) pthread_cond_wait(&f,&m)
#define delevent(f)   pthread_cond_destroy(&f)
#define waitthread(f) pthread_join(f,NULL)
#define cratethread(f,func,arg) pthread_create(&f,NULL,func,arg)
#define THRETVAL      NULL

// type definition ----------------------------------------------------------- 
typedef fftwf_complex cpx_t; // complex type for fft  

// sdr initialization struct  
typedef struct {
        int fend;        // front end type  
        int f_gain[2]; // front end rx gain  
        int f_bias[2]; // front end bias-tee  
        int f_clock[2]; // front end clock ref  
        double f_cf[2];  // center frequency (Hz)  
        double f_sf[2];  // sampling frequency (Hz)  
        double f_if[2];  // intermediate frequency (Hz)  
        int dtype[2];    // data type (DTYPEI/DTYPEIQ)  
        FILE *fp1;       // IF1 file pointer  
        FILE *fp2;       // IF2 file pointer  
        char file1[1024]; // IF1 file path  
        char file2[1024]; // IF2 file path  
        char fontfile[1024]; // font file path, DK added
        int useif1;      // IF1 flag  
        int useif2;      // IF2 flag  
        int nch;         // number of sdr channels  
        int nchL1;       // number of L1 channels  
        int nchL2;       // number of L2 channels  
        int nchL5;       // number of L5 channels  
        int nchL6;       // number of L6 channels  
        int prn[MAXSAT]; // PRN of channels  
        int sys[MAXSAT]; // satellite system type of channels (SYS_*)  
        int ctype[MAXSAT]; // code type of channels (CTYPE_* ) 
        int ftype[MAXSAT]; // front end type of channels (FTYPE1/FTYPE2)  
        int pltacq;      // plot acquisition flag  
        int plttrk;      // plot tracking flag  
        int pltspec;     // plot spectrum flag  
        int outms;       // output interval (ms)  
        int sbas;        // SBAS/QZSS L1SAIF output flag  
        int snrThreshold; // SNR threshold
        int xu0_v[3];     // Initial receiver location (ECEF)
        int sbasport;    // SBAS/L1-SAIF TCP/IP port  
        int trkcorrn;    // number of correlation points  
        int trkcorrd;    // interval of correlation points (sample)  
        int trkcorrp;    // correlation points (sample)  
        double trkdllb[2]; // dll noise bandwidth (Hz)  
        double trkpllb[2]; // pll noise bandwidth (Hz)  
        double trkfllb[2]; // fll noise bandwidth (Hz)  
        int rtlsdrppmerr; // clock collection for RTL-SDR  
        int ekfFilterOn;  // flag to run EKF (rather than BLS)
} sdrini_t;

// sdr current state struct  
typedef struct {
        int stopflag;    // stop flag  
        int resetsyncthreadflag;   // DK added
        int specflag;    // spectrum flag  
        int buffsize;    // data buffer size  
        int fendbuffsize; // front end data buffer size  
        unsigned char *buff; // IF data buffer  
        unsigned char *buff2;// IF data buffer (for file input)  
        unsigned char *tmpbuff; // USB temporary buffer (for STEREO_V26)  
        uint64_t buffcnt; // current buffer location  
        int printflag; // DK added, flag for printing obs and nav file
        double lat;
        double lon;
        double hgt;
        double gdop;
        int nsat;
        int nsatRaw;
        int nsatValid;
        int satList[MAXSAT];
        int obsValidList[MAXSAT];
        double obs_v[11*MAXSAT]; // Array to hold all obs data
        double vk1_v[MAXSAT]; // Estimator residuals
        double clkBias;
        double xyzdt[4];
        double elapsedTime;
        int azElCalculatedflag;
} sdrstat_t;

// sdr observation struct  
typedef struct {
        int prn;         // PRN  
        int sys;         // satellite system  
        double tow;      // time of week (s)  
        int week;        // week number  
        double P;        // pseudo range (m)  
        double L;        // carrier phase (cycle)  
        double D;        // doppler frequency (Hz)  
        double S;        // SNR (dB-Hz)  
} sdrobs_t;

// sdr acquisition struct  
typedef struct {
        int intg;        // number of integration  
        double hband;    // half band of search frequency (Hz)  
        double step;     // frequency search step (Hz)  
        int nfreq;       // number of search frequency  
        double *freq;    // search frequency (Hz)  
        int acqcodei;    // acquired code phase  
        int freqi;       // acquired frequency index  
        double acqfreq;  // acquired frequency (Hz)  
        int nfft;        // number of FFT points  
        double cn0;      // signal C/N0  
        double peakr;    // first/second peak ratio  
} sdracq_t;

// sdr tracking parameter struct  
typedef struct {
        double pllb;     // noise bandwidth of PLL (Hz)  
        double dllb;     // noise bandwidth of DLL (Hz)  
        double fllb;     // noise bandwidth of FLL (Hz)  
        double dllw2;    // DLL coefficient  
        double dllaw;    // DLL coefficient  
        double pllw2;    // PLL coefficient  
        double pllaw;    // PLL coefficient  
        double fllw;     // FLL coefficient  
} sdrtrkprm_t;

// sdr tracking struct  
typedef struct {
        double codefreq; // code frequency (Hz)  
        double carrfreq; // carrier frequency (Hz)  
        double remcode;  // remained code phase (chip)  
        double remcarr;  // remained carrier phase (rad)  
        double oldremcode; // previous remained code phase (chip)  
        double oldremcarr; // previous remained carrier phase (chip)  
        double codeNco;  // code NCO  
        double codeErr;  // code tracking error  
        double carrNco;  // carrier NCO  
        double carrErr;  // carrier tracking error  
        double freqErr;  // frequencyr error in FLL  
        uint64_t buffloc; // current buffer location  
        double tow[OBSINTERPN]; // time of week (s)  
        uint64_t codei[OBSINTERPN]; // code phase (sample)  
        uint64_t codeisum[OBSINTERPN]; // code phase for SNR computation (sample)  
        uint64_t cntout[OBSINTERPN]; // loop counter  
        double remcout[OBSINTERPN]; // remained code phase (chip) 
        double L[OBSINTERPN];// carrier phase (cycle)  
        double D[OBSINTERPN];// doppler frequency (Hz)  
        double S[OBSINTERPN];// signal to noise ratio (dB-Hz)  
        double *II;      // correlation (in-phase)  
        double *QQ;      // correlation (quadrature-phase)  
        double *oldI;    // previous correlation (I-phase)  
        double *oldQ;    // previous correlation (Q-phase)  
        double *sumI;    // integrated correlation (I-phase)  
        double *sumQ;    // integrated correlation (Q-phase)  
        double *oldsumI; // previous integrated correlation (I-phase)  
        double *oldsumQ; // previous integrated correlation (Q-phase)  
        double Isum;     // correlation for SNR computation (I-phase)  
        int loop;        // loop filter interval  
        int loopms;      // loop filter interval (ms)  
        int flagpolarityadd; // polarity (half cycle ambiguity) add flag  
        int flagremcarradd; // remained carrier phase add flag  
        int flagloopfilter; // loop filter update flag  
        int corrn;       // number of correlation points  
        int *corrp;      // correlation points (sample)  
        double *corrx;   // correlation points (for plotting)  
        int ne,nl;       // early/late correlation point  
        sdrtrkprm_t prm1; // tracking parameter struct  
        sdrtrkprm_t prm2; // tracking parameter struct  
} sdrtrk_t;

// sdr ephemeris struct  
typedef struct {
        eph_t eph;       // GPS/QZS/GAL/COM ephemeris struct (from rtklib.h)  
        //geph_t geph;     // GLO ephemeris struct (from rtklib.h)  
        int ctype;       // code type  
        double tow_gpst; // ephemeris tow in GPST  
        int week_gpst;   // ephemeris week in GPST  
        int cnt;         // ephemeris decode counter  
        int cntth;       // ephemeris decode count threshold  
        int update;      // ephemeris update flag  
        int prn;         // PRN  
        int tk[3],nt,n4,s1cnt; // temporary variables for decoding GLONASS  
        double toc_gst;  // temporary variables for decoding Galileo  
        int week_gst;
} sdreph_t;

// sdr SBAS struct  
typedef struct {
        unsigned char msg[LENSBASMSG]; // SBAS message (250bits/s)  
        unsigned char novatelmsg[LENSBASNOV]; // NovAtel format (80bytes/s)  
        int id;          // message ID  
        int week;        // GPS week  
        double tow;      // GPS time of week (s)  
} sdrsbas_t;

// sdr navigation struct  
typedef struct {
        FILE *fpnav;     // for navigation bit logging  
        int ctype;       // code type  
        int rate;        // navigation data rate (ms)  
        int flen;        // frame length (bits)  
        int addflen;     // additional frame bits (bits)  
        int prebits[32]; // preamble bits  
        int prelen;      // preamble bits length (bits)  
        int bit;         // current navigation bit  
        int biti;        // current navigation bit index  
        int cnt;         // navigation bit counter for synchronization  
        double bitIP;    // current navigation bit (IP data)  
        int *fbits;      // frame bits  
        int *fbitsdec;   // decoded frame bits  
        int update;      // decode interval (ms)  
        int *bitsync;    // frame bits synchronization count  
        int synci;       // frame bits synchronization index  
        uint64_t firstsf; // first subframe location (sample)  
        uint64_t firstsfcnt; // first subframe count  
        double firstsftow; // tow of first subframe  
        int polarity;    // bit polarity  
        int flagpol;     // bit polarity flag (only used in L1-SAIF)  
        void *fec;       // FEC (fec.h)   
        short *ocode;    // overlay code (secondary code)  
        int ocodei;      // current overray code index  
        int swsync;      // switch of frame synchronization (last nav bit)  
        int swreset;     // switch of frame synchronization (first nav bit)  
        int swloop;      // switch of loop filter  
        int flagsync;    // navigation bit synchronization flag  
        int flagsyncf;   // navigation frame synchronization (preamble found)  
        int flagtow;     // first subframe found flag  
        int flagdec;     // navigation data decoded flag  
        sdreph_t sdreph; // sdr ephemeris struct  
        sdrsbas_t sbas;  // SBAS message struct  
} sdrnav_t;

// sdr channel struct  
typedef struct {
        thread_t hsdr;   // thread handle  
        int no;          // channel number  
        int sat;         // satellite number  
        int sys;         // satellite system  
        int prn;         // PRN  
        char satstr[5];  // PRN string  
        int ctype;       // code type  
        int dtype;       // data type  
        int ftype;       // front end type  
        double f_cf;     // carrier frequency (Hz)  
        double f_sf;     // sampling rate (Hz)  
        double f_if;     // intermediate frequency (Hz)  
        int f_gain;  // front end rx gain  
        int f_bias;  // front end bias-tee  
        int f_clock; // front end clock ref  
        double foffset;  // frequency offset (Hz)  
        short *code;     // original code  
        cpx_t *xcode;    // resampled code in frequency domain  
        int clen;        // code length  
        double crate;    // code chip rate (Hz)  
        double ctime;    // code period (s)  
        double ti;       // sampling interval (s)  
        double ci;       // chip interval (s)  
        int nsamp;       // number of samples in one code (doppler=0Hz)  
        int currnsamp;   // current number of samples in one code  
        int nsampchip;   // number of samples in one code chip (doppler=0Hz)  
        sdracq_t acq;    // acquisition struct  
        sdrtrk_t trk;    // tracking struct  
        sdrnav_t nav;    // navigation struct  
        int flagacq;     // acquisition flag  
        int flagtrk;     // tracking flag  
        double elapsed_time_snr;
        double elapsed_time_nav;
} sdrch_t;

// EKF struct
typedef struct {
  double rk1_v[MAXSAT];
  double varR;
} sdrekf_t;

// GUI struct
typedef struct {
  int message_count;
  char *messages[100];
} sdrgui_t;

// global variables -----------------------------------------------------------
extern thread_t hmainthread;  // main thread handle  
extern thread_t hsyncthread;  // synchronization thread handle  
extern thread_t hkeythread;   // keyboard thread handle  
extern thread_t hdatathread;   // keyboard thread handle  
extern thread_t hserverthread;   // server thread  
extern thread_t hmsgthread;   // GUI messages thread  

extern mlock_t hbuffmtx;      // buffer access mutex  
extern mlock_t hreadmtx;      // buffloc access mutex  
extern mlock_t hfftmtx;       // fft function mutex  
extern mlock_t hobsmtx;       // observation data access mutex  
extern mlock_t hresetmtx;     // sdr channel reset flag mutex  
extern mlock_t hobsvecmtx;    // observation vector access mutex  
extern mlock_t hmsgmtx;       // messages access mutex  

extern sdrini_t sdrini;       // sdr initialization struct  
extern sdrstat_t sdrstat;     // sdr state struct  
extern sdrch_t sdrch[MAXSAT]; // sdr channel structs  
extern sdrekf_t sdrekf;       // sdr EKF struct
extern sdrgui_t sdrgui;       // GUI

// sdrmain.c ------------------------------------------------------------------
extern void startsdr(void);
extern void quitsdr(sdrini_t *ini, int stop);
extern void *sdrthread(void *arg);
extern void *datathread(void *arg);
extern int resetStructs(void *arg);
extern int checkObsDelay(int prn);

// sdrsync.c ------------------------------------------------------------------
extern void *syncthread(void * arg);

// sdracq.c -------------------------------------------------------------------
extern uint64_t sdraqcuisition(sdrch_t *sdr, double *power);
extern int checkacquisition(double *P, sdrch_t *sdr);

// sdrtrk.c -------------------------------------------------------------------
extern uint64_t sdrtracking(sdrch_t *sdr, uint64_t buffloc, uint64_t cnt);
extern void cumsumcorr(sdrtrk_t *trk, int polarity);
extern void clearcumsumcorr(sdrtrk_t *trk);
extern void pll(sdrch_t *sdr, sdrtrkprm_t *prm, double dt);
extern void dll(sdrch_t *sdr, sdrtrkprm_t *prm, double dt);
extern void setobsdata(sdrch_t *sdr, uint64_t buffloc, uint64_t cnt,
                       sdrtrk_t *trk, int flag);

// sdrinit.c ------------------------------------------------------------------
extern int readinifile(sdrini_t *ini);
extern int chk_initvalue(sdrini_t *ini);
extern void openhandles(void);
extern void closehandles(void);
extern void initacqstruct(int sys, int ctype, int prn, sdracq_t *acq);
extern void inittrkprmstruct(sdrtrk_t *trk);
extern int inittrkstruct(int sat, int ctype, double ctime, sdrtrk_t *trk);
extern int initnavstruct(int sys, int ctype, int prn, sdrnav_t *nav);
extern int initsdrch(int chno, int sys, int prn, int ctype, int dtype,
                     int ftype, int f_gain, int f_bias, int f_clock,
                     double f_cf, double f_sf, double f_if,
                     sdrch_t *sdr);
extern void freesdrch(sdrch_t *sdr);

// sdrcmn.c -------------------------------------------------------------------
extern int getfullpath(char *relpath, char *abspath);
extern unsigned long tickgetus(void);
extern void sleepus(int usec);
extern void settimeout(struct timespec *timeout, int waitms);
extern double log2(double n);
extern int calcfftnum(double x, int next);
extern void *sdrmalloc(size_t size);
extern void sdrfree(void *p);
extern cpx_t *cpxmalloc(int n);
extern void cpxfree(cpx_t *cpx);
extern void cpxfft(fftwf_plan plan, cpx_t *cpx, int n);
extern void cpxifft(fftwf_plan plan, cpx_t *cpx, int n);
extern void cpxcpx(const short *II, const short *QQ, double scale, int n,
                   cpx_t *cpx);
extern void cpxcpxf(const float *II, const float *QQ, double scale,  int n,
                    cpx_t *cpx);
extern void cpxconv(fftwf_plan plan, fftwf_plan iplan, cpx_t *cpxa, cpx_t *cpxb,
                    int m, int n, int flagsum, double *conv);
extern void cpxpspec(fftwf_plan plan, cpx_t *cpx, int n, int flagsum,
                     double *pspec);
extern void dot_21(const short *a1, const short *a2, const short *b, int n,
                   double *d1, double *d2);
extern void dot_22(const short *a1, const short *a2, const short *b1,
                   const short *b2, int n, double *d1, double *d2);
extern void dot_23(const short *a1, const short *a2, const short *b1,
                   const short *b2, const short *b3, int n, double *d1,
                   double *d2);
extern double mixcarr(const char *data, int dtype, double ti, int n,
                      double freq, double phi0, short *II, short *QQ);
extern void mulvcs(const char *data1, const short *data2, int n, short *out);
extern void sumvf(const float *data1, const float *data2, int n, float *out);
extern void sumvd(const double *data1, const double *data2, int n, double *out);
extern int maxvi(const int *data, int n, int exinds, int exinde, int *ind);
extern float maxvf(const float *data, int n, int exinds, int exinde, int *ind);
extern double maxvd(const double *data, int n, int exinds, int exinde,int *ind);
extern double meanvd(const double *data, int n, int exinds, int exinde);
extern double interp1(double *x, double *y, int n, double t);
extern void uint64todouble(uint64_t *data, uint64_t base, int n, double *out);
extern void ind2sub(int ind, int nx, int ny, int *subx, int *suby);
extern void shiftdata(void *dst, void *src, size_t size, int n);
extern double rescode(const short *code, int len, double coff, int smax,
                      double ci, int n, short *rcode);
extern void pcorrelator(const char *data, int dtype, double ti, int n,
                        double *freq, int nfreq, double crate, int m,
                        cpx_t* codex, double *P);
extern void correlator(const char *data, int dtype, double ti, int n,
                       double freq, double phi0, double crate, double coff,
                       int* s, int ns, double *II, double *QQ, double *remc,
                       double *remp, short* codein, int coden);
extern int leap_seconds(long gps_seconds);
extern time_t gps_to_utc(int gps_week, double gps_tow);

// sdrcode.c ------------------------------------------------------------------
extern short *gencode(int prn, int ctype, int *len, double *crate);

// sdrpvt.c -------------------------------------------------------------------
extern int pvtProcessor(void);
extern int blsFilter(double *xs_v, double *pr_v, int numSat,
                 double xyzdt_v[], double *gdop);
extern void ecef2lla(double x, double y, double z,
                    double *latitude, double *longitude, double *height);
extern void check_t(double time, double *corrTime);
extern int satPos(sdreph_t *sdreph, double transmitTime, double svPos[3],
            double *svClkCorr);
extern void rot(double R[9], double angle, int axis);
extern void precheckObs();
extern int tropo(double sinel, double hsta, double p, double tkel,
                 double hum, double hp, double htkel, double hhum,
                 double *ddr);
extern int togeod(double a, double finv, double X, double Y, double Z,
                  double *dphi, double *dlambda, double *h);
extern int topocent(double X[], double dx[], double *Az, double *El, double *D);
extern int updateObsList(void);

// sdrekf.c -------------------------------------------------------------------

// sdrnav.c -------------------------------------------------------------------
extern void sdrnavigation(sdrch_t *sdr, uint64_t buffloc, uint64_t cnt);
extern uint32_t getbitu2(const uint8_t *buff, int p1, int l1, int p2, int l2);
extern int32_t getbits2(const uint8_t *buff, int p1, int l1, int p2, int l2);
extern uint32_t getbitu3(const uint8_t *buff, int p1, int l1, int p2, int l2,
                         int p3, int l3);
extern int32_t getbits3(const uint8_t *buff, int p1, int l1, int p2, int l2,
                        int p3, int l3);
extern uint32_t merge_two_u(const uint32_t a, const uint32_t b, int n);
extern int32_t merge_two_s(const int32_t a, const uint32_t b, int n);
extern void bits2byte(int *bits, int nbits, int nbin, int right, uint8_t *bin);
extern void interleave(const int *in, int row, int col, int *out);
extern int checksync(double IP, double IPold, sdrnav_t *nav);
extern int checkbit(double IP, int loopms, sdrnav_t *nav);
extern void predecodefec(sdrnav_t *nav);
extern int paritycheck(sdrnav_t *nav);
extern int findpreamble(sdrnav_t *nav);
extern int decodenav(sdrnav_t *nav);
extern void check_hamming(int *hamming, int n, int parity, int m);

// sdrnav_gps/gal/glo.c/sbs.c -------------------------------------------------
extern int decode_l1ca(sdrnav_t *nav);
extern int decode_e1b(sdrnav_t *nav);
extern int decode_g1(sdrnav_t *nav);
extern int decode_b1i(sdrnav_t *nav);
extern int decode_l1sbas(sdrnav_t *nav);
extern int paritycheck_l1ca(int *bits);

// sdrrcv.c -------------------------------------------------------------------
extern int rcvinit(sdrini_t *ini);
extern int rcvquit(sdrini_t *ini);
extern int rcvgrabstart(sdrini_t *ini);
extern int rcvgrabdata(sdrini_t *ini);
extern int rcvgetbuff(sdrini_t *ini, uint64_t buffloc, int n, int ftype,
                      int dtype, char *expbuf);
extern void file_pushtomembuf(void);
extern void file_getbuff(uint64_t buffloc, int n, int ftype, int dtype,
                         char *expbuf);

// sdrmsg.c -------------------------------------------------------------------
//extern void *clientthread(void *arg);
//extern void *serverthread(void *arg);

// sdrgui.c -------------------------------------------------------------------
extern void add_message(const char *msg);
extern void updateNavStatusWin(WINDOW *win1, int counter);
extern void updateProgramStatusWin(WINDOW *win2, int height);

#ifdef __cplusplus
}
#endif
#endif // SDR_H  
