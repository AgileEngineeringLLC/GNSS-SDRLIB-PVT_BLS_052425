//-----------------------------------------------------------------------------
// rtkcmn.c : rtklib common functions
//
// This is a greatly-reduced version of rtkcmn.c, pulling out only what is 
// needed for GNSS-SDRCLI
//------------------------------------------------------------------------------
#include "sdr.h"
#include <stdarg.h>
#include <ctype.h>

// Constants ------------------------------------------------------------------
#define POLYCRC32   0xEDB88320u // CRC32 polynomial     
#define POLYCRC24Q  0x1864CFBu  // CRC24Q polynomial     

const static double gpst0[]={1980,1, 6,0,0,0}; // gps time reference     

const static double leaps[][7]={ // leap seconds {y,m,d,h,m,s,utc-gpst,...}     
    {2016,1,1,0,0,0,-18},
    {2015,7,1,0,0,0,-17},
    {2012,7,1,0,0,0,-16},
    {2009,1,1,0,0,0,-15},
    {2006,1,1,0,0,0,-14},
    {1999,1,1,0,0,0,-13},
    {1997,7,1,0,0,0,-12},
    {1996,1,1,0,0,0,-11},
    {1994,7,1,0,0,0,-10},
    {1993,7,1,0,0,0, -9},
    {1992,7,1,0,0,0, -8},
    {1991,1,1,0,0,0, -7},
    {1990,1,1,0,0,0, -6},
    {1988,1,1,0,0,0, -5},
    {1985,7,1,0,0,0, -4},
    {1983,7,1,0,0,0, -3},
    {1982,7,1,0,0,0, -2},
    {1981,7,1,0,0,0, -1}
};

static const unsigned int tbl_CRC24Q[]={
    0x000000,0x864CFB,0x8AD50D,0x0C99F6,0x93E6E1,0x15AA1A,0x1933EC,0x9F7F17,
    0xA18139,0x27CDC2,0x2B5434,0xAD18CF,0x3267D8,0xB42B23,0xB8B2D5,0x3EFE2E,
    0xC54E89,0x430272,0x4F9B84,0xC9D77F,0x56A868,0xD0E493,0xDC7D65,0x5A319E,
    0x64CFB0,0xE2834B,0xEE1ABD,0x685646,0xF72951,0x7165AA,0x7DFC5C,0xFBB0A7,
    0x0CD1E9,0x8A9D12,0x8604E4,0x00481F,0x9F3708,0x197BF3,0x15E205,0x93AEFE,
    0xAD50D0,0x2B1C2B,0x2785DD,0xA1C926,0x3EB631,0xB8FACA,0xB4633C,0x322FC7,
    0xC99F60,0x4FD39B,0x434A6D,0xC50696,0x5A7981,0xDC357A,0xD0AC8C,0x56E077,
    0x681E59,0xEE52A2,0xE2CB54,0x6487AF,0xFBF8B8,0x7DB443,0x712DB5,0xF7614E,
    0x19A3D2,0x9FEF29,0x9376DF,0x153A24,0x8A4533,0x0C09C8,0x00903E,0x86DCC5,
    0xB822EB,0x3E6E10,0x32F7E6,0xB4BB1D,0x2BC40A,0xAD88F1,0xA11107,0x275DFC,
    0xDCED5B,0x5AA1A0,0x563856,0xD074AD,0x4F0BBA,0xC94741,0xC5DEB7,0x43924C,
    0x7D6C62,0xFB2099,0xF7B96F,0x71F594,0xEE8A83,0x68C678,0x645F8E,0xE21375,
    0x15723B,0x933EC0,0x9FA736,0x19EBCD,0x8694DA,0x00D821,0x0C41D7,0x8A0D2C,
    0xB4F302,0x32BFF9,0x3E260F,0xB86AF4,0x2715E3,0xA15918,0xADC0EE,0x2B8C15,
    0xD03CB2,0x567049,0x5AE9BF,0xDCA544,0x43DA53,0xC596A8,0xC90F5E,0x4F43A5,
    0x71BD8B,0xF7F170,0xFB6886,0x7D247D,0xE25B6A,0x641791,0x688E67,0xEEC29C,
    0x3347A4,0xB50B5F,0xB992A9,0x3FDE52,0xA0A145,0x26EDBE,0x2A7448,0xAC38B3,
    0x92C69D,0x148A66,0x181390,0x9E5F6B,0x01207C,0x876C87,0x8BF571,0x0DB98A,
    0xF6092D,0x7045D6,0x7CDC20,0xFA90DB,0x65EFCC,0xE3A337,0xEF3AC1,0x69763A,
    0x578814,0xD1C4EF,0xDD5D19,0x5B11E2,0xC46EF5,0x42220E,0x4EBBF8,0xC8F703,
    0x3F964D,0xB9DAB6,0xB54340,0x330FBB,0xAC70AC,0x2A3C57,0x26A5A1,0xA0E95A,
    0x9E1774,0x185B8F,0x14C279,0x928E82,0x0DF195,0x8BBD6E,0x872498,0x016863,
    0xFAD8C4,0x7C943F,0x700DC9,0xF64132,0x693E25,0xEF72DE,0xE3EB28,0x65A7D3,
    0x5B59FD,0xDD1506,0xD18CF0,0x57C00B,0xC8BF1C,0x4EF3E7,0x426A11,0xC426EA,
    0x2AE476,0xACA88D,0xA0317B,0x267D80,0xB90297,0x3F4E6C,0x33D79A,0xB59B61,
    0x8B654F,0x0D29B4,0x01B042,0x87FCB9,0x1883AE,0x9ECF55,0x9256A3,0x141A58,
    0xEFAAFF,0x69E604,0x657FF2,0xE33309,0x7C4C1E,0xFA00E5,0xF69913,0x70D5E8,
    0x4E2BC6,0xC8673D,0xC4FECB,0x42B230,0xDDCD27,0x5B81DC,0x57182A,0xD154D1,
    0x26359F,0xA07964,0xACE092,0x2AAC69,0xB5D37E,0x339F85,0x3F0673,0xB94A88,
    0x87B4A6,0x01F85D,0x0D61AB,0x8B2D50,0x145247,0x921EBC,0x9E874A,0x18CBB1,
    0xE37B16,0x6537ED,0x69AE1B,0xEFE2E0,0x709DF7,0xF6D10C,0xFA48FA,0x7C0401,
    0x42FA2F,0xC4B6D4,0xC82F22,0x4E63D9,0xD11CCE,0x575035,0x5BC9C3,0xDD8538
};

