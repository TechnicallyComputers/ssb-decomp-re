/**************************************************************************
 *									  *
 *		 Copyright (C) 1994, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#include <PR/mbi.h>
#include <PR/gu.h>

typedef union
{
	struct
	{
		unsigned int hi;
		unsigned int lo;
	} word;

	double d;
} du;

typedef union
{
	unsigned int i;
	float f;
} fu;

/*
 * PORT: du constants in the libultra gu trig (sinf.c/cosf.c) are written as { hi, lo }
 * 32-bit halves of an IEEE-754 double in big-endian N64 memory order. Reading union
 * member .d on a little-endian host (x86/x64/aarch64) byte-swaps the halves, producing
 * garbage constants — e.g. rpi/pihi/pilo overflow the Cody-Waite range reduction so
 * __cosf(0) returns +inf instead of 1.0. Initialize via SSB64_DU_HL so .d reconstructs
 * the intended double regardless of host endianness. Big-endian keeps the N64 order;
 * little-endian (and unknown, i.e. all current PC targets) swaps the halves.
 * See docs/bugs/netplay_cross_isa_libm_trig_2026-06-04.md.
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SSB64_DU_HL(hi, lo) { { (unsigned int)(hi), (unsigned int)(lo) } }
#else
#define SSB64_DU_HL(hi, lo) { { (unsigned int)(lo), (unsigned int)(hi) } }
#endif

#ifndef __GL_GL_H__

typedef float Matrix[4][4];

#endif

#define ROUND(d) (int)(((d) >= 0.0) ? ((d) + 0.5) : ((d)-0.5))
#define ABS(d) ((d) > 0) ? (d) : -(d)

extern float __libm_qnan_f;
