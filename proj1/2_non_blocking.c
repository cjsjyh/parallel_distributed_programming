#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define PROBLEM_SIZE 1000

void synchronize(int , int , int* , int* , int );
void updateBuffer(int* , int , int* , int , int , int );
void printArray(int*);

int main(int argc, char *argv[]) {
  int numprocs, rank, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  double start, finish;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int x[PROBLEM_SIZE] = {0, };
  int y[PROBLEM_SIZE] = {0, };
  int n = PROBLEM_SIZE;

  if(rank == 0) start = MPI_Wtime();

  // Generate, send receive random numbers
  MPI_Datatype int_arr;
  MPI_Status status[2];
  MPI_Request reqs[2];
  if (rank == 0){
    MPI_Type_contiguous(n, MPI_INT, &int_arr);
    MPI_Type_commit(&int_arr);
    srand((unsigned int)time(NULL));
    for(int i=0; i<PROBLEM_SIZE; i++)
      x[i] = rand() % 100;

    for(int i=1; i<numprocs; i++){
      MPI_Isend(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[0]);
      MPI_Isend(x, 1, int_arr, i, 0, MPI_COMM_WORLD, &reqs[1]);
      MPI_Waitall(2, reqs, status);
    }
  }
  else {
    MPI_Irecv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Waitall(1, reqs, status);
    MPI_Type_contiguous(n, MPI_INT, &int_arr);
    MPI_Type_commit(&int_arr);
    MPI_Irecv(x, 1, int_arr, 0, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Waitall(1, reqs, status);
  }
  
  int startIndex, endIndex;
  startIndex = n/numprocs * rank;
  endIndex = startIndex + n/numprocs;
  if (rank == numprocs - 1)
    endIndex = n;

  for(int i=0; i<ceil(log2(n)); i++){
    // Initialize Y with X
    for(int k=0; k<n; k++) y[k] = x[k];

    for(int j=startIndex; j<endIndex; j++){
      if(j >= pow(2, i))
        y[j] = x[j] + x[j-(int)pow(2,i)];
    }

    // Move Y to X
    for(int k=0; k<n; k++) x[k] = y[k];

    synchronize(rank, numprocs, x, y, n);

    MPI_Barrier(MPI_COMM_WORLD);
  }

  if(rank == 0){
    //printArray(x);
    finish = MPI_Wtime();
    printf("[Non-Block] Elapsed time: %e seconds\n", finish-start);
  }

  MPI_Finalize();
}

void synchronize(int rank, int numprocs, int* sendbuf, int* recvbuf, int n){
  MPI_Status status[2];
  MPI_Request reqs[2];

  for(int i=0; i<numprocs; i++){
    if (rank == i) continue;
    if(i < rank){
      MPI_Irecv(recvbuf, n, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[0]);
      MPI_Isend(sendbuf, n, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[1]);
    }
    else{
      MPI_Isend(sendbuf, n, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[0]);
      MPI_Irecv(recvbuf, n, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[1]);
    }
    MPI_Waitall(2, reqs, status);
    updateBuffer(sendbuf, rank, recvbuf, i, n, numprocs);
  }
}

void updateBuffer(int* local, int rank, int* recvbuf, int recv_rank, int n, int numprocs){
  int stride = n / numprocs;
  int count = stride;
  if (recv_rank == numprocs-1) count = n - stride * rank;
  
  for(int i=0; i<count; i++)
    local[stride*recv_rank + i] = recvbuf[stride*recv_rank + i];
}

void printArray(int* arr){
  for(int i=0; i<PROBLEM_SIZE; i++)
    printf("%d ", arr[i]);
  printf("\n");
}