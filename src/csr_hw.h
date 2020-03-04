#ifndef __CSR_HW_H__
#define __CSR_HW_H__

#include "util.h"
#include <sds_lib.h>
#include "csr.h"

const int MAX_BLOCKS = 32;

typedef struct csr_hw_header {
    IndexType expanded_nr_rows;
    IndexType expanded_nr_cols;
    IndexType expanded_nr_nzeros;

    int blocks;
} csr_hw_header;

typedef struct csr_hw_matrix {
    BusDataType **col_ind;
    BusDataType **values;
    IndexType   *nr_rows;
    IndexType   *nr_cols;
    IndexType   *nr_nzeros;

    int blocks;
} csr_hw_matrix;

typedef struct csr_hw_vector {
    BusDataType **values;
    IndexType   *nr_values;

    int blocks;
} csr_hw_vector;

int scan_matrix(char *Filename, IndexType *data_per_block, IndexType *thres_l, IndexType *thres_h, IndexType **block_row_ptr, csr_hw_header *hw_hdr);
csr_hw_matrix *hw_matrix_alloc(int blocks, IndexType *nr_rows, IndexType *nr_nzeros, IndexType *thres_l, IndexType *thres_h);
csr_matrix *create_block_matrix(csr_matrix *matrix, csr_hw_header hw_hdr, IndexType block_nr_nzeros, IndexType thres_l, IndexType thres_h);
void generate_balanced_hw_submatrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                    IndexType nr_rows, IndexType *v_cnt, IndexType offset,
                                    BusDataType *hw_col_ind, BusDataType *hw_values);


#if CU == 1
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    
                                IndexType *nr_nzeros1,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1);
void create_csr_hw_matrix(csr_matrix *matrix,
						  csr_hw_matrix **hw_matrix1);


#elif CU == 2
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,        
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2);
void create_csr_hw_matrix(csr_matrix *matrix,
						  csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2);


#elif CU == 4
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2,
                                 IndexType nr_rows3, BusDataType *hw_col_ind3, BusDataType *hw_values3,
                                 IndexType nr_rows4, BusDataType *hw_col_ind4, BusDataType *hw_values4);
void create_csr_hw_matrix(csr_matrix *matrix,
						  csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4);


#elif CU == 8
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2,
                                 IndexType nr_rows3, BusDataType *hw_col_ind3, BusDataType *hw_values3,
                                 IndexType nr_rows4, BusDataType *hw_col_ind4, BusDataType *hw_values4,
                                 IndexType nr_rows5, BusDataType *hw_col_ind5, BusDataType *hw_values5,
                                 IndexType nr_rows6, BusDataType *hw_col_ind6, BusDataType *hw_values6,
                                 IndexType nr_rows7, BusDataType *hw_col_ind7, BusDataType *hw_values7,
                                 IndexType nr_rows8, BusDataType *hw_col_ind8, BusDataType *hw_values8);
void create_csr_hw_matrix(csr_matrix *matrix,
						  csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8);
#endif

ValueType storage_overhead(csr_hw_matrix *matrix);
void _delete_csr_hw_matrix(csr_hw_matrix **matrix);

void create_csr_hw_vector(csr_hw_vector **hw_vector, int blocks, IndexType *nr_values);
void _delete_csr_hw_vector(csr_hw_vector **vector);
void write_csr_hw_vector(csr_hw_vector *hw_vector, csr_vector *vector);
void print_wide(BusDataType *values, IndexType nr_values, int flag);
void accum_results(IndexType nr_values, BusDataType *values, IndexType cnt, ValueType *hw_y, IndexType nr_rows);
int verification(IndexType nr_values, ValueType *sw_values, ValueType *hw_values, int verbose);
#endif //__CSR_HW_H__
