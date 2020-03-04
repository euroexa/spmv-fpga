#include "spmv.h"
#include <hls_stream.h>

const int BUFFER_SIZE=128;

void read_data_col_ind(IndexType nr_nzeros, BusDataType *col_ind, hls::stream<IndexType> &col_fifo)
{
	#pragma HLS INLINE
	for (IndexType i=0; i<nr_nzeros/RATIO_i; i++) {
		#pragma HLS PIPELINE
		BusDataType  tmp_wide = col_ind[i];
		for(int k=0; k<RATIO_i; k++){
			#pragma HLS unroll
			col_fifo << tmp_wide.range(INDEX_TYPE_BIT_WIDTH*(k+1)-1,INDEX_TYPE_BIT_WIDTH*k);
		}
	}
}

void read_data_values(IndexType nr_nzeros, BusDataType *values, hls::stream<ValueType> &values_fifo)
{
	#pragma HLS INLINE
	for (IndexType i=0; i<nr_nzeros/RATIO_v; i++) {
		#pragma HLS PIPELINE
		BusDataType  tmp_wide = values[i];
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			values_fifo << tmp.f;
		}
	}
}

void compute_results(hls::stream<IndexType> &col_fifo, hls::stream<ValueType> &values_fifo, IndexType nr_nzeros, ValueType *x, hls::stream<ValueType> &results_fifo)
{
	#pragma HLS INLINE
	ValueType sum = 0;
	ValueType term[VectFactor];
	#pragma HLS ARRAY_PARTITION variable=term complete dim=1
	BoolType row_end[VectFactor];
	#pragma HLS ARRAY_PARTITION variable=row_end complete dim=1
	for (IndexType i=0; i<nr_nzeros; i+=VectFactor) {
		#pragma HLS PIPELINE

		ValueType value;
		IndexType col;
		IndexType realcol;
		for (IndexType j=0; j<VectFactor; j++) {
			#pragma HLS dependence variable=term inter false
			#pragma HLS dependence variable=row_end inter false
			#pragma HLS unroll
			value   = values_fifo.read();
			col     = col_fifo.read();
			realcol = col.range(30,0);
			term[j] = value * x[realcol];
			row_end[j] = col.range(31,31);
		}	
		
		ValueType sum_tmp = 0;
		for (IndexType j=0; j<VectFactor; j++) {
			#pragma HLS dependence variable=term inter false
			#pragma HLS unroll
			sum_tmp += term[j];
		}
		sum += sum_tmp;

		if(row_end[VectFactor-1]==1){
			results_fifo << sum;
			sum = 0;
		}
	}
}

void write_back_results(hls::stream<ValueType> &results_fifo, IndexType nr_rows, BusDataType *y)
{
	#pragma HLS INLINE
	for (IndexType i=0; i<nr_rows/RATIO_v; i++) {
		#pragma HLS PIPELINE
		BusDataType  tmp_wide;
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.f = results_fifo.read();
			tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k) = tmp.apint;
		}
		y[i] = tmp_wide;
	}
}



#if CU == 1
void spmv_kernel(
	IndexType nr_rows1,
	IndexType nr_nzeros1,

	BusDataType *col_ind1,
	BusDataType *values1,

	BusDataType *y1,

	ValueType *x1
){
	#pragma HLS DATAFLOW
	/*---------------------col_fifo--------------------------------------*/
	hls::stream<IndexType> col_fifo_1;
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1;
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1;
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_col_ind(nr_nzeros1, col_ind1, col_fifo_1);
	
	read_data_values(nr_nzeros1, values1, values_fifo_1);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);

	return;
}

int spmv(
	IndexType nr_rows1,
	IndexType nr_nzeros1,

	BusDataType	*col_ind1,
	BusDataType *values1,

	BusDataType *y1,
	BusDataType *x, 		IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];

	L0:
	for (IndexType i=0; i<nr_cols/RATIO_v; i+=1){
		#pragma HLS PIPELINE
		BusDataType tmp_wide = x[i];
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			x1_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,
				nr_nzeros1,

				col_ind1,
				values1,

				y1,
				x1_local
				);
	return 0;
}



