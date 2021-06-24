#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/time.h>
#include <iomanip>

#include "ap_int.h"

typedef ap_uint<32> IndexType;
#define INDEX_FMT  "u"
#define INDEX_TYPE_BIT_WIDTH 32

typedef ap_uint<16> CompressedIndexType;
#define COMPRESSED_INDEX_TYPE_BIT_WIDTH 16

typedef ap_uint<1> BoolType;

#if DOUBLE == 0
	typedef float ValueType;
	#define VALUE_FMT  "f"
	#define VALUE_TYPE_BIT_WIDTH 32
#elif DOUBLE == 1
	typedef double ValueType;
	#define VALUE_FMT "lf"
	#define VALUE_TYPE_BIT_WIDTH 64
#endif

#define HEADER_FMT "%" INDEX_FMT " %" INDEX_FMT " %" INDEX_FMT "\n"
#define LINE_FMT   "%" INDEX_FMT " %" INDEX_FMT " %" VALUE_FMT "\n"

#if VF == 1
	#define VectFactor 1
#elif VF == 2
	#define VectFactor 2
#elif VF == 4
	#define VectFactor 4
#elif VF == 8
	#define VectFactor 8
#endif

#if CU == 1
	#define ComputeUnits 1
	#define COLS_DIV_BLOCKS (32768)
#elif CU == 2
	#define ComputeUnits 2
	#define COLS_DIV_BLOCKS (32768)
#elif CU == 4
	#define ComputeUnits 4
	#define COLS_DIV_BLOCKS (32768)
#elif CU == 8
	#define ComputeUnits 8
	#define COLS_DIV_BLOCKS (32768)
#elif CU == 10
	#define ComputeUnits 10
	#define COLS_DIV_BLOCKS (16384)
#elif CU == 12
	#define ComputeUnits 12
	#define COLS_DIV_BLOCKS (16384)
#endif

#define BUS_BIT_WIDTH 128

#define RATIO_i (BUS_BIT_WIDTH/INDEX_TYPE_BIT_WIDTH)
#define RATIO_v (BUS_BIT_WIDTH/VALUE_TYPE_BIT_WIDTH)
#define RATIO_ci (BUS_BIT_WIDTH/COMPRESSED_INDEX_TYPE_BIT_WIDTH)

#define RATIO_col_val (RATIO_ci/RATIO_v+1)

typedef ap_uint<BUS_BIT_WIDTH> BusDataType;

union Union_double_uint {
	ValueType 			f;
	unsigned long int 	apint;
};

double getTimestamp();

#endif //__UTIL_H__
