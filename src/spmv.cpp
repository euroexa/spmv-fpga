#include "spmv.h"
#include <hls_stream.h>

const int BUFFER_SIZE=128;

void read_data_submatrix(IndexType nr_elem, BusDataType *submatrix, hls::stream<BusDataType> &col_fifo_wide, hls::stream<BusDataType> &values_fifo_wide)
{
	#pragma HLS INLINE
	/* Threshold is needed for column indices. nr_nzeros = nr_val * RATIO_v */
	for (IndexType i=0; i<nr_elem; i++) {
		#pragma HLS PIPELINE
		BusDataType  tmp_wide = submatrix[i];
		/* Check whether to stream elements in col_ind_fifo (RATIO_ci elements pushed) or values_fifo (RATIO_v elements pushed, but with different "frequency") */

		switch (i%RATIO_col_val)
		{
			case 0:
				col_fifo_wide << tmp_wide;
				break;
			case 1:
				values_fifo_wide << tmp_wide;
				break;
			case 2:
				values_fifo_wide << tmp_wide;
				break;
			case 3:
				values_fifo_wide << tmp_wide;
				break;
			case 4:
				values_fifo_wide << tmp_wide;
				break;
		}
	}
}

void stream_data_col_ind(IndexType nr_ci, IndexType nr_nzeros, hls::stream<BusDataType> &col_fifo_wide, hls::stream<CompressedIndexType> &col_fifo)
{
	#pragma HLS INLINE
	for (IndexType i=0; i<nr_ci; i++) {
		#pragma HLS PIPELINE
		BusDataType  tmp_wide = col_fifo_wide.read();
		for(int k=0; k<RATIO_ci; k++){
			#pragma HLS unroll
			if(i*RATIO_ci + k < nr_nzeros){ // check whether "useful" non-zeros are over
				col_fifo << tmp_wide.range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(k+1)-1,COMPRESSED_INDEX_TYPE_BIT_WIDTH*k);
			}
		}
	}
}

void stream_data_values(IndexType nr_val, hls::stream<BusDataType> &values_fifo_wide, hls::stream<ValueType> &values_fifo)
{
	#pragma HLS INLINE
	for (IndexType i=0; i<nr_val; i++) {	
		#pragma HLS PIPELINE
		BusDataType  tmp_wide = values_fifo_wide.read();
		Union_double_uint tmp;
		for(int k=0; k<RATIO_v; k++){
			#pragma HLS unroll
			tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
			values_fifo << tmp.f;
		}
	}
}

void compute_results(hls::stream<CompressedIndexType> &col_fifo, hls::stream<ValueType> &values_fifo, IndexType nr_nzeros, ValueType *x, hls::stream<ValueType> &results_fifo)
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
		CompressedIndexType col;
		CompressedIndexType realcol;
		for (IndexType j=0; j<VectFactor; j++) {
			#pragma HLS dependence variable=term inter false
			#pragma HLS dependence variable=row_end inter false
			#pragma HLS unroll
			value   = values_fifo.read();
			col     = col_fifo.read();
			realcol = col.range(COMPRESSED_INDEX_TYPE_BIT_WIDTH-2,0);
			term[j] = value * x[realcol];
			row_end[j] = col.range(COMPRESSED_INDEX_TYPE_BIT_WIDTH-1,COMPRESSED_INDEX_TYPE_BIT_WIDTH-1);
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

	BusDataType	*submatrix1,	
	IndexType nr_ci1,			
	IndexType nr_val1,			

	BusDataType *y1,			

	ValueType *x1				
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);

	return;
}

int spmv(
	IndexType nr_rows1,			
	IndexType nr_nzeros1,		

	BusDataType	*submatrix1,	
	IndexType nr_ci1,			
	IndexType nr_val1,			

	BusDataType *y1,			
	BusDataType *x, 			IndexType nr_cols)
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

				submatrix1,	
				nr_ci1,		
				nr_val1,	

				y1,			
				x1_local	
				);
	return 0;
}



#elif CU == 2
void spmv_kernel(
	IndexType nr_rows1,			IndexType nr_rows2,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	
	IndexType nr_ci1,			IndexType nr_ci2,			
	IndexType nr_val1,			IndexType nr_val2,			

	BusDataType *y1,			BusDataType *y2,			

	ValueType *x1,				ValueType *x2				
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_2("col_fifo_wide_2");
	#pragma HLS STREAM variable=col_fifo_wide_2 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_2("values_fifo_wide_2");
	#pragma HLS STREAM variable=values_fifo_wide_2 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_2("col_fifo_2");
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2("values_fifo_2");
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2("results_fifo_2");
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);
	read_data_submatrix(nr_ci2+nr_val2, submatrix2, col_fifo_wide_2, values_fifo_wide_2);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);
	stream_data_col_ind(nr_ci2, nr_nzeros2, col_fifo_wide_2, col_fifo_2);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);
	stream_data_values(nr_val2, values_fifo_wide_2, values_fifo_2);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);

	return;
}

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	
	IndexType nr_ci1,			IndexType nr_ci2,			
	IndexType nr_val1,			IndexType nr_val2,			

	BusDataType *y1,			BusDataType *y2,			
	BusDataType *x, 			IndexType nr_cols)
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

				submatrix1,	submatrix2,	
				nr_ci1,		nr_ci2,		
				nr_val1,	nr_val2,	

				y1,			y2,			
				x1_local,	x2_local	
				);
	return 0;
}



