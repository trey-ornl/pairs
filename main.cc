#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc,&argv);
  int rank = MPI_PROC_NULL;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  int size = 0;
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  if (size%2) MPI_Abort(MPI_COMM_WORLD,size);
  const int half = size/2;

  int n = 1024*1024*1024;
  if (rank == 0) {
    if (argc > 1) sscanf(argv[1],"%d",&n);
    if (n <= 0) MPI_Abort(MPI_COMM_WORLD,n);
  }
  MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
  const size_t bytes = size_t(n)*sizeof(long);
  if (rank == 0) {
    printf("%d longs, %lu bytes, %d pairs\n",n,bytes,half);
    fflush(stdout);
  }

  long *ping = reinterpret_cast<long*>(malloc(2*bytes));
  long *pong = ping+n;

  const bool lower = (rank < half);
  const int partner = lower ? rank+half : rank-half;
  const int tag = 1;

  for (int i = 0; i < n; i++) ping[i] = i;

  const double gb = double(1024)*double(1024)*double(1024);
  const double timeout = 1;

  if (rank == 0) {
    printf("Warmup...\n");
    fflush(stdout);
  }

  if (lower) {
    MPI_Send(ping,n,MPI_LONG,partner,tag,MPI_COMM_WORLD);
    MPI_Recv(pong,n,MPI_LONG,partner,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
  } else {
    MPI_Recv(pong,n,MPI_LONG,partner,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    MPI_Send(pong,n,MPI_LONG,partner,tag,MPI_COMM_WORLD);
  }

  for (int parts = 1; parts < n; parts += parts) {
    memset(pong,0,bytes);
    MPI_Barrier(MPI_COMM_WORLD);
    const double before = MPI_Wtime();
    const int count = n/parts;
    for (int p = 0; p < parts; p++) {
      const int offset = p*count;
      if (lower) {
        MPI_Send(ping+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD);
        MPI_Recv(pong+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      } else {
        MPI_Recv(pong+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Send(pong+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD);
      }
    }
    const double delta = MPI_Wtime()-before;
    double elapsed = 0;
    MPI_Allreduce(&delta,&elapsed,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
    const int pc = parts*count;
    if (rank == 0) {
      const double bw = 2.0*double(pc)*double(sizeof(long))/(elapsed*gb);
      const double rate = 2.0*double(parts)/elapsed;
      printf("%d ping-pongs of %d longs: %g seconds, %g GB/s, %g message/s\n",parts,count,elapsed,bw,rate);
      fflush(stdout);
    }
    for (int i = 0; i < pc; i++) if (ping[i] != pong[i]) MPI_Abort(MPI_COMM_WORLD,rank);
    if (elapsed > timeout) break;
  }
  MPI_Finalize();
  return 0;
}
