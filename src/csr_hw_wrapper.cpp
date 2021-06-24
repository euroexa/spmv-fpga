#include "csr_hw_wrapper.h"

void create_csr_hw_matrix(csr_matrix *matrix, csr_hw_matrix ***hw_matrix, bool ***empty_rows_bitmap)
{
	(*hw_matrix) = (csr_hw_matrix **)malloc(ComputeUnits*sizeof(csr_hw_matrix*));
	
#if CU == 1
	csr_hw_matrix *hw_matrix1;
	create_csr_hw_matrix(matrix, &hw_matrix1, 
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;

#elif CU == 2
	csr_hw_matrix *hw_matrix1, *hw_matrix2;
	create_csr_hw_matrix(matrix, &hw_matrix1, &hw_matrix2, 
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;
	(*hw_matrix)[1] = hw_matrix2;

#elif CU == 4
	csr_hw_matrix *hw_matrix1, *hw_matrix2, *hw_matrix3, *hw_matrix4;
	create_csr_hw_matrix(matrix, &hw_matrix1, &hw_matrix2, &hw_matrix3, &hw_matrix4, 
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;
	(*hw_matrix)[1] = hw_matrix2;
	(*hw_matrix)[2] = hw_matrix3;
	(*hw_matrix)[3] = hw_matrix4;

#elif CU == 8
	csr_hw_matrix *hw_matrix1, *hw_matrix2, *hw_matrix3, *hw_matrix4, *hw_matrix5, *hw_matrix6, *hw_matrix7, *hw_matrix8;
	create_csr_hw_matrix(matrix, &hw_matrix1, &hw_matrix2, &hw_matrix3, &hw_matrix4, &hw_matrix5, &hw_matrix6, &hw_matrix7, &hw_matrix8, 
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;
	(*hw_matrix)[1] = hw_matrix2;
	(*hw_matrix)[2] = hw_matrix3;
	(*hw_matrix)[3] = hw_matrix4;
	(*hw_matrix)[4] = hw_matrix5;
	(*hw_matrix)[5] = hw_matrix6;
	(*hw_matrix)[6] = hw_matrix7;
	(*hw_matrix)[7] = hw_matrix8;

#elif CU == 10
	csr_hw_matrix *hw_matrix1, *hw_matrix2, *hw_matrix3, *hw_matrix4, *hw_matrix5, *hw_matrix6, *hw_matrix7, *hw_matrix8, *hw_matrix9, *hw_matrix10;
	create_csr_hw_matrix(matrix, &hw_matrix1, &hw_matrix2, &hw_matrix3, &hw_matrix4, &hw_matrix5, &hw_matrix6, &hw_matrix7, &hw_matrix8, &hw_matrix9, &hw_matrix10,
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;
	(*hw_matrix)[1] = hw_matrix2;
	(*hw_matrix)[2] = hw_matrix3;
	(*hw_matrix)[3] = hw_matrix4;
	(*hw_matrix)[4] = hw_matrix5;
	(*hw_matrix)[5] = hw_matrix6;
	(*hw_matrix)[6] = hw_matrix7;
	(*hw_matrix)[7] = hw_matrix8;
	(*hw_matrix)[8] = hw_matrix9;
	(*hw_matrix)[9] = hw_matrix10;

#elif CU == 12
	csr_hw_matrix *hw_matrix1, *hw_matrix2, *hw_matrix3, *hw_matrix4, *hw_matrix5, *hw_matrix6, *hw_matrix7, *hw_matrix8, *hw_matrix9, *hw_matrix10, *hw_matrix11, *hw_matrix12;
	create_csr_hw_matrix(matrix, &hw_matrix1, &hw_matrix2, &hw_matrix3, &hw_matrix4, &hw_matrix5, &hw_matrix6, &hw_matrix7, &hw_matrix8, &hw_matrix9, &hw_matrix10, &hw_matrix11, &hw_matrix12, 
		empty_rows_bitmap);

	(*hw_matrix)[0] = hw_matrix1;
	(*hw_matrix)[1] = hw_matrix2;
	(*hw_matrix)[2] = hw_matrix3;
	(*hw_matrix)[3] = hw_matrix4;
	(*hw_matrix)[4] = hw_matrix5;
	(*hw_matrix)[5] = hw_matrix6;
	(*hw_matrix)[6] = hw_matrix7;
	(*hw_matrix)[7] = hw_matrix8;
	(*hw_matrix)[8] = hw_matrix9;
	(*hw_matrix)[9] = hw_matrix10;
	(*hw_matrix)[10] = hw_matrix11;
	(*hw_matrix)[11] = hw_matrix12;
#endif
}

void create_csr_hw_y_vector(csr_hw_matrix **hw_matrix, csr_hw_vector ***hw_vector)
{
	(*hw_vector) = (csr_hw_vector **)malloc(ComputeUnits*sizeof(csr_hw_vector*));

#if CU == 1
	csr_hw_vector *hw_y1;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);

	(*hw_vector)[0] = hw_y1;

#elif CU == 2
	csr_hw_vector *hw_y1, *hw_y2;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);
	create_csr_hw_vector(&hw_y2, hw_matrix[1]->blocks, hw_matrix[1]->nr_rows);

	(*hw_vector)[0] = hw_y1;
	(*hw_vector)[1] = hw_y2;

#elif CU == 4
	csr_hw_vector *hw_y1, *hw_y2, *hw_y3, *hw_y4;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);
	create_csr_hw_vector(&hw_y2, hw_matrix[1]->blocks, hw_matrix[1]->nr_rows);
	create_csr_hw_vector(&hw_y3, hw_matrix[2]->blocks, hw_matrix[2]->nr_rows);
	create_csr_hw_vector(&hw_y4, hw_matrix[3]->blocks, hw_matrix[3]->nr_rows);

	(*hw_vector)[0] = hw_y1;
	(*hw_vector)[1] = hw_y2;
	(*hw_vector)[2] = hw_y3;
	(*hw_vector)[3] = hw_y4;

#elif CU == 8
	csr_hw_vector *hw_y1, *hw_y2, *hw_y3, *hw_y4, *hw_y5, *hw_y6, *hw_y7, *hw_y8;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);
	create_csr_hw_vector(&hw_y2, hw_matrix[1]->blocks, hw_matrix[1]->nr_rows);
	create_csr_hw_vector(&hw_y3, hw_matrix[2]->blocks, hw_matrix[2]->nr_rows);
	create_csr_hw_vector(&hw_y4, hw_matrix[3]->blocks, hw_matrix[3]->nr_rows);
	create_csr_hw_vector(&hw_y5, hw_matrix[4]->blocks, hw_matrix[4]->nr_rows);
	create_csr_hw_vector(&hw_y6, hw_matrix[5]->blocks, hw_matrix[5]->nr_rows);
	create_csr_hw_vector(&hw_y7, hw_matrix[6]->blocks, hw_matrix[6]->nr_rows);
	create_csr_hw_vector(&hw_y8, hw_matrix[7]->blocks, hw_matrix[7]->nr_rows);

	(*hw_vector)[0] = hw_y1;
	(*hw_vector)[1] = hw_y2;
	(*hw_vector)[2] = hw_y3;
	(*hw_vector)[3] = hw_y4;
	(*hw_vector)[4] = hw_y5;
	(*hw_vector)[5] = hw_y6;
	(*hw_vector)[6] = hw_y7;
	(*hw_vector)[7] = hw_y8;	

#elif CU == 10
	csr_hw_vector *hw_y1, *hw_y2, *hw_y3, *hw_y4, *hw_y5, *hw_y6, *hw_y7, *hw_y8, *hw_y9, *hw_y10;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);
	create_csr_hw_vector(&hw_y2, hw_matrix[1]->blocks, hw_matrix[1]->nr_rows);
	create_csr_hw_vector(&hw_y3, hw_matrix[2]->blocks, hw_matrix[2]->nr_rows);
	create_csr_hw_vector(&hw_y4, hw_matrix[3]->blocks, hw_matrix[3]->nr_rows);
	create_csr_hw_vector(&hw_y5, hw_matrix[4]->blocks, hw_matrix[4]->nr_rows);
	create_csr_hw_vector(&hw_y6, hw_matrix[5]->blocks, hw_matrix[5]->nr_rows);
	create_csr_hw_vector(&hw_y7, hw_matrix[6]->blocks, hw_matrix[6]->nr_rows);
	create_csr_hw_vector(&hw_y8, hw_matrix[7]->blocks, hw_matrix[7]->nr_rows);
	create_csr_hw_vector(&hw_y9, hw_matrix[8]->blocks, hw_matrix[8]->nr_rows);
	create_csr_hw_vector(&hw_y10, hw_matrix[9]->blocks, hw_matrix[9]->nr_rows);

	(*hw_vector)[0] = hw_y1;
	(*hw_vector)[1] = hw_y2;
	(*hw_vector)[2] = hw_y3;
	(*hw_vector)[3] = hw_y4;
	(*hw_vector)[4] = hw_y5;
	(*hw_vector)[5] = hw_y6;
	(*hw_vector)[6] = hw_y7;
	(*hw_vector)[7] = hw_y8;
	(*hw_vector)[8] = hw_y9;
	(*hw_vector)[9] = hw_y10;

#elif CU == 12
	csr_hw_vector *hw_y1, *hw_y2, *hw_y3, *hw_y4, *hw_y5, *hw_y6, *hw_y7, *hw_y8, *hw_y9, *hw_y10, *hw_y11, *hw_y12;
	create_csr_hw_vector(&hw_y1, hw_matrix[0]->blocks, hw_matrix[0]->nr_rows);
	create_csr_hw_vector(&hw_y2, hw_matrix[1]->blocks, hw_matrix[1]->nr_rows);
	create_csr_hw_vector(&hw_y3, hw_matrix[2]->blocks, hw_matrix[2]->nr_rows);
	create_csr_hw_vector(&hw_y4, hw_matrix[3]->blocks, hw_matrix[3]->nr_rows);
	create_csr_hw_vector(&hw_y5, hw_matrix[4]->blocks, hw_matrix[4]->nr_rows);
	create_csr_hw_vector(&hw_y6, hw_matrix[5]->blocks, hw_matrix[5]->nr_rows);
	create_csr_hw_vector(&hw_y7, hw_matrix[6]->blocks, hw_matrix[6]->nr_rows);
	create_csr_hw_vector(&hw_y8, hw_matrix[7]->blocks, hw_matrix[7]->nr_rows);
	create_csr_hw_vector(&hw_y9, hw_matrix[8]->blocks, hw_matrix[8]->nr_rows);
	create_csr_hw_vector(&hw_y10, hw_matrix[9]->blocks, hw_matrix[9]->nr_rows);
	create_csr_hw_vector(&hw_y11, hw_matrix[10]->blocks, hw_matrix[10]->nr_rows);
	create_csr_hw_vector(&hw_y12, hw_matrix[11]->blocks, hw_matrix[11]->nr_rows);

	(*hw_vector)[0] = hw_y1;
	(*hw_vector)[1] = hw_y2;
	(*hw_vector)[2] = hw_y3;
	(*hw_vector)[3] = hw_y4;
	(*hw_vector)[4] = hw_y5;
	(*hw_vector)[5] = hw_y6;
	(*hw_vector)[6] = hw_y7;
	(*hw_vector)[7] = hw_y8;
	(*hw_vector)[8] = hw_y9;
	(*hw_vector)[9] = hw_y10;
	(*hw_vector)[10] = hw_y11;
	(*hw_vector)[11] = hw_y12;

#endif
}

