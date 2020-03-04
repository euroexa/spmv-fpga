#include "csr_hw.h"

/*
	Scan matrix for empty rows and to find the exact number of rows, columns, non-zeros, so that they satisfy the padding conditions
	Store the number of non-zeros of each block that the matrix will be split to in (data_per_block)
*/
int scan_matrix(char *Filename, IndexType *data_per_block, IndexType *thres_l, IndexType *thres_h, IndexType **block_row_ptr, csr_hw_header *hw_hdr)
{
	IndexType curr_row_length[MAX_BLOCKS];

    for(IndexType i=0; i<MAX_BLOCKS; i++){
        curr_row_length[i] = 0;
        thres_l[i] = 0;
        thres_h[i] = 0;
    }

	char curr_line[1000];
	IndexType r, c;			/* row and column in original matrix */
	ValueType v;			/* input value */
    IndexType nr_rows, 			nr_cols, 			nr_nzeros;
	IndexType expanded_nr_rows, expanded_nr_cols, 	expanded_nr_nzeros;

	FILE *fp;
	fp = fopen(Filename, "r");
	fgets(curr_line, sizeof curr_line, fp); /* read first line once again */
	sscanf(curr_line, LINE_FMT, &nr_rows, &nr_cols, &nr_nzeros);
    
    expanded_nr_rows = nr_rows;
    expanded_nr_cols = nr_cols;
    expanded_nr_nzeros = nr_nzeros; 

	/*
	    RATIO is the number of elements packed inside a single elemenent of the structs that will be sent to and received from the FPGA
	    Due to the fact that x(vector) and the y(result) will be packed in such structs, we need to have number of columns (due to x vector)
	    and number of rows (due to y vector) that are multiples of RATIO. Therefore, we check if nr_rows and nr_cols satisfy this condition
	    For number of columns, we also need it to be a multiple of the number of blocks that the matrix will be split to.
	*/

	int blocks = nr_cols/COLS_DIV_BLOCKS + 1;
	if(nr_cols%COLS_DIV_BLOCKS == 0) // in case nr_cols is divided perfectly, no need to have an extra block
		blocks--;

	/*could be RATIO_v too, because the hw_x vector needs to have multiple of RATIO_v elements, but (RATIO_i >= RATIO_v), so no prob!*/
    IndexType N = RATIO_i * blocks; 
    IndexType cols_mod_N = nr_cols % N;  
    if (cols_mod_N != 0) // need to pad the number of columns
        expanded_nr_cols = nr_cols + (N-cols_mod_N);

    /* pad the number of rows to whichever is greater among RATIO_i and ComputeUnits. So that every Compute Unit gets a multiple of N number of rows */
    N = RATIO_i;
    if(ComputeUnits>N)
    	N = ComputeUnits;
    IndexType row_mod_N = nr_rows % N;
    if(row_mod_N != 0){
    	expanded_nr_rows   += (N-row_mod_N);
    	expanded_nr_nzeros += blocks*VectFactor*(N-row_mod_N);
    }
    
    /*
    	block_row_ptr will hold the number of non-zeros per block. This will be used to allocate 
    	the proper memory space for our hw_matrices in hw_matrix_alloc.
    */
    for(int i=0; i<blocks;i++){
    	block_row_ptr[i] = (IndexType*)malloc((expanded_nr_rows+1)*sizeof(IndexType));
    	block_row_ptr[i][0] = 0;
    }

    thres_l[0] = 0;
    if(blocks==1)
        thres_h[0] = expanded_nr_cols-1;
    else
	   thres_h[0] = COLS_DIV_BLOCKS-1;

    for(int i=1; i<blocks; i++){
		thres_l[i] = thres_h[i-1]+1;
        if(i==blocks-1)
            thres_h[i] = expanded_nr_cols-1;
        else
            thres_h[i] = thres_l[i] + COLS_DIV_BLOCKS-1;
    }

    IndexType *row_ptr_prev;
    row_ptr_prev = (IndexType*)malloc(blocks*sizeof(IndexType));
    for(int i=0; i<blocks; i++)
    	row_ptr_prev[i] = 0;
	while (fgets(curr_line, sizeof curr_line, fp) != NULL) { /* read a line from a file */
		/* parse the line */
		if (sscanf(curr_line, LINE_FMT, &r, &c, &v) != 3) {
			printf("parse error: %s\n", curr_line);
			return 1;
		}
		/*
			Check if a change of line occured. In this case, we need to check if padding will be needed for the row that just finished
			It either has curr_row_length elements, or 0. We need to apply zero-padding later, so that every row has multiple of VectFactor elements.
			For now, we only count the number of elements needed.
		*/
		for(int i=0; i<blocks; i++){
            if(row_ptr_prev[i] != r && (c-1) <= thres_h[i] && (c-1) >= thres_l[i]){
                block_row_ptr[i][row_ptr_prev[i]] += curr_row_length[i];
                IndexType padding_needed = 0;
                if(curr_row_length[i] % VectFactor==0)
                    padding_needed = 0;
                else
                    padding_needed = VectFactor - (curr_row_length[i] % VectFactor);
				block_row_ptr[i][row_ptr_prev[i]] += padding_needed;

            	/*   Means we "passed" a row (or more) with no non-zeros. Apply padding for all empty rows   */
            	/* The difference between previous row_ptr_prev and current r shows the number of empty rows */
                for(IndexType j=row_ptr_prev[i]+1; j<r; j++){
                	padding_needed += VectFactor;
                	block_row_ptr[i][j] = block_row_ptr[i][j-1] + VectFactor;
                }
                expanded_nr_nzeros += padding_needed;

            	block_row_ptr[i][r] = block_row_ptr[i][r-1];
                curr_row_length[i] = 0; // reset for new line
            }
        }
        /* Find the block that will store the value that was read. Increase counter of elements where it needs to. */
        for(int i=0; i<blocks; i++){
			if( (c-1) <= thres_h[i] && (c-1) >= thres_l[i]){
				curr_row_length[i]++;
				row_ptr_prev[i] = r;
		        break;
    		}                        
		}
	}
	/*  Final check to see if any padding is needed for final row or for rows that are used for padding */
    for(int i=0; i<blocks; i++){
        block_row_ptr[i][row_ptr_prev[i]] += curr_row_length[i];
        IndexType padding_needed = 0;
        if(curr_row_length[i] % VectFactor==0)
            padding_needed = 0;
        else
            padding_needed = VectFactor - (curr_row_length[i] % VectFactor);
        block_row_ptr[i][row_ptr_prev[i]] += padding_needed;

        for(IndexType j=row_ptr_prev[i]+1; j<expanded_nr_rows+1; j++){
        	padding_needed += VectFactor;
        	block_row_ptr[i][j] = block_row_ptr[i][j-1] + VectFactor;
        }
        expanded_nr_nzeros += padding_needed;
    }
    free(row_ptr_prev);

    if (ferror(fp)) {
		printf("i/o error\n");
		return 2;
	}
	fclose(fp);

	/* 
		For last row only. We need to check if the number of non-zeros is a multiple of RATIO_i*ComputeUnits 
		or VectFactor (whichever is greater). This is why we add the appropriate number of zeros to the last row,
		in order to avoid unecessary if-conditionals in read_data_col_ind and read_data_values later
	*/
	N = RATIO_i * ComputeUnits;
	if(VectFactor > N)
		N = VectFactor;
	for(int i=0; i<blocks;i++){		
		IndexType data_per_block_mod_N = block_row_ptr[i][expanded_nr_rows] % N;
		if( data_per_block_mod_N != 0){
			block_row_ptr[i][expanded_nr_rows] += (N-data_per_block_mod_N);
		}
		data_per_block[i] = block_row_ptr[i][expanded_nr_rows];
	}

	hw_hdr->expanded_nr_rows = expanded_nr_rows;
	hw_hdr->expanded_nr_cols = expanded_nr_cols;
	hw_hdr->expanded_nr_nzeros = expanded_nr_nzeros;
	hw_hdr->blocks = blocks;

	return 0;
}

