#ifndef __NV50_DISPLAY_H__
#define __NV50_DISPLAY_H__

#include "nv50_type.h"

Bool G80DispPreInit(ScrnInfoPtr);
Bool G80DispInit(ScrnInfoPtr);
void G80DispShutdown(ScrnInfoPtr);

void G80DispCommand(ScrnInfoPtr, CARD32 addr, CARD32 data);
#define C(mthd, data) G80DispCommand(pScrn, (mthd), (data))

Head G80CrtcGetHead(xf86CrtcPtr);

void G80CrtcBlankScreen(xf86CrtcPtr, Bool blank);
void G80CrtcEnableCursor(xf86CrtcPtr, Bool update);
void G80CrtcDisableCursor(xf86CrtcPtr, Bool update);
void G80CrtcSetCursorPosition(xf86CrtcPtr, int x, int y);

void G80DispCreateCrtcs(ScrnInfoPtr pScrn);

#endif
