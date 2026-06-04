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

/* PORT (ungated, netmenu-only TU): IDO used *(int*)&x bit-pun on float exponent.
 * Undefined behavior on Clang strict-aliasing → intermittent wrong trig / vanish.
 * Offline (SSB64_NETMENU=OFF) does not compile this file. See
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
 * Module: fsin.c
 * $Revision: 1.3 $
 * $Date: 1998/10/09 06:14:51 $
 * $Author: has $
 * $Source: /exdisk2/cvs/N64OS/Master/cvsmdev2/PR/libultra/monegi/gu/sinf.c,v $
 *
 * Revision history:
 *  09-Jun-93 - Original Version
 *
 * Description:	source code for fsin function
 *
 * ====================================================================
 * ====================================================================
 */

#pragma weak fsin = __sinf
#if !(defined(PORT) && defined(SSB64_NETMENU))
/* PORT (gated): N64 ROM weak alias sinf=__sinf. Netmenu omits — sim uses __sinf
 * explicitly; bare sinf() must stay libc for audio LFO (alCents2Ratio hang).
 * See docs/bugs/netplay_cross_isa_libm_trig_2026-06-04.md. */
#pragma weak sinf = __sinf
#endif
#define	fsin __sinf

/* coefficients for polynomial approximation of sin on +/- pi/2 */

/* PORT: SSB64_DU_HL reorders the { hi, lo } halves for the host endianness so .d is the
 * intended double. fsin dodges the broken range-reduction constants for |x|<1.5 via its
 * fast path, but is still wrong for larger angles without this. See guint.h. */
static const du	P[] =
{
	SSB64_DU_HL(0x3ff00000,	0x00000000),
	SSB64_DU_HL(0xbfc55554,	0xbc83656d),
	SSB64_DU_HL(0x3f8110ed,	0x3804c2a0),
	SSB64_DU_HL(0xbf29f6ff,	0xeea56814),
	SSB64_DU_HL(0x3ec5dbdf,	0x0e314bfe),
};

static const du	rpi =
SSB64_DU_HL(0x3fd45f30,	0x6dc9c883);

static const du	pihi =
SSB64_DU_HL(0x400921fb,	0x50000000);

static const du	pilo =
SSB64_DU_HL(0x3e6110b4,	0x611a6263);

static const fu	zero = {0x00000000};


/* ====================================================================
 *
 * FunctionName		fsin
 *
 * Description		computes sine of arg
 *
 * ====================================================================
 */

float
fsin( float x )
{
double	dx, xsq, poly;
double	dn;
int	n;
double	result;
int	ix, xpt;


	/* PORT (ungated): replaces IDO *(int*)&x — ssb64_float_bits avoids UB (see helper above). */
	ix = ssb64_float_bits(x);
	xpt = (ix >> 22);
	xpt &= 0x1ff;

	/* xpt is exponent(x) + 1 bit of mantissa */

	if ( xpt < 0xff )
	{
		/* |x| < 1.5 */

		dx = x;

		if ( xpt >= 0xe6 )
		{
			/* |x| >= 2^(-12) */

			/* compute sin(x) with a standard polynomial approximation */

			xsq = dx*dx;

			poly = ((P[4].d*xsq + P[3].d)*xsq + P[2].d)*xsq + P[1].d;

			result = dx + (dx*xsq)*poly;

			return ( (float)result );
		}

		return ( x );
	}

	if ( xpt < 0x136 )
	{
		/* |x| < 2^28 */

		dx = x;

		/*  reduce argument to +/- pi/2  */

		dn = dx*rpi.d;

		n = ROUND(dn);
		dn = n;

		dx = dx - dn*pihi.d;
		dx = dx - dn*pilo.d;	/* dx = x - n*pi */

		/* compute sin(dx) as before, negating result if n is odd
		*/

		xsq = dx*dx;

		poly = ((P[4].d*xsq + P[3].d)*xsq + P[2].d)*xsq + P[1].d;

		result = dx + (dx*xsq)*poly;


		if ( (n & 1) == 0 )
			return ( (float)result );

		return ( -(float)result );
	}

	if ( x != x )
	{
		/* x is a NaN; return a quiet NaN */

#ifdef _IP_NAN_SETS_ERRNO

		*__errnoaddr = EDOM;
#endif

		return ( __libm_qnan_f );
	}

	/* just give up and return 0.0 */

	return ( zero.f );
}