#elif CU == 4
void spmv_kernel(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			

	ValueType *x1,				ValueType *x2,				ValueType *x3,				ValueType *x4				
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_2("col_fifo_wide_2");
	#pragma HLS STREAM variable=col_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_3("col_fifo_wide_3");
	#pragma HLS STREAM variable=col_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_4("col_fifo_wide_4");
	#pragma HLS STREAM variable=col_fifo_wide_4 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_2("values_fifo_wide_2");
	#pragma HLS STREAM variable=values_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_3("values_fifo_wide_3");
	#pragma HLS STREAM variable=values_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_4("values_fifo_wide_4");
	#pragma HLS STREAM variable=values_fifo_wide_4 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_2("col_fifo_2");
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_3("col_fifo_3");
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_4("col_fifo_4");
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2("values_fifo_2");
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3("values_fifo_3");
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4("values_fifo_4");
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2("results_fifo_2");
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3("results_fifo_3");
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4("results_fifo_4");
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);
	read_data_submatrix(nr_ci2+nr_val2, submatrix2, col_fifo_wide_2, values_fifo_wide_2);
	read_data_submatrix(nr_ci3+nr_val3, submatrix3, col_fifo_wide_3, values_fifo_wide_3);
	read_data_submatrix(nr_ci4+nr_val4, submatrix4, col_fifo_wide_4, values_fifo_wide_4);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);
	stream_data_col_ind(nr_ci2, nr_nzeros2, col_fifo_wide_2, col_fifo_2);
	stream_data_col_ind(nr_ci3, nr_nzeros3, col_fifo_wide_3, col_fifo_3);
	stream_data_col_ind(nr_ci4, nr_nzeros4, col_fifo_wide_4, col_fifo_4);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);
	stream_data_values(nr_val2, values_fifo_wide_2, values_fifo_2);
	stream_data_values(nr_val3, values_fifo_wide_3, values_fifo_3);
	stream_data_values(nr_val4, values_fifo_wide_4, values_fifo_4);

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
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			
	BusDataType *x, 			IndexType nr_cols)
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

				submatrix1,	submatrix2,	submatrix3,	submatrix4,	
				nr_ci1,		nr_ci2,		nr_ci3,		nr_ci4,		
				nr_val1,	nr_val2,	nr_val3,	nr_val4,	

				y1,			y2,			y3,			y4,			
				x1_local,	x2_local,	x3_local,	x4_local	
				);
	return 0;
}