/*
    Allocate memory for csr hw matrix
*/
csr_hw_matrix *hw_matrix_alloc(int blocks, IndexType *nr_rows, IndexType *nr_nzeros, IndexType *thres_l, IndexType *thres_h)
{
    csr_hw_matrix *hw_matrix;

    hw_matrix = (csr_hw_matrix*)malloc(sizeof(csr_hw_matrix));
    hw_matrix->blocks = blocks;
    hw_matrix->col_ind   = (BusDataType**)malloc(blocks*sizeof(BusDataType*));
    hw_matrix->values    = (BusDataType**)malloc(blocks*sizeof(BusDataType*));
    hw_matrix->nr_rows   = (IndexType*)malloc(blocks*sizeof(IndexType));
    hw_matrix->nr_cols   = (IndexType*)malloc(blocks*sizeof(IndexType));
    hw_matrix->nr_nzeros = (IndexType*)malloc(blocks*sizeof(IndexType));
    
    for(int i=0; i<blocks; i++){
        hw_matrix->nr_rows[i]   = nr_rows[i];
        hw_matrix->nr_cols[i]   = thres_h[i] - thres_l[i] + 1;
        hw_matrix->nr_nzeros[i] = nr_nzeros[i];

        hw_matrix->col_ind[i] = (BusDataType*)sds_alloc_non_cacheable((nr_nzeros[i]/RATIO_i)*sizeof(BusDataType));
        hw_matrix->values[i] = (BusDataType*)sds_alloc_non_cacheable((nr_nzeros[i]/RATIO_v)*sizeof(BusDataType));
    }
    return hw_matrix;
}

