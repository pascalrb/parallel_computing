#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include <omp.h>
#include <mpi.h>
#include "timer.h"

#include "assert.h"
#include <stdbool.h>

// if flag = 0; 1xN   <-- d.Nx1  
// if flag = 1; d.Nx1 <-- 1xN
void transpose_vect(data_struct * d, double * d_transposed, bool is_transposed){

     assert(d->cols == 1); // sanity checks because d_transposed is only allocated 1 double* pointer 

    if (!is_transposed){        // 1xN <-- d.Nx1 

        for(int i = 0; i < d->rows; i++){
            for(int j = 0; j < d->cols; j++){
                d_transposed[i] = d->data_point[i][j];
            }
        }

    }else{                      // d.Nx1 <-- 1xN

        for(int i = 0; i < d->rows; i++){
            for(int j = 0; j < d->cols; j++){
                d->data_point[i][j] = d_transposed[i];
            }
        }
    }
}


int main(int argc, char *argv[]) {

    if(argc != 3){
        printf("ERROR: Please specify only 2 files.\n");
        exit(EXIT_FAILURE);
    }

    int rank, ranks_amnt;
    int elem_num; 
    struct timespec start;
    data_struct *d_1;
    data_struct *d_2;
    double *d_1_transposed, *d_2_transposed, *d_3_transposed; 

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks_amnt);

    if(rank == 0){
        start_timer(&start);

        d_1 = get_data_struct(argv[1]);
        d_2 = get_data_struct(argv[2]);
        
        stop_timer(&start);
        fprintf(stderr, " (reading input)\n");
    

        //Vector is of size (nx1), it will always return coloumn =1
        if(d_1->cols != 1 || d_2->cols !=1){
            printf("ERROR: The dimension of vector is not correct\n");
            exit(EXIT_FAILURE);
        }
        //Vectors' row numbers have to match
        if(d_1->rows != d_2->rows){
            printf("ERROR: Vector dimensions mismatch.\n");
            exit(EXIT_FAILURE);
        }

        // Setting process global variables
        elem_num = d_1->rows;

        // Transpose into a 1xN vector
        d_1_transposed = malloc(elem_num * sizeof(double));
        d_2_transposed = malloc(elem_num * sizeof(double));
        transpose_vect(d_1, d_1_transposed, 0);
        transpose_vect(d_2, d_2_transposed, 0);

        d_3_transposed = malloc(elem_num * sizeof(double));

        // ---------- Timing the calculation of result matrix/vector
        start_timer(&start);
    }

    // Broadcast total num of elements
    MPI_Bcast(&elem_num, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // TODO: NOTE: Not covering the case where number of elments is not divisible by number of processes
    int elem_per_proc = elem_num/ranks_amnt;

    double *d_1_transposed_subsection = malloc(elem_per_proc * sizeof(double));
    double *d_2_transposed_subsection = malloc(elem_per_proc * sizeof(double));


    MPI_Scatter(d_1_transposed, elem_per_proc, MPI_DOUBLE, 
                    d_1_transposed_subsection, elem_per_proc, MPI_DOUBLE, 
                    0, MPI_COMM_WORLD);
    MPI_Scatter(d_2_transposed, elem_per_proc, MPI_DOUBLE, 
                    d_2_transposed_subsection, elem_per_proc, MPI_DOUBLE, 
                    0, MPI_COMM_WORLD);
    

    double *d_3_transposed_subsection = malloc(elem_per_proc * sizeof(double));

    #pragma omp parallel for default(shared)
    for(int i = 0; i < elem_per_proc; i++){
        #pragma omp critical 
        {
            d_3_transposed_subsection[i] = d_1_transposed_subsection[i] + d_2_transposed_subsection[i];
        }
    }

    MPI_Gather(d_3_transposed_subsection, elem_per_proc, MPI_DOUBLE, 
                    d_3_transposed, elem_per_proc, MPI_DOUBLE, 
                    0, MPI_COMM_WORLD);


    if(rank == 0){
        // -------- Stop timer 
        stop_timer(&start);
        fprintf(stderr, " (calculating answer)\n");


        // Allocating space for the result matrix/vector
        data_struct *d_3 = malloc(sizeof(data_struct));
        d_3->rows = d_1->rows;
        d_3->cols = d_1->cols;
        d_3->data_point = calloc(d_3->rows, sizeof(double*)); 
        for(int i = 0; i < d_3->rows; i++){
            d_3->data_point[i]=calloc(d_3->cols, sizeof(double));
        }
        transpose_vect(d_3, d_3_transposed, 1);

        /* Printing output */
        start_timer(&start);
        print_data(d_3);
        stop_timer(&start);
        fprintf(stderr, " (printing output)\n");
    }

    MPI_Finalize();
    
}
