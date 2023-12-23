#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "data.h"
#include <omp.h>
#include "timer.h"

int main(int argc, char **argv)
{
    if(argc != 4){
        printf("Usage: <num_thread> <vec_a> <vec_b>.\n");
        exit(EXIT_FAILURE);
    }

    // int numThreads = omp_get_max_threads();
    // convinient for use in partitioning
    int numThreads = atoi(argv[1]);
    omp_set_num_threads(numThreads);
        
    struct timespec start;
    start_timer(&start);

    data_struct *d_1 = get_data_struct(argv[2]);
    data_struct *d_2 = get_data_struct(argv[3]);

    stop_timer(&start);
    fprintf(stderr, " (reading input)\n");

    if(d_1->cols != d_2->cols || d_1->rows != d_2->rows){
        printf("ERROR: Matrix dimension mismatch.\n");
        printf("d1 rows: %d d1 cols: %d", d_1->rows, d_1->cols);
        printf("d2 rows: %d d2 cols: %d", d_2->rows, d_2->cols);
        exit(EXIT_FAILURE);
    }

    // Allocating space for the result matrix/vector
    data_struct *d_3 = malloc(sizeof(data_struct));
    d_3->rows = d_1->rows;
    d_3->cols = d_1->cols;

    d_3->data_point = calloc(d_3->rows, sizeof(double*)); 
    for(int i = 0; i < d_3->rows; i++){
        d_3->data_point[i]=calloc(d_3->cols, sizeof(double));
    }

    start_timer(&start);

    #pragma omp parallel for default(shared)
    for(int i = 0; i<d_3->rows; i++){
        for(int j = 0; j<d_3->cols; j++){
            #pragma omp critical
            {
                d_3->data_point[i][j] = d_1->data_point[i][j] + d_2->data_point[i][j];
            } //critical
        }
    }

    stop_timer(&start);
    fprintf(stderr, " (calculating answer)\n");

    start_timer(&start);
    print_data(d_3);
    stop_timer(&start);
    fprintf(stderr, " (printing output)\n");
}