/*
    Create a separate csr_matrix instance for the non-zero elements of current block
    Read the already stored matrix (matrix) and return the new one (block_matrix)
    block_matrix has (block_nr_nzeros) non-zeros, as we measured them earlier, during scan_matrix
*/
csr_matrix *create_block_matrix(csr_matrix *matrix, csr_hw_header hw_hdr, IndexType block_nr_nzeros, IndexType thres_l, IndexType thres_h)
{
    csr_matrix *block_matrix;

    block_matrix = (csr_matrix*)malloc(sizeof(csr_matrix));
    block_matrix->nr_rows   = hw_hdr.expanded_nr_rows;
    block_matrix->nr_cols   = thres_h - thres_l + 1;
    block_matrix->nr_nzeros = block_nr_nzeros;

    block_matrix->row_ptr   = (IndexType*)malloc((hw_hdr.expanded_nr_rows + 1)*sizeof(IndexType));
    block_matrix->col_ind   = (IndexType*)malloc(block_nr_nzeros*sizeof(IndexType));
    block_matrix->values    = (ValueType*)malloc(block_nr_nzeros*sizeof(ValueType));

    IndexType m_cnt = 0, n_cnt = 0; // m_cnt : old-big matrix, n_mtx : new block_matrix
    block_matrix->row_ptr[0] = 0;
    /* For every row of the matrix, check which elements belong to this block, assign them to the block_matrix and pad appropriately */
    for(IndexType i=1; i<matrix->nr_rows+1; i++){
        IndexType row_length = matrix->row_ptr[i] - matrix->row_ptr[i-1];
        IndexType curr_row_elements = 0;

        for(IndexType j=0; j<row_length; j++){
            /* keep currently checked element in temp variables */
            IndexType curr_col_ind = matrix->col_ind[m_cnt];
            ValueType curr_val = matrix->values[m_cnt];
            if(curr_col_ind <= thres_h && curr_col_ind >= thres_l){
                block_matrix->col_ind[n_cnt] = curr_col_ind - thres_l;
                block_matrix->values[n_cnt] = curr_val;
                n_cnt++;
                curr_row_elements++;
            }
            m_cnt++;
        }

        block_matrix->row_ptr[i] = block_matrix->row_ptr[i-1] + curr_row_elements;
        IndexType padding_needed;
        if(curr_row_elements % VectFactor == 0)
            padding_needed = 0;
        else
            padding_needed = VectFactor - (curr_row_elements%VectFactor);

        if(curr_row_elements == 0)
            padding_needed = VectFactor;

        for(int j=0; j<padding_needed; j++){
            block_matrix->col_ind[n_cnt] = 0;
            block_matrix->values[n_cnt] = 0;
            n_cnt++;
            block_matrix->row_ptr[i]++;
        }
    }
    // for all the rows that are used for padding (due to wide range for representation of matrix) just apply Vectorization Factor
    for(IndexType i=matrix->nr_rows+1; i<hw_hdr.expanded_nr_rows+1; i++){
        IndexType curr_row_elements = 0;
        
        block_matrix->row_ptr[i] = block_matrix->row_ptr[i-1] + curr_row_elements;
        IndexType padding_needed;
        padding_needed = VectFactor;

        for(int j=0; j<padding_needed; j++){
            block_matrix->col_ind[n_cnt] = 0;
            block_matrix->values[n_cnt] = 0;
            n_cnt++;
            block_matrix->row_ptr[i]++;
        }
    }

    /* 
        For last row, we need to pad if needed to fully fill the hw (wide) representation vectors, so that we avoid 
        if conditionals in the hw function later, when these data are transferred from sw to hw.
    */
    IndexType N = RATIO_i * ComputeUnits;
    if(VectFactor > N)
        N = VectFactor;
    
    IndexType data_per_block_mod_N = block_matrix->row_ptr[hw_hdr.expanded_nr_rows] % N;
    if( data_per_block_mod_N != 0){
        IndexType padding_needed = (N-data_per_block_mod_N);
        for(int j=0; j<padding_needed; j++){
            block_matrix->col_ind[n_cnt] = 0;
            block_matrix->values[n_cnt] = 0;
            n_cnt++;
            block_matrix->row_ptr[hw_hdr.expanded_nr_rows]++;   
        }
    }

    block_matrix->nr_nzeros = n_cnt;
    block_matrix->row_ptr[hw_hdr.expanded_nr_rows] = n_cnt;

    return block_matrix;
}

