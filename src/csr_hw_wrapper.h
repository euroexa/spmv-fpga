#ifndef __CSR_WRAPPER_HW_H__
#define __CSR_WRAPPER_HW_H__

#include "util.h"
#include "csr.h"
#include "csr_hw.h"
#include "spmv.h"

void create_csr_hw_matrix(csr_matrix *matrix, csr_hw_matrix ***hw_matrix);
void create_csr_hw_y_vector(csr_hw_matrix **hw_matrix, csr_hw_vector ***hw_vector);
void create_csr_hw_x_vector(csr_hw_vector **hw_x, csr_vector *x, int blocks, IndexType *nr_cols);

void spmv_hw(csr_hw_matrix **hw_matrix, csr_hw_vector *hw_x, csr_vector *y_fpga);

void delete_csr_hw_matrix(csr_hw_matrix **hw_matrix);
void delete_csr_hw_y_vector(csr_hw_vector **hw_vector);
void delete_csr_hw_x_vector(csr_hw_vector *hw_vector);

#endif //__CSR_WRAPPER_HW_H__
