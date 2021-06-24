#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string>

#include <sds_lib.h>

#include "util.h"
#include "csr.h"
#include "csr_hw.h"
#include "csr_hw_wrapper.h"
#include "spmv.h"

int main(int argc, char** argv)
{
	std::string version;
	#if DOUBLE == 0
		version = "CU"+ std::to_string(ComputeUnits) + "_VF"+ std::to_string(VectFactor) + "_float";
		std::cout << "Welcome to SpMV (Compute Units : " << ComputeUnits << ", Vectorization Factor : " << VectFactor << ", single-precision arithmetic)\n";
	#elif DOUBLE == 1
		version = "CU"+ std::to_string(ComputeUnits) + "_VF"+ std::to_string(VectFactor) + "_double";
		std::cout << "Welcome to SpMV (Compute Units : " << ComputeUnits << ", Vectorization Factor : " << VectFactor << ", double-precision arithmetic)\n";
	#endif

	if (argc != 2) {
		printf("please enter the input file name  \n");
		return 1;
	}

	double sw_s, sw_f, sw_exec;
	double mr_s, mr_f, mr_exec;

	csr_header hdr;
	
	csr_matrix *matrix;
	csr_vector *x, *y;
	
	bool **empty_rows_bitmap;
	csr_hw_matrix **hw_matrix;
	csr_hw_vector *hw_x;
	csr_vector *y_fpga;


	if (read_csr_header(&hdr, argv[1]) != 0){
		std::cout << "Error reading matrix header\n";
		return 1;
	}
	matrix = create_csr_matrix(hdr);
	if(read_csr_matrix(matrix, argv[1]) !=0){
		std::cout << "Error reading matrix\n";
		return 1;
	}

	x = create_csr_vector(hdr.nr_cols);
	init_vector_rand(x, 1);

	y = create_csr_vector(hdr.nr_rows);

	sw_s = getTimestamp();
	spmv_gold(matrix, x->values, y->values);
	sw_f = getTimestamp();
	sw_exec = (sw_f - sw_s)/(1000);
	printf("Software execution time : %.6f ms elapsed\n", sw_exec);

	mr_s = getTimestamp();
	create_csr_hw_matrix(matrix, &hw_matrix, &empty_rows_bitmap);
	create_csr_hw_x_vector(&hw_x, x, hw_matrix[0]->blocks, hw_matrix[0]->nr_cols);
	mr_f = getTimestamp();
	mr_exec = (mr_f - mr_s)/(1000);
	printf("Matrix read time        : %.6f ms elapsed\n", mr_exec);

	y_fpga = create_csr_vector(hdr.nr_rows);
	spmv_hw(hw_matrix, hw_x, y_fpga, empty_rows_bitmap);

	int status = verification(y->nr_values, y->values, y_fpga->values, 0);

	if(status == 0)
		std::cout << "Verification PASSED!\n";
	else
		std::cout << "Verification FAILED!\n";

	ValueType mem=0, csr_mem;
	csr_mem = ((matrix->nr_rows+1)*INDEX_TYPE_BIT_WIDTH + matrix->nr_nzeros*(INDEX_TYPE_BIT_WIDTH+VALUE_TYPE_BIT_WIDTH))/(8.0*1024*1024); // CSR in MB
	for(int i=0; i<ComputeUnits; i++)
		mem += storage_overhead(hw_matrix[i]);
	std::cout << "CSR representation : " << csr_mem << " MB. Our representation : " << mem << " MB. Storage Overhead : " << (mem-csr_mem)/csr_mem*100 << " %\n";

	delete_csr_matrix(matrix);
	delete_csr_vector(x);
	delete_csr_vector(y);

	delete_csr_hw_matrix(hw_matrix);
	free(empty_rows_bitmap);
	delete_csr_hw_x_vector(hw_x);
	delete_csr_vector(y_fpga);

	return 0;
}