/* 
    Helper function for generate_balanced_hw_matrix_CU*, due to the replication of the process for many Compute Units
*/
void generate_balanced_hw_submatrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                    IndexType nr_rows, IndexType *v_cnt, IndexType offset,
                                    BusDataType *hw_col_ind, BusDataType *hw_values)
{
    IndexType curr_row_ptr = 0, prev_row_ptr = row_ptr[offset-1], curr_row_len = 0;
    IndexType row_cnt = 0;
    IndexType c_i = 0, v_i = 0, c_cp = 0, v_cp = 0;

    while(row_cnt < nr_rows){
        curr_row_ptr = row_ptr[row_cnt + offset];
        row_cnt++;
        curr_row_len = curr_row_ptr - prev_row_ptr;
        prev_row_ptr = curr_row_ptr;

        for(IndexType i=0; i<curr_row_len; i++){
            hw_col_ind[c_i].range(INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1,INDEX_TYPE_BIT_WIDTH*c_cp) = col_ind[*v_cnt];
            if(i==curr_row_len-1)
                hw_col_ind[c_i].range(INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1,INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1) = 1;
            else
                hw_col_ind[c_i].range(INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1,INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1) = 0;

            c_cp++;
            if(c_cp == RATIO_i){
                c_i++;
                c_cp = 0;
            }

            Union_double_uint tmp;
            tmp.f = values[*v_cnt];
            hw_values[v_i].range(VALUE_TYPE_BIT_WIDTH*(v_cp+1)-1,VALUE_TYPE_BIT_WIDTH*v_cp)  = tmp.apint;
            v_cp++;
            if(v_cp == RATIO_v){
                v_i++;
                v_cp = 0;
            }
            (*v_cnt)++;
        }
    }
}

#if CU == 1
/* 
	Find the sizes for the representation matrices that will be sent to the FPGA
	When using more than 1 Compute Units, Load Balancing is applied, so that each
	Compute Unit is assigned with (approximately) equal number of non-zeros.
	For now, with 1 Compute Unit, just assign all rows and non-zeros to it.
*/
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    
                                IndexType *nr_nzeros1,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block)
{
	for(int i=0; i<hw_hdr.blocks; i++){
		nr_rows1[i] = hw_hdr.expanded_nr_rows;

		IndexType offset = 0;
		nr_nzeros1[i] = block_row_ptr[i][offset + nr_rows1[i]] - block_row_ptr[i][offset];
	}
}

/*
    Assign the matrix elements that belong to current block to the appropriate csr_hw_matrix structs that will be transferred
    to the FPGA. Size of those representations is previously calculated in prepare_balanced_hw_matrix
*/
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_col_ind1, hw_values1);
}

/*
    Create the matrix representation that will be transferred to the FPGA
*/
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1)
{
    csr_hw_header hw_hdr;
    IndexType data_per_block[MAX_BLOCKS], thres_l[MAX_BLOCKS], thres_h[MAX_BLOCKS];
    IndexType *block_row_ptr[MAX_BLOCKS];
    scan_matrix(matrix->Filename, data_per_block, thres_l, thres_h, block_row_ptr, &hw_hdr);
    
    IndexType *nr_rows1;
    IndexType *nr_nzeros1;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    prepare_balanced_hw_matrix(nr_rows1,    
                               nr_nzeros1,  
                               block_row_ptr, hw_hdr, data_per_block);
    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);

    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        block_matrix = create_block_matrix(matrix, hw_hdr, data_per_block[i], thres_l[i], thres_h[i]);

        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->col_ind[i], (*hw_matrix1)->values[i]);

        delete_csr_matrix(block_matrix);
    }

    free(nr_rows1);
    free(nr_nzeros1);
    free(*block_row_ptr);
}