// satellite system+prn/slot number to satellite number -----------------------
// convert satellite system+prn/slot number to satellite number
// args   : int    sys       I   satellite system (SYS_GPS,SYS_GLO,...)
//          int    prn       I   satellite prn/slot number
// return : satellite number (0:error)
//-----------------------------------------------------------------------------
extern int satno(int sys, int prn)
{
    if (prn<=0) return 0;
    switch (sys) {
        case SYS_GPS:
            if (prn<MINPRNGPS||MAXPRNGPS<prn) return 0;
            return prn-MINPRNGPS+1;
        case SYS_SBS:
            if (prn<MINPRNSBS||MAXPRNSBS<prn) return 0;
            //return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+NSATLEO+prn-MINPRNSBS+1;
            return NSATGPS+prn-MINPRNSBS+1; // DK added
    }
    return 0;
}

// satellite number to satellite system ----------------------------------------
//convert satellite number to satellite system
//args   : int    sat       I   satellite number (1-MAXSAT)
//         int    *prn      IO  satellite prn/slot number (NULL: no output)
//return : satellite system (SYS_GPS,SYS_GLO,...)
//----------------------------------------------------------------------------
extern int satsys(int sat, int *prn)
{
    int sys=SYS_NONE;
    if (sat<=0||MAXSAT<sat) sat=0;
    else if (sat<=NSATGPS) {
        sys=SYS_GPS; sat+=MINPRNGPS-1;
    }
    // May need this back in for SBAS
    //else if ((sat-=NSATLEO)<=NSATSBS) {
    //    sys=SYS_SBS; sat+=MINPRNSBS-1;
    //}
    else sat=0;
    if (prn) *prn=sat;
    return sys;
}

