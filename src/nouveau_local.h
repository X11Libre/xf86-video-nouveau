/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_LOCAL_H__
#define __NOUVEAU_LOCAL_H__

#include "compiler.h"
#include "xf86_OSproc.h"

#define NOUVEAU_PRIVATE _X_HIDDEN
#define NOUVEAU_PUBLIC _X_EXPORT

#if EXA_VERSION_MINOR >= 4
#define NOUVEAU_EXA_PIXMAPS 0
struct nouveau_pixmap {
	struct nouveau_bo *bo;
	int mapped;
};
#endif

/* Debug output */
#define NOUVEAU_MSG(fmt,args...) ErrorF(fmt, ##args)
#define NOUVEAU_ERR(fmt,args...) \
	ErrorF("%s:%d - "fmt, __func__, __LINE__, ##args)
#if 0
#define NOUVEAU_FALLBACK(fmt,args...) do {    \
	NOUVEAU_ERR("FALLBACK: "fmt, ##args); \
	return FALSE;                         \
} while(0)
#else
#define NOUVEAU_FALLBACK(fmt,args...) do {    \
	return FALSE;                         \
} while(0)
#endif

#define NOUVEAU_TIME_MSEC() GetTimeInMillis()

#define NOUVEAU_ALIGN(x,bytes) (((x) + ((bytes) - 1)) & ~((bytes) - 1))

/* User FIFO control */
//#define NOUVEAU_DMA_TRACE
//#define NOUVEAU_DMA_DEBUG
//#define NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
#define NOUVEAU_DMA_BARRIER mem_barrier();
#define NOUVEAU_DMA_TIMEOUT 2000

#include "nouveau_channel.h"
#include "nouveau_pushbuf.h"
#include "nouveau_grobj.h"
#include "nouveau_bo.h"

NOUVEAU_PRIVATE int
nouveau_pushbuf_flush(struct nouveau_channel *, unsigned);
NOUVEAU_PRIVATE int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *, void *ptr,
			   struct nouveau_bo *, uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor);
NOUVEAU_PRIVATE void
nouveau_grobj_autobind(struct nouveau_grobj *);

/* Push buffer access macros */
static __inline__ void
OUT_RING(struct nouveau_channel *chan, unsigned data)
{
	*(chan->pushbuf->cur++) = (data);
}

static __inline__ void
OUT_RINGp(struct nouveau_channel *chan, const void *data, unsigned size)
{
	memcpy(chan->pushbuf->cur, data, size * 4);
	chan->pushbuf->cur += size;
}

static __inline__ void
OUT_RINGf(struct nouveau_channel *chan, float f)
{
	union { uint32_t i; float f; } c;
	c.f = f;
	OUT_RING(chan, c.i);
}

static __inline__ void
BEGIN_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned size)
{
	if (gr->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_grobj_autobind(gr);
	chan->subc[gr->subc].sequence = chan->subc_sequence++;

	if (chan->pushbuf->remaining < (size + 1))
		nouveau_pushbuf_flush(chan, (size + 1));
	OUT_RING(chan, (gr->subc << 13) | (size << 18) | mthd);
	chan->pushbuf->remaining -= (size + 1);
}

static __inline__ void
FIRE_RING(struct nouveau_channel *chan)
{
	nouveau_pushbuf_flush(chan, 0);
}

static __inline__ void
BIND_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr, unsigned sc)
{
	struct nouveau_subchannel *subc = &gr->channel->subc[sc];
	
	if (subc->gr) {
		if (subc->gr->bound == NOUVEAU_GROBJ_BOUND_EXPLICIT)
			assert(0);
		subc->gr->bound = NOUVEAU_GROBJ_UNBOUND;
	}
	subc->gr = gr;
	subc->gr->subc = sc;
	subc->gr->bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;

	BEGIN_RING(chan, gr, 0x0000, 1);
	OUT_RING  (chan, gr->handle);
}

static __inline__ void
OUT_RELOC(struct nouveau_channel *chan, struct nouveau_bo *bo,
	  unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	nouveau_pushbuf_emit_reloc(chan, chan->pushbuf->cur++, bo,
				   data, flags, vor, tor);
}

/* Raw data + flags depending on FB/TT buffer */
static __inline__ void
OUT_RELOCd(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	OUT_RELOC(chan, bo, data, flags | NOUVEAU_BO_OR, vor, tor);
}

/* FB/TT object handle */
static __inline__ void
OUT_RELOCo(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned flags)
{
	OUT_RELOC(chan, bo, 0, flags | NOUVEAU_BO_OR,
		  chan->vram->handle, chan->gart->handle);
}

/* Low 32-bits of offset */
static __inline__ void
OUT_RELOCl(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_LOW, 0, 0);
}

/* High 32-bits of offset */
static __inline__ void
OUT_RELOCh(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_HIGH, 0, 0);
}

/* Alternate versions of OUT_RELOCx above, takes pixmaps instead of BOs */
#if NOUVEAU_EXA_PIXMAPS
#define OUT_PIXMAPd(chan,pm,data,flags,vor,tor) do {                           \
	struct nouveau_pixmap *nvpix = exaGetPixmapDriverPrivate((pm));        \
	struct nouveau_bo *pmo = nvpix->bo;                                    \
	OUT_RELOCd((chan), pmo, (data), (flags), (vor), (tor));                \
} while(0)
#define OUT_PIXMAPo(chan,pm,flags) do {                                        \
	struct nouveau_pixmap *nvpix = exaGetPixmapDriverPrivate((pm));        \
	struct nouveau_bo *pmo = nvpix->bo;                                    \
	OUT_RELOCo((chan), pmo, (flags));                                      \
} while(0)
#define OUT_PIXMAPl(chan,pm,delta,flags) do {                                  \
	struct nouveau_pixmap *nvpix = exaGetPixmapDriverPrivate((pm));        \
	struct nouveau_bo *pmo = nvpix->bo;                                    \
	OUT_RELOCl((chan), pmo, (delta), (flags));                             \
} while(0)
#define OUT_PIXMAPh(chan,pm,delta,flags) do {                                  \
	struct nouveau_pixmap *nvpix = exaGetPixmapDriverPrivate((pm));        \
	struct nouveau_bo *pmo = nvpix->bo;                                    \
	OUT_RELOCh((chan), pmo, (delta), (flags));                             \
} while(0)
#else
#define OUT_PIXMAPd(chan,pm,data,flags,vor,tor) do {                           \
	OUT_RELOCd((chan), pNv->FB, (data), (flags), (vor), (tor));            \
} while(0)
#define OUT_PIXMAPo(chan,pm,flags) do {                                        \
	OUT_RELOCo((chan), pNv->FB, (flags));                                  \
} while(0)
#define OUT_PIXMAPl(chan,pm,delta,flags) do {                                  \
	OUT_RELOCl((chan), pNv->FB, exaGetPixmapOffset(pm) + (delta), (flags));\
} while(0)
#define OUT_PIXMAPh(chan,pm,delta,flags) do {                                  \
	OUT_RELOCh((chan), pNv->FB, exaGetPixmapOffset(pm) + (delta), (flags));\
} while(0)
#endif

#endif