#elif CU == 2
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,        
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            curr_CU_nz += block_row_ptr[i][j+1] - block_row_ptr[i][j];
            row_cnt++;    
		/*            
            Double check if this is a proper line to break the representation between two or more CUs. 
            Firstly, check if the number of non-zeros for each CU is balanced.
            Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_i elements (RATIO_i > RATIO_v), so that a fully filled representation is created
            Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for y_ vectors)
            If all conditions success, then we found a breaking point!
        */            
            if( (curr_CU_nz > (data_per_block[i]/2)) && (curr_CU_nz % RATIO_i == 0) && (row_cnt%RATIO_v==0) ){ 
                if(CU_examined == 1)
                    nr_rows1[i] = row_cnt;
                curr_CU_nz = 0;
                row_cnt = 0;
                CU_examined++;
            }
        }
        nr_rows2[i] = row_cnt;

        IndexType offset = 0;
        nr_nzeros1[i] = block_row_ptr[i][offset + nr_rows1[i]] - block_row_ptr[i][offset];
        offset += nr_rows1[i];

        nr_nzeros2[i] = block_row_ptr[i][offset + nr_rows2[i]] - block_row_ptr[i][offset];
	}
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_col_ind1, hw_values1);    
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_col_ind2, hw_values2);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2)
{
    csr_hw_header hw_hdr;
    IndexType data_per_block[MAX_BLOCKS], thres_l[MAX_BLOCKS], thres_h[MAX_BLOCKS];
    IndexType *block_row_ptr[MAX_BLOCKS];
    scan_matrix(matrix->Filename, data_per_block, thres_l, thres_h, block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2;
    IndexType *nr_nzeros1,  *nr_nzeros2;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    
    prepare_balanced_hw_matrix(nr_rows1,    nr_rows2,   
                               nr_nzeros1,  nr_nzeros2, 
                               block_row_ptr, hw_hdr, data_per_block);
    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);

    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        block_matrix = create_block_matrix(matrix, hw_hdr, data_per_block[i], thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->col_ind[i], (*hw_matrix1)->values[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->col_ind[i], (*hw_matrix2)->values[i]);
        
        delete_csr_matrix(block_matrix);
    }

    free(nr_rows1);
    free(nr_rows2);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(*block_row_ptr);
}


