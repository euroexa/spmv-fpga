#ifndef __CSR_H__
#define __CSR_H__

#include "util.h"
#include <math.h>

typedef struct csr_header {
	IndexType nr_rows;
	IndexType nr_cols;
	IndexType nr_nzeros;

	int blocks;
} csr_header;

typedef struct csr_matrix {
	IndexType *row_ptr;
	IndexType *col_ind;
	ValueType *values;
	IndexType nr_nzeros;
	IndexType nr_rows;
	IndexType nr_cols;

	char  *Filename;
} csr_matrix;

typedef struct csr_vector {
	ValueType *values;
	IndexType  nr_values;
} csr_vector;

int read_csr_header(csr_header *hdr, char* Filename);
csr_matrix *create_csr_matrix(csr_header hdr);
void delete_csr_matrix(csr_matrix *matrix);
int read_csr_matrix(csr_matrix *matrix, char* Filename);

csr_vector *create_csr_vector(IndexType nr_values);
void delete_csr_vector(csr_vector *vector);
void init_vector_rand(csr_vector *vector, ValueType max);

void spmv_gold(csr_matrix *matrix, ValueType *x, ValueType *y);
#endif //__CSR_H__