#elif CU == 2
void spmv_kernel(
	IndexType nr_rows1,		IndexType nr_rows2,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,

	BusDataType *col_ind1,	BusDataType *col_ind2,
	BusDataType *values1,	BusDataType *values2,

	BusDataType *y1,		BusDataType *y2,

	ValueType *x1,			ValueType *x2
){
	#pragma HLS DATAFLOW
	/*---------------------col_fifo--------------------------------------*/
	hls::stream<IndexType> col_fifo_1;
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_2;
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1;
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2;
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1;
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2;
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_col_ind(nr_nzeros1, col_ind1, col_fifo_1);
	read_data_col_ind(nr_nzeros2, col_ind2, col_fifo_2);
	
	read_data_values(nr_nzeros1, values1, values_fifo_1);
	read_data_values(nr_nzeros2, values2, values_fifo_2);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);

	return;
}

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,

	BusDataType	*col_ind1,	BusDataType	*col_ind2,
	BusDataType *values1,	BusDataType *values2,

	BusDataType *y1,		BusDataType *y2,
	BusDataType *x, 		IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];
	ValueType x2_local[COLS_DIV_BLOCKS];

	L0:
	for (IndexType i=0; i<nr_cols/RATIO_v; i+=1){
		#pragma HLS PIPELINE
		BusDataType tmp_wide = x[i];
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			x1_local[i*RATIO_v+k] = tmp.f;
			x2_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,	nr_rows2,
				nr_nzeros1,	nr_nzeros2,

				col_ind1,	col_ind2,
				values1,	values2,

				y1,			y2,
				x1_local,	x2_local
				);
	return 0;
}



#elif CU == 4
void spmv_kernel(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,

	BusDataType *col_ind1,	BusDataType *col_ind2,	BusDataType *col_ind3,	BusDataType *col_ind4,
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,

	ValueType *x1,			ValueType *x2,			ValueType *x3,			ValueType *x4
){
	#pragma HLS DATAFLOW
	/*---------------------col_fifo--------------------------------------*/
	hls::stream<IndexType> col_fifo_1;
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_2;
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_3;
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_4;
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1;
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2;
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3;
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4;
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1;
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2;
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3;
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4;
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_col_ind(nr_nzeros1, col_ind1, col_fifo_1);
	read_data_col_ind(nr_nzeros2, col_ind2, col_fifo_2);
	read_data_col_ind(nr_nzeros3, col_ind3, col_fifo_3);
	read_data_col_ind(nr_nzeros4, col_ind4, col_fifo_4);
	
	read_data_values(nr_nzeros1, values1, values_fifo_1);
	read_data_values(nr_nzeros2, values2, values_fifo_2);
	read_data_values(nr_nzeros3, values3, values_fifo_3);
	read_data_values(nr_nzeros4, values4, values_fifo_4);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);
	compute_results(col_fifo_3, values_fifo_3, nr_nzeros3, x3, results_fifo_3);
	compute_results(col_fifo_4, values_fifo_4, nr_nzeros4, x4, results_fifo_4);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);
	write_back_results(results_fifo_3, nr_rows3, y3);
	write_back_results(results_fifo_4, nr_rows4, y4);

	return;
}

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,

	BusDataType	*col_ind1,	BusDataType	*col_ind2,	BusDataType	*col_ind3,	BusDataType	*col_ind4,
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,
	BusDataType *x, 		IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];
	ValueType x2_local[COLS_DIV_BLOCKS];
	ValueType x3_local[COLS_DIV_BLOCKS];
	ValueType x4_local[COLS_DIV_BLOCKS];

	L0:
	for (IndexType i=0; i<nr_cols/RATIO_v; i+=1){
		#pragma HLS PIPELINE
		BusDataType tmp_wide = x[i];
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			x1_local[i*RATIO_v+k] = tmp.f;
			x2_local[i*RATIO_v+k] = tmp.f;
			x3_local[i*RATIO_v+k] = tmp.f;
			x4_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,	nr_rows2,	nr_rows3,	nr_rows4,
				nr_nzeros1,	nr_nzeros2,	nr_nzeros3,	nr_nzeros4,

				col_ind1,	col_ind2,	col_ind3,	col_ind4,
				values1,	values2,	values3,	values4,

				y1,			y2,			y3,			y4,
				x1_local,	x2_local,	x3_local,	x4_local
				);
	return 0;
}