#elif CU == 4
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            curr_CU_nz += block_row_ptr[i][j+1] - block_row_ptr[i][j];
            row_cnt++;    
        /*            
            Double check if this is a proper line to break the representation between two or more CUs. 
            Firstly, check if the number of non-zeros for each CU is balanced.
            Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_i elements (RATIO_i > RATIO_v), so that a fully filled representation is created
            Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for y_ vectors)
            If all conditions success, then we found a breaking point!
        */            
            if( (curr_CU_nz > (data_per_block[i]/4)) && (curr_CU_nz % RATIO_i == 0) && (row_cnt%RATIO_v==0) ){ 
                if(CU_examined == 1)
                    nr_rows1[i] = row_cnt;
                else if(CU_examined == 2)
                    nr_rows2[i] = row_cnt;
                else if(CU_examined == 3)
                    nr_rows3[i] = row_cnt;
                curr_CU_nz = 0;
                row_cnt = 0;
                CU_examined++;
            }
        }
        nr_rows4[i] = row_cnt;

        IndexType offset = 0;
        nr_nzeros1[i] = block_row_ptr[i][offset + nr_rows1[i]] - block_row_ptr[i][offset];
        offset += nr_rows1[i];

        nr_nzeros2[i] = block_row_ptr[i][offset + nr_rows2[i]] - block_row_ptr[i][offset];
        offset += nr_rows2[i];

        nr_nzeros3[i] = block_row_ptr[i][offset + nr_rows3[i]] - block_row_ptr[i][offset];
        offset += nr_rows3[i];

        nr_nzeros4[i] = block_row_ptr[i][offset + nr_rows4[i]] - block_row_ptr[i][offset];
    }
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2,
                                 IndexType nr_rows3, BusDataType *hw_col_ind3, BusDataType *hw_values3,
                                 IndexType nr_rows4, BusDataType *hw_col_ind4, BusDataType *hw_values4)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_col_ind1, hw_values1);   
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_col_ind2, hw_values2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_col_ind3, hw_values3);   
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_col_ind4, hw_values4);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4)
{
    csr_hw_header hw_hdr;
    IndexType data_per_block[MAX_BLOCKS], thres_l[MAX_BLOCKS], thres_h[MAX_BLOCKS];
    IndexType *block_row_ptr[MAX_BLOCKS];
    scan_matrix(matrix->Filename, data_per_block, thres_l, thres_h, block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2,      *nr_rows3,      *nr_rows4;
    IndexType *nr_nzeros1,  *nr_nzeros2,    *nr_nzeros3,    *nr_nzeros4;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    prepare_balanced_hw_matrix(nr_rows1,    nr_rows2,   nr_rows3,   nr_rows4,   
                               nr_nzeros1,  nr_nzeros2, nr_nzeros3, nr_nzeros4, 
                               block_row_ptr, hw_hdr, data_per_block);
    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);

    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        block_matrix = create_block_matrix(matrix, hw_hdr, data_per_block[i], thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->col_ind[i], (*hw_matrix1)->values[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->col_ind[i], (*hw_matrix2)->values[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->col_ind[i], (*hw_matrix3)->values[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->col_ind[i], (*hw_matrix4)->values[i]);
        
        delete_csr_matrix(block_matrix);
    }

    free(nr_rows1);
    free(nr_rows2);
    free(nr_rows3);
    free(nr_rows4);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(nr_nzeros3);
    free(nr_nzeros4);
    free(*block_row_ptr);
}


#elif CU == 8
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, IndexType *data_per_block)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            curr_CU_nz += block_row_ptr[i][j+1] - block_row_ptr[i][j];
            row_cnt++;    
        /*            
            Double check if this is a proper line to break the representation between two or more CUs. 
            Firstly, check if the number of non-zeros for each CU is balanced.
            Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_i elements (RATIO_i > RATIO_v), so that a fully filled representation is created
            Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for y_ vectors)
            If all conditions success, then we found a breaking point!
        */            
            if( (curr_CU_nz > (data_per_block[i]/8)) && (curr_CU_nz % RATIO_i == 0) && (row_cnt%RATIO_v==0) ){ 
                if(CU_examined == 1)
                    nr_rows1[i] = row_cnt;
                else if(CU_examined == 2)
                    nr_rows2[i] = row_cnt;
                else if(CU_examined == 3)
                    nr_rows3[i] = row_cnt;
                else if(CU_examined == 4)
                    nr_rows4[i] = row_cnt;
                else if(CU_examined == 5)
                    nr_rows5[i] = row_cnt;
                else if(CU_examined == 6)
                    nr_rows6[i] = row_cnt;
                else if(CU_examined == 7)
                    nr_rows7[i] = row_cnt;
                curr_CU_nz = 0;
                row_cnt = 0;
                CU_examined++;
            }
        }
        nr_rows8[i] = row_cnt;

        IndexType offset = 0;
        nr_nzeros1[i] = block_row_ptr[i][offset + nr_rows1[i]] - block_row_ptr[i][offset];
        offset += nr_rows1[i];

        nr_nzeros2[i] = block_row_ptr[i][offset + nr_rows2[i]] - block_row_ptr[i][offset];
        offset += nr_rows2[i];

        nr_nzeros3[i] = block_row_ptr[i][offset + nr_rows3[i]] - block_row_ptr[i][offset];
        offset += nr_rows3[i];

        nr_nzeros4[i] = block_row_ptr[i][offset + nr_rows4[i]] - block_row_ptr[i][offset];
        offset += nr_rows4[i];

        nr_nzeros5[i] = block_row_ptr[i][offset + nr_rows5[i]] - block_row_ptr[i][offset];
        offset += nr_rows5[i];

        nr_nzeros6[i] = block_row_ptr[i][offset + nr_rows6[i]] - block_row_ptr[i][offset];
        offset += nr_rows6[i];

        nr_nzeros7[i] = block_row_ptr[i][offset + nr_rows7[i]] - block_row_ptr[i][offset];
        offset += nr_rows7[i];

        nr_nzeros8[i] = block_row_ptr[i][offset + nr_rows8[i]] - block_row_ptr[i][offset];
        offset += nr_rows8[i];
    }
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_col_ind1, BusDataType *hw_values1,
                                 IndexType nr_rows2, BusDataType *hw_col_ind2, BusDataType *hw_values2,
                                 IndexType nr_rows3, BusDataType *hw_col_ind3, BusDataType *hw_values3,
                                 IndexType nr_rows4, BusDataType *hw_col_ind4, BusDataType *hw_values4,
                                 IndexType nr_rows5, BusDataType *hw_col_ind5, BusDataType *hw_values5,
                                 IndexType nr_rows6, BusDataType *hw_col_ind6, BusDataType *hw_values6,
                                 IndexType nr_rows7, BusDataType *hw_col_ind7, BusDataType *hw_values7,
                                 IndexType nr_rows8, BusDataType *hw_col_ind8, BusDataType *hw_values8)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_col_ind1, hw_values1);   
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_col_ind2, hw_values2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_col_ind3, hw_values3);   
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_col_ind4, hw_values4);
    offset += nr_rows4;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows5, &v_cnt, offset, hw_col_ind5, hw_values5);
    offset += nr_rows5;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows6, &v_cnt, offset, hw_col_ind6, hw_values6);   
    offset += nr_rows6;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows7, &v_cnt, offset, hw_col_ind7, hw_values7);
    offset += nr_rows7;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows8, &v_cnt, offset, hw_col_ind8, hw_values8);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8)
{
    csr_hw_header hw_hdr;
    IndexType data_per_block[MAX_BLOCKS], thres_l[MAX_BLOCKS], thres_h[MAX_BLOCKS];
    IndexType *block_row_ptr[MAX_BLOCKS];
    scan_matrix(matrix->Filename, data_per_block, thres_l, thres_h, block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2,      *nr_rows3,      *nr_rows4,      *nr_rows5,    *nr_rows6,      *nr_rows7,      *nr_rows8;
    IndexType *nr_nzeros1,  *nr_nzeros2,    *nr_nzeros3,    *nr_nzeros4,    *nr_nzeros5,  *nr_nzeros6,    *nr_nzeros7,    *nr_nzeros8;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
 
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   nr_rows3,   nr_rows4,   nr_rows5,   nr_rows6,   nr_rows7,   nr_rows8,   
                               nr_nzeros1,  nr_nzeros2, nr_nzeros3, nr_nzeros4, nr_nzeros5, nr_nzeros6, nr_nzeros7, nr_nzeros8, 
                               block_row_ptr, hw_hdr, data_per_block);
    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);
    *hw_matrix5 = hw_matrix_alloc(hw_hdr.blocks, nr_rows5, nr_nzeros5, thres_l, thres_h);
    *hw_matrix6 = hw_matrix_alloc(hw_hdr.blocks, nr_rows6, nr_nzeros6, thres_l, thres_h);
    *hw_matrix7 = hw_matrix_alloc(hw_hdr.blocks, nr_rows7, nr_nzeros7, thres_l, thres_h);
    *hw_matrix8 = hw_matrix_alloc(hw_hdr.blocks, nr_rows8, nr_nzeros8, thres_l, thres_h);

    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        block_matrix = create_block_matrix(matrix, hw_hdr, data_per_block[i], thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->col_ind[i], (*hw_matrix1)->values[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->col_ind[i], (*hw_matrix2)->values[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->col_ind[i], (*hw_matrix3)->values[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->col_ind[i], (*hw_matrix4)->values[i],
                                    (*hw_matrix5)->nr_rows[i], (*hw_matrix5)->col_ind[i], (*hw_matrix5)->values[i],
                                    (*hw_matrix6)->nr_rows[i], (*hw_matrix6)->col_ind[i], (*hw_matrix6)->values[i],
                                    (*hw_matrix7)->nr_rows[i], (*hw_matrix7)->col_ind[i], (*hw_matrix7)->values[i],
                                    (*hw_matrix8)->nr_rows[i], (*hw_matrix8)->col_ind[i], (*hw_matrix8)->values[i]);
 
        delete_csr_matrix(block_matrix);
    }

    free(nr_rows1);
    free(nr_rows2);
    free(nr_rows3);
    free(nr_rows4);
    free(nr_rows5);
    free(nr_rows6);
    free(nr_rows7);
    free(nr_rows8);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(nr_nzeros3);
    free(nr_nzeros4);
    free(nr_nzeros5);
    free(nr_nzeros6);
    free(nr_nzeros7);
    free(nr_nzeros8);
    free(*block_row_ptr);
}
#endif

/*
    Return size (in MB) of hw_matrix representation for each Compute Unit separately
*/
ValueType storage_overhead(csr_hw_matrix *matrix){
    IndexType mem = 0;
    mem += (matrix->blocks)*(INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH); // nr_rows, nr_columns, nr_nzeros
    for(int i=0; i<matrix->blocks; i++){
        mem += (matrix->nr_nzeros[i]/RATIO_i)*BUS_BIT_WIDTH; // col_ind
        mem += (matrix->nr_nzeros[i]/RATIO_v)*BUS_BIT_WIDTH; // values
    }
    // std::cout << mem/(8.0*1024*1024) << " MB\n";
    return mem/(8.0*1024*1024);
}

/*
	Free memory allocated for csr hw matrix
*/
void _delete_csr_hw_matrix(csr_hw_matrix **matrix)
{
	if (!*matrix)
        return;

    free((*matrix)->nr_rows);
    free((*matrix)->nr_cols);
    free((*matrix)->nr_nzeros);

    for(int i=0; i<(*matrix)->blocks; i++){
    	sds_free((*matrix)->col_ind[i]);
    	sds_free((*matrix)->values[i]);
    }
    free((*matrix)->col_ind);
    free((*matrix)->values);
    free((*matrix));
    return;
}

/*
	Allocate memory for csr hw vector (x or y)
*/
void create_csr_hw_vector(csr_hw_vector **hw_vector, int blocks, IndexType *nr_values)
{
	(*hw_vector) = (csr_hw_vector*)malloc(sizeof(csr_hw_vector));
	(*hw_vector)->nr_values = (IndexType*)malloc(blocks*sizeof(IndexType));
	(*hw_vector)->values    = (BusDataType**)malloc(blocks*sizeof(BusDataType*));

	(*hw_vector)->blocks = blocks;

	for(int i=0; i<blocks; i++){
		(*hw_vector)->nr_values[i] = nr_values[i];
		(*hw_vector)->values[i] = (BusDataType*)sds_alloc_non_cacheable((nr_values[i]/RATIO_v)*sizeof(BusDataType));
	}
}

/*
	Free memory allocated for csr hw vector
*/
void _delete_csr_hw_vector(csr_hw_vector **vector)
{
	if (!*vector)
        return;
	
	free((*vector)->nr_values);

	for(int i=0; i<(*vector)->blocks; i++)
		sds_free((*vector)->values[i]);
    free((*vector)->values);
    free(*vector);
    return;
}

/*
	Write a software vector in a representation vector that will be transferred to the FPGA
*/
void write_csr_hw_vector(csr_hw_vector *hw_vector, csr_vector *vector)
{   
    Union_double_uint tmp;
    BusDataType tmp_wide;
    IndexType cnt = 0;
    for(int i=0; i<hw_vector->blocks; i++){
    	for(IndexType j=0; j<hw_vector->nr_values[i]/RATIO_v; j++){
    		for(int k=0; k<RATIO_v; k++){
    			if(cnt<vector->nr_values)
	    			tmp.f = vector->values[cnt];
	    		else
	    			tmp.f = 0; // zero-pad the remaining columns (until we reach COLS_DIV_BLOCKS) 
    			cnt++;	
    			tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k) = tmp.apint;
    		}
    		hw_vector->values[i][j] = tmp_wide;
    	}
    }
}