void create_csr_hw_x_vector(csr_hw_vector **hw_x, csr_vector *x, int blocks, IndexType *nr_cols)
{
	create_csr_hw_vector(hw_x, blocks, nr_cols);
	write_csr_hw_vector(*hw_x, x);
}

void spmv_hw(csr_hw_matrix **hw_matrix, csr_hw_vector *hw_x, csr_vector *y_fpga, bool **empty_rows_bitmap)
{	
	double hw_s, hw_f, hw_exec;
	double ra_s, ra_f, ra_exec;
	csr_hw_vector **hw_y;
	create_csr_hw_y_vector(hw_matrix, &hw_y);

	hw_s = getTimestamp();
	
	for(int i=0; i<hw_matrix[0]->blocks; i++){
		// std::cout << " BLOCK " << i << "\n";
#if CU == 1
		spmv(
			hw_matrix[0]->nr_rows[i],	
			hw_matrix[0]->nr_nzeros[i],	

			hw_matrix[0]->submatrix[i],	
			hw_matrix[0]->nr_ci[i],		
			hw_matrix[0]->nr_val[i],	

			hw_y[0]->values[i],			
			hw_x->values[i],			hw_x->nr_values[i]);
#elif CU == 2
		spmv(
			hw_matrix[0]->nr_rows[i],	hw_matrix[1]->nr_rows[i],
			hw_matrix[0]->nr_nzeros[i],	hw_matrix[1]->nr_nzeros[i],

			hw_matrix[0]->submatrix[i],	hw_matrix[1]->submatrix[i],	
			hw_matrix[0]->nr_ci[i],		hw_matrix[1]->nr_ci[i],		
			hw_matrix[0]->nr_val[i],	hw_matrix[1]->nr_val[i],	

			hw_y[0]->values[i],			hw_y[1]->values[i],
			hw_x->values[i],			hw_x->nr_values[i]);
#elif CU == 4
		spmv(
			hw_matrix[0]->nr_rows[i],	hw_matrix[1]->nr_rows[i],	hw_matrix[2]->nr_rows[i],	hw_matrix[3]->nr_rows[i],
			hw_matrix[0]->nr_nzeros[i],	hw_matrix[1]->nr_nzeros[i],	hw_matrix[2]->nr_nzeros[i],	hw_matrix[3]->nr_nzeros[i],

			hw_matrix[0]->submatrix[i],	hw_matrix[1]->submatrix[i],	hw_matrix[2]->submatrix[i],	hw_matrix[3]->submatrix[i],	
			hw_matrix[0]->nr_ci[i],		hw_matrix[1]->nr_ci[i],		hw_matrix[2]->nr_ci[i],		hw_matrix[3]->nr_ci[i],		
			hw_matrix[0]->nr_val[i],	hw_matrix[1]->nr_val[i],	hw_matrix[2]->nr_val[i],	hw_matrix[3]->nr_val[i],	

			hw_y[0]->values[i],			hw_y[1]->values[i],			hw_y[2]->values[i],			hw_y[3]->values[i],
			hw_x->values[i],			hw_x->nr_values[i]);
#elif CU == 8
		spmv(
			hw_matrix[0]->nr_rows[i],	hw_matrix[1]->nr_rows[i],	hw_matrix[2]->nr_rows[i],	hw_matrix[3]->nr_rows[i],	hw_matrix[4]->nr_rows[i],	hw_matrix[5]->nr_rows[i],	hw_matrix[6]->nr_rows[i],	hw_matrix[7]->nr_rows[i],
			hw_matrix[0]->nr_nzeros[i],	hw_matrix[1]->nr_nzeros[i],	hw_matrix[2]->nr_nzeros[i],	hw_matrix[3]->nr_nzeros[i],	hw_matrix[4]->nr_nzeros[i],	hw_matrix[5]->nr_nzeros[i],	hw_matrix[6]->nr_nzeros[i],	hw_matrix[7]->nr_nzeros[i],

			hw_matrix[0]->submatrix[i],	hw_matrix[1]->submatrix[i],	hw_matrix[2]->submatrix[i],	hw_matrix[3]->submatrix[i],	hw_matrix[4]->submatrix[i],	hw_matrix[5]->submatrix[i],	hw_matrix[6]->submatrix[i],	hw_matrix[7]->submatrix[i],	
			hw_matrix[0]->nr_ci[i],		hw_matrix[1]->nr_ci[i],		hw_matrix[2]->nr_ci[i],		hw_matrix[3]->nr_ci[i],		hw_matrix[4]->nr_ci[i],		hw_matrix[5]->nr_ci[i],		hw_matrix[6]->nr_ci[i],		hw_matrix[7]->nr_ci[i],		
			hw_matrix[0]->nr_val[i],	hw_matrix[1]->nr_val[i],	hw_matrix[2]->nr_val[i],	hw_matrix[3]->nr_val[i],	hw_matrix[4]->nr_val[i],	hw_matrix[5]->nr_val[i],	hw_matrix[6]->nr_val[i],	hw_matrix[7]->nr_val[i],	

			hw_y[0]->values[i],			hw_y[1]->values[i],			hw_y[2]->values[i],			hw_y[3]->values[i],			hw_y[4]->values[i],			hw_y[5]->values[i],			hw_y[6]->values[i],			hw_y[7]->values[i],
			hw_x->values[i],			hw_x->nr_values[i]);
#elif CU == 10
		spmv(
			hw_matrix[0]->nr_rows[i],	hw_matrix[1]->nr_rows[i],	hw_matrix[2]->nr_rows[i],	hw_matrix[3]->nr_rows[i],	hw_matrix[4]->nr_rows[i],	hw_matrix[5]->nr_rows[i],	hw_matrix[6]->nr_rows[i],	hw_matrix[7]->nr_rows[i],	hw_matrix[8]->nr_rows[i],	hw_matrix[9]->nr_rows[i],
			hw_matrix[0]->nr_nzeros[i],	hw_matrix[1]->nr_nzeros[i],	hw_matrix[2]->nr_nzeros[i],	hw_matrix[3]->nr_nzeros[i],	hw_matrix[4]->nr_nzeros[i],	hw_matrix[5]->nr_nzeros[i],	hw_matrix[6]->nr_nzeros[i],	hw_matrix[7]->nr_nzeros[i],	hw_matrix[8]->nr_nzeros[i],	hw_matrix[9]->nr_nzeros[i],

			hw_matrix[0]->submatrix[i],	hw_matrix[1]->submatrix[i],	hw_matrix[2]->submatrix[i],	hw_matrix[3]->submatrix[i],	hw_matrix[4]->submatrix[i],	hw_matrix[5]->submatrix[i],	hw_matrix[6]->submatrix[i],	hw_matrix[7]->submatrix[i],	hw_matrix[8]->submatrix[i],	hw_matrix[9]->submatrix[i],
			hw_matrix[0]->nr_ci[i],		hw_matrix[1]->nr_ci[i],		hw_matrix[2]->nr_ci[i],		hw_matrix[3]->nr_ci[i],		hw_matrix[4]->nr_ci[i],		hw_matrix[5]->nr_ci[i],		hw_matrix[6]->nr_ci[i],		hw_matrix[7]->nr_ci[i],		hw_matrix[8]->nr_ci[i],		hw_matrix[9]->nr_ci[i],
			hw_matrix[0]->nr_val[i],	hw_matrix[1]->nr_val[i],	hw_matrix[2]->nr_val[i],	hw_matrix[3]->nr_val[i],	hw_matrix[4]->nr_val[i],	hw_matrix[5]->nr_val[i],	hw_matrix[6]->nr_val[i],	hw_matrix[7]->nr_val[i],	hw_matrix[8]->nr_val[i],	hw_matrix[9]->nr_val[i],

			hw_y[0]->values[i],			hw_y[1]->values[i],			hw_y[2]->values[i],			hw_y[3]->values[i],			hw_y[4]->values[i],			hw_y[5]->values[i],			hw_y[6]->values[i],			hw_y[7]->values[i],			hw_y[8]->values[i],			hw_y[9]->values[i],
			hw_x->values[i],			hw_x->nr_values[i]);
#elif CU == 12
		spmv(
			hw_matrix[0]->nr_rows[i],	hw_matrix[1]->nr_rows[i],	hw_matrix[2]->nr_rows[i],	hw_matrix[3]->nr_rows[i],	hw_matrix[4]->nr_rows[i],	hw_matrix[5]->nr_rows[i],	hw_matrix[6]->nr_rows[i],	hw_matrix[7]->nr_rows[i],	hw_matrix[8]->nr_rows[i],	hw_matrix[9]->nr_rows[i],	hw_matrix[10]->nr_rows[i],		hw_matrix[11]->nr_rows[i],
			hw_matrix[0]->nr_nzeros[i],	hw_matrix[1]->nr_nzeros[i],	hw_matrix[2]->nr_nzeros[i],	hw_matrix[3]->nr_nzeros[i],	hw_matrix[4]->nr_nzeros[i],	hw_matrix[5]->nr_nzeros[i],	hw_matrix[6]->nr_nzeros[i],	hw_matrix[7]->nr_nzeros[i],	hw_matrix[8]->nr_nzeros[i],	hw_matrix[9]->nr_nzeros[i],	hw_matrix[10]->nr_nzeros[i],	hw_matrix[11]->nr_nzeros[i],

			hw_matrix[0]->submatrix[i],	hw_matrix[1]->submatrix[i],	hw_matrix[2]->submatrix[i],	hw_matrix[3]->submatrix[i],	hw_matrix[4]->submatrix[i],	hw_matrix[5]->submatrix[i],	hw_matrix[6]->submatrix[i],	hw_matrix[7]->submatrix[i],	hw_matrix[8]->submatrix[i],	hw_matrix[9]->submatrix[i],	hw_matrix[10]->submatrix[i],	hw_matrix[11]->submatrix[i],
			hw_matrix[0]->nr_ci[i],		hw_matrix[1]->nr_ci[i],		hw_matrix[2]->nr_ci[i],		hw_matrix[3]->nr_ci[i],		hw_matrix[4]->nr_ci[i],		hw_matrix[5]->nr_ci[i],		hw_matrix[6]->nr_ci[i],		hw_matrix[7]->nr_ci[i],		hw_matrix[8]->nr_ci[i],		hw_matrix[9]->nr_ci[i],		hw_matrix[10]->nr_ci[i],		hw_matrix[11]->nr_ci[i],
			hw_matrix[0]->nr_val[i],	hw_matrix[1]->nr_val[i],	hw_matrix[2]->nr_val[i],	hw_matrix[3]->nr_val[i],	hw_matrix[4]->nr_val[i],	hw_matrix[5]->nr_val[i],	hw_matrix[6]->nr_val[i],	hw_matrix[7]->nr_val[i],	hw_matrix[8]->nr_val[i],	hw_matrix[9]->nr_val[i],	hw_matrix[10]->nr_val[i],		hw_matrix[11]->nr_val[i],

			hw_y[0]->values[i],			hw_y[1]->values[i],			hw_y[2]->values[i],			hw_y[3]->values[i],			hw_y[4]->values[i],			hw_y[5]->values[i],			hw_y[6]->values[i],			hw_y[7]->values[i],			hw_y[8]->values[i],			hw_y[9]->values[i],			hw_y[10]->values[i],			hw_y[11]->values[i],
			hw_x->values[i],			hw_x->nr_values[i]);
#endif
	}
	hw_f = getTimestamp();
	hw_exec = (hw_f - hw_s)/(1000);
	printf("Hardware execution time : %.6f ms elapsed\n", hw_exec);

	ra_s = getTimestamp();
	for(int i=0; i<hw_matrix[0]->blocks; i++){
		IndexType offset = 0;
		for(int k=0; k<ComputeUnits; k++)
			accum_results(hw_y[k]->values[i], hw_y[k]->nr_values[i], y_fpga->values, y_fpga->nr_values, &offset, &(empty_rows_bitmap[i][offset]));
	}
	ra_f = getTimestamp();
	ra_exec = (ra_f - ra_s)/(1000);
	printf("Result accumulation time : %.6f ms elapsed\n", ra_exec);
	printf("Total time  : %.6f ms elapsed\n", hw_exec+ra_exec);

	delete_csr_hw_y_vector(hw_y);
}


void delete_csr_hw_matrix(csr_hw_matrix **hw_matrix)
{
	for(int i=0; i<ComputeUnits; i++)
		_delete_csr_hw_matrix(&hw_matrix[i]);
	free(hw_matrix);
}

void delete_csr_hw_y_vector(csr_hw_vector **hw_vector)
{
	for(int i=0; i<ComputeUnits; i++)
		_delete_csr_hw_vector(&hw_vector[i]);
	free(hw_vector);
}

void delete_csr_hw_x_vector(csr_hw_vector *hw_vector)
{
	_delete_csr_hw_vector(&hw_vector);
}