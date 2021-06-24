#include "csr_hw.h"

/*
    Scan matrix for empty rows and to find the exact number of rows, columns, non-zeros, so that they satisfy the padding conditions
    Store the number of non-zeros of each row of each block that the matrix will be split to in (block_row_ptr)
*/
int scan_matrix(csr_matrix *matrix, IndexType **thres_l, IndexType **thres_h, IndexType ***block_row_ptr, csr_hw_header *hw_hdr)
{
    double sc_s, sc_f, sc_exec;
    sc_s = getTimestamp();

    IndexType expanded_nr_rows, expanded_nr_cols,   expanded_nr_nzeros;
    
    expanded_nr_rows   = matrix->nr_rows;
    expanded_nr_cols   = matrix->nr_cols;
    expanded_nr_nzeros = 0; 

    /*
        RATIO is the number of elements packed inside a single elemenent of the structs that will be sent to and received from the FPGA
        Due to the fact that x(vector) and the y(result) will be packed in such structs, we need to have number of columns (due to x vector)
        and number of rows (due to y vector) that are multiples of RATIO. Therefore, we check if nr_rows and nr_cols satisfy this condition
        For number of columns, we also need it to be a multiple of the number of blocks that the matrix will be split to.
    */

    int blocks = matrix->nr_cols/COLS_DIV_BLOCKS + 1;
    if(matrix->nr_cols%COLS_DIV_BLOCKS == 0) // in case nr_cols is divided perfectly, no need to have an extra block
        blocks--;

    /* We care about is hw_x to be packed fully, therefore cols needs to be a multiple of RATIO_v and blocks */
    IndexType N = RATIO_v * blocks; 
    IndexType cols_mod_N = matrix->nr_cols % N;  
    if (cols_mod_N != 0) // need to pad the number of columns
        expanded_nr_cols += (N-cols_mod_N);
    
    /* pad the number of rows to whichever is greater among RATIO_ci and ComputeUnits. So that every Compute Unit gets a multiple of N number of rows */
    /* This here has to be padded 
          either to be a multiple of ComputeUnits*RATIO_v (in order for each y_hw (1 for each CU) to have RATIO_v values) 
          or for each CU, to fit in col_ind, therefore RATIO_ci/VectFactor*ComputeUnits <- for now ignore this, fix it in hw
    */
    /* NEW NOTE : Padding was applied previously. Now, since that we will store only the non-empty rows, no use in padding before deciding which rows are useful and which not.
                  Therefore, leave it as it is, and we will care about it later (perhaps padding needed for last Compute Unit, so that y_hw can be packed correctly on RATIO_v struct)
                  I believe padding number of rows is useless, since there will be empty rows, so not sure whether padding is  essential here! For example when trying str_600, 
                  first 128-block returned last Compute Unit with odd number of rows. Remove it for now */
    // N = RATIO_ci;
    // if(ComputeUnits>N)
    //     N = ComputeUnits;
    // N = ComputeUnits*RATIO_v;
    // IndexType rows_mod_N = matrix->nr_rows % N;
    // if(rows_mod_N != 0)
    //     expanded_nr_rows += (N-rows_mod_N);

    /*
        block_row_ptr will hold the number of non-zeros per block. This will be used to allocate 
        the proper memory space for our hw_matrices in hw_matrix_alloc.
    */
    *block_row_ptr = (IndexType**)malloc(blocks*sizeof(IndexType*));
    *thres_l = (IndexType*)malloc(blocks*sizeof(IndexType));
    *thres_h = (IndexType*)malloc(blocks*sizeof(IndexType));
    for(int i=0; i<blocks;i++){
        (*block_row_ptr)[i] = (IndexType*)malloc((expanded_nr_rows+1)*sizeof(IndexType));
        (*block_row_ptr)[i][0] = (*block_row_ptr)[i][1] = 0;
    }

    (*thres_l)[0] = 0;
    if(blocks==1)
        (*thres_h)[0] = expanded_nr_cols-1;
    else
       (*thres_h)[0] = COLS_DIV_BLOCKS-1;

    for(int i=1; i<blocks; i++){
        (*thres_l)[i] = (*thres_h)[i-1]+1;
        if(i==blocks-1)
            (*thres_h)[i] = expanded_nr_cols-1;
        else
            (*thres_h)[i] = (*thres_l)[i] + COLS_DIV_BLOCKS-1;
    }

    IndexType *curr_row_length, m_cnt = 0;
    // IndexType *row_ptr_prev;
    // row_ptr_prev = (IndexType*)malloc(blocks*sizeof(IndexType));
    curr_row_length = (IndexType*)malloc(blocks*sizeof(IndexType));
    for(int i=0; i<blocks; i++){
        // row_ptr_prev[i] = 0;
        curr_row_length[i] = 0;
    }

    for(IndexType i=1; i<matrix->nr_rows+1; i++){
        IndexType row_length = matrix->row_ptr[i] - matrix->row_ptr[i-1];

        /* store info for each non-zero of current row */
        for(IndexType j=0; j<row_length; j++){
            /* keep currently checked element in temp variables */
            IndexType curr_col_ind = matrix->col_ind[m_cnt];
            m_cnt++;
            
            /* Find the block that will store the value that was read. Increase counter of elements where it needs to. */
            for(int k=0; k<blocks; k++){
                if(curr_col_ind <= (*thres_h)[k] && curr_col_ind >= (*thres_l)[k]){
                    curr_row_length[k]++;
                    // row_ptr_prev[k] = i;
                    break;
                }
            }
        }

        /* store everything on block_row_ptr, prepare for next row */
        for(int k=0; k<blocks; k++){
            IndexType padding_needed = 0;
            if(curr_row_length[k] % VectFactor==0 || curr_row_length[k] == 0)
                padding_needed = 0;
            else
                padding_needed = VectFactor - (curr_row_length[k] % VectFactor);

            (*block_row_ptr)[k][i] += curr_row_length[k] + padding_needed;
            curr_row_length[k] = 0; // reset for new line, if not last row, prepare for next row
            if(i!=matrix->nr_rows)
                (*block_row_ptr)[k][i+1] = (*block_row_ptr)[k][i];
        }
    }
    // free(row_ptr_prev);
    free(curr_row_length);

    /* For the rows that were used for padding, bring the block_row_ptr to the right state */
    for(int i=0; i<blocks;i++){     
        // for(IndexType j=matrix->nr_rows+1; j<expanded_nr_rows+1; j++)
        //     (*block_row_ptr)[i][j] = (*block_row_ptr)[i][j-1];

        expanded_nr_nzeros += (*block_row_ptr)[i][expanded_nr_rows];
        // std::cout << "BLOCK " << i << "\tnr_nzeros = " << (*block_row_ptr)[i][expanded_nr_rows] << "\n";
    }

    hw_hdr->expanded_nr_rows = expanded_nr_rows;
    hw_hdr->expanded_nr_cols = expanded_nr_cols;
    hw_hdr->expanded_nr_nzeros = expanded_nr_nzeros;
    hw_hdr->blocks = blocks;

    // std::cout << "\n FROM matrix->nr_cols : " << matrix->nr_cols << " TO expanded_nr_cols : " << expanded_nr_cols << "\n";
    // std::cout << " FROM matrix->nr_rows : " << matrix->nr_rows << " TO expanded_nr_rows : " << expanded_nr_rows << "\n";
    // std::cout << " FROM matrix->nr_nzeros : " << matrix->nr_nzeros << " TO expanded_nr_nzeros : " << expanded_nr_nzeros << "\n\n";

    sc_f = getTimestamp();
    sc_exec = (sc_f - sc_s)/(1000);
    printf("\tScan matrix time : %.6f ms elapsed\n", sc_exec);

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
    hw_matrix->submatrix = (BusDataType**)malloc(blocks*sizeof(BusDataType*));
    hw_matrix->nr_rows   = (IndexType*)malloc(blocks*sizeof(IndexType));
    hw_matrix->nr_cols   = (IndexType*)malloc(blocks*sizeof(IndexType));
    hw_matrix->nr_nzeros = (IndexType*)malloc(blocks*sizeof(IndexType));
    
    hw_matrix->nr_ci     = (IndexType*)malloc(blocks*sizeof(IndexType));
    hw_matrix->nr_val    = (IndexType*)malloc(blocks*sizeof(IndexType));
    
    for(int i=0; i<blocks; i++){
        hw_matrix->nr_rows[i]   = nr_rows[i];
        hw_matrix->nr_cols[i]   = thres_h[i] - thres_l[i] + 1;
        hw_matrix->nr_nzeros[i] = nr_nzeros[i];

        /* Especially with small sized blocks, there may be empty ones!*/
        if(nr_nzeros[i] == 0) std::cout << "WARNING !!!!! block " << i << " is empty!\n";

        /* Due to how distribution of nonzeros among Compute Units is performed, only col_ind is threatened and needs to be checked here. */
        if(nr_nzeros[i]%RATIO_ci !=0) 
            hw_matrix->nr_ci[i] = nr_nzeros[i]/RATIO_ci+1;
            // std::cout << "\tblock " << i <<  " col_ind needs extra. nr_nzeros=" << nr_nzeros[i] << " ("<<nr_nzeros[i]<<"%"<<RATIO_ci<<"="<<nr_nzeros[i]%RATIO_ci << "), therefore " << (nr_nzeros[i]/RATIO_ci+1) << " needed\n";
        else
            hw_matrix->nr_ci[i] = nr_nzeros[i]/RATIO_ci;
        hw_matrix->nr_val[i] = nr_nzeros[i]/RATIO_v;
        hw_matrix->submatrix[i] = (BusDataType*)sds_alloc_non_cacheable((hw_matrix->nr_ci[i]+hw_matrix->nr_val[i])*sizeof(BusDataType));
    }
    return hw_matrix;
}

