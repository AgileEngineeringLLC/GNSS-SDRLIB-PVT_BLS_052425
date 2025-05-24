
#ifndef BLADE_RF_H_
#define BLADE_RF_H_

#define BLADERF_DATABUFF_SIZE 32768
extern int bladerf_init(void);
extern void bladerf_quit(void);
extern int bladerf_initconf(void);
extern int bladerf_start(void);
extern int bladerf_stop(void);
extern void calibration_dcoffset(double *inbuf, int n, int dtype, char *outbuf);
extern void bladerf_exp(unsigned char *buf, int n, char *expbuf);
extern void bladerf_getbuff(uint64_t buffloc, int n, char *expbuf);
extern void fbladerf_pushtomembuf(void);

#endif /* BLADE_RF_H_ */