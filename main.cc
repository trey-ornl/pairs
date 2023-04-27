#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <hip/hip_runtime.h>
#include <mpi.h>

void checkHip(const hipError_t err, const char *const file, const int line)
{
  if (err == hipSuccess) return;
  fprintf(stderr,"HIP ERROR AT LINE %d OF FILE '%s': %s %s\n",line,file,hipGetErrorName(err),hipGetErrorString(err));
  fflush(stderr);
  exit(err);
}

#define CHECK(X) checkHip(X,__FILE__,__LINE__)

int main(int argc, char **argv)
{
  MPI_Init(&argc,&argv);
  int rank = MPI_PROC_NULL;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  int size = 0;
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  if (size%2) {
    if (rank == 0) fprintf(stderr,"%s: ERROR, number of MPI tasks must be even (unlike %d)\n",argv[0],size);
    MPI_Finalize();
    return size;
  }
  const int half = size/2;

  int n = 128*1024*1024;
  char mem = '0';
  if (rank == 0) {
    if (argc > 1) {
      int in = 0;
      sscanf(argv[1],"%d",&in);
      if (in > 0) n = in;
    }
    if (argc > 2) mem = argv[2][0];
  }
  MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&mem,1,MPI_CHAR,0,MPI_COMM_WORLD);
  const size_t bytes = size_t(n)*sizeof(long);
  if (rank == 0) {
    if (mem == 'd') printf("Using hipMalloc");
    else if (mem == 'h') printf("Using hipHostMalloc");
    else printf("Using malloc");
    printf(" with %d longs, %lu bytes, %d pairs\n",n,bytes,half);
    fflush(stdout);
  }

  long *hping = nullptr;
  long *hpong = nullptr;
  long *ping = nullptr;
  long *pong = nullptr;
  if (mem == 'd') {
    CHECK(hipHostMalloc(&hping,bytes));
    CHECK(hipHostMalloc(&hpong,bytes));
    CHECK(hipMalloc(&ping,bytes));
    CHECK(hipMalloc(&pong,bytes));
    for (int i = 0; i < n; i ++) hping[i] = i;
    CHECK(hipMemset(pong,0,bytes));
    CHECK(hipMemcpy(ping,hping,bytes,hipMemcpyHostToDevice));
    memset(hpong,0,bytes);
  } else {
    if (mem == 'h') {
      CHECK(hipHostMalloc(&ping,bytes));
      CHECK(hipHostMalloc(&pong,bytes));
    } else {
      ping = reinterpret_cast<long*>(malloc(bytes));
      pong = reinterpret_cast<long*>(malloc(bytes));
    }
    for (int i = 0; i < n; i++) ping[i] = i;
    memset(pong,0,bytes);
  }

  const int tag = 1;
  const double gb = double(1024)*double(1024)*double(1024);
  const double timeout = 1;

  MPI_Request req;

  for (int stride = half; stride > 0; stride--) if (size%(2*stride) == 0) {

    const bool lower = ((rank/stride)%2 == 0);
    const int partner = lower ? rank+stride : rank-stride;
    assert(partner >= 0);
    assert(partner < size);
    assert(partner != rank);
    if (rank == 0) {
      printf("\nWarmup for %d pairs of stride %d...\n",half,stride);
      fflush(stdout);
    }

    MPI_Irecv(pong,n,MPI_LONG,partner,tag,MPI_COMM_WORLD,&req);
    if (lower) {
      MPI_Send(ping,n,MPI_LONG,partner,tag,MPI_COMM_WORLD);
      MPI_Wait(&req,MPI_STATUS_IGNORE);
    } else {
      MPI_Wait(&req,MPI_STATUS_IGNORE);
      MPI_Send(pong,n,MPI_LONG,partner,tag,MPI_COMM_WORLD);
    }

    for (int parts = 1; parts < n; parts += parts) {

      if (mem == 'd') {
        CHECK(hipMemset(pong,0,bytes));
        CHECK(hipDeviceSynchronize());
      }
      else memset(pong,0,bytes);

      const int count = n/parts;
      const int end = parts*count;
      if (!lower) MPI_Irecv(pong,count,MPI_LONG,partner,tag,MPI_COMM_WORLD,&req);
      MPI_Barrier(MPI_COMM_WORLD);
      const double before = MPI_Wtime();
      for (int offset = 0; offset < end; offset += count) {
        if (lower) {
          MPI_Irecv(pong+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD,&req);
          MPI_Send(ping+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD);
          MPI_Wait(&req,MPI_STATUS_IGNORE);
        } else {
          MPI_Wait(&req,MPI_STATUS_IGNORE);
          if (offset+count < end) MPI_Irecv(pong+offset+count,count,MPI_LONG,partner,tag,MPI_COMM_WORLD,&req);
          MPI_Send(pong+offset,count,MPI_LONG,partner,tag,MPI_COMM_WORLD);
        }
      }
      const double delta = MPI_Wtime()-before;
      double elapsed = 0;
      MPI_Allreduce(&delta,&elapsed,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
      if (rank == 0) {
        const double bw = 2.0*double(end)*double(sizeof(long))/(elapsed*gb);
        const double rate = 2.0*double(parts)/elapsed;
        printf("%d ping-pongs of %d longs: %g seconds, %g GB/s, %g message/s\n",parts,count,elapsed,bw,rate);
        fflush(stdout);
      }

      if (mem == 'd') {
        CHECK(hipMemcpy(hpong,pong,bytes,hipMemcpyDeviceToHost));
        for (int i = 0; i < end; i++) if (hping[i] != hpong[i]) MPI_Abort(MPI_COMM_WORLD,i);
      } else {
        for (int i = 0; i < end; i++) if (ping[i] != pong[i]) MPI_Abort(MPI_COMM_WORLD,i);
      }
      if (elapsed > timeout) break;
    }
  }
  MPI_Finalize();
  return 0;
}
