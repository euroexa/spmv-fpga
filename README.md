# SpMV on FPGA

### How-to build
Specify the following parameters when building :

| Option | Description |
|:------ |:----------- |
| TARGET | Compile for hardware (hw) or emulation (emu) (default : emu) |
| CU     | Number of Compute Units instantiated, that work will be distributed to (default : 1) |
| VF     | Vectorization-factor applied to computations (Unroll factor on computations kernel) (default : 1) |
| DOUBLE | Whether single-(0) or double-precision(1) arithmetic will be used (default : 1) |

Valid inputs for CU and VF are 1, 2, 4, 8

#### Example make usage 

```
make TARGET=hw CU=8 VF=4 DOUBLE=1
```

Note : Code is targeted for Xilinx ZCU102 platform only. Building for different platform requires modification of [Makefile](./Makefile) and [util.h](./src/util.h) files, so as to consider board-specific constraints.

### How-to run

```
./run.elf <matrix-file>
```

Note : Input matrix values have to be sorted row-wise.

---

## Code
Functions defined in [csr_hw_wrapper.cpp](./src/csr_hw_wrapper.cpp) are to be used by the programmer :

| Function | Description |
|:-------- |:----------- |
| ``` create_csr_hw_matrix(matrix, hw_matrix) ```  | Given a matrix in CSR representation format ( ***matrix*** ), return the matrix in FPGA-optimized representation ( ***hw_matrix*** ). ***hw_matrix*** is an array of *csr_hw_matrix* type. Each element of it corresponds to a different Compute Unit. Separate representation is created for each Compute Unit.|
| ``` create_csr_hw_x_vector(hw_x, x, blocks, nr_cols) ``` | Since the matrix is split in blocks, only a part of x vector is transferred to the FPGA for each block. Fill ***hw_x*** with the values of ***x*** that was previously created.|
| ``` create_csr_hw_y_vector(hw_matrix, hw_vector) ``` | For each Compute Unit and for each block that the matrix will be split to, create intermediate-results y vectors.|
| ``` spmv_hw(hw_matrix, hw_x, y_fpga) ``` | Create intermediate-results y vectors for each Compute Unit and for each block. Then, call the hardware function *spmv* that runs on FPGA, calculating the result of multiplying ***hw_matrix*** with ***hw_x*** . Finally, accumulate the results of the intermediate y vectors, storing them in ***y_fpga*** , that is returned to main function.|
| ``` delete_csr_hw_matrix(hw_matrix) ``` | Free allocated memory for hw_matrix|
| ``` delete_csr_hw_x_vector(hw_vector) ``` | Free allocated memory for (hw) x vector |
| ``` delete_csr_hw_y_vector(hw_vector) ``` | Free allocated memory for (hw) y vector |

Note : Intermediate-results hw y vector is allocated and freed in **spmv_hw** function, therefore functions **create_csr_hw_y_vector** and **delete_csr_hw_y_vector** are not used in 'main' function.

---

## Images

A Sparse matrix :
<p align="center"><img src="./images/1_matrix.svg" width="250"/></p>

CSR Representation format :
<p align="center"><img src="./images/2_csr.svg" width="350"/></p>

HW Representation format :
<p align="center"><img src="./images/3_hw_representation.svg" width="1000"/></p>

For each Compute Unit, two structs are used for representation of the sparse matrix; **col_idx** and **values**. These structs are 128-bits wide, in order to fully utilize the available memory bandwidth of HP ports of Xilinx ZCU102.

- **col_idx** : For each non-zero element, the first 31 bits are used to encode its column index. Since we apply 2D Blocking to the sparse matrix, 31 bits are enough to represent the column index of each non-zero. The last bit indicates whether the current non-zero is the last element of its row (1 when last, 0 otherwise). We need to know when the last element of a row is being processed, so as to forward the result to the results-stream in hardware function.

- **values** : Contains the non-zero values of the matrix, in double- (or single-) precision. 


---

## HW Functions

Functions defined in [spmv.cpp](./src/spmv.cpp). 

For each Compute Unit, these functions are instantiated, and perform in Dataflow model.

| Function | Description |
|:-------- |:----------- |
| ``` read_data_col_ind(nr_nzeros, col_ind, col_fifo) ``` | Read column-indices from ***col_ind*** struct, that is sent from host to FPGA, and push them to *hls::stream* ***col_fifo*** . |
| ``` read_data_values(nr_nzeros, values, values_fifo) ``` | Read values from ***values*** struct, that is sent from host to FPGA, and push them to *hls::stream* ***values_fifo*** . |
| ``` compute_results(col_fifo, values_fifo, nr_nzeros, x, results_fifo) ``` | Pop data from ***col_fifo*** and ***values_fifo*** and perform computations, with the respective elements of ***x*** . Push results for each row to *hls::stream* ***results_fifo*** . |
| ``` write_back_results(results_fifo, nr_rows, y) ``` | Pop results from results_fifo and write them to y vector, that will be sent from FPGA to host. |

Different **spmv** and **spmv_kernel** functions are used for different number of Compute Units. Here, we present the case for 2 Compute Units.

``` c
spmv(
    nr_rows1,   nr_rows2,
    nr_nzeros1, nr_nzeros2,
    col_ind1,   col_ind2,
    values1,    values2,
    y1,         y2,
    x,  nr_cols)
```
**spmv** : This is the function that is being called from host side. For each Compute Unit, the number of rows and number of non-zeros that are processed are given, since these are the sizes of the y vector and col_ind,values structs respectively. In addition, a separate copy of x vector (of size nr_cols) is created for each Compute Unit, since data-sharing between Compute Units is not allowed in HLS. Finally, the **spmv_kernel** function is called.

``` c 
spmv_kernel(
    nr_rows1,   nr_rows2,
    nr_nzeros1, nr_nzeros2,
    col_ind1,   col_ind2,
    values1,    values2,
    y1,         y2,
    x1,         x2)
```

**spmv_kernel** : In this function, instances of the previously presented functions are created for each Compute Unit. These function calls are independent between Compute Units and operate in parallel.