#elif CU == 8
void spmv_kernel(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			

	ValueType *x1,				ValueType *x2,				ValueType *x3,				ValueType *x4,				ValueType *x5,				ValueType *x6,				ValueType *x7,				ValueType *x8				
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_2("col_fifo_wide_2");
	#pragma HLS STREAM variable=col_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_3("col_fifo_wide_3");
	#pragma HLS STREAM variable=col_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_4("col_fifo_wide_4");
	#pragma HLS STREAM variable=col_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_5("col_fifo_wide_5");
	#pragma HLS STREAM variable=col_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_6("col_fifo_wide_6");
	#pragma HLS STREAM variable=col_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_7("col_fifo_wide_7");
	#pragma HLS STREAM variable=col_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_8("col_fifo_wide_8");
	#pragma HLS STREAM variable=col_fifo_wide_8 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_2("values_fifo_wide_2");
	#pragma HLS STREAM variable=values_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_3("values_fifo_wide_3");
	#pragma HLS STREAM variable=values_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_4("values_fifo_wide_4");
	#pragma HLS STREAM variable=values_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_5("values_fifo_wide_5");
	#pragma HLS STREAM variable=values_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_6("values_fifo_wide_6");
	#pragma HLS STREAM variable=values_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_7("values_fifo_wide_7");
	#pragma HLS STREAM variable=values_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_8("values_fifo_wide_8");
	#pragma HLS STREAM variable=values_fifo_wide_8 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_2("col_fifo_2");
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_3("col_fifo_3");
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_4("col_fifo_4");
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_5("col_fifo_5");
	#pragma HLS STREAM variable=col_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_6("col_fifo_6");
	#pragma HLS STREAM variable=col_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_7("col_fifo_7");
	#pragma HLS STREAM variable=col_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_8("col_fifo_8");
	#pragma HLS STREAM variable=col_fifo_8 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2("values_fifo_2");
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3("values_fifo_3");
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4("values_fifo_4");
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_5("values_fifo_5");
	#pragma HLS STREAM variable=values_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_6("values_fifo_6");
	#pragma HLS STREAM variable=values_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_7("values_fifo_7");
	#pragma HLS STREAM variable=values_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_8("values_fifo_8");
	#pragma HLS STREAM variable=values_fifo_8 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2("results_fifo_2");
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3("results_fifo_3");
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4("results_fifo_4");
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_5("results_fifo_5");
	#pragma HLS STREAM variable=results_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_6("results_fifo_6");
	#pragma HLS STREAM variable=results_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_7("results_fifo_7");
	#pragma HLS STREAM variable=results_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_8("results_fifo_8");
	#pragma HLS STREAM variable=results_fifo_8 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);
	read_data_submatrix(nr_ci2+nr_val2, submatrix2, col_fifo_wide_2, values_fifo_wide_2);
	read_data_submatrix(nr_ci3+nr_val3, submatrix3, col_fifo_wide_3, values_fifo_wide_3);
	read_data_submatrix(nr_ci4+nr_val4, submatrix4, col_fifo_wide_4, values_fifo_wide_4);
	read_data_submatrix(nr_ci5+nr_val5, submatrix5, col_fifo_wide_5, values_fifo_wide_5);
	read_data_submatrix(nr_ci6+nr_val6, submatrix6, col_fifo_wide_6, values_fifo_wide_6);
	read_data_submatrix(nr_ci7+nr_val7, submatrix7, col_fifo_wide_7, values_fifo_wide_7);
	read_data_submatrix(nr_ci8+nr_val8, submatrix8, col_fifo_wide_8, values_fifo_wide_8);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);
	stream_data_col_ind(nr_ci2, nr_nzeros2, col_fifo_wide_2, col_fifo_2);
	stream_data_col_ind(nr_ci3, nr_nzeros3, col_fifo_wide_3, col_fifo_3);
	stream_data_col_ind(nr_ci4, nr_nzeros4, col_fifo_wide_4, col_fifo_4);
	stream_data_col_ind(nr_ci5, nr_nzeros5, col_fifo_wide_5, col_fifo_5);
	stream_data_col_ind(nr_ci6, nr_nzeros6, col_fifo_wide_6, col_fifo_6);
	stream_data_col_ind(nr_ci7, nr_nzeros7, col_fifo_wide_7, col_fifo_7);
	stream_data_col_ind(nr_ci8, nr_nzeros8, col_fifo_wide_8, col_fifo_8);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);
	stream_data_values(nr_val2, values_fifo_wide_2, values_fifo_2);
	stream_data_values(nr_val3, values_fifo_wide_3, values_fifo_3);
	stream_data_values(nr_val4, values_fifo_wide_4, values_fifo_4);
	stream_data_values(nr_val5, values_fifo_wide_5, values_fifo_5);
	stream_data_values(nr_val6, values_fifo_wide_6, values_fifo_6);
	stream_data_values(nr_val7, values_fifo_wide_7, values_fifo_7);
	stream_data_values(nr_val8, values_fifo_wide_8, values_fifo_8);

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
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,		
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,	

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,		
	BusDataType *x, 			IndexType nr_cols)
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

				submatrix1,	submatrix2,	submatrix3,	submatrix4,	submatrix5,	submatrix6,	submatrix7,	submatrix8,	
				nr_ci1,		nr_ci2,		nr_ci3,		nr_ci4,		nr_ci5,		nr_ci6,		nr_ci7,		nr_ci8,		
				nr_val1,	nr_val2,	nr_val3,	nr_val4,	nr_val5,	nr_val6,	nr_val7,	nr_val8,	

				y1,			y2,			y3,			y4,			y5,			y6,			y7,			y8,			
				x1_local,	x2_local,	x3_local,	x4_local,	x5_local,	x6_local,	x7_local,	x8_local	
				);
	return 0;
}




