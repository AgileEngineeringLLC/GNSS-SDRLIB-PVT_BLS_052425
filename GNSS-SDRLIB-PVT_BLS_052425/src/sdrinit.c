//-----------------------------------------------------------------------------
// sdrinit.c : SDR initialize/cleanup functions
//
// Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
// Edits from Don Kelly, don.kelly@mac.com, 2025
//-----------------------------------------------------------------------------
#include "sdr.h"

// read ini file --------------------------------------------------------------
static int GetFileAttributes(const char *file)
{
    FILE *fp;
    if (!(fp=fopen(file,"r"))) return -1;
    fclose(fp);
    return 0;
}
static void GetPrivateProfileString(const char *sec, const char *key,
    const char *def, char *str, int len, const char *file)
{
    FILE *fp;
    char buff[1024],*p,*q;
    int enter=0;

    strncpy(str,def,len-1); str[len-1]='\0';

    if (!(fp=fopen(file,"r"))) {
        fprintf(stderr,"ini file open error [%s]\n",file);
        return;
    }
    while (fgets(buff,sizeof(buff),fp)) {
        if ((p=strchr(buff,';'))) *p='\0';
        if ((p=strchr(buff,'['))&&(q=strchr(p+1,']'))) {
            *q='\0';
            enter=!strcmp(p+1,sec);
        }
        else if (enter&&(p=strchr(buff,'='))) {
            *p='\0';
            for (q=p-1;q>=buff&&(*q==' '||*q=='\t');) *q--='\0';
            if (strcmp(buff,key)) continue;
            for (q=p+1+strlen(p+1)-1;q>=p+1&&(*q=='\r'||*q=='\n');) *q--='\0';
            strncpy(str,p+1,len-1); str[len-1]='\0';
            break;
        }
    }
    fclose(fp);
}

// functions used in read ini file --------------------------------------------
//note : these functions are only used in CLI application
//-----------------------------------------------------------------------------
int splitint(char *src, char *dlm, int *out, int n)
{
    int i;
    char *str;
    for (i=0;i<n;i++) {
        if ((str=strtok((i==0)?src:NULL,dlm))==NULL) return -1;
        out[i]=atoi(str);
    }
    return 0;
}
int splitdouble(char *src, char *dlm, double *out, int n)
{
    int i;
    char *str;
    for (i=0;i<n;i++) {
        if ((str=strtok((i==0)?src:NULL,dlm))==NULL) return -1;
        out[i]=atof(str);
    }
    return 0;
}
int readiniint(char *file, char *sec, char *key)
{
    char str[256];
    GetPrivateProfileString(sec,key,"",str,256,file);
    return atoi(str);
}
int readiniints(char *file, char *sec, char *key, int *out, int n)
{
    char str[256];
    GetPrivateProfileString(sec,key,"",str,256,file);
    return splitint(str,",",out,n);
}
double readinidouble(char *file, char *sec, char *key)
{
    char str[256];
    GetPrivateProfileString(sec,key,"",str,256,file);
    return atof(str);
}
int readinidoubles(char *file, char *sec, char *key, double *out, int n)
{
    char str[256];
    GetPrivateProfileString(sec,key,"",str,256,file);
    return splitdouble(str,",",out,n);
}
void readinistr(char *file, char *sec, char *key, char *out)
{
    GetPrivateProfileString(sec,key,"",out,256,file);
}

