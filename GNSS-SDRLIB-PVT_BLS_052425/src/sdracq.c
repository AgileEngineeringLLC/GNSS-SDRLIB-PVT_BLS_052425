//-----------------------------------------------------------------------------
// sdracq.c : SDR acquisition functions
//
// Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
// Edits from Don Kelly, don.kelly@mac.com, 2025
//-----------------------------------------------------------------------------
#include "sdr.h"

/* sdr acquisition function ----------------------------------------------------
* sdr acquisition function called from sdr channel thread
* args   : sdrch_t *sdr     I/O sdr channel struct
*          double *power    O   normalized correlation power vector (2D array)
* return : uint64_t             current buffer location
*-----------------------------------------------------------------------------*/
extern uint64_t sdraqcuisition(sdrch_t *sdr, double *power)
{
    int i;
    char *data;
    uint64_t buffloc;

    /* memory allocation */
    data=(char*)sdrmalloc(sizeof(char)*2*sdr->nsamp*sdr->dtype);

    /* current buffer location */
    mlock(hreadmtx);
    buffloc=(sdrstat.fendbuffsize*sdrstat.buffcnt)-(sdr->acq.intg+1)*sdr->nsamp;
    unmlock(hreadmtx);

    /* acquisition integration */
    for (i=0;i<sdr->acq.intg;i++) {
        /* get current 1ms data */
        rcvgetbuff(&sdrini,buffloc,2*sdr->nsamp,sdr->ftype,sdr->dtype,data);
        buffloc+=sdr->nsamp;

        /*
        printf("i: %d, intg: %d, buffloc %ld, nsamp %d, ftype %d, dtype %d\n",
              i, sdr->acq.intg, buffloc, sdr->nsamp, sdr->ftype,sdr->dtype);
        printf("data: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
           data[0], data[1], data[2], data[3], data[4],
           data[5], data[6], data[7], data[8], data[9],
           data[10], data[11], data[12], data[13], data[14],
           data[15], data[16], data[17], data[18], data[19]);
        */

        /* fft correlation */
        pcorrelator(data,sdr->dtype,sdr->ti,sdr->nsamp,sdr->acq.freq,
            sdr->acq.nfreq,sdr->crate,sdr->acq.nfft,sdr->xcode,power);

        /* check acquisition result */
        if (checkacquisition(power,sdr)) {
            sdr->flagacq=ON;
            break;
        }
    }

    // Display acquisition results
    /*
    if (sdr->flagacq) SDRPRINTF(BGRN);
    SDRPRINTF("%s, C/N0=%4.1f, peak=%3.1f, codei=%5d, freq=%8.1f\n",
        sdr->satstr,sdr->acq.cn0,sdr->acq.peakr,sdr->acq.acqcodei,
        sdr->acq.acqfreq-sdr->f_if-sdr->foffset);
    SDRPRINTF(reset);
    */

    // Set acquisition result
    if (sdr->flagacq) {
        /* set buffer location at top of code */
        buffloc+=-(i+1)*sdr->nsamp+sdr->acq.acqcodei;
        sdr->trk.carrfreq=sdr->acq.acqfreq;
        sdr->trk.codefreq=sdr->crate;
    }
    else {
        sleepms(ACQSLEEP);
    }
    sdrfree(data);
    return buffloc;
}
/* check acquisition result ----------------------------------------------------
* check GNSS signal exists or not
* carrier frequency is computed
* args   : sdrch_t *sdr     I/0 sdr channel struct
*          double *P        I   normalized correlation power vector
* return : int                  acquisition flag (0: not acquired, 1: acquired) 
* note : first/second peak ratio and c/n0 computation
*-----------------------------------------------------------------------------*/
extern int checkacquisition(double *P, sdrch_t *sdr)
{
    int maxi,codei,freqi,exinds,exinde;
    double maxP,maxP2,meanP;

    maxP=maxvd(P,sdr->nsamp*sdr->acq.nfreq,-1,-1,&maxi);
    ind2sub(maxi,sdr->nsamp,sdr->acq.nfreq,&codei,&freqi);

    /* C/N0 calculation */
    /* excluded index */
    exinds=codei-2*sdr->nsampchip; if(exinds<0) exinds+=sdr->nsamp;
    exinde=codei+2*sdr->nsampchip; if(exinde>=sdr->nsamp) exinde-=sdr->nsamp;
    meanP=meanvd(&P[freqi*sdr->nsamp],sdr->nsamp,exinds,exinde); /* mean */
    sdr->acq.cn0=10*log10(maxP/meanP/sdr->ctime);

    /* peak ratio */
    maxP2=maxvd(&P[freqi*sdr->nsamp],sdr->nsamp,exinds,exinde,&maxi);

    sdr->acq.peakr=maxP/maxP2;
    sdr->acq.acqcodei=codei;
    sdr->acq.freqi=freqi;
    sdr->acq.acqfreq=sdr->acq.freq[freqi];

    return sdr->acq.peakr>ACQTH;
}