// satellite id to satellite number --------------------------------------------
//convert satellite id to satellite number
//args   : char   *id       I   satellite id (nn,Gnn,Rnn,Enn,Jnn,Cnn or Snn)
//return : satellite number (0: error)
//notes  : 120-138 and 193-195 are also recognized as sbas and qzss
//----------------------------------------------------------------------------
extern int satid2no(const char *id)
{
    int sys,prn;
    char code;
    
    if (sscanf(id,"%d",&prn)==1) {
        if      (MINPRNGPS<=prn&&prn<=MAXPRNGPS) sys=SYS_GPS;
        else if (MINPRNSBS<=prn&&prn<=MAXPRNSBS) sys=SYS_SBS;
        //else if (MINPRNQZS<=prn&&prn<=MAXPRNQZS) sys=SYS_QZS;
        else return 0;
        return satno(sys,prn);
    }
    if (sscanf(id,"%c%d",&code,&prn)<2) return 0;
    
    switch (code) {
        case 'G': sys=SYS_GPS; prn+=MINPRNGPS-1; break;
        case 'S': sys=SYS_SBS; prn+=100; break;
        default: return 0;
    }
    return satno(sys,prn);
}

// satellite number to satellite id --------------------------------------------
//convert satellite number to satellite id
//args   : int    sat       I   satellite number
//         char   *id       O   satellite id (Gnn,Rnn,Enn,Jnn,Cnn or nnn)
//return : none
//----------------------------------------------------------------------------
extern void satno2id(int sat, char *id)
{
    int prn;
    switch (satsys(sat,&prn)) {
        case SYS_GPS: sprintf(id,"G%02d",prn-MINPRNGPS+1); return;
        case SYS_SBS: sprintf(id,"%03d" ,prn); return;
    }
    strcpy(id,"");
}

// extract unsigned/signed bits ------------------------------------------------
//extract unsigned/signed bits from byte data
//args   : unsigned char *buff I byte data
//         int    pos    I      bit position from start of data (bits)
//         int    len    I      bit length (bits) (len<=32)
//return : extracted unsigned/signed bits
//----------------------------------------------------------------------------
extern unsigned int getbitu(const unsigned char *buff, int pos, int len)
{
    unsigned int bits=0;
    int i;
    for (i=pos;i<pos+len;i++) bits=(bits<<1)+((buff[i/8]>>(7-i%8))&1u);
    return bits;
}
extern int getbits(const unsigned char *buff, int pos, int len)
{
    unsigned int bits=getbitu(buff,pos,len);
    if (len<=0||32<=len||!(bits&(1u<<(len-1)))) return (int)bits;
    return (int)(bits|(~0u<<len)); // extend sign     
}

// set unsigned/signed bits ----------------------------------------------------
//set unsigned/signed bits to byte data
//args   : unsigned char *buff IO byte data
//         int    pos    I      bit position from start of data (bits)
//         int    len    I      bit length (bits) (len<=32)
//        (unsigned) int I      unsigned/signed data
//return : none
//----------------------------------------------------------------------------
extern void setbitu(unsigned char *buff, int pos, int len, unsigned int data)
{
    unsigned int mask=1u<<(len-1);
    int i;
    if (len<=0||32<len) return;
    for (i=pos;i<pos+len;i++,mask>>=1) {
        if (data&mask) buff[i/8]|=1u<<(7-i%8); else buff[i/8]&=~(1u<<(7-i%8));
    }
}
extern void setbits(unsigned char *buff, int pos, int len, int data)
{
    if (data<0) data|=1<<(len-1); else data&=~(1<<(len-1)); // set sign bit     
    setbitu(buff,pos,len,(unsigned int)data);
}

// crc-32 parity ---------------------------------------------------------------
//compute crc-32 parity for novatel raw
//args   : unsigned char *buff I data
//         int    len    I      data length (bytes)
//return : crc-32 parity
//notes  : see NovAtel OEMV firmware manual 1.7 32-bit CRC
//----------------------------------------------------------------------------
extern unsigned int crc32(const unsigned char *buff, int len)
{
    unsigned int crc=0;
    int i,j;
    
    //trace(4,"crc32: len=%d\n",len);
    
    for (i=0;i<len;i++) {
        crc^=buff[i];
        for (j=0;j<8;j++) {
            if (crc&1) crc=(crc>>1)^POLYCRC32; else crc>>=1;
        }
    }
    return crc;
}