/*
    Create a separate csr_matrix instance for the non-zero elements of current block
    Read the already stored matrix (matrix) and return the new one (block_matrix)
    block_matrix has (block_nr_nzeros) non-zeros, as we measured them earlier, during scan_matrix
*/
csr_matrix *create_block_matrix(csr_matrix *matrix, IndexType *block_row_ptr, IndexType block_nr_rows, IndexType block_nr_nzeros, IndexType thres_l, IndexType thres_h)
{
    double cb_s, cb_f, cb_exec;
    cb_s = getTimestamp();

    csr_matrix *block_matrix;

    block_matrix = (csr_matrix*)malloc(sizeof(csr_matrix));
    block_matrix->nr_rows   = block_nr_rows;
    block_matrix->nr_cols   = thres_h - thres_l + 1;
    block_matrix->nr_nzeros = block_nr_nzeros;

    block_matrix->row_ptr   = (IndexType*)malloc((block_nr_rows + 1)*sizeof(IndexType));
    block_matrix->col_ind   = (IndexType*)malloc(block_nr_nzeros*sizeof(IndexType));
    block_matrix->values    = (ValueType*)malloc(block_nr_nzeros*sizeof(ValueType));

    IndexType c_cnt = 0, r_cnt = 1; // c_cnt : new block_matrix, r_cnt : new block_matrix, for rows
    block_matrix->row_ptr[0] = 0;
    /* For every row of the matrix, check which elements belong to this block, assign them to the block_matrix and pad appropriately */
    for(IndexType i=1; i<matrix->nr_rows+1; i++){
        IndexType row_length = matrix->row_ptr[i] - matrix->row_ptr[i-1];
        IndexType real_row_length = block_row_ptr[i] - block_row_ptr[i-1];
        IndexType curr_row_elements = 0;
        if(real_row_length != 0){
            IndexType m_cnt = matrix->row_ptr[i-1];
            for(IndexType j=0; j<row_length; j++){
                /* keep currently checked element in temp variables */
                IndexType curr_col_ind = matrix->col_ind[m_cnt];
                ValueType curr_val = matrix->values[m_cnt];
                if(curr_col_ind <= thres_h && curr_col_ind >= thres_l){
                    block_matrix->col_ind[c_cnt] = curr_col_ind - thres_l;
                    block_matrix->values[c_cnt] = curr_val;
                    c_cnt++;
                    curr_row_elements++;
                }
                m_cnt++;
            }
            
            /* Padding applied due to Vectorization Factor! */
            IndexType padding_needed = 0;
            if(curr_row_elements % VectFactor != 0)
                padding_needed = VectFactor - (curr_row_elements%VectFactor);

            for(IndexType j=0; j<padding_needed; j++){
                block_matrix->col_ind[c_cnt] = 0;
                block_matrix->values[c_cnt] = 0;
                c_cnt++;
                curr_row_elements++;
            }

            block_matrix->row_ptr[r_cnt] = block_matrix->row_ptr[r_cnt-1] + curr_row_elements;
            r_cnt++;
        }
    }

    /* If any rows are used for padding, pad with VectFactor */
    for(IndexType i=r_cnt; i<block_nr_rows+1; i++){        
        block_matrix->row_ptr[i] = block_matrix->row_ptr[i-1] + VectFactor;
        IndexType padding_needed = VectFactor;

        for(IndexType j=0; j<padding_needed; j++){
            block_matrix->col_ind[c_cnt] = 0;
            block_matrix->values[c_cnt] = 0;
            c_cnt++;
        }
    }

    block_matrix->nr_nzeros = c_cnt;
    block_matrix->row_ptr[block_nr_rows] = c_cnt;

    cb_f = getTimestamp();
    cb_exec = (cb_f - cb_s)/(1000);
    // printf("\tCreate block matrix time : %.6f ms elapsed\n", cb_exec);

    return block_matrix;
}

