/*
\section{Bit-Fields of various Sizes}

This file contains definitions of bitfields of various sizes.  The
definitions vary by compiler and architectures.  The unsigned types
defined here are "Ubits8", "Ubits16", "Ubits32", and "Ubits64".
Note that unsigned arithmetic can lead to unexpected results.

*/

#ifndef _BITS_H
#define _BITS_H

#include <stdint.h>

#include "assert.h"

#define byte_bits 8
typedef unsigned int Uint;

typedef uint8_t Ubits8;
typedef uint16_t Ubits16;
typedef uint32_t Ubits32;
typedef uint64_t Ubits64;

typedef int8_t Bits8;
typedef int16_t Bits16;
typedef int32_t Bits32;
typedef int64_t Bits64;

#define BITS(datatype) (int)(sizeof(datatype) * byte_bits)
#define INT_BITS (int)(sizeof(int) * byte_bits)
#define LONG_BITS (int)(sizeof(long) * byte_bits)

#define INT_SIGN_BIT_MASK (0x1 << (IN_BITS - 1))
#define LONG_SIGN_BIT_MASK (0x1 << (LONG_BITS - 1))

#define POINTER_INT intptr_t

#endif  // _BITS_H