// read ini file --------------------------------------------------------------
//read ini file and set value to sdrini struct
//args   : sdrini_t *ini    I/0 sdrini struct
//return : int                  0:okay -1:error
//note : this function is only used in CLI application
//-----------------------------------------------------------------------------
extern int readinifile(sdrini_t *ini)
{
    int i,ret;
    char inifile[]="./gnss-sdrcli.ini";
    char fendfile[256],str[256];

    // check ini file   
    if ((ret=GetFileAttributes(inifile))<0){
        SDRPRINTF("error: gnss-sdrcli.ini doesn't exist\n");
        return -1;
    }
    // receiver setting   
    readinistr(inifile,"RCV","FENDCONF",fendfile);

    // check front-end configuration  file   
    if ((ret=GetFileAttributes(fendfile))<0){
        SDRPRINTF("error: %s doesn't exist\n",fendfile);
        return -1;
    }

    // Read in front end (receiver) file, such as for RTLSDR or BLADERF
    readinistr(fendfile,"FEND","TYPE",str);
    if (strcmp(str,"BLADERF")==0)     ini->fend=FEND_BLADERF;
    else if (strcmp(str,"RTLSDR")==0)     ini->fend=FEND_RTLSDR;
    else if (strcmp(str,"FILEBLADERF")==0) ini->fend=FEND_FBLADERF;
    else if (strcmp(str,"FILERTLSDR")==0) ini->fend=FEND_FRTLSDR;
    else if (strcmp(str,"FILE")==0)       ini->fend=FEND_FILE;
    else { SDRPRINTF("error: wrong frontend type: %s\n",str); return -1; }

    if (ini->fend==FEND_FILE    ||
        ini->fend==FEND_FBLADERF||ini->fend==FEND_FRTLSDR) {
        readinistr(fendfile,"FEND","FILE1",ini->file1);
        if (strcmp(ini->file1,"")!=0) ini->useif1=ON;
    }
    if (ini->fend==FEND_FILE) {
        readinistr(fendfile,"FEND","FILE2",ini->file2);
        if (strcmp(ini->file2,"")!=0) ini->useif2=ON;
    }
    ini->f_cf[0]=readinidouble(fendfile,"FEND","CF1");
    ini->f_sf[0]=readinidouble(fendfile,"FEND","SF1");
    ini->f_if[0]=readinidouble(fendfile,"FEND","IF1");
    ini->f_gain[0]=readiniint(fendfile,"FEND", "GAIN");
    ini->f_bias[0]=readiniint(fendfile,"FEND", "BIAS");
    ini->f_clock[0]=readiniint(fendfile,"FEND", "CLOCK");
    ini->dtype[0]=readiniint(fendfile,"FEND","DTYPE1");
    ini->f_cf[1]=readinidouble(fendfile,"FEND","CF2");
    ini->f_sf[1]=readinidouble(fendfile,"FEND","SF2");
    ini->f_if[1]=readinidouble(fendfile,"FEND","IF2");
    ini->f_gain[1]=readiniint(fendfile,"FEND", "GAIN");
    ini->f_bias[1]=readiniint(fendfile,"FEND", "BIAS");
    ini->f_clock[1]=readiniint(fendfile,"FEND", "CLOCK");
    ini->dtype[1]=readiniint(fendfile,"FEND","DTYPE2");

    // RTL-SDR only   
    ini->rtlsdrppmerr=readiniint(fendfile,"FEND","PPMERR");

    // Tracking parameter setting
    ini->trkcorrn=readiniint(fendfile,"TRACK","CORRN");
    ini->trkcorrd=readiniint(fendfile,"TRACK","CORRD");
    ini->trkcorrp=readiniint(fendfile,"TRACK","CORRP");
    ini->trkdllb[0]=readinidouble(fendfile,"TRACK","DLLB1");
    ini->trkpllb[0]=readinidouble(fendfile,"TRACK","PLLB1");
    ini->trkfllb[0]=readinidouble(fendfile,"TRACK","FLLB1");
    ini->trkdllb[1]=readinidouble(fendfile,"TRACK","DLLB2");
    ini->trkpllb[1]=readinidouble(fendfile,"TRACK","PLLB2");
    ini->trkfllb[1]=readinidouble(fendfile,"TRACK","FLLB2");

    // Read in gnss-sdrcli.ini file
    // Channel setting
    ini->nch=readiniint(inifile,"CHANNEL","NCH");
    if (ini->nch<1) {
        SDRPRINTF("error: wrong inifile value NCH=%d\n",ini->nch);
        return -1;
    }
    if ((ret=readiniints(inifile,"CHANNEL","PRN",ini->prn,ini->nch))<0 ||
        (ret=readiniints(inifile,"CHANNEL","SYS",ini->sys,ini->nch))<0 ||
        (ret=readiniints(inifile,"CHANNEL","CTYPE",ini->ctype,ini->nch))<0 ||
        (ret=readiniints(inifile,"CHANNEL","FTYPE",ini->ftype,ini->nch))<0) {
            SDRPRINTF("error: wrong inifile value NCH=%d\n",ini->nch);
            return -1;
    }

    // Plot settings
    ini->pltacq=readiniint(inifile,"PLOT","ACQ");
    ini->plttrk=readiniint(inifile,"PLOT","TRK");

    // Output setting
    ini->outms   =readiniint(inifile,"OUTPUT","OUTMS");
    ini->sbas    =readiniint(inifile,"OUTPUT","SBAS");

    // Spectrum setting
    ini->pltspec=readiniint(inifile,"SPECTRUM","SPEC");

    // PVT setting
    if((ret=readiniints(inifile,"PVT","XUINITIAL",ini->xu0_v,3))<0 ) {
        SDRPRINTF("error: wrong inifile value NCH=%d\n",3);
        return -1;
    }
    readinistr(inifile,"PVT","FONTFILE",ini->fontfile);
    //printf("FONTFILE: %s\n", ini->fontfile);
    ini->ekfFilterOn=readiniint(inifile,"PVT","EKFFILTER");

    // SDR channel setting
    for (i=0;i<sdrini.nch;i++) {
        if (sdrini.ctype[i]==CTYPE_L1CA) {
            sdrini.nchL1++;
        }
    }
    return 0;
}

