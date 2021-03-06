/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
*/
/*
 * Copyright 2017 Saso Kiselkov. All rights reserved.
 */

#if	IBM
#include <windows.h>
#else	/* !IBM */
#include <sys/time.h>
#endif	/* !IBM */
#include <stdlib.h>

#include <acfutils/time.h>

uint64_t
microclock(void)
{
#if	IBM
	LARGE_INTEGER val, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&val);
	return (((double)val.QuadPart / (double)freq.QuadPart) * 1000000.0);
#else	/* !IBM */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000000llu) + tv.tv_usec);
#endif	/* !IBM */
}
