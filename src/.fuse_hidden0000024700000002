#include "csr.h"

/*
	Read the first line of matrix file (fp)
    Header struct (hdr) keeps number of rows, columns and non-zeros of the matrix. In addition, the number of blocks that the matrix will be
    split to, when it will be sent to the FPGA. 
    Number of blocks is determined by the number of columns of the matrix and the maximum number of vector elements that can be stored on FPGA's BRAM.
	Return 0 on success, !=0 on error.
*/
int read_csr_header(csr_header *hdr, char* Filename)
{
    FILE *fp;
	fp = fopen(Filename, "r");

    if ((fp = fopen(Filename, "r")) == NULL){
        std::cout << "Could not open file " << Filename << "\n";
        return 1;
    }

    int nr_matched;
    nr_matched = fscanf(fp, HEADER_FMT, &hdr->nr_rows, &hdr->nr_cols, &hdr->nr_nzeros);
    
    if (nr_matched == EOF) {
        std::cout << "unexpected eof found\n";
        return 1;
    }

    if (ferror(fp)) {
        std::cout << "i/o error\n";
        return 2;
    }

    if (nr_matched != 3) {
        std::cout << "parse error\n";
        return 3;
    }
    fclose(fp);

    int blocks = (hdr->nr_cols)/COLS_DIV_BLOCKS + 1;
    if((hdr->nr_cols)%COLS_DIV_BLOCKS == 0) // in case nr_cols is divided perfectly, no need to have an extra block
        blocks--;

    hdr->blocks = blocks;

    return 0;
}

/*
	Allocate memory for csr matrix
*/
csr_matrix *create_csr_matrix(csr_header hdr)
{	
	csr_matrix *matrix;

	if (hdr.nr_rows < 0 || hdr.nr_cols < 0 || hdr.nr_nzeros < 0)
        return NULL;

    matrix = (csr_matrix*)malloc(sizeof(csr_matrix));
    matrix->nr_rows   = hdr.nr_rows;
    matrix->nr_cols   = hdr.nr_cols;
    matrix->nr_nzeros = hdr.nr_nzeros;
    matrix->row_ptr   = (IndexType*)malloc((hdr.nr_rows + 1)*sizeof(IndexType));
    matrix->col_ind   = (IndexType*)malloc(hdr.nr_nzeros*sizeof(IndexType));
    matrix->values    = (ValueType*)malloc(hdr.nr_nzeros*sizeof(ValueType));
    return matrix;
}

/*
	Free memory allocated for csr matrix
*/
void delete_csr_matrix(csr_matrix *matrix)
{
	if (!matrix)
        return;

    free(matrix->values);
    free(matrix->col_ind);
    free(matrix->row_ptr);
    free(matrix);
    return;
}

/*
	Read the file (fp) and return a struct that contains all info about the matrix (matrix)
	Return 0 on success, !=0 on error.	
*/
int read_csr_matrix(csr_matrix *matrix, char* Filename)
{
	char curr_line[1000];
	IndexType row_ptr_pos;	/* next free position in `row_ptr' array */
	IndexType values_pos;	/* next free position in `values' array */
	IndexType r, c;			/* row and column in original matrix */
	ValueType v;			/* input value */
	IndexType i;
    IndexType nr_rows, nr_cols, nr_nzeros;
    
	row_ptr_pos = 0;
	values_pos  = 0;

	FILE *fp;
	fp = fopen(Filename, "r");
    fgets(curr_line, sizeof curr_line, fp); /* read first line once again */
    sscanf(curr_line, HEADER_FMT, &nr_rows, &nr_cols, &nr_nzeros);
    
    matrix->nr_rows = nr_rows;
    matrix->nr_cols = nr_cols;
    matrix->nr_nzeros = nr_nzeros;

    while (fgets(curr_line, sizeof curr_line, fp) != NULL) { /* read a line from a file */
		/* parse the line */
		if (sscanf(curr_line, LINE_FMT, &r, &c, &v) != 3) {
			printf("parse error: %s\n", curr_line);
			return 1;
		}
		for (i = row_ptr_pos; i < r; i++)
			matrix->row_ptr[i] = values_pos;   /* empty rows */
        row_ptr_pos = r;
		matrix->col_ind[values_pos] = c - 1;

        /* mmarket files are not zero-base */
		matrix->values[values_pos] = v;
		values_pos++;
	}
	
	/* fill the special last element of row_ptr; shall always be == number of non-zero elements */
	matrix->row_ptr[matrix->nr_rows] = matrix->nr_nzeros;
    
    if (ferror(fp)) {
		printf("i/o error\n");
		return 2;
	}
	fclose(fp);

    matrix->Filename = Filename;
	return 0;
}

/*
	Allocate memory for csr vector (x or y)
*/
csr_vector *create_csr_vector(IndexType nr_values)
{	
	csr_vector *vector;

    vector = (csr_vector*)malloc(sizeof(csr_vector));
    vector->nr_values = nr_values;
    vector->values    = (ValueType*)malloc(nr_values*sizeof(ValueType));
    for(IndexType i=0; i<nr_values; i++)
        vector->values[i] = 0;

    return vector;
}

/*
	Free memory allocated for csr vector
*/
void delete_csr_vector(csr_vector *vector)
{
	if (!vector)
        return;

    free(vector->values);
    free(vector);
    return;
}

/*
	Initialize vector with random value
*/
void init_vector_rand(csr_vector *vector, ValueType max)
{
    if (!vector)
        return;

    for (IndexType i = 0; i < vector->nr_values; i++)
        vector->values[i] = max * (rand() / (ValueType) RAND_MAX);

    return;
}

/*
	Calculate result (y) of SpMV (matrix*x) 
*/
void spmv_gold(csr_matrix *matrix, ValueType *x, ValueType *y)
{
    IndexType i, j;
    for (i = 0; i < matrix->nr_rows; i++) {
        ValueType _y = 0.0;
        for (j = matrix->row_ptr[i]; j < matrix->row_ptr[i+1]; j++)
            _y += matrix->values[j]*x[matrix->col_ind[j]];
        y[i] = _y;
    }
    return;
}