/* 
    Helper function for generate_balanced_hw_matrix_CU*, due to the replication of the process for many Compute Units
*/
void generate_balanced_hw_submatrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                    IndexType nr_rows, IndexType *v_cnt, IndexType offset,
                                    BusDataType *hw_submatrix)
{
    double gb_s, gb_f, gb_exec;
    gb_s = getTimestamp();

    IndexType curr_row_ptr = 0, prev_row_ptr = row_ptr[offset-1], curr_row_len = 0;
    IndexType row_cnt = 0;
    IndexType c_i = 0, v_i = 1, c_cp = 0, v_cp = 0;

    while(row_cnt < nr_rows){
        curr_row_ptr = row_ptr[row_cnt + offset];
        row_cnt++;
        curr_row_len = curr_row_ptr - prev_row_ptr;
        prev_row_ptr = curr_row_ptr;

        for(IndexType i=0; i<curr_row_len; i++){
            hw_submatrix[c_i].range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(c_cp+1)-2,COMPRESSED_INDEX_TYPE_BIT_WIDTH*c_cp) = col_ind[*v_cnt];
            if(i==curr_row_len-1) // final element of current row, set appropriate flag to 1
                hw_submatrix[c_i].range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1,COMPRESSED_INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1) = 1;
            else
                hw_submatrix[c_i].range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1,COMPRESSED_INDEX_TYPE_BIT_WIDTH*(c_cp+1)-1) = 0;

            c_cp++;
            if(c_cp == RATIO_ci){
                c_i += RATIO_col_val; // skip next RATIO_col_val rows of submatrix struct, since values are stored there
                c_cp = 0;
            }

            Union_double_uint tmp;
            tmp.f = values[*v_cnt];
            hw_submatrix[v_i].range(VALUE_TYPE_BIT_WIDTH*(v_cp+1)-1,VALUE_TYPE_BIT_WIDTH*v_cp)  = tmp.apint;
            
            v_cp++;
            if(v_cp == RATIO_v){
                v_i++;
                if(v_i % RATIO_col_val == 0) //means that we are on a region dedicated for column-indices, skip next line of submatrix struct
                    v_i++;
                v_cp = 0;
            }
            (*v_cnt)++;
        }
    }

    gb_f = getTimestamp();
    gb_exec = (gb_f - gb_s)/(1000);
    // printf("\t\tGenerate balanced submatrix time : %.6f ms elapsed\n", gb_exec);
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
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            }
        }
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows1[i] = row_cnt;
        nr_nzeros1[i] = curr_CU_nz;
    }
}

/*
    Assign the matrix elements that belong to current block to the appropriate csr_hw_matrix structs that will be transferred
    to the FPGA. Size of those representations is previously calculated in prepare_balanced_hw_matrix
*/
void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
}

/*
    Create the matrix representation that will be transferred to the FPGA
*/
void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

    IndexType *nr_rows1;
    IndexType *nr_nzeros1;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   
                               nr_nzeros1, 
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i];
        IndexType block_nr_nzeros = nr_nzeros1[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);

        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

    free(nr_rows1);
    free(nr_nzeros1);
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h); 
}