#elif CU == 8
void spmv_kernel(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,		IndexType nr_rows5,		IndexType nr_rows6,		IndexType nr_rows7,		IndexType nr_rows8,		
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,	IndexType nr_nzeros5,	IndexType nr_nzeros6,	IndexType nr_nzeros7,	IndexType nr_nzeros8,	

	BusDataType *col_ind1,	BusDataType *col_ind2,	BusDataType *col_ind3,	BusDataType *col_ind4,	BusDataType *col_ind5,	BusDataType *col_ind6,	BusDataType *col_ind7,	BusDataType *col_ind8,	
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,	BusDataType *values5,	BusDataType *values6,	BusDataType *values7,	BusDataType *values8,	

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,		BusDataType *y5,		BusDataType *y6,		BusDataType *y7,		BusDataType *y8,		

	ValueType *x1,			ValueType *x2,			ValueType *x3,			ValueType *x4,			ValueType *x5,			ValueType *x6,			ValueType *x7,			ValueType *x8
){
	#pragma HLS DATAFLOW
	/*---------------------col_fifo--------------------------------------*/
	hls::stream<IndexType> col_fifo_1;
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_2;
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_3;
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_4;
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_5;
	#pragma HLS STREAM variable=col_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_6;
	#pragma HLS STREAM variable=col_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_7;
	#pragma HLS STREAM variable=col_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<IndexType> col_fifo_8;
	#pragma HLS STREAM variable=col_fifo_8 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1;
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2;
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3;
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4;
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_5;
	#pragma HLS STREAM variable=values_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_6;
	#pragma HLS STREAM variable=values_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_7;
	#pragma HLS STREAM variable=values_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_8;
	#pragma HLS STREAM variable=values_fifo_8 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1;
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2;
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3;
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4;
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_5;
	#pragma HLS STREAM variable=results_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_6;
	#pragma HLS STREAM variable=results_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_7;
	#pragma HLS STREAM variable=results_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_8;
	#pragma HLS STREAM variable=results_fifo_8 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_col_ind(nr_nzeros1, col_ind1, col_fifo_1);
	read_data_col_ind(nr_nzeros2, col_ind2, col_fifo_2);
	read_data_col_ind(nr_nzeros3, col_ind3, col_fifo_3);
	read_data_col_ind(nr_nzeros4, col_ind4, col_fifo_4);
	read_data_col_ind(nr_nzeros5, col_ind5, col_fifo_5);
	read_data_col_ind(nr_nzeros6, col_ind6, col_fifo_6);
	read_data_col_ind(nr_nzeros7, col_ind7, col_fifo_7);
	read_data_col_ind(nr_nzeros8, col_ind8, col_fifo_8);
	
	read_data_values(nr_nzeros1, values1, values_fifo_1);
	read_data_values(nr_nzeros2, values2, values_fifo_2);
	read_data_values(nr_nzeros3, values3, values_fifo_3);
	read_data_values(nr_nzeros4, values4, values_fifo_4);
	read_data_values(nr_nzeros5, values5, values_fifo_5);
	read_data_values(nr_nzeros6, values6, values_fifo_6);
	read_data_values(nr_nzeros7, values7, values_fifo_7);
	read_data_values(nr_nzeros8, values8, values_fifo_8);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);
	compute_results(col_fifo_3, values_fifo_3, nr_nzeros3, x3, results_fifo_3);
	compute_results(col_fifo_4, values_fifo_4, nr_nzeros4, x4, results_fifo_4);
	compute_results(col_fifo_5, values_fifo_5, nr_nzeros5, x5, results_fifo_5);
	compute_results(col_fifo_6, values_fifo_6, nr_nzeros6, x6, results_fifo_6);
	compute_results(col_fifo_7, values_fifo_7, nr_nzeros7, x7, results_fifo_7);
	compute_results(col_fifo_8, values_fifo_8, nr_nzeros8, x8, results_fifo_8);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);
	write_back_results(results_fifo_3, nr_rows3, y3);
	write_back_results(results_fifo_4, nr_rows4, y4);
	write_back_results(results_fifo_5, nr_rows5, y5);
	write_back_results(results_fifo_6, nr_rows6, y6);
	write_back_results(results_fifo_7, nr_rows7, y7);
	write_back_results(results_fifo_8, nr_rows8, y8);

	return;
}

