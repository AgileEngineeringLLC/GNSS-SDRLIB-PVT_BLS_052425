//-----------------------------------------------------------------------------
// sdrsync.c : SDR sync thread functions
//
// Edits from Don Kelly, don.kelly@mac.com, 2025
//-----------------------------------------------------------------------------
#include "sdr.h"

// synchronization thread ------------------------------------------------------
// synchronization thread for pseudo range computation
// args   : void   *arg      I   not used
//* return : none
// note : this thread collects all data of sdr channel thread and compute pseudo
//        range at every output timing.
//*-----------------------------------------------------------------------------
extern void *syncthread(void * arg)
{
    int i,j,nsat,isat[MAXOBS],ind[MAXSAT]={0},refi;
    uint64_t sampref,sampbase,codei[MAXSAT],diffcnt,mincodei;
    double codeid[OBSINTERPN],remcode[MAXSAT],samprefd,reftow=0,oldreftow;
    sdrobs_t obs[MAXSAT];
    sdrtrk_t trk[MAXSAT]={{0}};
    int ret=0; // used for function output
    char bufferSync[MSG_LENGTH];

    while (!sdrstat.stopflag) {

        mlock(hobsmtx);
         // copy all tracking data   
        for (i=nsat=0;i<sdrini.nch;i++) {
            if (sdrch[i].nav.flagdec&&sdrch[i].nav.sdreph.eph.week!=0) {
                memcpy(&trk[nsat],&sdrch[i].trk,sizeof(sdrch[i].trk));
                isat[nsat]=i;
                nsat++;
            }
        }
        unmlock(hobsmtx);

         // find minimum tow channel (most distant satellite)   
        oldreftow=reftow;
        reftow=3600*24*7;
        for (i=0;i<nsat;i++) {
            if (trk[i].tow[0]<reftow)
                reftow=trk[i].tow[0];
        }
         // output timing check   
        if (nsat==0||oldreftow==reftow||((int)(reftow*1000)%sdrini.outms)!=0) {
            continue;
        }
         // select same timing index   
        for (i=0;i<nsat;i++) {
            for (j=0;j<OBSINTERPN;j++) {
                if (fabs(trk[i].tow[j]-reftow)<1E-4) {
                    ind[i]=j;
                    break;
                }
            }
            if (j==OBSINTERPN-1&&ind[i]==0)
                SDRPRINTF("error:%s reftow=%.1f tow=%.1f\n",
                    sdrch[isat[i]].satstr,trk[i].tow[OBSINTERPN-1],reftow);
        }

         // decide reference satellite (nearest satellite)   
        mincodei=UINT64_MAX;
        refi=0;
        for (i=0;i<nsat;i++) {
            codei[i]=trk[i].codei[ind[i]];
            remcode[i]=trk[i].remcout[ind[i]];
            if (trk[i].codei[ind[i]]<mincodei) {
                refi=i;
                mincodei=trk[i].codei[ind[i]];
            }
        }
         // reference satellite   
        diffcnt=trk[refi].cntout[ind[refi]]-sdrch[isat[refi]].nav.firstsfcnt;
        sampref=sdrch[isat[refi]].nav.firstsf+
            (uint64_t)(sdrch[isat[refi]].nsamp*
            (-PTIMING/(1000*sdrch[isat[refi]].ctime)+diffcnt));
        sampbase=trk[refi].codei[OBSINTERPN-1]-10*sdrch[isat[refi]].nsamp;
        samprefd=(double)(sampref-sampbase);

         // computation observation data   
        for (i=0;i<nsat;i++) {
            obs[i].sys=sdrch[isat[i]].sys;
            obs[i].prn=sdrch[isat[i]].prn;
            obs[i].week=sdrch[isat[i]].nav.sdreph.week_gpst;
            obs[i].tow=reftow+(double)(PTIMING)/1000;
            obs[i].P=CLIGHT*sdrch[isat[i]].ti*
                ((double)(codei[i]-sampref)-remcode[i]);  // pseudo range   

             // uint64 to double for interp1   
            uint64todouble(trk[i].codei,sampbase,OBSINTERPN,codeid);
            obs[i].L=interp1(codeid,trk[i].L,OBSINTERPN,samprefd);
            obs[i].D=interp1(codeid,trk[i].D,OBSINTERPN,samprefd);
            obs[i].S=trk[i].S[0];
        }

        // Populate the obs_v vector with the initial obs data and nsat.
        // First, zero out obs_v, altho this is not really necessary, but
        // is a good precaution.
        sdrstat.nsatValid = nsat;

        mlock(hobsvecmtx);
        // Zero out everything but PRN, az and el values
        for (int j=0; j<32; j++) {
          sdrstat.obs_v[j*11+0] = j + 1;
          sdrstat.obs_v[j*11+1] = 0;
          sdrstat.obs_v[j*11+2] = 0;
          sdrstat.obs_v[j*11+3] = 0;
          sdrstat.obs_v[j*11+4] = 0;
          sdrstat.obs_v[j*11+5] = 0;
          sdrstat.obs_v[j*11+6] = 0;
          sdrstat.obs_v[j*11+7] = 0;
          sdrstat.obs_v[j*11+8] = 0;
          sdrstat.obsValidList[j] = 0; // Reset obsValidList
        }

        // Load raw obs into obs_v and set validity flag field to 1
        int prn = 0;
        for (i=0;i<nsat;i++) {
            prn = obs[i].prn;  // PRN
            sdrstat.obs_v[(prn-1)*11+0] = (double)prn;         // PRN
            sdrstat.obs_v[(prn-1)*11+1] = 1;           // Validity flag
            sdrstat.obs_v[(prn-1)*11+5] = obs[i].P;    // PR
            sdrstat.obs_v[(prn-1)*11+6] = obs[i].tow;  // TOW
            sdrstat.obs_v[(prn-1)*11+7] = (double)obs[i].week; // GPS week
            sdrstat.obs_v[(prn-1)*11+8] = obs[i].S;    // SNR
        }
        unmlock(hobsvecmtx);

        // Update nsatValid and obsValidList
        ret = updateObsList();

        // Perform prechecks on obs and eph data
        precheckObs();

        // Call pvtProcessor if there are at least four observations and
        // that the eph appears valid.
        mlock(hobsvecmtx);
        nsat = sdrstat.nsatValid;
        unmlock(hobsvecmtx);

        if (nsat >= 4) {
            ret = pvtProcessor();
            if (ret != 0) {
              printf("errorDetected: exiting pvtProcessor\n");
            }
        }
        else {
            snprintf(bufferSync, sizeof(bufferSync),
                 "%.3f  pvtProcessor: PVT not solved for, less than four SVs",
                 sdrstat.elapsedTime);
            add_message(bufferSync);
        }

        // Print obs and nav data to file if printflag selected
        if (sdrstat.printflag) {
          FILE *fptr;
          char *fname = "obsData.txt";
          fptr = fopen(fname,"w");
          if (fptr == NULL) {
            perror("Error opening file\n");
          }

          for (int k=0;k<nsat;k++) {
            fprintf(fptr, "%d\n", obs[k].prn);
            fprintf(fptr, "%.14e\n", obs[k].tow);
            fprintf(fptr, "%.14e\n", obs[k].P);
          }
          for (int k=0;k<nsat;k++) {
             int ind = obs[k].prn-1;
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.sat);
             fprintf(fptr, "%.14e\n", 0.0);
             //fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.toc);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.toes);
             fprintf(fptr, "%.14e\n", 0.0);
             //fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.ttr);
             fprintf(fptr, "%.14e\n", sqrt(sdrch[ind].nav.sdreph.eph.A));
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.e);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.M0);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.omg);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.i0);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.OMG0);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.deln);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.idot);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.OMGd);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.cuc);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.cus);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.crc);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.crs);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.cic);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.cis);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.f0);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.f1);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.f2);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.tgd[0]);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.week);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.iodc);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.iode);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.sat);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.svh);
             fprintf(fptr, "%d\n", sdrch[ind].nav.sdreph.eph.sva);
             fprintf(fptr, "%.14e\n", sdrch[ind].nav.sdreph.eph.fit);
          }

          SDRPRINTF("sdrData file written.\n");
          //fprintf(fptr, " Test write to file.\n");
          fclose(fptr);
          sdrstat.printflag = 0;
        }
    }

    SDRPRINTF("SDR syncthread finished!\n");
    return 0;
}