// crc-24q parity --------------------------------------------------------------
//compute crc-24q parity for sbas, rtcm3
//args   : unsigned char *buff I data
//         int    len    I      data length (bytes)
//return : crc-24Q parity
//notes  : see reference [2] A.4.3.3 Parity
//----------------------------------------------------------------------------
extern unsigned int crc24q(const unsigned char *buff, int len)
{
    unsigned int crc=0;
    int i;
    
    //trace(4,"crc24q: len=%d\n",len);
    
    for (i=0;i<len;i++) crc=((crc<<8)&0xFFFFFF)^tbl_CRC24Q[(crc>>16)^buff[i]];
    return crc;
}

// convert calendar day/time to time -------------------------------------------
//convert calendar day/time to gtime_t struct
//args   : double *ep       I   day/time {year,month,day,hour,min,sec}
//return : gtime_t struct
//notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
//----------------------------------------------------------------------------
extern gtime_t epoch2time(const double *ep)
{
    const int doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
    gtime_t time={0};
    int days,sec,year=(int)ep[0],mon=(int)ep[1],day=(int)ep[2];
    
    if (year<1970||2099<year||mon<1||12<mon) return time;
    
    // leap year if year%4==0 in 1901-2099     
    days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
    sec=(int)floor(ep[5]);
    time.time=(time_t)days*86400+(int)ep[3]*3600+(int)ep[4]*60+sec;
    time.sec=ep[5]-sec;
    return time;
}

// gps time to time ------------------------------------------------------------
//convert week and tow in gps time to gtime_t struct
//args   : int    week      I   week number in gps time
//         double sec       I   time of week in gps time (s)
//return : gtime_t struct
//----------------------------------------------------------------------------
extern gtime_t gpst2time(int week, double sec)
{
    gtime_t t=epoch2time(gpst0);
    
    if (sec<-1E9||1E9<sec) sec=0.0;
    t.time+=86400*7*week+(int)sec;
    t.sec=sec-(int)sec;
    return t;
}

// time to gps time ------------------------------------------------------------
//convert gtime_t struct to week and tow in gps time
//args   : gtime_t t        I   gtime_t struct
//         int    *week     IO  week number in gps time (NULL: no output)
//return : time of week in gps time (s)
//----------------------------------------------------------------------------
extern double time2gpst(gtime_t t, int *week)
{
    gtime_t t0=epoch2time(gpst0);
    time_t sec=t.time-t0.time;
    int w=(int)(sec/(86400*7));
    
    if (week) *week=w;
    return (double)(sec-w*86400*7)+t.sec;
}

// add time --------------------------------------------------------------------
//add time to gtime_t struct
//args   : gtime_t t        I   gtime_t struct
//         double sec       I   time to add (s)
//return : gtime_t struct (t+sec)
//----------------------------------------------------------------------------
extern gtime_t timeadd(gtime_t t, double sec)
{
    double tt;
    
    t.sec+=sec; tt=floor(t.sec); t.time+=(int)tt; t.sec-=tt;
    return t;
}

// time difference -------------------------------------------------------------
//difference between gtime_t structs
//args   : gtime_t t1,t2    I   gtime_t structs
//return : time difference (t1-t2) (s)
//----------------------------------------------------------------------------
extern double timediff(gtime_t t1, gtime_t t2)
{
    return difftime(t1.time,t2.time)+t1.sec-t2.sec;
}

// get current time in utc -----------------------------------------------------
//get current time in utc
//args   : none
//return : current time in utc
//----------------------------------------------------------------------------
static double timeoffset_=0.0;        // time offset (s)

extern gtime_t timeget(void)
{
    double ep[6]={0};
    struct timeval tv;
    struct tm *tt;
    
    if (!gettimeofday(&tv,NULL)&&(tt=gmtime(&tv.tv_sec))) {
        ep[0]=tt->tm_year+1900; ep[1]=tt->tm_mon+1; ep[2]=tt->tm_mday;
        ep[3]=tt->tm_hour; ep[4]=tt->tm_min; ep[5]=tt->tm_sec+tv.tv_usec*1E-6;
    }

    return timeadd(epoch2time(ep),timeoffset_);
}

