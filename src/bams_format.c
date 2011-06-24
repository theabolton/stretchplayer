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

#include "bams_format.h"

#include "jack_memops.h"

#include <endian.h>
#include <assert.h>
#include <string.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
/* ok */
#elif __BYTE_ORDER == __BIG_ENDIAN
/* ok */
#else
#error __BYTE_ORDER is not declared for this arch.  Shoulde be __LITTLE_ENDIAN or __BIG_ENDIAN
#endif

#if defined __cplusplus
extern "C"
{
#endif

void
bams_byte_reorder_in_place(void *buf, int size, int stride, unsigned long count)
{
	char tmp[4];
	char *t, *b;

	assert(size < 5);
	while(count--) {
		memcpy(tmp, buf, size);
		b = (char*)buf;
		t = (char*)&tmp[size - 1];
		switch(size) {
		case 4: (*b++) = (*t--);
		case 3: (*b++) = (*t--);
		case 2: (*b++) = (*t--);
		case 1: (*b++) = (*t--);
		}
		buf += stride * size;
	}
}

void
bams_convert_int_to_uint(void *buf, int size, int stride, unsigned long count)
{
	switch(size) {
	case 1: {
		const char c = 0x80;
		char *b = (char*)buf;
		while(count--) {
			(*b) += c;
			b += stride;
		}
	}       break;
	case 2: {
		const int16_t c = 0x8000;
		int16_t *b = (int16_t*)buf;
		while(count--) {
			(*b) += c;
			b += stride;
		}
	}       break;
	case 3: {
		const int32_t c = 0x800000;
		int32_t tmp;
		char *b = (char*)buf;
		while(count--) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			tmp = (int32_t)b[0]
				+ ((int32_t)b[1] << 8)
				+ ((int32_t)b[2] << 16);
#else
			tmp = (int32_t)b[2]
				+ ((int32_t)b[1] << 8)
				+ ((int32_t)b[0] << 16);
#endif
			tmp += c;
#if __BYTE_ORDER == __LITTLE_ENDIAN
			b[0] = (char)(tmp & 0x000000FF);
			b[1] = (char)( (tmp & 0x0000FF00) >> 8 );
			b[2] = (char)( (tmp & 0x00FF0000) >> 16 );
#else
			b[2] = (char)(tmp & 0x000000FF);
			b[1] = (char)( (tmp & 0x0000FF00) >> 8 );
			b[0] = (char)( (tmp & 0x00FF0000) >> 16 );
#endif

			b += 3*stride;
		}
	}       break;
	case 4: {
		const int32_t c = 0x80000000;
		int32_t *b = (int32_t*)buf;
		while(count--) {
			(*b) += c;
			b += stride;
		}
	}       break;
	default:
		assert(0);
	}
}

void
bams_convert_uint_to_int(void *buf, int size, int stride, unsigned long count)
{
	bams_convert_int_to_uint(buf, size, stride, count);
}

void
bams_copy_s16le_floatle(
	bams_sample_s16le_t *dst,
	int dst_stride,
	bams_sample_floatle_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16le_t), 0);
#else
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16le_t), 0);
	bams_byte_reorder_in_place(dst, sizeof(bams_sample_s16le_t), dst_stride, count);
#endif
}

void
bams_copy_s16be_floatle(
	bams_sample_s16be_t *dst,
	int dst_stride,
	bams_sample_floatle_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16be_t), 0);
	bams_byte_reorder_in_place((char*)dst, sizeof(bams_sample_s16be_t), dst_stride, count);
#else
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16be_t), 0);
#endif
}

void
bams_copy_s16le_floatbe(
	bams_sample_s16le_t *dst,
	int dst_stride,
	bams_sample_floatbe_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16le_t), 0);
#else
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16le_t), 0);
	bams_byte_reorder_in_place(dst, sizeof(bams_sample_s16le_t), dst_stride, count);
#endif
}


void
bams_copy_s16be_floatbe(
	bams_sample_s16be_t *dst,
	int dst_stride,
	bams_sample_floatbe_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16be_t), 0);
	bams_byte_reorder_in_place((char*)dst, sizeof(bams_sample_s16be_t), dst_stride, count);
#else
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_s16be_t), 0);
#endif
}

void
bams_copy_u16le_floatle(
	bams_sample_u16le_t *dst,
	int dst_stride,
	bams_sample_floatle_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16be_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16le_t), dst_stride, 0);
#else
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16le_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16le_t), dst_stride, 0);
	bams_byte_reorder_in_place(dst, sizeof(bams_sample_s16le_t), dst_stride, count);
#endif
}

void
bams_copy_u16be_floatle(
	bams_sample_u16be_t *dst,
	int dst_stride,
	bams_sample_floatle_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16be_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16be_t), dst_stride, 0);
	bams_byte_reorder_in_place((char*)dst, sizeof(bams_sample_s16be_t), dst_stride, count);
#else
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16be_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16be_t), dst_stride, 0);
#endif
}

void
bams_copy_u16le_floatbe(
	bams_sample_u16le_t *dst,
	int dst_stride,
	bams_sample_floatbe_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16le_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16le_t), dst_stride, 0);
#else
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16le_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16le_t), dst_stride, 0);
	bams_byte_reorder_in_place(dst, sizeof(bams_sample_s16le_t), dst_stride, count);
#endif
}

void
bams_copy_u16be_floatbe(
	bams_sample_u16be_t *dst,
	int dst_stride,
	bams_sample_floatbe_t *src,
	int src_stride,
	unsigned long count )
{
	assert(src_stride == 1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	sample_move_d16_sSs((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16be_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16be_t), dst_stride, 0);
	bams_byte_reorder_in_place((char*)dst, sizeof(bams_sample_s16be_t), dst_stride, count);
#else
	sample_move_d16_sS((char*)dst, src, count, dst_stride * sizeof(bams_sample_u16be_t), 0);
	bams_convert_int_to_uint(dst, sizeof(bams_sample_u16be_t), dst_stride, 0);
#endif
}

#if defined __cplusplus
} /* extern "C" */
#endif
