#ifndef __NV50_OUTPUT_H__
#define __NV50_OUTPUT_H__

typedef struct G80OutputPrivRec {
    ORType type;
    ORNum or;

    xf86OutputPtr partner;
    I2CBusPtr i2c;

    xf86OutputStatus cached_status;

    void (*set_pclk)(xf86OutputPtr, int pclk);
} G80OutputPrivRec, *G80OutputPrivPtr;

void G80OutputSetPClk(xf86OutputPtr, int pclk);
int G80OutputModeValid(xf86OutputPtr, DisplayModePtr);
Bool G80OutputModeFixup(xf86OutputPtr, DisplayModePtr mode, DisplayModePtr adjusted_mode);
void G80OutputPrepare(xf86OutputPtr);
void G80OutputCommit(xf86OutputPtr);
void G80OutputPartnersDetect(xf86OutputPtr dac, xf86OutputPtr sor, I2CBusPtr i2c);
void G80OutputResetCachedStatus(ScrnInfoPtr);
DisplayModePtr G80OutputGetDDCModes(xf86OutputPtr);
void G80OutputDestroy(xf86OutputPtr);
Bool G80CreateOutputs(ScrnInfoPtr);

/* g80_dac.c */
xf86OutputPtr G80CreateDac(ScrnInfoPtr, ORNum);
Bool G80DacLoadDetect(xf86OutputPtr);

/* g80_sor.c */
xf86OutputPtr G80CreateSor(ScrnInfoPtr, ORNum);

#endif
