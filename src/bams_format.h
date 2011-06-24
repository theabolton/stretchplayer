/*
 * Copyright(c) 2011 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This file is part of BAMS (Basic Audio Mixing Subroutines)
 *
 * BAMS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Tritium is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __LIBBAMS_BAMS_FORMAT_H__
#define __LIBBAMS_BAMS_FORMAT_H__

#include <stdint.h>

#if defined __cplusplus
extern "C"
{
#endif


/**
 * Format conversion routines
 *
 * Naming convention:
 *
 *    bams_copy_DSTFMT_SRCFMT(DSTFMT *dst,
 *                            int stride,
 *                            SRCFMT *src,
 *                            int stride,
 *                            unsigned long count)
 *
 * Where DSTFMT and SRCFMT are one of:
 *
 *   s8
 *   u8
 *   s16le
 *   s16be
 *   u16le
 *   u16be
 *   s24le3
 *   s24be3
 *   u24le3
 *   u24be3
 *   s24le4
 *   s24be4
 *   u24le4
 *   u24be4
 *   s32le
 *   s32be
 *   u32le
 *   u32be
 *   floatle
 *   floatbe
 *
 * N.B. - Sample conversions may not all be available, since there are
 * currently 380 combinations to write.
 *
 */

/* Type definitions
 *
 */

typedef  int8_t bams_sample_s8_t;
typedef  uint8_t bams_sample_u8_t;
typedef  int16_t bams_sample_s16le_t;
typedef  int16_t bams_sample_s16be_t;
typedef  uint16_t bams_sample_u16le_t;
typedef  uint16_t bams_sample_u16be_t;
/*
typedef  bams_sample_s24le3_t;
typedef  bams_sample_s24be3_t;
typedef  bams_sample_u24le3_t;
typedef  bams_sample_u24be3_t;
*/
typedef  int32_t bams_sample_s24le4_t;
typedef  int32_t bams_sample_s24be4_t;
typedef  uint32_t bams_sample_u24le4_t;
typedef  uint32_t bams_sample_u24be4_t;
typedef  int32_t bams_sample_s32le_t;
typedef  int32_t bams_sample_s32be_t;
typedef  uint32_t bams_sample_u32le_t;
typedef  uint32_t bams_sample_u32be_t;
typedef  float bams_sample_floatle_t;
typedef  float bams_sample_floatbe_t;

/* Macro for generating function prototypes
 */
#define BAMS_COPY(d, s)				    \
	void bams_copy_ ## d ## _ ## s(			    \
		bams_sample_ ## d ## _t *dst,		    \
		int dest_stride,		    \
		bams_sample_ ## s ## _t *src,		    \
		int src_stride,			    \
		unsigned long count		    \
		)
	

/* Function prototypes
 *
 * N.B. DO NOT USE src_stride != 1.  It is currently unsupported, but
 * here for future use.
 */
BAMS_COPY(s16le, floatle);
BAMS_COPY(s16be, floatle);
BAMS_COPY(s16le, floatbe);
BAMS_COPY(s16be, floatbe);
BAMS_COPY(u16le, floatle);
BAMS_COPY(u16be, floatle);
BAMS_COPY(u16le, floatbe);
BAMS_COPY(u16be, floatbe);

/* UTILITY FUNCTIONS
 *
 * size is in bytes, not bits.
 */
void
bams_byte_reorder_in_place(void *buf, int size, int stride, unsigned long count);

/* native byte ordering
 */
void
bams_convert_int_to_uint(void *buf, int size, int stride, unsigned long count);

/* native byte ordering
 */
void
bams_convert_uint_to_int(void *buf, int size, int stride, unsigned long count);


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* __LIBBAMS_BAMS_FORMAT_H__ */