#elif CU == 10
void spmv_kernel(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,

	ValueType *x1,				ValueType *x2,				ValueType *x3,				ValueType *x4,				ValueType *x5,				ValueType *x6,				ValueType *x7,				ValueType *x8,				ValueType *x9,				ValueType *x10
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_2("col_fifo_wide_2");
	#pragma HLS STREAM variable=col_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_3("col_fifo_wide_3");
	#pragma HLS STREAM variable=col_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_4("col_fifo_wide_4");
	#pragma HLS STREAM variable=col_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_5("col_fifo_wide_5");
	#pragma HLS STREAM variable=col_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_6("col_fifo_wide_6");
	#pragma HLS STREAM variable=col_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_7("col_fifo_wide_7");
	#pragma HLS STREAM variable=col_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_8("col_fifo_wide_8");
	#pragma HLS STREAM variable=col_fifo_wide_8 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_9("col_fifo_wide_9");
	#pragma HLS STREAM variable=col_fifo_wide_9 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_10("col_fifo_wide_10");
	#pragma HLS STREAM variable=col_fifo_wide_10 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_2("values_fifo_wide_2");
	#pragma HLS STREAM variable=values_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_3("values_fifo_wide_3");
	#pragma HLS STREAM variable=values_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_4("values_fifo_wide_4");
	#pragma HLS STREAM variable=values_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_5("values_fifo_wide_5");
	#pragma HLS STREAM variable=values_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_6("values_fifo_wide_6");
	#pragma HLS STREAM variable=values_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_7("values_fifo_wide_7");
	#pragma HLS STREAM variable=values_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_8("values_fifo_wide_8");
	#pragma HLS STREAM variable=values_fifo_wide_8 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_9("values_fifo_wide_9");
	#pragma HLS STREAM variable=values_fifo_wide_9 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_10("values_fifo_wide_10");
	#pragma HLS STREAM variable=values_fifo_wide_10 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_2("col_fifo_2");
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_3("col_fifo_3");
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_4("col_fifo_4");
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_5("col_fifo_5");
	#pragma HLS STREAM variable=col_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_6("col_fifo_6");
	#pragma HLS STREAM variable=col_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_7("col_fifo_7");
	#pragma HLS STREAM variable=col_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_8("col_fifo_8");
	#pragma HLS STREAM variable=col_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_9("col_fifo_9");
	#pragma HLS STREAM variable=col_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_10("col_fifo_10");
	#pragma HLS STREAM variable=col_fifo_10 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2("values_fifo_2");
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3("values_fifo_3");
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4("values_fifo_4");
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_5("values_fifo_5");
	#pragma HLS STREAM variable=values_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_6("values_fifo_6");
	#pragma HLS STREAM variable=values_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_7("values_fifo_7");
	#pragma HLS STREAM variable=values_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_8("values_fifo_8");
	#pragma HLS STREAM variable=values_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_9("values_fifo_9");
	#pragma HLS STREAM variable=values_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_10("values_fifo_10");
	#pragma HLS STREAM variable=values_fifo_10 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2("results_fifo_2");
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3("results_fifo_3");
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4("results_fifo_4");
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_5("results_fifo_5");
	#pragma HLS STREAM variable=results_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_6("results_fifo_6");
	#pragma HLS STREAM variable=results_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_7("results_fifo_7");
	#pragma HLS STREAM variable=results_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_8("results_fifo_8");
	#pragma HLS STREAM variable=results_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_9("results_fifo_9");
	#pragma HLS STREAM variable=results_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_10("results_fifo_10");
	#pragma HLS STREAM variable=results_fifo_10 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);
	read_data_submatrix(nr_ci2+nr_val2, submatrix2, col_fifo_wide_2, values_fifo_wide_2);
	read_data_submatrix(nr_ci3+nr_val3, submatrix3, col_fifo_wide_3, values_fifo_wide_3);
	read_data_submatrix(nr_ci4+nr_val4, submatrix4, col_fifo_wide_4, values_fifo_wide_4);
	read_data_submatrix(nr_ci5+nr_val5, submatrix5, col_fifo_wide_5, values_fifo_wide_5);
	read_data_submatrix(nr_ci6+nr_val6, submatrix6, col_fifo_wide_6, values_fifo_wide_6);
	read_data_submatrix(nr_ci7+nr_val7, submatrix7, col_fifo_wide_7, values_fifo_wide_7);
	read_data_submatrix(nr_ci8+nr_val8, submatrix8, col_fifo_wide_8, values_fifo_wide_8);
	read_data_submatrix(nr_ci9+nr_val9, submatrix9, col_fifo_wide_9, values_fifo_wide_9);
	read_data_submatrix(nr_ci10+nr_val10, submatrix10, col_fifo_wide_10, values_fifo_wide_10);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);
	stream_data_col_ind(nr_ci2, nr_nzeros2, col_fifo_wide_2, col_fifo_2);
	stream_data_col_ind(nr_ci3, nr_nzeros3, col_fifo_wide_3, col_fifo_3);
	stream_data_col_ind(nr_ci4, nr_nzeros4, col_fifo_wide_4, col_fifo_4);
	stream_data_col_ind(nr_ci5, nr_nzeros5, col_fifo_wide_5, col_fifo_5);
	stream_data_col_ind(nr_ci6, nr_nzeros6, col_fifo_wide_6, col_fifo_6);
	stream_data_col_ind(nr_ci7, nr_nzeros7, col_fifo_wide_7, col_fifo_7);
	stream_data_col_ind(nr_ci8, nr_nzeros8, col_fifo_wide_8, col_fifo_8);
	stream_data_col_ind(nr_ci9, nr_nzeros9, col_fifo_wide_9, col_fifo_9);
	stream_data_col_ind(nr_ci10, nr_nzeros10, col_fifo_wide_10, col_fifo_10);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);
	stream_data_values(nr_val2, values_fifo_wide_2, values_fifo_2);
	stream_data_values(nr_val3, values_fifo_wide_3, values_fifo_3);
	stream_data_values(nr_val4, values_fifo_wide_4, values_fifo_4);
	stream_data_values(nr_val5, values_fifo_wide_5, values_fifo_5);
	stream_data_values(nr_val6, values_fifo_wide_6, values_fifo_6);
	stream_data_values(nr_val7, values_fifo_wide_7, values_fifo_7);
	stream_data_values(nr_val8, values_fifo_wide_8, values_fifo_8);
	stream_data_values(nr_val9, values_fifo_wide_9, values_fifo_9);
	stream_data_values(nr_val10, values_fifo_wide_10, values_fifo_10);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);
	compute_results(col_fifo_3, values_fifo_3, nr_nzeros3, x3, results_fifo_3);
	compute_results(col_fifo_4, values_fifo_4, nr_nzeros4, x4, results_fifo_4);
	compute_results(col_fifo_5, values_fifo_5, nr_nzeros5, x5, results_fifo_5);
	compute_results(col_fifo_6, values_fifo_6, nr_nzeros6, x6, results_fifo_6);
	compute_results(col_fifo_7, values_fifo_7, nr_nzeros7, x7, results_fifo_7);
	compute_results(col_fifo_8, values_fifo_8, nr_nzeros8, x8, results_fifo_8);
	compute_results(col_fifo_9, values_fifo_9, nr_nzeros9, x9, results_fifo_9);
	compute_results(col_fifo_10, values_fifo_10, nr_nzeros10, x10, results_fifo_10);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);
	write_back_results(results_fifo_3, nr_rows3, y3);
	write_back_results(results_fifo_4, nr_rows4, y4);
	write_back_results(results_fifo_5, nr_rows5, y5);
	write_back_results(results_fifo_6, nr_rows6, y6);
	write_back_results(results_fifo_7, nr_rows7, y7);
	write_back_results(results_fifo_8, nr_rows8, y8);
	write_back_results(results_fifo_9, nr_rows9, y9);
	write_back_results(results_fifo_10, nr_rows10, y10);

	return;
}

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,
	BusDataType *x, 			IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];
	ValueType x2_local[COLS_DIV_BLOCKS];
	ValueType x3_local[COLS_DIV_BLOCKS];
	ValueType x4_local[COLS_DIV_BLOCKS];
	ValueType x5_local[COLS_DIV_BLOCKS];
	ValueType x6_local[COLS_DIV_BLOCKS];
	ValueType x7_local[COLS_DIV_BLOCKS];
	ValueType x8_local[COLS_DIV_BLOCKS];
	ValueType x9_local[COLS_DIV_BLOCKS];
	ValueType x10_local[COLS_DIV_BLOCKS];

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
			x9_local[i*RATIO_v+k] = tmp.f;
			x10_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,	nr_rows2,	nr_rows3,	nr_rows4,	nr_rows5,	nr_rows6,	nr_rows7,	nr_rows8,	nr_rows9,	nr_rows10,
				nr_nzeros1,	nr_nzeros2,	nr_nzeros3,	nr_nzeros4,	nr_nzeros5,	nr_nzeros6,	nr_nzeros7,	nr_nzeros8,	nr_nzeros9,	nr_nzeros10,

				submatrix1,	submatrix2,	submatrix3,	submatrix4,	submatrix5,	submatrix6,	submatrix7,	submatrix8,	submatrix9,	submatrix10,
				nr_ci1,		nr_ci2,		nr_ci3,		nr_ci4,		nr_ci5,		nr_ci6,		nr_ci7,		nr_ci8,		nr_ci9,		nr_ci10,
				nr_val1,	nr_val2,	nr_val3,	nr_val4,	nr_val5,	nr_val6,	nr_val7,	nr_val8,	nr_val9,	nr_val10,

				y1,			y2,			y3,			y4,			y5,			y6,			y7,			y8,			y9,			y10,
				x1_local,	x2_local,	x3_local,	x4_local,	x5_local,	x6_local,	x7_local,	x8_local,	x9_local,	x10_local
				);
	return 0;
}