// check initial value ---------------------------------------------------------
//checking value in sdrini struct
//args   : sdrini_t *ini    I   sdrini struct
//return : int                  0:okay -1:error
//----------------------------------------------------------------------------
extern int chk_initvalue(sdrini_t *ini)
{
    int ret;

    // checking frequency input   
    if ((ini->f_sf[0]<=0||ini->f_sf[0]>100e6) ||
        (ini->f_if[0]<0 ||ini->f_if[0]>100e6)) {
            SDRPRINTF("error: wrong freq. input sf1: %.0f if1: %.0f\n",
                ini->f_sf[0],ini->f_if[0]);
            return -1;
    }

    // checking frequency input   
    if(ini->useif2) {
        if ((ini->f_sf[1]<=0||ini->f_sf[1]>100e6) ||
            (ini->f_if[1]<0 ||ini->f_if[1]>100e6)) {
                SDRPRINTF("error: wrong freq. input sf2: %.0f if2: %.0f\n",
                    ini->f_sf[1],ini->f_if[1]);
                return -1;
        }
    }

    // checking filepath   
    if (ini->fend==FEND_FILE   ||
        ini->fend==FEND_FRTLSDR||ini->fend==FEND_FBLADERF) {
        if (ini->useif1&&((ret=GetFileAttributes(ini->file1))<0)){
            SDRPRINTF("error: file1 doesn't exist: %s\n",ini->file1);
            return -1;
        }
        if (ini->useif2&&((ret=GetFileAttributes(ini->file2))<0)){
            SDRPRINTF("error: file2 doesn't exist: %s\n",ini->file2);
            return -1;
        }
        if ((!ini->useif1)&&(!ini->useif2)) {
            SDRPRINTF("error: file1 or file2 are not selected\n");
            return -1;
        }
    }

    return 0;
}

// initialize mutex and event --------------------------------------------------
//create mutex and event handles
//args   : none
//return : none
//----------------------------------------------------------------------------
extern void openhandles(void)
{
    // mutexes   
    initmlock(hbuffmtx);
    initmlock(hreadmtx);
    initmlock(hfftmtx);
    initmlock(hobsmtx);
    initmlock(hresetmtx);
    initmlock(hobsvecmtx);
    initmlock(hmsgmtx);
}

// close mutex and event -------------------------------------------------------
//close mutex and event handles
//args   : none
//return : none
//----------------------------------------------------------------------------
extern void closehandles(void)
{
    // mutexes   
    delmlock(hbuffmtx);
    delmlock(hreadmtx);
    delmlock(hfftmtx);
    delmlock(hobsmtx);
    delmlock(hresetmtx);
    delmlock(hobsvecmtx);
    delmlock(hmsgmtx);
}

// initialize acquisition struct -----------------------------------------------
//set value to acquisition struct
//args   : int sys          I   system type (SYS_GPS...)
//         int ctype        I   code type (CTYPE_L1CA...)
//         int prn          I   PRN
//         sdracq_t *acq    I/0 acquisition struct
//return : none
//----------------------------------------------------------------------------
extern void initacqstruct(int sys, int ctype, int prn, sdracq_t *acq)
{
    if (ctype==CTYPE_L1CA) acq->intg=ACQINTG_L1CA;

    acq->hband=ACQHBAND;
    acq->step=ACQSTEP;
    acq->nfreq=2*(ACQHBAND/ACQSTEP)+1;
}