/*
    Print the contents of a BusDataType vector (values). (flag) indicates whether to print ValueType or IndexType values.
*/
void print_wide(BusDataType *values, IndexType nr_values, int flag)
{
    if(flag==0){
        Union_double_uint tmp;
        BusDataType tmp_wide;
        for(IndexType j=0; j<nr_values; j++){
            tmp_wide = values[j];
            for(int k=0; k<RATIO_v; k++){
                tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
                std::cout << std::setprecision(14) << "| (" << VALUE_TYPE_BIT_WIDTH*(k+1)-1 << " : " << VALUE_TYPE_BIT_WIDTH*k << ") = " << tmp.f << " ";
            }
            std::cout << "|\n";
        }
    }
    else{
        IndexType tmp;
        BoolType tmp2;
        BusDataType tmp_wide;
        for(IndexType j=0; j<nr_values; j++){
            tmp_wide = values[j];
            for(int k=0; k<RATIO_i; k++){
                tmp = tmp_wide.range(INDEX_TYPE_BIT_WIDTH*(k+1)-2,INDEX_TYPE_BIT_WIDTH*k);
                tmp2 = tmp_wide.range(INDEX_TYPE_BIT_WIDTH*(k+1)-1,INDEX_TYPE_BIT_WIDTH*(k+1)-1);
                std::cout << "| ("<< INDEX_TYPE_BIT_WIDTH*(k+1)-1 << " : " << INDEX_TYPE_BIT_WIDTH*k << ") = " << tmp << " <"<< tmp2 << ">\t";
            }
            std::cout << "|\n";
        }
    }
}