// utc to gpstime --------------------------------------------------------------
//convert utc to gpstime considering leap seconds
//args   : gtime_t t        I   time expressed in utc
//return : time expressed in gpstime
//notes  : ignore slight time offset under 100 ns
//----------------------------------------------------------------------------
extern gtime_t utc2gpst(gtime_t t)
{
    int i;
    
    for (i=0;i<(int)sizeof(leaps)/(int)sizeof(*leaps);i++) {
        if (timediff(t,epoch2time(leaps[i]))>=0.0) return timeadd(t,-leaps[i][6]);
    }
    return t;
}

// adjust gps week number ------------------------------------------------------
//adjust gps week number using cpu time
//args   : int   week       I   not-adjusted gps week number
//return : adjusted gps week number
//----------------------------------------------------------------------------
extern int adjgpsweek(int week)
{
    int w;
    (void)time2gpst(utc2gpst(timeget()),&w);
    if (w<1560) w=1560; // use 2009/12/1 if time is earlier than 2009/12/1     
    return week+(w-week+512)/1024*1024;
}

// sleep ms --------------------------------------------------------------------
//sleep ms
//args   : int   ms         I   miliseconds to sleep (<0:no sleep)
//return : none
//----------------------------------------------------------------------------
extern void sleepms(int ms)
{
    struct timespec ts;
    if (ms<=0) return;
    ts.tv_sec=(time_t)(ms/1000);
    ts.tv_nsec=(long)(ms%1000*1000000);
    nanosleep(&ts,NULL);
}

