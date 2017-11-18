/*
 * This file is part of the Scale2x project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef SCALER_SCALE6X_H
#define SCALER_SCALE6X_H

#if defined(_MSC_VER)
#define __restrict__
#endif

#ifdef __sgi
#define __restrict__ __restrict
#endif

typedef unsigned char scale6x_uint8;
typedef unsigned short scale6x_uint16;
typedef unsigned scale6x_uint32;

void scale6x_8_def(scale6x_uint8* dst0, scale6x_uint8* dst1, scale6x_uint8* dst2, scale6x_uint8* dst3, scale6x_uint8* dst4, scale6x_uint8* dst5, const scale6x_uint8* src0, const scale6x_uint8* src1, const scale6x_uint8* src2, unsigned count);
void scale6x_16_def(scale6x_uint16* dst0, scale6x_uint16* dst1, scale6x_uint16* dst2, scale6x_uint16* dst3, scale6x_uint16* dst4, scale6x_uint16* dst5, const scale6x_uint16* src0, const scale6x_uint16* src1, const scale6x_uint16* src2, unsigned count);
void scale6x_32_def(scale6x_uint32* dst0, scale6x_uint32* dst1, scale6x_uint32* dst2, scale6x_uint32* dst3, scale6x_uint32* dst4, scale6x_uint32* dst5, const scale6x_uint32* src0, const scale6x_uint32* src1, const scale6x_uint32* src2, unsigned count);

#endif
