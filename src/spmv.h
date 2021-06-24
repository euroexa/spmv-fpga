#ifndef __SPMV_H__
#define __SPMV_H__

#include "util.h"

#if CU == 1
#pragma SDS data sys_port(submatrix1:AFI)
#pragma SDS data sys_port(y1:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			
	IndexType nr_nzeros1,		

	BusDataType	*submatrix1,	
	IndexType nr_ci1,			
	IndexType nr_val1,			

	BusDataType *y1,			
	BusDataType *x, 			IndexType nr_cols);



#elif CU == 2
#pragma SDS data sys_port(submatrix1:AFI,	submatrix2:AFI)
#pragma SDS data sys_port(y1:AFI,			y2:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL,		submatrix2:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,				y2:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)],	submatrix2[0:(nr_ci2+nr_val2)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	
	IndexType nr_ci1,			IndexType nr_ci2,			
	IndexType nr_val1,			IndexType nr_val2,			

	BusDataType *y1,			BusDataType *y2,			
	BusDataType *x, 			IndexType nr_cols);



#elif CU == 4
#pragma SDS data sys_port(submatrix1:AFI,	submatrix2:AFI,		submatrix3:AFI,		submatrix4:AFI)
#pragma SDS data sys_port(y1:AFI,			y2:AFI,				y3:AFI,				y4:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL,		submatrix2:SEQUENTIAL,		submatrix3:SEQUENTIAL,		submatrix4:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,				y2:SEQUENTIAL,				y3:SEQUENTIAL,				y4:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)],	submatrix2[0:(nr_ci2+nr_val2)],	submatrix3[0:(nr_ci3+nr_val3)],	submatrix4[0:(nr_ci4+nr_val4)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			
	BusDataType *x, 			IndexType nr_cols);



#elif CU == 8
#pragma SDS data sys_port(submatrix1:AFI,	submatrix2:AFI,		submatrix3:AFI,		submatrix4:AFI,		submatrix5:AFI,		submatrix6:AFI,		submatrix7:AFI,		submatrix8:AFI)
#pragma SDS data sys_port(y1:AFI,			y2:AFI,				y3:AFI,				y4:AFI,				y5:AFI,				y6:AFI,				y7:AFI,				y8:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL,		submatrix2:SEQUENTIAL,		submatrix3:SEQUENTIAL,		submatrix4:SEQUENTIAL,		submatrix5:SEQUENTIAL,		submatrix6:SEQUENTIAL,		submatrix7:SEQUENTIAL,		submatrix8:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,				y2:SEQUENTIAL,				y3:SEQUENTIAL,				y4:SEQUENTIAL,				y5:SEQUENTIAL,				y6:SEQUENTIAL,				y7:SEQUENTIAL,				y8:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)],	submatrix2[0:(nr_ci2+nr_val2)],	submatrix3[0:(nr_ci3+nr_val3)],	submatrix4[0:(nr_ci4+nr_val4)],	submatrix5[0:(nr_ci5+nr_val5)],	submatrix6[0:(nr_ci6+nr_val6)],	submatrix7[0:(nr_ci7+nr_val7)],	submatrix8[0:(nr_ci8+nr_val8)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v],			y5[0:nr_rows5/RATIO_v],			y6[0:nr_rows6/RATIO_v],			y7[0:nr_rows7/RATIO_v],			y8[0:nr_rows8/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,		
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,	

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,		
	BusDataType *x, 			IndexType nr_cols);



#elif CU == 10
#pragma SDS data sys_port(submatrix1:AFI,	submatrix2:AFI,		submatrix3:AFI,		submatrix4:AFI,		submatrix5:AFI,		submatrix6:AFI,		submatrix7:AFI,		submatrix8:AFI,		submatrix9:AFI,		submatrix10:AFI)
#pragma SDS data sys_port(y1:AFI,			y2:AFI,				y3:AFI,				y4:AFI,				y5:AFI,				y6:AFI,				y7:AFI,				y8:AFI,				y9:AFI,				y10:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL,		submatrix2:SEQUENTIAL,		submatrix3:SEQUENTIAL,		submatrix4:SEQUENTIAL,		submatrix5:SEQUENTIAL,		submatrix6:SEQUENTIAL,		submatrix7:SEQUENTIAL,		submatrix8:SEQUENTIAL,		submatrix9:SEQUENTIAL,		submatrix10:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,				y2:SEQUENTIAL,				y3:SEQUENTIAL,				y4:SEQUENTIAL,				y5:SEQUENTIAL,				y6:SEQUENTIAL,				y7:SEQUENTIAL,				y8:SEQUENTIAL,				y9:SEQUENTIAL,				y10:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)],	submatrix2[0:(nr_ci2+nr_val2)],	submatrix3[0:(nr_ci3+nr_val3)],	submatrix4[0:(nr_ci4+nr_val4)],	submatrix5[0:(nr_ci5+nr_val5)],	submatrix6[0:(nr_ci6+nr_val6)],	submatrix7[0:(nr_ci7+nr_val7)],	submatrix8[0:(nr_ci8+nr_val8)],	submatrix9[0:(nr_ci9+nr_val9)],	submatrix10[0:(nr_ci10+nr_val10)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v],			y5[0:nr_rows5/RATIO_v],			y6[0:nr_rows6/RATIO_v],			y7[0:nr_rows7/RATIO_v],			y8[0:nr_rows8/RATIO_v],			y9[0:nr_rows9/RATIO_v],			y10[0:nr_rows10/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix9:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix10:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y9:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y10:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,
	BusDataType *x, 			IndexType nr_cols);