#elif CU == 2
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            /*            
                Double check if this is a proper line to break the representation between two or more CUs. 
                Firstly, check if the number of non-zeros for each CU is balanced.
                Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_v elements, so that a fully filled representation (at least for hw_values) is created
                Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for hw_y vectors)
                If all conditions success, then we found a breaking point!
            */            
                bool S1 = (curr_CU_nz>(block_nr_nzeros/ComputeUnits)), S2 = (curr_CU_nz%RATIO_v == 0), S3 = (row_cnt%RATIO_v==0);
                if( S1 && S2 && S3 ){ 
                    if(CU_examined == 1){
                        nr_rows1[i] = row_cnt;
                        nr_nzeros1[i] = curr_CU_nz;
                    }
                    curr_CU_nz = 0;
                    row_cnt = 0;
                    CU_examined++;
                }
            }
        }
        /* Previous CUs are assigned with  multiple of RATIO_v number of rows and number of nonzeros. */
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows2[i] = row_cnt;
        nr_nzeros2[i] = curr_CU_nz;
    }
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_submatrix2);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2;
    IndexType *nr_nzeros1,  *nr_nzeros2;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   
                               nr_nzeros1, nr_nzeros2, 
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i]+nr_rows2[i];
        IndexType block_nr_nzeros = nr_nzeros1[i]+nr_nzeros2[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << " " << nr_rows2[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << " " << nr_nzeros2[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

    free(nr_rows1);
    free(nr_rows2);
    free(nr_nzeros1);
    free(nr_nzeros2);
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h);
}


#elif CU == 4
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            /*            
                Double check if this is a proper line to break the representation between two or more CUs. 
                Firstly, check if the number of non-zeros for each CU is balanced.
                Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_v elements, so that a fully filled representation (at least for hw_values) is created
                Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for hw_y vectors)
                If all conditions success, then we found a breaking point!
            */            
                bool S1 = (curr_CU_nz>(block_nr_nzeros/ComputeUnits)), S2 = (curr_CU_nz%RATIO_v == 0), S3 = (row_cnt%RATIO_v==0);
                if( S1 && S2 && S3 ){ 
                    if(CU_examined == 1){
                        nr_rows1[i] = row_cnt;
                        nr_nzeros1[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 2){
                        nr_rows2[i] = row_cnt;
                        nr_nzeros2[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 3){
                        nr_rows3[i] = row_cnt;
                        nr_nzeros3[i] = curr_CU_nz;
                    }
                    curr_CU_nz = 0;
                    row_cnt = 0;
                    CU_examined++;
                }
            }
        }
        /* Previous CUs are assigned with  multiple of RATIO_v number of rows and number of nonzeros. */
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows4[i] = row_cnt;
        nr_nzeros4[i] = curr_CU_nz;
    }
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_submatrix2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_submatrix3);
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_submatrix4);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

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

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   nr_rows3,   nr_rows4,   
                               nr_nzeros1, nr_nzeros2, nr_nzeros3, nr_nzeros4, 
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i]+nr_rows2[i]+nr_rows3[i]+nr_rows4[i];
        IndexType block_nr_nzeros = nr_nzeros1[i]+nr_nzeros2[i]+nr_nzeros3[i]+nr_nzeros4[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << " " << nr_rows2[i] << " " << nr_rows3[i] << " " << nr_rows4[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << " " << nr_nzeros2[i] << " " << nr_nzeros3[i] << " " << nr_nzeros4[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->submatrix[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->submatrix[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

    free(nr_rows1);
    free(nr_rows2);
    free(nr_rows3);
    free(nr_rows4);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(nr_nzeros3);
    free(nr_nzeros4);
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h);
}


#elif CU == 8
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            /*            
                Double check if this is a proper line to break the representation between two or more CUs. 
                Firstly, check if the number of non-zeros for each CU is balanced.
                Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_v elements, so that a fully filled representation (at least for hw_values) is created
                Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for hw_y vectors)
                If all conditions success, then we found a breaking point!
            */            
                bool S1 = (curr_CU_nz>(block_nr_nzeros/ComputeUnits)), S2 = (curr_CU_nz%RATIO_v == 0), S3 = (row_cnt%RATIO_v==0);
                if( S1 && S2 && S3 ){ 
                    if(CU_examined == 1){
                        nr_rows1[i] = row_cnt;
                        nr_nzeros1[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 2){
                        nr_rows2[i] = row_cnt;
                        nr_nzeros2[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 3){
                        nr_rows3[i] = row_cnt;
                        nr_nzeros3[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 4){
                        nr_rows4[i] = row_cnt;
                        nr_nzeros4[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 5){
                        nr_rows5[i] = row_cnt;
                        nr_nzeros5[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 6){
                        nr_rows6[i] = row_cnt;
                        nr_nzeros6[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 7){
                        nr_rows7[i] = row_cnt;
                        nr_nzeros7[i] = curr_CU_nz;
                    }
                    curr_CU_nz = 0;
                    row_cnt = 0;
                    CU_examined++;
                }
            }
        }
        /* Previous CUs are assigned with  multiple of RATIO_v number of rows and number of nonzeros. */
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows8[i] = row_cnt;
        nr_nzeros8[i] = curr_CU_nz;
    }
}

void generate_balanced_hw_matrix(IndexType *row_ptr, IndexType *col_ind, ValueType *values, 
                                 IndexType nr_rows1, BusDataType *hw_submatrix1,
                                 IndexType nr_rows2, BusDataType *hw_submatrix2,
                                 IndexType nr_rows3, BusDataType *hw_submatrix3,
                                 IndexType nr_rows4, BusDataType *hw_submatrix4,
                                 IndexType nr_rows5, BusDataType *hw_submatrix5,
                                 IndexType nr_rows6, BusDataType *hw_submatrix6,
                                 IndexType nr_rows7, BusDataType *hw_submatrix7,
                                 IndexType nr_rows8, BusDataType *hw_submatrix8)
{
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_submatrix2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_submatrix3);
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_submatrix4);
    offset += nr_rows4;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows5, &v_cnt, offset, hw_submatrix5);
    offset += nr_rows5;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows6, &v_cnt, offset, hw_submatrix6);
    offset += nr_rows6;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows7, &v_cnt, offset, hw_submatrix7);
    offset += nr_rows7;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows8, &v_cnt, offset, hw_submatrix8);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

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

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   nr_rows3,   nr_rows4,   nr_rows5,   nr_rows6,   nr_rows7,   nr_rows8,   
                               nr_nzeros1, nr_nzeros2, nr_nzeros3, nr_nzeros4, nr_nzeros5, nr_nzeros6, nr_nzeros7, nr_nzeros8, 
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);
    *hw_matrix5 = hw_matrix_alloc(hw_hdr.blocks, nr_rows5, nr_nzeros5, thres_l, thres_h);
    *hw_matrix6 = hw_matrix_alloc(hw_hdr.blocks, nr_rows6, nr_nzeros6, thres_l, thres_h);
    *hw_matrix7 = hw_matrix_alloc(hw_hdr.blocks, nr_rows7, nr_nzeros7, thres_l, thres_h);
    *hw_matrix8 = hw_matrix_alloc(hw_hdr.blocks, nr_rows8, nr_nzeros8, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i]+nr_rows2[i]+nr_rows3[i]+nr_rows4[i]+nr_rows5[i]+nr_rows6[i]+nr_rows7[i]+nr_rows8[i];
        IndexType block_nr_nzeros = nr_nzeros1[i]+nr_nzeros2[i]+nr_nzeros3[i]+nr_nzeros4[i]+nr_nzeros5[i]+nr_nzeros6[i]+nr_nzeros7[i]+nr_nzeros8[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << " " << nr_rows2[i] << " " << nr_rows3[i] << " " << nr_rows4[i] << " " <<  nr_rows5[i] << " " <<   nr_rows6[i] << " " <<   nr_rows7[i] << " " <<   nr_rows8[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << " " << nr_nzeros2[i] << " " << nr_nzeros3[i] << " " << nr_nzeros4[i] << " " <<  nr_nzeros5[i] << " " <<   nr_nzeros6[i] << " " <<   nr_nzeros7[i] << " " <<   nr_nzeros8[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->submatrix[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->submatrix[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->submatrix[i],
                                    (*hw_matrix5)->nr_rows[i], (*hw_matrix5)->submatrix[i],
                                    (*hw_matrix6)->nr_rows[i], (*hw_matrix6)->submatrix[i],
                                    (*hw_matrix7)->nr_rows[i], (*hw_matrix7)->submatrix[i],
                                    (*hw_matrix8)->nr_rows[i], (*hw_matrix8)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

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
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h); 
}


#elif CU == 10
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    IndexType *nr_rows9,    IndexType *nr_rows10,
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  IndexType *nr_nzeros9,  IndexType *nr_nzeros10,
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            /*            
                Double check if this is a proper line to break the representation between two or more CUs. 
                Firstly, check if the number of non-zeros for each CU is balanced.
                Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_v elements, so that a fully filled representation (at least for hw_values) is created
                Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for hw_y vectors)
                If all conditions success, then we found a breaking point!
            */            
                bool S1 = (curr_CU_nz>(block_nr_nzeros/ComputeUnits)), S2 = (curr_CU_nz%RATIO_v == 0), S3 = (row_cnt%RATIO_v==0);
                if( S1 && S2 && S3 ){ 
                    if(CU_examined == 1){
                        nr_rows1[i] = row_cnt;
                        nr_nzeros1[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 2){
                        nr_rows2[i] = row_cnt;
                        nr_nzeros2[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 3){
                        nr_rows3[i] = row_cnt;
                        nr_nzeros3[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 4){
                        nr_rows4[i] = row_cnt;
                        nr_nzeros4[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 5){
                        nr_rows5[i] = row_cnt;
                        nr_nzeros5[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 6){
                        nr_rows6[i] = row_cnt;
                        nr_nzeros6[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 7){
                        nr_rows7[i] = row_cnt;
                        nr_nzeros7[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 8){
                        nr_rows8[i] = row_cnt;
                        nr_nzeros8[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 9){
                        nr_rows9[i] = row_cnt;
                        nr_nzeros9[i] = curr_CU_nz;
                    }
                    curr_CU_nz = 0;
                    row_cnt = 0;
                    CU_examined++;
                }
            }
        }
        /* Previous CUs are assigned with  multiple of RATIO_v number of rows and number of nonzeros. */
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows10[i] = row_cnt;
        nr_nzeros10[i] = curr_CU_nz;
    }
}

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
                                 IndexType nr_rows10, BusDataType *hw_submatrix10){
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_submatrix2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_submatrix3);
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_submatrix4);
    offset += nr_rows4;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows5, &v_cnt, offset, hw_submatrix5);
    offset += nr_rows5;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows6, &v_cnt, offset, hw_submatrix6);
    offset += nr_rows6;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows7, &v_cnt, offset, hw_submatrix7);
    offset += nr_rows7;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows8, &v_cnt, offset, hw_submatrix8);
    offset += nr_rows8;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows9, &v_cnt, offset, hw_submatrix9);
    offset += nr_rows9;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows10, &v_cnt, offset, hw_submatrix10);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,   csr_hw_matrix **hw_matrix9,   csr_hw_matrix **hw_matrix10,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2,      *nr_rows3,      *nr_rows4,      *nr_rows5,    *nr_rows6,      *nr_rows7,      *nr_rows8,      *nr_rows9,      *nr_rows10;
    IndexType *nr_nzeros1,  *nr_nzeros2,    *nr_nzeros3,    *nr_nzeros4,    *nr_nzeros5,  *nr_nzeros6,    *nr_nzeros7,    *nr_nzeros8,    *nr_nzeros9,    *nr_nzeros10;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows9 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows10 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros9 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros10 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   nr_rows3,   nr_rows4,   nr_rows5,   nr_rows6,   nr_rows7,   nr_rows8,   nr_rows9,   nr_rows10,
                               nr_nzeros1, nr_nzeros2, nr_nzeros3, nr_nzeros4, nr_nzeros5, nr_nzeros6, nr_nzeros7, nr_nzeros8, nr_nzeros9, nr_nzeros10,
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);
    *hw_matrix5 = hw_matrix_alloc(hw_hdr.blocks, nr_rows5, nr_nzeros5, thres_l, thres_h);
    *hw_matrix6 = hw_matrix_alloc(hw_hdr.blocks, nr_rows6, nr_nzeros6, thres_l, thres_h);
    *hw_matrix7 = hw_matrix_alloc(hw_hdr.blocks, nr_rows7, nr_nzeros7, thres_l, thres_h);
    *hw_matrix8 = hw_matrix_alloc(hw_hdr.blocks, nr_rows8, nr_nzeros8, thres_l, thres_h);
    *hw_matrix9 = hw_matrix_alloc(hw_hdr.blocks, nr_rows9, nr_nzeros9, thres_l, thres_h);
    *hw_matrix10 = hw_matrix_alloc(hw_hdr.blocks, nr_rows10, nr_nzeros10, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i]+nr_rows2[i]+nr_rows3[i]+nr_rows4[i]+nr_rows5[i]+nr_rows6[i]+nr_rows7[i]+nr_rows8[i]+nr_rows9[i]+nr_rows10[i];
        IndexType block_nr_nzeros = nr_nzeros1[i]+nr_nzeros2[i]+nr_nzeros3[i]+nr_nzeros4[i]+nr_nzeros5[i]+nr_nzeros6[i]+nr_nzeros7[i]+nr_nzeros8[i]+nr_nzeros9[i]+nr_nzeros10[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << " " << nr_rows2[i] << " " << nr_rows3[i] << " " << nr_rows4[i] << " " <<  nr_rows5[i] << " " <<   nr_rows6[i] << " " <<   nr_rows7[i] << " " <<   nr_rows8[i] << " " <<   nr_rows9[i] << " " <<   nr_rows10[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << " " << nr_nzeros2[i] << " " << nr_nzeros3[i] << " " << nr_nzeros4[i] << " " <<  nr_nzeros5[i] << " " <<   nr_nzeros6[i] << " " <<   nr_nzeros7[i] << " " <<   nr_nzeros8[i] << " " <<   nr_nzeros9[i] << " " <<   nr_nzeros10[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->submatrix[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->submatrix[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->submatrix[i],
                                    (*hw_matrix5)->nr_rows[i], (*hw_matrix5)->submatrix[i],
                                    (*hw_matrix6)->nr_rows[i], (*hw_matrix6)->submatrix[i],
                                    (*hw_matrix7)->nr_rows[i], (*hw_matrix7)->submatrix[i],
                                    (*hw_matrix8)->nr_rows[i], (*hw_matrix8)->submatrix[i],
                                    (*hw_matrix9)->nr_rows[i], (*hw_matrix9)->submatrix[i],
                                    (*hw_matrix10)->nr_rows[i], (*hw_matrix10)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

    free(nr_rows1);
    free(nr_rows2);
    free(nr_rows3);
    free(nr_rows4);
    free(nr_rows5);
    free(nr_rows6);
    free(nr_rows7);
    free(nr_rows8);
    free(nr_rows9);
    free(nr_rows10);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(nr_nzeros3);
    free(nr_nzeros4);
    free(nr_nzeros5);
    free(nr_nzeros6);
    free(nr_nzeros7);
    free(nr_nzeros8);
    free(nr_nzeros9);
    free(nr_nzeros10);
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h); 
}



#elif CU == 12
void prepare_balanced_hw_matrix(IndexType *nr_rows1,    IndexType *nr_rows2,    IndexType *nr_rows3,    IndexType *nr_rows4,    IndexType *nr_rows5,    IndexType *nr_rows6,    IndexType *nr_rows7,    IndexType *nr_rows8,    IndexType *nr_rows9,    IndexType *nr_rows10,    IndexType *nr_rows11,    IndexType *nr_rows12,
                                IndexType *nr_nzeros1,  IndexType *nr_nzeros2,  IndexType *nr_nzeros3,  IndexType *nr_nzeros4,  IndexType *nr_nzeros5,  IndexType *nr_nzeros6,  IndexType *nr_nzeros7,  IndexType *nr_nzeros8,  IndexType *nr_nzeros9,  IndexType *nr_nzeros10,  IndexType *nr_nzeros11,  IndexType *nr_nzeros12,
                                IndexType **block_row_ptr, csr_hw_header hw_hdr, bool **empty_rows_bitmap)
{
    for(int i=0; i<hw_hdr.blocks; i++){
        /* Calculate number of rows and values that will be assigned to each Compute Unit trying to balance the load between them */
        IndexType block_nr_nzeros = block_row_ptr[i][hw_hdr.expanded_nr_rows];
        IndexType curr_CU_nz = 0;
        IndexType row_cnt = 0;
        int CU_examined = 1;
        for(IndexType j=0; j<hw_hdr.expanded_nr_rows; j++){
            IndexType curr_row_length = block_row_ptr[i][j+1] - block_row_ptr[i][j];

            if(curr_row_length == 0){ /* For this block, this row has no nonzeros. Keep it on bitmap for later use (correct store of (partial) y_hw on y_fpga*/
                empty_rows_bitmap[i][j] = 1;
            }
            else{
                empty_rows_bitmap[i][j] = 0;
                curr_CU_nz += curr_row_length;
                row_cnt++;
            /*            
                Double check if this is a proper line to break the representation between two or more CUs. 
                Firstly, check if the number of non-zeros for each CU is balanced.
                Secondly, we need each representation vector of non-zeros to have a multiple of RATIO_v elements, so that a fully filled representation (at least for hw_values) is created
                Thirdly, due to the representation width (affected by RATIO_v), we need each CU to be assigned with a multiple of RATIO_v rows. (for hw_y vectors)
                If all conditions success, then we found a breaking point!
            */            
                bool S1 = (curr_CU_nz>(block_nr_nzeros/ComputeUnits)), S2 = (curr_CU_nz%RATIO_v == 0), S3 = (row_cnt%RATIO_v==0);
                if( S1 && S2 && S3 ){ 
                    if(CU_examined == 1){
                        nr_rows1[i] = row_cnt;
                        nr_nzeros1[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 2){
                        nr_rows2[i] = row_cnt;
                        nr_nzeros2[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 3){
                        nr_rows3[i] = row_cnt;
                        nr_nzeros3[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 4){
                        nr_rows4[i] = row_cnt;
                        nr_nzeros4[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 5){
                        nr_rows5[i] = row_cnt;
                        nr_nzeros5[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 6){
                        nr_rows6[i] = row_cnt;
                        nr_nzeros6[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 7){
                        nr_rows7[i] = row_cnt;
                        nr_nzeros7[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 8){
                        nr_rows8[i] = row_cnt;
                        nr_nzeros8[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 9){
                        nr_rows9[i] = row_cnt;
                        nr_nzeros9[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 10){
                        nr_rows10[i] = row_cnt;
                        nr_nzeros10[i] = curr_CU_nz;
                    }
                    else if(CU_examined == 11){
                        nr_rows11[i] = row_cnt;
                        nr_nzeros11[i] = curr_CU_nz;
                    }
                    curr_CU_nz = 0;
                    row_cnt = 0;
                    CU_examined++;
                }
            }
        }
        /* Previous CUs are assigned with  multiple of RATIO_v number of rows and number of nonzeros. */
        /* Perhaps now, the number of rows (not non-zeros, since they are safe by applying VectFactor) will not be exact multiple of RATIO_v. */
        /* This is why we need to address it here right now, not later. Apply padding to nr_rows, and appropriate padding to non-zeros (unfortunately, VectFactor 0s will be added)*/
        IndexType N = RATIO_v;
        IndexType rows_mod_N = row_cnt % N;
        if(rows_mod_N != 0){
            row_cnt += N - rows_mod_N;
            curr_CU_nz += (N - rows_mod_N)*VectFactor;
        }

        nr_rows12[i] = row_cnt;
        nr_nzeros12[i] = curr_CU_nz;
    }
}

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
                                 IndexType nr_rows12, BusDataType *hw_submatrix12){
    IndexType offset = 1, v_cnt = 0;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows1, &v_cnt, offset, hw_submatrix1);
    offset += nr_rows1;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows2, &v_cnt, offset, hw_submatrix2);
    offset += nr_rows2;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows3, &v_cnt, offset, hw_submatrix3);
    offset += nr_rows3;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows4, &v_cnt, offset, hw_submatrix4);
    offset += nr_rows4;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows5, &v_cnt, offset, hw_submatrix5);
    offset += nr_rows5;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows6, &v_cnt, offset, hw_submatrix6);
    offset += nr_rows6;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows7, &v_cnt, offset, hw_submatrix7);
    offset += nr_rows7;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows8, &v_cnt, offset, hw_submatrix8);
    offset += nr_rows8;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows9, &v_cnt, offset, hw_submatrix9);
    offset += nr_rows9;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows10, &v_cnt, offset, hw_submatrix10);
    offset += nr_rows10;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows11, &v_cnt, offset, hw_submatrix11);
    offset += nr_rows11;
    generate_balanced_hw_submatrix(row_ptr, col_ind, values, nr_rows12, &v_cnt, offset, hw_submatrix12);
}

