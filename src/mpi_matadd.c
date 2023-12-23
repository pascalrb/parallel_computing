#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "data.h"
#include <timer.h>

//#include "assert.h"
//#include <stdbool.h>


// if flag = 0; NxM   <-- d.MxN  
// if flag = 1; d.MxN <-- NxM
//void transpose_mat(data_struct * d, double * d_transposed, bool is_transposed){
//
//
//    if (!is_transposed){        // 1xN <-- d.Nx1 
//
//        for(int i = 0; i < d->rows; i++){
//            for(int j = 0; j < d->cols; j++){
//                d_transposed[i] = d->data_point[i][j];
//            }
//        }
//
//    }else{                      // d.Nx1 <-- 1xN
//
//        for(int i = 0; i < d->rows; i++){
//            for(int j = 0; j < d->cols; j++){
//                d->data_point[i][j] = d_transposed[i];
//            }
//        }
//    }
//}


int main(int argc, char *argv[]) {

    if(argc != 3){
        printf("ERROR: Please specify only 2 files.\n");
        exit(EXIT_FAILURE);
    }

    int rank, ranks_amnt;
    struct timespec start;
    data_struct *d_1;
    data_struct *d_2;

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
        if(d_1->cols != d_2->cols || d_1->rows != d_2->rows){
            printf("ERROR: Matrix dimension mismatch.\n");
            printf("d1 rows: %d d1 cols: %d", d_1->rows, d_1->cols);
            printf("d2 rows: %d d2 cols: %d", d_2->rows, d_2->cols);
            exit(EXIT_FAILURE);
        }


        // Setting process global variables
        rows = d_1->rows;
        cols = d_1->rows;


        // ---------- Timing the calculation of result matrix/vector
        start_timer(&start);
    }

    // Broadcast total num of elements
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);


    // TODO: NOTE: Not covering the case where number of elments is not divisible by number of processes
    //int elem_per_proc = rows/ranks_amnt; 
    int rows_per_proc = rows/ranks_amnt; 


    double *d_1_subsection = malloc(rows_per_proc * sizeof(double*));
    double *d_2_subsection = malloc(rows_per_proc * sizeof(double*));


    for(int i = 0; i < rows; i++){

        for(int j = 0; j < rows_per_proc; j++){

            // rows % ranks

            if(rank == 0){
                MPI_Send(d_1->data_point[i+j], cols, MPI_DOUBLE, i+j, 0, MPI_COMM_WORLD);
                MPI_Send(d_2->data_point[i+j], cols, MPI_DOUBLE, i+j, 0, MPI_COMM_WORLD);
            }else{

                d_1_subsection[j] = malloc(cols * sizeof(double));
                d_2_subsection[j] = malloc(cols * sizeof(double));

                MPI_Recv(d_1_subsection[j], cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
                MPI_Recv(d_2_subsection[j], cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

            }

            //MPI_Bcast(d_1->data_point[i+j], cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            //MPI_Bcast(d_2->data_point[i+j], cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            //MPI_Scatter(d_1->data_point[i+j], cols, MPI_DOUBLE, 
            //                d_1_subsection[j], cols, MPI_DOUBLE, 
            //                0, MPI_COMM_WORLD);
            //
            //MPI_Scatter(d_2->data_point, cols, MPI_DOUBLE, 
            //                d_2_subsection[j], cols, MPI_DOUBLE, 
            //                0, MPI_COMM_WORLD);
        }
    }

    double *d_3_subsection = malloc(rows_per_proc * sizeof(double*));
    d_3_subsection[j] = malloc(cols * sizeof(double));

    for(int i = 0; i < rows_per_proc; i++){
        for(int j = 0; j < cols; j++){
            d_3_subsection[i][j] = d_1_subsection[i][j] + d_2_subsection[i][j];
        }
    }

    
    
    data_struct *d_3 = malloc(sizeof(data_struct));
    d_3->rows = rows;
    d_3->cols = cols;
    d_3->data_point = calloc(d_3->rows, sizeof(double*)); 

    for(int i = 0; i < d_3->rows; i++){
        d_3->data_point[i]=calloc(d_3->cols, sizeof(double));

        for(int j = 0; j < rows_per_proc; j++){
            d_3->data_point[i+j][j] = d_1_subsection[i][j] + d_2_subsection[i][j];
        }
    }


    if(rank == 0){
        // -------- Stop timer 
        stop_timer(&start);
        fprintf(stderr, " (calculating answer)\n");


        // Allocating space for the result matrix/vector
        transpose_vect(d_3, d_3_transposed, 1);

        /* Printing output */
        start_timer(&start);
        print_data(d_3);
        stop_timer(&start);
        fprintf(stderr, " (printing output)\n");
    }

    MPI_Finalize();
}