int spmv(
	IndexType nr_rows1,		IndexType nr_rows2,		IndexType nr_rows3,		IndexType nr_rows4,		IndexType nr_rows5,		IndexType nr_rows6,		IndexType nr_rows7,		IndexType nr_rows8,		
	IndexType nr_nzeros1,	IndexType nr_nzeros2,	IndexType nr_nzeros3,	IndexType nr_nzeros4,	IndexType nr_nzeros5,	IndexType nr_nzeros6,	IndexType nr_nzeros7,	IndexType nr_nzeros8,	

	BusDataType	*col_ind1,	BusDataType	*col_ind2,	BusDataType	*col_ind3,	BusDataType	*col_ind4,	BusDataType	*col_ind5,	BusDataType	*col_ind6,	BusDataType	*col_ind7,	BusDataType	*col_ind8,	
	BusDataType *values1,	BusDataType *values2,	BusDataType *values3,	BusDataType *values4,	BusDataType *values5,	BusDataType *values6,	BusDataType *values7,	BusDataType *values8,	

	BusDataType *y1,		BusDataType *y2,		BusDataType *y3,		BusDataType *y4,		BusDataType *y5,		BusDataType *y6,		BusDataType *y7,		BusDataType *y8,		
	BusDataType *x, 		IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];
	ValueType x2_local[COLS_DIV_BLOCKS];
	ValueType x3_local[COLS_DIV_BLOCKS];
	ValueType x4_local[COLS_DIV_BLOCKS];
	ValueType x5_local[COLS_DIV_BLOCKS];
	ValueType x6_local[COLS_DIV_BLOCKS];
	ValueType x7_local[COLS_DIV_BLOCKS];
	ValueType x8_local[COLS_DIV_BLOCKS];

	L0:
	for (IndexType i=0; i<nr_cols/RATIO_v; i+=1){
		#pragma HLS PIPELINE
		BusDataType tmp_wide = x[i];
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			x1_local[i*RATIO_v+k] = tmp.f;
			x2_local[i*RATIO_v+k] = tmp.f;
			x3_local[i*RATIO_v+k] = tmp.f;
			x4_local[i*RATIO_v+k] = tmp.f;
			x5_local[i*RATIO_v+k] = tmp.f;
			x6_local[i*RATIO_v+k] = tmp.f;
			x7_local[i*RATIO_v+k] = tmp.f;
			x8_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,	nr_rows2,	nr_rows3,	nr_rows4,	nr_rows5,	nr_rows6,	nr_rows7,	nr_rows8,	
				nr_nzeros1,	nr_nzeros2,	nr_nzeros3,	nr_nzeros4,	nr_nzeros5,	nr_nzeros6,	nr_nzeros7,	nr_nzeros8,	

				col_ind1,	col_ind2,	col_ind3,	col_ind4,	col_ind5,	col_ind6,	col_ind7,	col_ind8,	
				values1,	values2,	values3,	values4,	values5,	values6,	values7,	values8,	

				y1,			y2,			y3,			y4,			y5,			y6,			y7,			y8,			
				x1_local,	x2_local,	x3_local,	x4_local,	x5_local,	x6_local,	x7_local,	x8_local
				);
	return 0;
}
#endif