void create_csr_hw_matrix(csr_matrix *matrix,
                          csr_hw_matrix **hw_matrix1,   csr_hw_matrix **hw_matrix2,   csr_hw_matrix **hw_matrix3,   csr_hw_matrix **hw_matrix4,   csr_hw_matrix **hw_matrix5,   csr_hw_matrix **hw_matrix6,   csr_hw_matrix **hw_matrix7,   csr_hw_matrix **hw_matrix8,   csr_hw_matrix **hw_matrix9,   csr_hw_matrix **hw_matrix10,   csr_hw_matrix **hw_matrix11,   csr_hw_matrix **hw_matrix12,
                          bool ***empty_rows_bitmap)
{
    csr_hw_header hw_hdr;
    IndexType *thres_l, *thres_h;
    IndexType **block_row_ptr;
    scan_matrix(matrix, &thres_l, &thres_h, &block_row_ptr, &hw_hdr);

    IndexType *nr_rows1,    *nr_rows2,      *nr_rows3,      *nr_rows4,      *nr_rows5,    *nr_rows6,      *nr_rows7,      *nr_rows8,      *nr_rows9,      *nr_rows10,      *nr_rows11,      *nr_rows12;
    IndexType *nr_nzeros1,  *nr_nzeros2,    *nr_nzeros3,    *nr_nzeros4,    *nr_nzeros5,  *nr_nzeros6,    *nr_nzeros7,    *nr_nzeros8,    *nr_nzeros9,    *nr_nzeros10,    *nr_nzeros11,    *nr_nzeros12;
    nr_rows1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows9 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows10 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows11 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_rows12 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros1 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros2 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros3 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros4 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros5 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros6 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros7 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros8 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros9 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros10 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros11 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));
    nr_nzeros12 = (IndexType*)malloc(hw_hdr.blocks*sizeof(IndexType));

    *empty_rows_bitmap = (bool**)malloc(hw_hdr.blocks*sizeof(bool*));
    for(int i=0; i<hw_hdr.blocks; i++)
        (*empty_rows_bitmap)[i] = (bool*)malloc(hw_hdr.expanded_nr_rows*sizeof(bool));
    
    prepare_balanced_hw_matrix(nr_rows1,   nr_rows2,   nr_rows3,   nr_rows4,   nr_rows5,   nr_rows6,   nr_rows7,   nr_rows8,   nr_rows9,   nr_rows10,   nr_rows11,   nr_rows12,
                               nr_nzeros1, nr_nzeros2, nr_nzeros3, nr_nzeros4, nr_nzeros5, nr_nzeros6, nr_nzeros7, nr_nzeros8, nr_nzeros9, nr_nzeros10, nr_nzeros11, nr_nzeros12,
                               block_row_ptr, hw_hdr, *empty_rows_bitmap);

    *hw_matrix1 = hw_matrix_alloc(hw_hdr.blocks, nr_rows1, nr_nzeros1, thres_l, thres_h);
    *hw_matrix2 = hw_matrix_alloc(hw_hdr.blocks, nr_rows2, nr_nzeros2, thres_l, thres_h);
    *hw_matrix3 = hw_matrix_alloc(hw_hdr.blocks, nr_rows3, nr_nzeros3, thres_l, thres_h);
    *hw_matrix4 = hw_matrix_alloc(hw_hdr.blocks, nr_rows4, nr_nzeros4, thres_l, thres_h);
    *hw_matrix5 = hw_matrix_alloc(hw_hdr.blocks, nr_rows5, nr_nzeros5, thres_l, thres_h);
    *hw_matrix6 = hw_matrix_alloc(hw_hdr.blocks, nr_rows6, nr_nzeros6, thres_l, thres_h);
    *hw_matrix7 = hw_matrix_alloc(hw_hdr.blocks, nr_rows7, nr_nzeros7, thres_l, thres_h);
    *hw_matrix8 = hw_matrix_alloc(hw_hdr.blocks, nr_rows8, nr_nzeros8, thres_l, thres_h);
    *hw_matrix9 = hw_matrix_alloc(hw_hdr.blocks, nr_rows9, nr_nzeros9, thres_l, thres_h);
    *hw_matrix10 = hw_matrix_alloc(hw_hdr.blocks, nr_rows10, nr_nzeros10, thres_l, thres_h);
    *hw_matrix11 = hw_matrix_alloc(hw_hdr.blocks, nr_rows11, nr_nzeros11, thres_l, thres_h);
    *hw_matrix12 = hw_matrix_alloc(hw_hdr.blocks, nr_rows12, nr_nzeros12, thres_l, thres_h);

    IndexType tot_nonzeros = 0;
    for(int i=0; i<hw_hdr.blocks; i++){
        csr_matrix *block_matrix;
        IndexType block_nr_rows = nr_rows1[i]+nr_rows2[i]+nr_rows3[i]+nr_rows4[i]+nr_rows5[i]+nr_rows6[i]+nr_rows7[i]+nr_rows8[i]+nr_rows9[i]+nr_rows10[i]+nr_rows11[i]+nr_rows12[i];
        IndexType block_nr_nzeros = nr_nzeros1[i]+nr_nzeros2[i]+nr_nzeros3[i]+nr_nzeros4[i]+nr_nzeros5[i]+nr_nzeros6[i]+nr_nzeros7[i]+nr_nzeros8[i]+nr_nzeros9[i]+nr_nzeros10[i]+nr_nzeros11[i]+nr_nzeros12[i];
        tot_nonzeros += block_nr_nzeros;

        // std::cout << "\nBLOCK " << i << " : " << block_nr_rows << " non-empty rows, " << block_nr_nzeros << " non-zeros\n";
        // std::cout << "nr_rows   : " << nr_rows1[i] << " " << nr_rows2[i] << " " << nr_rows3[i] << " " << nr_rows4[i] << " " <<  nr_rows5[i] << " " <<   nr_rows6[i] << " " <<   nr_rows7[i] << " " <<   nr_rows8[i] << " " <<   nr_rows9[i] << " " <<   nr_rows10[i] << " " <<   nr_rows11[i] << " " <<   nr_rows12[i] << "\n";
        // std::cout << "nr_nzeros : " << nr_nzeros1[i] << " " << nr_nzeros2[i] << " " << nr_nzeros3[i] << " " << nr_nzeros4[i] << " " <<  nr_nzeros5[i] << " " <<   nr_nzeros6[i] << " " <<   nr_nzeros7[i] << " " <<   nr_nzeros8[i] << " " <<   nr_nzeros9[i] << " " <<   nr_nzeros10[i] << " " <<   nr_nzeros11[i] << " " <<   nr_nzeros12[i] << "\n";

        block_matrix = create_block_matrix(matrix, block_row_ptr[i], block_nr_rows, block_nr_nzeros, thres_l[i], thres_h[i]);
        
        generate_balanced_hw_matrix(block_matrix->row_ptr, block_matrix->col_ind, block_matrix->values,
                                    (*hw_matrix1)->nr_rows[i], (*hw_matrix1)->submatrix[i],
                                    (*hw_matrix2)->nr_rows[i], (*hw_matrix2)->submatrix[i],
                                    (*hw_matrix3)->nr_rows[i], (*hw_matrix3)->submatrix[i],
                                    (*hw_matrix4)->nr_rows[i], (*hw_matrix4)->submatrix[i],
                                    (*hw_matrix5)->nr_rows[i], (*hw_matrix5)->submatrix[i],
                                    (*hw_matrix6)->nr_rows[i], (*hw_matrix6)->submatrix[i],
                                    (*hw_matrix7)->nr_rows[i], (*hw_matrix7)->submatrix[i],
                                    (*hw_matrix8)->nr_rows[i], (*hw_matrix8)->submatrix[i],
                                    (*hw_matrix9)->nr_rows[i], (*hw_matrix9)->submatrix[i],
                                    (*hw_matrix10)->nr_rows[i], (*hw_matrix10)->submatrix[i],
                                    (*hw_matrix11)->nr_rows[i], (*hw_matrix11)->submatrix[i],
                                    (*hw_matrix12)->nr_rows[i], (*hw_matrix12)->submatrix[i]);

        delete_csr_matrix(block_matrix);
    }

    ValueType input = tot_nonzeros*(VALUE_TYPE_BIT_WIDTH + COMPRESSED_INDEX_TYPE_BIT_WIDTH)/(8.0*1024*1024), output = hw_hdr.blocks*hw_hdr.expanded_nr_rows*VALUE_TYPE_BIT_WIDTH/(8.0*1024*1024);
    std::cout << "Total non-zeros : " << tot_nonzeros << ". Total " << input+output << " MB transferred ( in : " <<  input << ", out : " << output << ")\n";

    free(nr_rows1);
    free(nr_rows2);
    free(nr_rows3);
    free(nr_rows4);
    free(nr_rows5);
    free(nr_rows6);
    free(nr_rows7);
    free(nr_rows8);
    free(nr_rows9);
    free(nr_rows10);
    free(nr_rows11);
    free(nr_rows12);
    free(nr_nzeros1);
    free(nr_nzeros2);
    free(nr_nzeros3);
    free(nr_nzeros4);
    free(nr_nzeros5);
    free(nr_nzeros6);
    free(nr_nzeros7);
    free(nr_nzeros8);
    free(nr_nzeros9);
    free(nr_nzeros10);
    free(nr_nzeros11);
    free(nr_nzeros12);
    for(int i=0; i<hw_hdr.blocks; i++)
        free(block_row_ptr[i]);
    free(thres_l);
    free(thres_h); 
}
#endif