#elif CU == 12
#pragma SDS data sys_port(submatrix1:AFI,	submatrix2:AFI,		submatrix3:AFI,		submatrix4:AFI,		submatrix5:AFI,		submatrix6:AFI,		submatrix7:AFI,		submatrix8:AFI,		submatrix9:AFI,		submatrix10:AFI,		submatrix11:AFI,		submatrix12:AFI)
#pragma SDS data sys_port(y1:AFI,			y2:AFI,				y3:AFI,				y4:AFI,				y5:AFI,				y6:AFI,				y7:AFI,				y8:AFI,				y9:AFI,				y10:AFI,				y11:AFI,				y12:AFI)
#pragma SDS data sys_port(x:AFI)

#pragma SDS data access_pattern(submatrix1:SEQUENTIAL,		submatrix2:SEQUENTIAL,		submatrix3:SEQUENTIAL,		submatrix4:SEQUENTIAL,		submatrix5:SEQUENTIAL,		submatrix6:SEQUENTIAL,		submatrix7:SEQUENTIAL,		submatrix8:SEQUENTIAL,		submatrix9:SEQUENTIAL,		submatrix10:SEQUENTIAL,		submatrix11:SEQUENTIAL,		submatrix12:SEQUENTIAL)
#pragma SDS data access_pattern(y1:SEQUENTIAL,				y2:SEQUENTIAL,				y3:SEQUENTIAL,				y4:SEQUENTIAL,				y5:SEQUENTIAL,				y6:SEQUENTIAL,				y7:SEQUENTIAL,				y8:SEQUENTIAL,				y9:SEQUENTIAL,				y10:SEQUENTIAL,				y11:SEQUENTIAL,				y12:SEQUENTIAL)
#pragma SDS data access_pattern(x:SEQUENTIAL)

#pragma SDS data zero_copy(submatrix1[0:(nr_ci1+nr_val1)],	submatrix2[0:(nr_ci2+nr_val2)],	submatrix3[0:(nr_ci3+nr_val3)],	submatrix4[0:(nr_ci4+nr_val4)],	submatrix5[0:(nr_ci5+nr_val5)],	submatrix6[0:(nr_ci6+nr_val6)],	submatrix7[0:(nr_ci7+nr_val7)],	submatrix8[0:(nr_ci8+nr_val8)],	submatrix9[0:(nr_ci9+nr_val9)],	submatrix10[0:(nr_ci10+nr_val10)],	submatrix11[0:(nr_ci11+nr_val11)],	submatrix12[0:(nr_ci12+nr_val12)])
#pragma SDS data zero_copy(y1[0:nr_rows1/RATIO_v],			y2[0:nr_rows2/RATIO_v],			y3[0:nr_rows3/RATIO_v],			y4[0:nr_rows4/RATIO_v],			y5[0:nr_rows5/RATIO_v],			y6[0:nr_rows6/RATIO_v],			y7[0:nr_rows7/RATIO_v],			y8[0:nr_rows8/RATIO_v],			y9[0:nr_rows9/RATIO_v],			y10[0:nr_rows10/RATIO_v],			y11[0:nr_rows11/RATIO_v],			y12[0:nr_rows12/RATIO_v])
#pragma SDS data zero_copy(x[0:nr_cols/RATIO_v])

#pragma SDS data mem_attribute(submatrix1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix9:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix10:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix11:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,	submatrix12:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(y1:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y2:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y3:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y4:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y5:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y6:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y7:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y8:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y9:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y10:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y11:PHYSICAL_CONTIGUOUS|NON_CACHEABLE,			y12:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(x:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,		IndexType nr_rows11,		IndexType nr_rows12,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,		IndexType nr_nzeros11,		IndexType nr_nzeros12,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,	BusDataType	*submatrix11,	BusDataType	*submatrix12,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,			IndexType nr_ci11,			IndexType nr_ci12,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,			IndexType nr_val11,			IndexType nr_val12,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,			BusDataType *y11,			BusDataType *y12,
	BusDataType *x, 			IndexType nr_cols);



#endif

#endif //__SPMV_H__
