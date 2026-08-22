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

#include <PR/guint.h>
#include <string.h> /* PORT (ungated, netmenu-only TU): for ssb64_float_bits memcpy. */

/* PORT (ungated, netmenu-only TU): see sinf.c ssb64_float_bits — memcpy avoids
 * strict-aliasing UB on Clang. Offline build does not compile this file. See
 * docs/bugs/netplay_cross_isa_libm_trig_2026-06-04.md. */
static int ssb64_float_bits(float x)
{
	int bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

/* ====================================================================
 * ====================================================================
 *
 * Module: fcos.c
 * $Revision: 1.2 $
 * $Date: 1995/07/12 17:47:57 $
 * $Author: jeffd $
 * $Source: /disk6/Master/cvsmdev2/PR/libultra/gu/cosf.c,v $
 *
 * Revision history:
 *  09-Jun-93 - Original Version
 *
 * Description:	source code for fcos function
 *
 * ====================================================================
 * ====================================================================
 */

#pragma weak fcos = __cosf
#if !(defined(PORT) && defined(SSB64_NETMENU))
/* PORT (gated): N64 ROM weak alias cosf=__cosf. Netmenu omits — gameplay uses __cosf
 * explicitly; bare cosf() would hijack audio-adjacent libc paths if sin were aliased too.
 * See sinf.c and docs/bugs/netplay_cross_isa_libm_trig_2026-06-04.md. */
#pragma weak cosf = __cosf
#endif
#define fcos __cosf

/* coefficients for polynomial approximation of cos on +/- pi/2 */

/* PORT: DU_INIT reorders the { hi, lo } halves for the host endianness so .d is
 * the intended double. Without it, __cosf(0)=+inf on little-endian. See guint.h. */
static const du P[] = {
	DU_INIT(0x3ff00000, 0x00000000), DU_INIT(0xbfc55554, 0xbc83656d), DU_INIT(0x3f8110ed, 0x3804c2a0),
	DU_INIT(0xbf29f6ff, 0xeea56814), DU_INIT(0x3ec5dbdf, 0x0e314bfe),
};

static const du rpi = DU_INIT(0x3fd45f30, 0x6dc9c883);

static const du pihi = DU_INIT(0x400921fb, 0x50000000);

static const du pilo = DU_INIT(0x3e6110b4, 0x611a6263);

static const fu zero = { 0x00000000 };

/* ====================================================================
 *
 * FunctionName		fcos
 *
 * Description		computes cosine of arg
 *
 * ====================================================================
 */

float fcos(float x)
{
	float absx;
	double dx, xsq, poly;
	double dn;
	int n;
	double result;
	int ix, xpt;

	/* PORT (ungated): replaces IDO *(int*)&x — ssb64_float_bits avoids UB (see helper above). */
	ix = ssb64_float_bits(x);
	xpt = (ix >> 22);
	xpt &= 0x1ff;

	/* xpt is exponent(x) + 1 bit of mantissa */

	if (xpt < 0x136)
	{
		/* |x| < 2^28 */

		/* use the standard algorithm from Cody and Waite, doing
		   the computations in double precision
		*/

		absx = ABS(x);

		dx = absx;

		dn = dx * rpi.d + 0.5;
		n = ROUND(dn);
		dn = n;

		dn -= 0.5;

		dx = dx - dn * pihi.d;
		dx = dx - dn * pilo.d; /* dx = x - (n - 0.5)*pi */

		xsq = dx * dx;

		poly = ((P[4].d * xsq + P[3].d) * xsq + P[2].d) * xsq + P[1].d;

		result = dx + (dx * xsq) * poly;

		/* negate result if n is odd */

		if ((n & 1) == 0)
			return ((float)result);

		return (-(float)result);
	}

	if (x != x)
	{
		/* x is a NaN; return a quiet NaN */

#ifdef _IP_NAN_SETS_ERRNO

		*__errnoaddr = EDOM;
#endif

		return (__libm_qnan_f);
	}

	/* just give up and return 0.0 */

	return (zero.f);
}