/*
    Return size (in MB) of hw_matrix representation for each Compute Unit separately
*/
ValueType storage_overhead(csr_hw_matrix *matrix)
{
    IndexType mem = 0;
    mem += (matrix->blocks)*(INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH+INDEX_TYPE_BIT_WIDTH); // nr_rows, nr_columns, nr_nzeros, nr_ci, nr_val
    for(int i=0; i<matrix->blocks; i++)
        mem += (matrix->nr_ci[i]+matrix->nr_val[i])*BUS_BIT_WIDTH; // submatrix
    /* std::cout << mem/(8.0*1024*1024) << " MB\n"; */
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

    free((*matrix)->nr_ci);
    free((*matrix)->nr_val);

    for(int i=0; i<(*matrix)->blocks; i++)
        sds_free((*matrix)->submatrix[i]);
    free((*matrix)->submatrix);
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
            for(int k=0; k<RATIO_ci; k++){
                tmp = tmp_wide.range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(k+1)-2,COMPRESSED_INDEX_TYPE_BIT_WIDTH*k);
                tmp2 = tmp_wide.range(COMPRESSED_INDEX_TYPE_BIT_WIDTH*(k+1)-1,COMPRESSED_INDEX_TYPE_BIT_WIDTH*(k+1)-1);
                std::cout << "| ("<< COMPRESSED_INDEX_TYPE_BIT_WIDTH*(k+1)-1 << " : " << COMPRESSED_INDEX_TYPE_BIT_WIDTH*k << ") = " << tmp << " <"<< tmp2 << ">\t";
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
    New addition here is the empty_rows_bitmap, so that results from y_hw are written to proper positions.
*/

void accum_results(BusDataType *values, IndexType nr_values, ValueType *hw_y, IndexType nr_rows, IndexType *offset, bool *empty_rows_bitmap)
{
    IndexType non_empty_cnt = 0;
    IndexType i=0;
    // while(non_empty_cnt<nr_values){
    //     std::cout << " " << empty_rows_bitmap[i];
    //     if(empty_rows_bitmap[i] == 0)
    //         non_empty_cnt++;
    //     i++;
    // }
    // *offset += i;

    for(IndexType j=0; j<nr_values/RATIO_v; j++){
        Union_double_uint tmp;
        BusDataType tmp_wide;
        tmp_wide = values[j];
        for(int k=0; k<RATIO_v; k++){
            tmp.apint = tmp_wide.range(VALUE_TYPE_BIT_WIDTH*(k+1)-1,VALUE_TYPE_BIT_WIDTH*k);
            if( (i+(*offset)) < nr_rows ){
                while(empty_rows_bitmap[i] != 0){
                    // std::cout << " I GRAMMI " << i << " EINAI KENI\n";
                    i++;
                }
                non_empty_cnt++;
                hw_y[i+(*offset)] += tmp.f;
                // std::cout << "\t\tEGRAPSA TO " << tmp.f << " STI 8esh " << i+(*offset) <<"\n";
                i++;
            }
            else
                break;
        }
    }
    *offset += i;
    // std::cout << "\nTELOS non_empty_cnt = " << non_empty_cnt << ". offset = " << *offset << " (EIDA " << i << " grammes)\n";
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
            std::cout << std::setprecision(14) << i << " : y_gold = " << sw_values[i] << "\ty_hw = " << hw_values[i] << "\n";
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