// initialize tracking parameter struct ----------------------------------------
//set value to tracking parameter struct
//args   : sdrtrk_t *trk    I/0 tracking struct
//return : none
//----------------------------------------------------------------------------
extern void inittrkprmstruct(sdrtrk_t *trk)
{
    // set tracking parameter   
    trk->prm1.dllb=sdrini.trkdllb[0];
    trk->prm1.pllb=sdrini.trkpllb[0];
    trk->prm1.fllb=sdrini.trkfllb[0];
    trk->prm2.dllb=sdrini.trkdllb[1];
    trk->prm2.pllb=sdrini.trkpllb[1];
    trk->prm2.fllb=sdrini.trkfllb[1];

    // calculation loop filter parameters (before nav frame synchronization)   
    trk->prm1.dllw2=(trk->prm1.dllb/0.53)*(trk->prm1.dllb/0.53);
    trk->prm1.dllaw=1.414*(trk->prm1.dllb/0.53);
    trk->prm1.pllw2=(trk->prm1.pllb/0.53)*(trk->prm1.pllb/0.53);
    trk->prm1.pllaw=1.414*(trk->prm1.pllb/0.53);
    trk->prm1.fllw =trk->prm1.fllb/0.25;

    // calculation loop filter parameters (after nav frame synchronization)   
    trk->prm2.dllw2=(trk->prm2.dllb/0.53)*(trk->prm2.dllb/0.53);
    trk->prm2.dllaw=1.414*(trk->prm2.dllb/0.53);
    trk->prm2.pllw2=(trk->prm2.pllb/0.53)*(trk->prm2.pllb/0.53);
    trk->prm2.pllaw=1.414*(trk->prm2.pllb/0.53);
    trk->prm2.fllw =trk->prm2.fllb/0.25;
}