/*
	For each Compute Unit a separate y_hw (values) is used. We accumulate all results in one representation vector (hw_y)
	We use cnt to begin writing to the proper region of hw_y.
	Due to padding, the size of values may be larger than the actual number of rows of the tested matrix. This is why we
	check if cnt reached the limit.
*/
void accum_results(IndexType nr_values, BusDataType *values, IndexType cnt, ValueType *hw_y, IndexType nr_rows)
{
   for(IndexType j=0; j<nr_values/RATIO_v; j++){
    	Union_double_uint tmp;
        BusDataType tmp_wide;
        tmp_wide = values[j];
        for(int k=0; k<RATIO_v; k++){
            tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
            if(cnt<nr_rows){
                hw_y[cnt] += tmp.f;
                cnt++;
            }
            else
                break;
        }
    }
}

/* 
	Verify that results from hw (hw_values) and sw (sw_values) are (approximately, depending on diff_thres) equal
	verbose : 0=nothing, 1=only errors, 2=all
*/
int verification(IndexType nr_values, ValueType *sw_values, ValueType *hw_values, int verbose)
{
    ValueType diff_thres = 1e-5; // how precise we want the result verification
    int status = 0;
    IndexType err_cnt = 0;
    for(IndexType i=0; i<nr_values; i++){
        ValueType diff = fabs(sw_values[i]-hw_values[i]);
        if(verbose == 2)
            std::cout << std::setprecision(14) << i << " : y_gold = " << sw_values[i] << "\ty_hw = " << hw_values[i];
        if(diff >= diff_thres || diff != diff){
            status = 1;
            err_cnt++;
            if(verbose == 1 || verbose == 2)
            	std::cout << std::setprecision(14) << "\tError occurs at " << i << " : y_gold = " << sw_values[i] << ", y_hw = " << hw_values[i] << ". Relative difference is " << fabs(diff/sw_values[i]) << "\n";
        }
    }
    if(status)
	    std::cout << "Total errors : " << err_cnt << "\n";
    return status; 
}
