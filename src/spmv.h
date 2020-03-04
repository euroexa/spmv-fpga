#ifndef __SPMV_H__
#define __SPMV_H__

#include "util.h"

#if CU == 1
#pragma SDS data sys_port(col_ind1:AFI)
#pragma SDS data sys_port(values1:AFI)
#pragma SDS data sys_port(y1:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(col_ind1:SEQUENTIAL)
#pragma SDS data access_pattern(values1:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(col_ind1[0:nr_nzeros1/RATIO_i])
#pragma SDS data zero_copy(values1[0:nr_nzeros1/RATIO_v])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(col_ind1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(values1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,
	IndexType nr_nzeros1,

	BusDataType	*col_ind1,
	BusDataType *values1,

	BusDataType *y1,
	BusDataType *x, 		IndexType nr_cols);



#elif CU == 2
#pragma SDS data sys_port(col_ind1:AFI,	col_ind2:AFI)
#pragma SDS data sys_port(values1:AFI,	values2:AFI)
#pragma SDS data sys_port(y1:AFI,		y2:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(col_ind1:SEQUENTIAL,	col_ind2:SEQUENTIAL)
#pragma SDS data access_pattern(values1:SEQUENTIAL,		values2:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,			y2:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(col_ind1[0:nr_nzeros1/RATIO_i],	col_ind2[0:nr_nzeros2/RATIO_i])
#pragma SDS data zero_copy(values1[0:nr_nzeros1/RATIO_v],	values2[0:nr_nzeros2/RATIO_v])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(col_ind1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(values1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,

	BusDataType	*col_ind1,	BusDataType	*col_ind2,
	BusDataType *values1,	BusDataType *values2,

	BusDataType *y1,		BusDataType *y2,
	BusDataType *x, 		IndexType nr_cols);



#elif CU == 4
#pragma SDS data sys_port(col_ind1:AFI,	col_ind2:AFI, col_ind3:AFI,	col_ind4:AFI)
#pragma SDS data sys_port(values1:AFI,	values2:AFI,  values3:AFI,	values4:AFI)
#pragma SDS data sys_port(y1:AFI,		y2:AFI,		  y3:AFI,		y4:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(col_ind1:SEQUENTIAL,	col_ind2:SEQUENTIAL,	col_ind3:SEQUENTIAL,	col_ind4:SEQUENTIAL)
#pragma SDS data access_pattern(values1:SEQUENTIAL,		values2:SEQUENTIAL,		values3:SEQUENTIAL,		values4:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,			y2:SEQUENTIAL,			y3:SEQUENTIAL,			y4:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(col_ind1[0:nr_nzeros1/RATIO_i],	col_ind2[0:nr_nzeros2/RATIO_i],	col_ind3[0:nr_nzeros3/RATIO_i],	col_ind4[0:nr_nzeros4/RATIO_i])
#pragma SDS data zero_copy(values1[0:nr_nzeros1/RATIO_v],	values2[0:nr_nzeros2/RATIO_v],	values3[0:nr_nzeros3/RATIO_v],	values4[0:nr_nzeros4/RATIO_v])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(col_ind1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(values1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,

	BusDataType	*col_ind1,	BusDataType	*col_ind2,	BusDataType	*col_ind3,	BusDataType	*col_ind4,
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,
	BusDataType *x, 		IndexType nr_cols);



#elif CU == 8
#pragma SDS data sys_port(col_ind1:AFI,	col_ind2:AFI, col_ind3:AFI,	col_ind4:AFI,	col_ind5:AFI,	col_ind6:AFI,	col_ind7:AFI,	col_ind8:AFI)
#pragma SDS data sys_port(values1:AFI,	values2:AFI,  values3:AFI,	values4:AFI,	values5:AFI,	values6:AFI,	values7:AFI,	values8:AFI)
#pragma SDS data sys_port(y1:AFI,		y2:AFI,		  y3:AFI,		y4:AFI,			y5:AFI,			y6:AFI,			y7:AFI,			y8:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(col_ind1:SEQUENTIAL,	col_ind2:SEQUENTIAL,	col_ind3:SEQUENTIAL,	col_ind4:SEQUENTIAL,	col_ind5:SEQUENTIAL,	col_ind6:SEQUENTIAL,	col_ind7:SEQUENTIAL,	col_ind8:SEQUENTIAL)
#pragma SDS data access_pattern(values1:SEQUENTIAL,		values2:SEQUENTIAL,		values3:SEQUENTIAL,		values4:SEQUENTIAL,		values5:SEQUENTIAL,		values6:SEQUENTIAL,		values7:SEQUENTIAL,		values8:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,			y2:SEQUENTIAL,			y3:SEQUENTIAL,			y4:SEQUENTIAL,			y5:SEQUENTIAL,			y6:SEQUENTIAL,			y7:SEQUENTIAL,			y8:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(col_ind1[0:nr_nzeros1/RATIO_i],	col_ind2[0:nr_nzeros2/RATIO_i],	col_ind3[0:nr_nzeros3/RATIO_i],	col_ind4[0:nr_nzeros4/RATIO_i],	col_ind5[0:nr_nzeros5/RATIO_i],	col_ind6[0:nr_nzeros6/RATIO_i],	col_ind7[0:nr_nzeros7/RATIO_i],	col_ind8[0:nr_nzeros8/RATIO_i])
#pragma SDS data zero_copy(values1[0:nr_nzeros1/RATIO_v],	values2[0:nr_nzeros2/RATIO_v],	values3[0:nr_nzeros3/RATIO_v],	values4[0:nr_nzeros4/RATIO_v],	values5[0:nr_nzeros5/RATIO_v],	values6[0:nr_nzeros6/RATIO_v],	values7[0:nr_nzeros7/RATIO_v],	values8[0:nr_nzeros8/RATIO_v])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v],			y5[0:nr_rows5/RATIO_v],			y6[0:nr_rows6/RATIO_v],			y7[0:nr_rows7/RATIO_v],			y8[0:nr_rows8/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(col_ind1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	col_ind8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(values1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	values8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,		y8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,		IndexType nr_rows5,		IndexType nr_rows6,		IndexType nr_rows7,		IndexType nr_rows8,		
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,	IndexType nr_nzeros5,	IndexType nr_nzeros6,	IndexType nr_nzeros7,	IndexType nr_nzeros8,	

	BusDataType	*col_ind1,	BusDataType	*col_ind2,	BusDataType	*col_ind3,	BusDataType	*col_ind4,	BusDataType	*col_ind5,	BusDataType	*col_ind6,	BusDataType	*col_ind7,	BusDataType	*col_ind8,	
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,	BusDataType *values5,	BusDataType *values6,	BusDataType *values7,	BusDataType *values8,	

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,		BusDataType *y5,		BusDataType *y6,		BusDataType *y7,		BusDataType *y8,		
	BusDataType *x, 		IndexType nr_cols);



#endif

#endif //__SPMV_H__