/*
//-----------------------------------------------------------------------------
// Debug trace functions
//-----------------------------------------------------------------------------
#ifdef TRACE

static FILE *fp_trace=NULL;     // file pointer of trace     
static int level_trace=0;       // level of trace     
static unsigned int tick_trace=0; // tick time at traceopen (ms)     

extern void traceopen(const char *file)
{
    if (!*file||!(fp_trace=fopen(file,"w"))) fp_trace=stderr;
    tick_trace=tickget();
}
extern void traceclose(void)
{
    if (fp_trace&&fp_trace!=stderr) fclose(fp_trace);
    fp_trace=NULL;
}
extern void tracelevel(int level)
{
    level_trace=level;
}
extern void trace(int level, const char *format, ...)
{
    va_list ap;

    // print error message to stderr     
    if (level<=1) {
        va_start(ap,format); vfprintf(stderr,format,ap); va_end(ap);
    }
    if (!fp_trace||level>level_trace) return;
    fprintf(fp_trace,"%d ",level);
    va_start(ap,format); vfprintf(fp_trace,format,ap); va_end(ap);
    fflush(fp_trace);
}
extern void tracet(int level, const char *format, ...)
{
    va_list ap;

    if (!fp_trace||level>level_trace) return;
    fprintf(fp_trace,"%d %9.3f: ",level,(tickget()-tick_trace)/1000.0);
    va_start(ap,format); vfprintf(fp_trace,format,ap); va_end(ap);
    fflush(fp_trace);
}
extern void tracemat(int level, const double *A, int n, int m, int p, int q)
{
    if (!fp_trace||level>level_trace) return;
    matfprint(A,n,m,p,q,fp_trace); fflush(fp_trace);
}

extern void tracenav(int level, const nav_t *nav)
{
    char s1[64],s2[64],id[16];
    int i;

    if (!fp_trace||level>level_trace) return;
    for (i=0;i<nav->n;i++) {
        time2str(nav->eph[i].toe,s1,0);
        time2str(nav->eph[i].ttr,s2,0);
        satno2id(nav->eph[i].sat,id);
        fprintf(fp_trace,"(%3d) %-3s : %s %s %3d %3d %02x\n",i+1,
                id,s1,s2,nav->eph[i].iode,nav->eph[i].iodc,nav->eph[i].svh);
    }
    fprintf(fp_trace,"(ion) %9.4e %9.4e %9.4e %9.4e\n",nav->ion_gps[0],
            nav->ion_gps[1],nav->ion_gps[2],nav->ion_gps[3]);
    fprintf(fp_trace,"(ion) %9.4e %9.4e %9.4e %9.4e\n",nav->ion_gps[4],
            nav->ion_gps[5],nav->ion_gps[6],nav->ion_gps[7]);
    fprintf(fp_trace,"(ion) %9.4e %9.4e %9.4e %9.4e\n",nav->ion_gal[0],
            nav->ion_gal[1],nav->ion_gal[2],nav->ion_gal[3]);
}

extern void tracegnav(int level, const nav_t *nav)
{
    char s1[64],s2[64],id[16];
    int i;

    if (!fp_trace||level>level_trace) return;
    for (i=0;i<nav->ng;i++) {
        time2str(nav->geph[i].toe,s1,0);
        time2str(nav->geph[i].tof,s2,0);
        satno2id(nav->geph[i].sat,id);
        fprintf(fp_trace,"(%3d) %-3s : %s %s %2d %2d %8.3f\n",i+1,
                id,s1,s2,nav->geph[i].frq,nav->geph[i].svh,nav->geph[i].taun*1E6);
    }
}

extern void tracehnav(int level, const nav_t *nav)
{
    char s1[64],s2[64],id[16];
    int i;

    if (!fp_trace||level>level_trace) return;
    for (i=0;i<nav->ns;i++) {
        time2str(nav->seph[i].t0,s1,0);
        time2str(nav->seph[i].tof,s2,0);
        satno2id(nav->seph[i].sat,id);
        fprintf(fp_trace,"(%3d) %-3s : %s %s %2d %2d\n",i+1,
                id,s1,s2,nav->seph[i].svh,nav->seph[i].sva);
    }
}

extern void tracepeph(int level, const nav_t *nav)
{
    char s[64],id[16];
    int i,j;

    if (!fp_trace||level>level_trace) return;

    for (i=0;i<nav->ne;i++) {
        time2str(nav->peph[i].time,s,0);
        for (j=0;j<MAXSAT;j++) {
            satno2id(j+1,id);
            fprintf(fp_trace,"%-3s %d %-3s %13.3f %13.3f %13.3f %13.3f %6.3f %6.3f %6.3f %6.3f\n",
                    s,nav->peph[i].index,id,
                    nav->peph[i].pos[j][0],nav->peph[i].pos[j][1],
                    nav->peph[i].pos[j][2],nav->peph[i].pos[j][3]*1E9,
                    nav->peph[i].std[j][0],nav->peph[i].std[j][1],
                    nav->peph[i].std[j][2],nav->peph[i].std[j][3]*1E9);
        }
    }
}

extern void tracepclk(int level, const nav_t *nav)
{
    char s[64],id[16];
    int i,j;

    if (!fp_trace||level>level_trace) return;

    for (i=0;i<nav->nc;i++) {
        time2str(nav->pclk[i].time,s,0);
        for (j=0;j<MAXSAT;j++) {
            satno2id(j+1,id);
            fprintf(fp_trace,"%-3s %d %-3s %13.3f %6.3f\n",
                    s,nav->pclk[i].index,id,
                    nav->pclk[i].clk[j][0]*1E9,nav->pclk[i].std[j][0]*1E9);
        }
    }
}

extern void traceb(int level, const unsigned char *p, int n)
{
    int i;
    if (!fp_trace||level>level_trace) return;
    for (i=0;i<n;i++) fprintf(fp_trace,"%02X%s",*p++,i%8==7?" ":"");
    fprintf(fp_trace,"\n");
}
#else
extern void traceopen(const char *file) {}
extern void traceclose(void) {}
extern void tracelevel(int level) {}
extern void trace   (int level, const char *format, ...) {}
extern void tracet  (int level, const char *format, ...) {}
extern void tracemat(int level, const double *A, int n, int m, int p, int q) {}
//extern void traceobs(int level, const obsd_t *obs, int n) {}
extern void tracenav(int level, const nav_t *nav) {}
extern void tracegnav(int level, const nav_t *nav) {}
extern void tracehnav(int level, const nav_t *nav) {}
extern void tracepeph(int level, const nav_t *nav) {}
extern void tracepclk(int level, const nav_t *nav) {}
extern void traceb  (int level, const unsigned char *p, int n) {}

#endif // TRACE     
*/


// End of rtkcmn.c