#elif CU == 12
void spmv_kernel(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,			IndexType nr_rows11,			IndexType nr_rows12,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,			IndexType nr_nzeros11,			IndexType nr_nzeros12,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,		BusDataType	*submatrix11,		BusDataType	*submatrix12,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,				IndexType nr_ci11,				IndexType nr_ci12,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,				IndexType nr_val11,				IndexType nr_val12,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,				BusDataType *y11,				BusDataType *y12,

	ValueType *x1,				ValueType *x2,				ValueType *x3,				ValueType *x4,				ValueType *x5,				ValueType *x6,				ValueType *x7,				ValueType *x8,				ValueType *x9,				ValueType *x10,					ValueType *x11,					ValueType *x12
){
	#pragma HLS DATAFLOW
	/*-----------------------col_fifo_wide-------------------------------*/
	hls::stream<BusDataType> col_fifo_wide_1("col_fifo_wide_1");
	#pragma HLS STREAM variable=col_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_2("col_fifo_wide_2");
	#pragma HLS STREAM variable=col_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_3("col_fifo_wide_3");
	#pragma HLS STREAM variable=col_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_4("col_fifo_wide_4");
	#pragma HLS STREAM variable=col_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_5("col_fifo_wide_5");
	#pragma HLS STREAM variable=col_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_6("col_fifo_wide_6");
	#pragma HLS STREAM variable=col_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_7("col_fifo_wide_7");
	#pragma HLS STREAM variable=col_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_8("col_fifo_wide_8");
	#pragma HLS STREAM variable=col_fifo_wide_8 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_9("col_fifo_wide_9");
	#pragma HLS STREAM variable=col_fifo_wide_9 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_10("col_fifo_wide_10");
	#pragma HLS STREAM variable=col_fifo_wide_10 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_11("col_fifo_wide_11");
	#pragma HLS STREAM variable=col_fifo_wide_11 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> col_fifo_wide_12("col_fifo_wide_12");
	#pragma HLS STREAM variable=col_fifo_wide_12 depth=BUFFER_SIZE dim=1
	/*-----------------------values_fifo_wide----------------------------*/
	hls::stream<BusDataType> values_fifo_wide_1("values_fifo_wide_1");
	#pragma HLS STREAM variable=values_fifo_wide_1 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_2("values_fifo_wide_2");
	#pragma HLS STREAM variable=values_fifo_wide_2 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_3("values_fifo_wide_3");
	#pragma HLS STREAM variable=values_fifo_wide_3 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_4("values_fifo_wide_4");
	#pragma HLS STREAM variable=values_fifo_wide_4 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_5("values_fifo_wide_5");
	#pragma HLS STREAM variable=values_fifo_wide_5 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_6("values_fifo_wide_6");
	#pragma HLS STREAM variable=values_fifo_wide_6 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_7("values_fifo_wide_7");
	#pragma HLS STREAM variable=values_fifo_wide_7 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_8("values_fifo_wide_8");
	#pragma HLS STREAM variable=values_fifo_wide_8 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_9("values_fifo_wide_9");
	#pragma HLS STREAM variable=values_fifo_wide_9 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_10("values_fifo_wide_10");
	#pragma HLS STREAM variable=values_fifo_wide_10 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_11("values_fifo_wide_11");
	#pragma HLS STREAM variable=values_fifo_wide_11 depth=BUFFER_SIZE dim=1
	hls::stream<BusDataType> values_fifo_wide_12("values_fifo_wide_12");
	#pragma HLS STREAM variable=values_fifo_wide_12 depth=BUFFER_SIZE dim=1
	/*-------------------------------col_fifo----------------------------*/
	hls::stream<CompressedIndexType> col_fifo_1("col_fifo_1");
	#pragma HLS STREAM variable=col_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_2("col_fifo_2");
	#pragma HLS STREAM variable=col_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_3("col_fifo_3");
	#pragma HLS STREAM variable=col_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_4("col_fifo_4");
	#pragma HLS STREAM variable=col_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_5("col_fifo_5");
	#pragma HLS STREAM variable=col_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_6("col_fifo_6");
	#pragma HLS STREAM variable=col_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_7("col_fifo_7");
	#pragma HLS STREAM variable=col_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_8("col_fifo_8");
	#pragma HLS STREAM variable=col_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_9("col_fifo_9");
	#pragma HLS STREAM variable=col_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_10("col_fifo_10");
	#pragma HLS STREAM variable=col_fifo_10 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_11("col_fifo_11");
	#pragma HLS STREAM variable=col_fifo_11 depth=BUFFER_SIZE dim=1
	hls::stream<CompressedIndexType> col_fifo_12("col_fifo_12");
	#pragma HLS STREAM variable=col_fifo_12 depth=BUFFER_SIZE dim=1
	/*---------------------values_fifo-----------------------------------*/
	hls::stream<ValueType> values_fifo_1("values_fifo_1");
	#pragma HLS STREAM variable=values_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_2("values_fifo_2");
	#pragma HLS STREAM variable=values_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_3("values_fifo_3");
	#pragma HLS STREAM variable=values_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_4("values_fifo_4");
	#pragma HLS STREAM variable=values_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_5("values_fifo_5");
	#pragma HLS STREAM variable=values_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_6("values_fifo_6");
	#pragma HLS STREAM variable=values_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_7("values_fifo_7");
	#pragma HLS STREAM variable=values_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_8("values_fifo_8");
	#pragma HLS STREAM variable=values_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_9("values_fifo_9");
	#pragma HLS STREAM variable=values_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_10("values_fifo_10");
	#pragma HLS STREAM variable=values_fifo_10 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_11("values_fifo_11");
	#pragma HLS STREAM variable=values_fifo_11 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> values_fifo_12("values_fifo_12");
	#pragma HLS STREAM variable=values_fifo_12 depth=BUFFER_SIZE dim=1
	/*---------------------results_fifo-----------------------------------*/
	hls::stream<ValueType> results_fifo_1("results_fifo_1");
	#pragma HLS STREAM variable=results_fifo_1 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_2("results_fifo_2");
	#pragma HLS STREAM variable=results_fifo_2 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_3("results_fifo_3");
	#pragma HLS STREAM variable=results_fifo_3 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_4("results_fifo_4");
	#pragma HLS STREAM variable=results_fifo_4 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_5("results_fifo_5");
	#pragma HLS STREAM variable=results_fifo_5 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_6("results_fifo_6");
	#pragma HLS STREAM variable=results_fifo_6 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_7("results_fifo_7");
	#pragma HLS STREAM variable=results_fifo_7 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_8("results_fifo_8");
	#pragma HLS STREAM variable=results_fifo_8 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_9("results_fifo_9");
	#pragma HLS STREAM variable=results_fifo_9 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_10("results_fifo_10");
	#pragma HLS STREAM variable=results_fifo_10 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_11("results_fifo_11");
	#pragma HLS STREAM variable=results_fifo_11 depth=BUFFER_SIZE dim=1
	hls::stream<ValueType> results_fifo_12("results_fifo_12");
	#pragma HLS STREAM variable=results_fifo_12 depth=BUFFER_SIZE dim=1
	/*--------------------------------------------------------------------*/
	read_data_submatrix(nr_ci1+nr_val1, submatrix1, col_fifo_wide_1, values_fifo_wide_1);
	read_data_submatrix(nr_ci2+nr_val2, submatrix2, col_fifo_wide_2, values_fifo_wide_2);
	read_data_submatrix(nr_ci3+nr_val3, submatrix3, col_fifo_wide_3, values_fifo_wide_3);
	read_data_submatrix(nr_ci4+nr_val4, submatrix4, col_fifo_wide_4, values_fifo_wide_4);
	read_data_submatrix(nr_ci5+nr_val5, submatrix5, col_fifo_wide_5, values_fifo_wide_5);
	read_data_submatrix(nr_ci6+nr_val6, submatrix6, col_fifo_wide_6, values_fifo_wide_6);
	read_data_submatrix(nr_ci7+nr_val7, submatrix7, col_fifo_wide_7, values_fifo_wide_7);
	read_data_submatrix(nr_ci8+nr_val8, submatrix8, col_fifo_wide_8, values_fifo_wide_8);
	read_data_submatrix(nr_ci9+nr_val9, submatrix9, col_fifo_wide_9, values_fifo_wide_9);
	read_data_submatrix(nr_ci10+nr_val10, submatrix10, col_fifo_wide_10, values_fifo_wide_10);
	read_data_submatrix(nr_ci11+nr_val11, submatrix11, col_fifo_wide_11, values_fifo_wide_11);
	read_data_submatrix(nr_ci12+nr_val12, submatrix12, col_fifo_wide_12, values_fifo_wide_12);

	stream_data_col_ind(nr_ci1, nr_nzeros1, col_fifo_wide_1, col_fifo_1);
	stream_data_col_ind(nr_ci2, nr_nzeros2, col_fifo_wide_2, col_fifo_2);
	stream_data_col_ind(nr_ci3, nr_nzeros3, col_fifo_wide_3, col_fifo_3);
	stream_data_col_ind(nr_ci4, nr_nzeros4, col_fifo_wide_4, col_fifo_4);
	stream_data_col_ind(nr_ci5, nr_nzeros5, col_fifo_wide_5, col_fifo_5);
	stream_data_col_ind(nr_ci6, nr_nzeros6, col_fifo_wide_6, col_fifo_6);
	stream_data_col_ind(nr_ci7, nr_nzeros7, col_fifo_wide_7, col_fifo_7);
	stream_data_col_ind(nr_ci8, nr_nzeros8, col_fifo_wide_8, col_fifo_8);
	stream_data_col_ind(nr_ci9, nr_nzeros9, col_fifo_wide_9, col_fifo_9);
	stream_data_col_ind(nr_ci10, nr_nzeros10, col_fifo_wide_10, col_fifo_10);
	stream_data_col_ind(nr_ci11, nr_nzeros11, col_fifo_wide_11, col_fifo_11);
	stream_data_col_ind(nr_ci12, nr_nzeros12, col_fifo_wide_12, col_fifo_12);

	stream_data_values(nr_val1, values_fifo_wide_1, values_fifo_1);
	stream_data_values(nr_val2, values_fifo_wide_2, values_fifo_2);
	stream_data_values(nr_val3, values_fifo_wide_3, values_fifo_3);
	stream_data_values(nr_val4, values_fifo_wide_4, values_fifo_4);
	stream_data_values(nr_val5, values_fifo_wide_5, values_fifo_5);
	stream_data_values(nr_val6, values_fifo_wide_6, values_fifo_6);
	stream_data_values(nr_val7, values_fifo_wide_7, values_fifo_7);
	stream_data_values(nr_val8, values_fifo_wide_8, values_fifo_8);
	stream_data_values(nr_val9, values_fifo_wide_9, values_fifo_9);
	stream_data_values(nr_val10, values_fifo_wide_10, values_fifo_10);
	stream_data_values(nr_val11, values_fifo_wide_11, values_fifo_11);
	stream_data_values(nr_val12, values_fifo_wide_12, values_fifo_12);

	/*--------------------------------------------------------------------*/
	compute_results(col_fifo_1, values_fifo_1, nr_nzeros1, x1, results_fifo_1);
	compute_results(col_fifo_2, values_fifo_2, nr_nzeros2, x2, results_fifo_2);
	compute_results(col_fifo_3, values_fifo_3, nr_nzeros3, x3, results_fifo_3);
	compute_results(col_fifo_4, values_fifo_4, nr_nzeros4, x4, results_fifo_4);
	compute_results(col_fifo_5, values_fifo_5, nr_nzeros5, x5, results_fifo_5);
	compute_results(col_fifo_6, values_fifo_6, nr_nzeros6, x6, results_fifo_6);
	compute_results(col_fifo_7, values_fifo_7, nr_nzeros7, x7, results_fifo_7);
	compute_results(col_fifo_8, values_fifo_8, nr_nzeros8, x8, results_fifo_8);
	compute_results(col_fifo_9, values_fifo_9, nr_nzeros9, x9, results_fifo_9);
	compute_results(col_fifo_10, values_fifo_10, nr_nzeros10, x10, results_fifo_10);
	compute_results(col_fifo_11, values_fifo_11, nr_nzeros11, x11, results_fifo_11);
	compute_results(col_fifo_12, values_fifo_12, nr_nzeros12, x12, results_fifo_12);

	/*--------------------------------------------------------------------*/
	write_back_results(results_fifo_1, nr_rows1, y1);
	write_back_results(results_fifo_2, nr_rows2, y2);
	write_back_results(results_fifo_3, nr_rows3, y3);
	write_back_results(results_fifo_4, nr_rows4, y4);
	write_back_results(results_fifo_5, nr_rows5, y5);
	write_back_results(results_fifo_6, nr_rows6, y6);
	write_back_results(results_fifo_7, nr_rows7, y7);
	write_back_results(results_fifo_8, nr_rows8, y8);
	write_back_results(results_fifo_9, nr_rows9, y9);
	write_back_results(results_fifo_10, nr_rows10, y10);
	write_back_results(results_fifo_11, nr_rows11, y11);
	write_back_results(results_fifo_12, nr_rows12, y12);

	return;
}