// initialize tracking struct --------------------------------------------------
//set value to tracking struct
//args   : int    sat       I   satellite number
//         int    ctype     I   code type (CTYPE_L1CA...)
//         double ctime     I   code period (s)
//         sdrtrk_t *trk    I/0 tracking struct
//return : int                  0:okay -1:error
//----------------------------------------------------------------------------
extern int inittrkstruct(int sat, int ctype, double ctime, sdrtrk_t *trk)
{
    int i;
    int ctimems=(int)(ctime*1000);

    // set tracking parameter   
    inittrkprmstruct(trk);

    // correlation point   
    trk->corrn=sdrini.trkcorrn;
    trk->corrp=(int *)malloc(sizeof(int)*trk->corrn);
    for (i=0;i<trk->corrn;i++) {
        trk->corrp[i]=sdrini.trkcorrd*(i+1);
        if (trk->corrp[i]==sdrini.trkcorrp){
            trk->ne=2*(i+1)-1; // Early   
            trk->nl=2*(i+1);   // Late   
        }
    }
    // correlation point for plot   
    (trk->corrx=(double *)calloc(2*trk->corrn+1,sizeof(double)));
    for (i=1;i<=trk->corrn;i++) {
        trk->corrx[2*i-1]=-sdrini.trkcorrd*i;
        trk->corrx[2*i  ]= sdrini.trkcorrd*i;
    }

    trk->II     =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->QQ     =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->oldI   =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->oldQ   =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->sumI   =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->sumQ   =(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->oldsumI=(double*)calloc(1+2*trk->corrn,sizeof(double));
    trk->oldsumQ=(double*)calloc(1+2*trk->corrn,sizeof(double));

    if (ctype==CTYPE_L1CA)   trk->loop=LOOP_L1CA;
    if (ctype==CTYPE_L1SBAS) trk->loop=LOOP_SBAS;

    // loop interval (ms)   
    trk->loopms=trk->loop*ctimems;

    if (!trk->II||!trk->QQ||!trk->oldI||!trk->oldQ||!trk->sumI||!trk->sumQ||
        !trk->oldsumI||!trk->oldsumQ) {
        SDRPRINTF("error: inittrkstruct memory allocation\n");
        return -1;
    }
    return 0;
}

// initialize navigation struct ------------------------------------------------
//set value to navigation struct
//args   : int sys          I   system type (SYS_GPS...)
//         int ctype        I   code type (CTYPE_L1CA...)
//         int prn          I   PRN (or SV) number
//         sdrnav_t *nav    I/0 navigation struct
//return : int                  0:okay -1:error
//----------------------------------------------------------------------------
extern int initnavstruct(int sys, int ctype, int prn, sdrnav_t *nav)
{
    int i;
    int pre_l1ca[8]= { 1,-1,-1,-1, 1,-1, 1, 1}; // L1CA preamble  
    int pre_sbs[24]= { 1,-1, 1,-1, 1, 1,-1,-1,-1, 1,
                       1,-1,-1, 1,-1, 1,-1,-1, 1, 1,
                       1 -1,-1, 1}; // SBAS L1/QZS L1SAIF preamble   

    int poly[2]={V27POLYB,V27POLYA};

    nav->ctype=ctype;
    nav->sdreph.ctype=ctype;
    nav->sdreph.prn=prn;
    nav->sdreph.eph.iodc=-1;

    // GPS/QZS L1CA   
    if (ctype==CTYPE_L1CA) {
        nav->rate=NAVRATE_L1CA;
        nav->flen=NAVFLEN_L1CA;
        nav->addflen=NAVADDFLEN_L1CA;
        nav->prelen=NAVPRELEN_L1CA;
        nav->sdreph.cntth=NAVEPHCNT_L1CA;
        nav->update=(int)(nav->flen*nav->rate);
        memcpy(nav->prebits,pre_l1ca,sizeof(int)*nav->prelen);

        // overlay code (all 1)   
        nav->ocode=(short *)calloc(nav->rate,sizeof(short));
        for (i=0;i<nav->rate;i++) nav->ocode[i]=1;
    }
    // SBAS/QZS L1SAIF   
    //if (ctype==CTYPE_L1SAIF||ctype==CTYPE_L1SBAS) {
    if (ctype==CTYPE_L1SBAS) {
        nav->rate=NAVRATE_SBAS;
        nav->flen=NAVFLEN_SBAS;
        nav->addflen=NAVADDFLEN_SBAS;
        nav->prelen=NAVPRELEN_SBAS;
        nav->sdreph.cntth=NAVEPHCNT_SBAS;
        nav->update=(int)(nav->flen/3*nav->rate);
        memcpy(nav->prebits,pre_sbs,sizeof(int)*nav->prelen);

        // create fec   
        if((nav->fec=create_viterbi27_port(NAVFLEN_SBAS/2))==NULL) {
            SDRPRINTF("error: create_viterbi27 failed\n");
            return -1;
        }
        // set polynomial   
        set_viterbi27_polynomial_port(poly);

        // overlay code (all 1)   
        nav->ocode=(short *)calloc(nav->rate,sizeof(short));
        for (i=0;i<nav->rate;i++) nav->ocode[i]=1;
    }

    if (!(nav->bitsync= (int *)calloc(nav->rate,sizeof(int))) ||
        !(nav->fbits=   (int *)calloc(nav->flen+nav->addflen,sizeof(int))) ||
        !(nav->fbitsdec=(int *)calloc(nav->flen+nav->addflen,sizeof(int)))) {
            SDRPRINTF("error: initnavstruct memory alocation\n");
            return -1;
    }
    return 0;
}

// initialize sdr channel struct -----------------------------------------------
//set value to sdr channel struct
//args   : int    chno      I   channel number (1,2,...)
//         int    sys       I   system type (SYS_***)
//         int    prn       I   PRN number
//         int    ctype     I   code type (CTYPE_***)
//         int    dtype     I   data type (DTYPEI or DTYPEIQ)
//         int    ftype     I   front end type (FTYPE1 or FTYPE2)
//         int    f_gain	I	rx gain
//         int    f_bias	I	rx bias-tee
//         int    f_clock   I   rx clock (internal or external)
//         double f_cf      I   center (carrier) frequency (Hz)
//         double f_sf      I   sampling frequency (Hz)
//         double f_if      I   intermidiate frequency (Hz)
//         sdrch_t *sdr     I/0 sdr channel struct
//return : int                  0:okay -1:error
//----------------------------------------------------------------------------
extern int initsdrch(int chno, int sys, int prn, int ctype, int dtype,
                     int ftype, int f_gain, int f_bias, int f_clock, double f_cf, double f_sf, double f_if,
                     sdrch_t *sdr)
{
    int i;
    short *rcode;

    sdr->no=chno;
    sdr->sys=sys;
    sdr->prn=prn;
    sdr->sat=satno(sys,prn);
    sdr->ctype=ctype;
    sdr->dtype=dtype;
    sdr->ftype=ftype;
    sdr->f_sf=f_sf;
    sdr->f_gain=f_gain;
    sdr->f_bias=f_bias;
    sdr->f_clock=f_clock;
    sdr->f_if=f_if;
    sdr->ti=1/f_sf;

    // code generation   
    if (!(sdr->code=gencode(prn,ctype,&sdr->clen,&sdr->crate))) {
        SDRPRINTF("error: gencode\n"); return -1;
    }
    sdr->ci=sdr->ti*sdr->crate;
    sdr->ctime=sdr->clen/sdr->crate;
    sdr->nsamp=(int)(f_sf*sdr->ctime);
    sdr->nsampchip=(int)(sdr->nsamp/sdr->clen);
    satno2id(sdr->sat,sdr->satstr);

    // set carrier frequency   
    if (sdrini.fend==FEND_FRTLSDR) {
        sdr->foffset=f_cf*sdrini.rtlsdrppmerr*1e-6;
        sdr->f_cf=f_cf; // DK added
    } else {
        sdr->f_cf=f_cf; // carrier frequency   
        sdr->foffset=0.0; // frequency offset   
    }

    // acquisition struct   
    initacqstruct(sys,ctype,prn,&sdr->acq);
    sdr->acq.nfft=2*sdr->nsamp;//calcfftnum(2*sdr->nsamp,0);

    // memory allocation   
    if (!(sdr->acq.freq=(double*)malloc(sizeof(double)*sdr->acq.nfreq))) {
        SDRPRINTF("error: initsdrch memory alocation\n"); return -1;
    }

    // doppler search frequency   
    for (i=0;i<sdr->acq.nfreq;i++)
        sdr->acq.freq[i]=sdr->f_if+((i-(sdr->acq.nfreq-1)/2)*sdr->acq.step)
                            +sdr->foffset;

    // tracking struct   
    if (inittrkstruct(sdr->sat,ctype,sdr->ctime,&sdr->trk)<0) return -1;

    // navigation struct   
    if (initnavstruct(sys,ctype,prn,&sdr->nav)<0) {
        return -1;
    }
    // memory allocation   
    if (!(rcode=(short *)sdrmalloc(sizeof(short)*sdr->acq.nfft)) ||
        !(sdr->xcode=cpxmalloc(sdr->acq.nfft))) {
            SDRPRINTF("error: initsdrch memory alocation\n"); return -1;
    }
    // other code generation   
    for (i=0;i<sdr->acq.nfft;i++) rcode[i]=0; // zero padding   
    rescode(sdr->code,sdr->clen,0,0,sdr->ci,sdr->nsamp,rcode); // resampling   
    cpxcpx(rcode,NULL,1.0,sdr->acq.nfft,sdr->xcode); // FFT for acquisition   
    cpxfft(NULL,sdr->xcode,sdr->acq.nfft);

    sdrfree(rcode);
    return 0;
}

// free sdr channel struct -----------------------------------------------------
//free memory in sdr channel struct
//args   : sdrch_t *sdr     I/0 sdr channel struct
//return : none
//----------------------------------------------------------------------------
extern void freesdrch(sdrch_t *sdr)
{
    free(sdr->code);
    cpxfree(sdr->xcode);
    free(sdr->nav.fbits);
    free(sdr->nav.fbitsdec);
    free(sdr->nav.bitsync);
    free(sdr->trk.II);
    free(sdr->trk.QQ);
    free(sdr->trk.oldI);
    free(sdr->trk.oldQ);
    free(sdr->trk.sumI);
    free(sdr->trk.sumQ);
    free(sdr->trk.oldsumI);
    free(sdr->trk.oldsumQ);
    free(sdr->trk.corrp);
    free(sdr->acq.freq);

    if (sdr->nav.fec!=NULL)
        delete_viterbi27_port(sdr->nav.fec);

    if (sdr->nav.ocode!=NULL)
        free(sdr->nav.ocode);
}
