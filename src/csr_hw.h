#ifndef __CSR_HW_H__
#define __CSR_HW_H__

#include "util.h"
#include <sds_lib.h>
#include "csr.h"

typedef struct csr_hw_header {
    IndexType expanded_nr_rows;
    IndexType expanded_nr_cols;
    IndexType expanded_nr_nzeros;

    int blocks;
} csr_hw_header;

typedef struct csr_hw_matrix {
    BusDataType **submatrix;
    IndexType   *nr_rows;
    IndexType   *nr_cols;
    IndexType   *nr_nzeros;
    
    IndexType   *nr_ci;
    IndexType   *nr_val;

    int blocks;
} csr_hw_matrix;

typedef struct csr_hw_vector {
    BusDataType **values;
    IndexType   *nr_values;

    int blocks;
} csr_hw_vector;

int scan_matrix(csr_matrix *matrix, IndexType **thres_l, IndexType **thres_h, IndexType ***block_row_ptr, csr_hw_header *hw_hdr);
csr_hw_matrix *hw_matrix_alloc(int blocks, IndexType *nr_rows, IndexType *nr_nzeros, IndexType *thres_l, IndexType *thres_h);
csr_matrix *create_block_matrix(csr_matrix *matrix, IndexType *block_row_ptr, IndexType block_nr_rows, IndexType block_nr_nzeros, IndexType thres_l, IndexType thres_h);
void generate_balanced_hw_submatrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                    IndexType nr_rows, IndexType *v_cnt, IndexType offset,
                                    BusDataType *hw_submatrix);


#if CU == 1
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    
                                IndexType *nr_nzeros1,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,
                          bool ***empty_rows_bitmap);


#elif CU == 2
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,
                          bool ***empty_rows_bitmap);


#elif CU == 4
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,
                          bool ***empty_rows_bitmap);


#elif CU == 8
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4,
                                 IndexType nr_rows5, BusDataType *hw_submatrix5,
                                 IndexType nr_rows6, BusDataType *hw_submatrix6,
                                 IndexType nr_rows7, BusDataType *hw_submatrix7,
                                 IndexType nr_rows8, BusDataType *hw_submatrix8);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,
                          bool ***empty_rows_bitmap);


#elif CU == 10
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    IndexType *nr_rows9,    IndexType *nr_rows10,
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  IndexType *nr_nzeros9,  IndexType *nr_nzeros10,
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4,
                                 IndexType nr_rows5, BusDataType *hw_submatrix5,
                                 IndexType nr_rows6, BusDataType *hw_submatrix6,
                                 IndexType nr_rows7, BusDataType *hw_submatrix7,
                                 IndexType nr_rows8, BusDataType *hw_submatrix8,
                                 IndexType nr_rows9, BusDataType *hw_submatrix9,
                                 IndexType nr_rows10, BusDataType *hw_submatrix10);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,   csr_hw_matrix **hw_matrix9,   csr_hw_matrix **hw_matrix10,
                          bool ***empty_rows_bitmap);


#elif CU == 12
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    IndexType *nr_rows9,    IndexType *nr_rows10,    IndexType *nr_rows11,    IndexType *nr_rows12,
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  IndexType *nr_nzeros9,  IndexType *nr_nzeros10,  IndexType *nr_nzeros11,  IndexType *nr_nzeros12,
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap);
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4,
                                 IndexType nr_rows5, BusDataType *hw_submatrix5,
                                 IndexType nr_rows6, BusDataType *hw_submatrix6,
                                 IndexType nr_rows7, BusDataType *hw_submatrix7,
                                 IndexType nr_rows8, BusDataType *hw_submatrix8,
                                 IndexType nr_rows9, BusDataType *hw_submatrix9,
                                 IndexType nr_rows10, BusDataType *hw_submatrix10,
                                 IndexType nr_rows11, BusDataType *hw_submatrix11,
                                 IndexType nr_rows12, BusDataType *hw_submatrix12);
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,   csr_hw_matrix **hw_matrix9,   csr_hw_matrix **hw_matrix10,   csr_hw_matrix **hw_matrix11,   csr_hw_matrix **hw_matrix12,
                          bool ***empty_rows_bitmap);
#endif

ValueType storage_overhead(csr_hw_matrix *matrix);
void _delete_csr_hw_matrix(csr_hw_matrix **matrix);

void create_csr_hw_vector(csr_hw_vector **hw_vector, int blocks, IndexType *nr_values);
void _delete_csr_hw_vector(csr_hw_vector **vector);
void write_csr_hw_vector(csr_hw_vector *hw_vector, csr_vector *vector);
void print_wide(BusDataType *values, IndexType nr_values, int flag);
void accum_results(BusDataType *values, IndexType nr_values, ValueType *hw_y, IndexType nr_rows, IndexType *offset, bool *empty_rows_bitmap);
int verification(IndexType nr_values, ValueType *sw_values, ValueType *hw_values, int verbose);
#endif //__CSR_HW_H__