int spmv(
	IndexType nr_rows1,			IndexType nr_rows2,			IndexType nr_rows3,			IndexType nr_rows4,			IndexType nr_rows5,			IndexType nr_rows6,			IndexType nr_rows7,			IndexType nr_rows8,			IndexType nr_rows9,			IndexType nr_rows10,		IndexType nr_rows11,		IndexType nr_rows12,
	IndexType nr_nzeros1,		IndexType nr_nzeros2,		IndexType nr_nzeros3,		IndexType nr_nzeros4,		IndexType nr_nzeros5,		IndexType nr_nzeros6,		IndexType nr_nzeros7,		IndexType nr_nzeros8,		IndexType nr_nzeros9,		IndexType nr_nzeros10,		IndexType nr_nzeros11,		IndexType nr_nzeros12,

	BusDataType	*submatrix1,	BusDataType	*submatrix2,	BusDataType	*submatrix3,	BusDataType	*submatrix4,	BusDataType	*submatrix5,	BusDataType	*submatrix6,	BusDataType	*submatrix7,	BusDataType	*submatrix8,	BusDataType	*submatrix9,	BusDataType	*submatrix10,	BusDataType	*submatrix11,	BusDataType	*submatrix12,
	IndexType nr_ci1,			IndexType nr_ci2,			IndexType nr_ci3,			IndexType nr_ci4,			IndexType nr_ci5,			IndexType nr_ci6,			IndexType nr_ci7,			IndexType nr_ci8,			IndexType nr_ci9,			IndexType nr_ci10,			IndexType nr_ci11,			IndexType nr_ci12,
	IndexType nr_val1,			IndexType nr_val2,			IndexType nr_val3,			IndexType nr_val4,			IndexType nr_val5,			IndexType nr_val6,			IndexType nr_val7,			IndexType nr_val8,			IndexType nr_val9,			IndexType nr_val10,			IndexType nr_val11,			IndexType nr_val12,

	BusDataType *y1,			BusDataType *y2,			BusDataType *y3,			BusDataType *y4,			BusDataType *y5,			BusDataType *y6,			BusDataType *y7,			BusDataType *y8,			BusDataType *y9,			BusDataType *y10,			BusDataType *y11,			BusDataType *y12,
	BusDataType *x, 			IndexType nr_cols)
{
	ValueType x1_local[COLS_DIV_BLOCKS];
	ValueType x2_local[COLS_DIV_BLOCKS];
	ValueType x3_local[COLS_DIV_BLOCKS];
	ValueType x4_local[COLS_DIV_BLOCKS];
	ValueType x5_local[COLS_DIV_BLOCKS];
	ValueType x6_local[COLS_DIV_BLOCKS];
	ValueType x7_local[COLS_DIV_BLOCKS];
	ValueType x8_local[COLS_DIV_BLOCKS];
	ValueType x9_local[COLS_DIV_BLOCKS];
	ValueType x10_local[COLS_DIV_BLOCKS];
	ValueType x11_local[COLS_DIV_BLOCKS];
	ValueType x12_local[COLS_DIV_BLOCKS];

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
			x9_local[i*RATIO_v+k] = tmp.f;
			x10_local[i*RATIO_v+k] = tmp.f;
			x11_local[i*RATIO_v+k] = tmp.f;
			x12_local[i*RATIO_v+k] = tmp.f;
		}
	}

	spmv_kernel(nr_rows1,	nr_rows2,	nr_rows3,	nr_rows4,	nr_rows5,	nr_rows6,	nr_rows7,	nr_rows8,	nr_rows9,	nr_rows10,	nr_rows11,	nr_rows12,
				nr_nzeros1,	nr_nzeros2,	nr_nzeros3,	nr_nzeros4,	nr_nzeros5,	nr_nzeros6,	nr_nzeros7,	nr_nzeros8,	nr_nzeros9,	nr_nzeros10,nr_nzeros11,nr_nzeros12,

				submatrix1,	submatrix2,	submatrix3,	submatrix4,	submatrix5,	submatrix6,	submatrix7,	submatrix8,	submatrix9,	submatrix10,submatrix11,submatrix12,	
				nr_ci1,		nr_ci2,		nr_ci3,		nr_ci4,		nr_ci5,		nr_ci6,		nr_ci7,		nr_ci8,		nr_ci9,		nr_ci10,	nr_ci11,	nr_ci12,
				nr_val1,	nr_val2,	nr_val3,	nr_val4,	nr_val5,	nr_val6,	nr_val7,	nr_val8,	nr_val9,	nr_val10,	nr_val11,	nr_val12,	

				y1,			y2,			y3,			y4,			y5,			y6,			y7,			y8,			y9,			y10,		y11,		y12,			
				x1_local,	x2_local,	x3_local,	x4_local,	x5_local,	x6_local,	x7_local,	x8_local,	x9_local,	x10_local,	x11_local,	x12_local
				);
	return 0;
}




#endif
