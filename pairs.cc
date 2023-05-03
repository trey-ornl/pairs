#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <hip/hip_runtime.h>
#include <iostream>
#include <mpi.h>
#include <sched.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

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
  int misalign = 0;
  if (rank == 0) {
    if (argc > 1) {
      int in = 0;
      sscanf(argv[1],"%d",&in);
      if (in > 0) n = in;
    }
    if (argc > 2) mem = argv[2][0];
    if (argc > 3) {
      int in = 0;
      sscanf(argv[3],"%d",&in);
      if (in > 0) misalign = in;
    }
  }
  MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Bcast(&mem,1,MPI_CHAR,0,MPI_COMM_WORLD);
  MPI_Bcast(&misalign,1,MPI_INT,0,MPI_COMM_WORLD);
  size_t bytes = size_t(n)*sizeof(long);

  if (rank == 0) printf("# %s: strided parallel pairs of MPI ping-pong partners\n",argv[0]);

  const int tag = 1;
  MPI_Request req;

  {
    if (rank == 0) printf("# rank: host cores device(closest cores)\n");

    std::stringstream binding;
    binding << "# " << rank << ": ";

    char proc[MPI_MAX_PROCESSOR_NAME];
    int length = 0;
    MPI_Get_processor_name(proc, &length);
    binding << proc << ' ';

    cpu_set_t set;
    sched_getaffinity(0,sizeof(set),&set);
    bool first = true;
    for (int j = 0; j < CPU_SETSIZE; j++) {
      if (CPU_ISSET(j,&set)) {
        if (first) first = false;
        else binding << ',';
        binding << j;
        const int jLo = j+2;
        for (j++; (j < CPU_SETSIZE) && CPU_ISSET(j,&set); j++);
        if (j > jLo) binding << '-' << (j-1);
        else if (j == jLo) binding << ',' << (j-1);
      }
    }

    int device;
    CHECK(hipGetDevice(&device));
    hipDeviceProp_t props;
    CHECK(hipGetDeviceProperties(&props,device));
    binding << ' ' << props.pciBusID;
    char pci[] = "0000:00:00.0";
    snprintf(pci,sizeof(pci),"%04x:%02x:%02x.0",props.pciDomainID,props.pciBusID,props.pciDeviceID);
    std::string closest;
    std::ifstream(std::string("/sys/bus/pci/devices/")+pci+"/local_cpulist") >> closest;
    binding << '(' << closest << ')';

    const int mylen = binding.str().size()+1;
    int maxlen = 0;
    MPI_Reduce(&mylen,&maxlen,1,MPI_INT,MPI_MAX,0,MPI_COMM_WORLD);
    if (rank == 0) {
      printf("%s\n",binding.str().c_str());
      char *const line = new char[maxlen];
      for (int i = 1; i < size; i++) {
        MPI_Irecv(line,maxlen,MPI_CHAR,i,tag,MPI_COMM_WORLD,&req);
        MPI_Send(0,0,MPI_CHAR,i,tag,MPI_COMM_WORLD);
        MPI_Wait(&req,MPI_STATUS_IGNORE);
        printf("%s\n",line);
      }
      fflush(stdout);
      delete [] line;
    } else {
      MPI_Recv(0,0,MPI_CHAR,0,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send(binding.str().c_str(),mylen,MPI_CHAR,0,tag,MPI_COMM_WORLD);
    }
  }

  if (rank == 0) {
    printf("# allocator: ");
    if (mem == 'd') printf("hipMalloc\n");
    else if (mem == 'h') printf("hipHostMalloc\n");
    else printf("malloc\n");
    printf("# pairs: %d\n",half);
    printf("# total count: %d longs (%g MiB)\n",n,double(bytes)/(1024.0*1024.0));
    if (misalign) printf("# misaligned by %lu bytes\n",misalign*sizeof(long));
    fflush(stdout);
  }

  const double timeout = 1;
  if (rank == 0) printf("# timeout: %gs\n",timeout);

  long *hping = nullptr;
  long *hpong = nullptr;
  long *ping = nullptr;
  long *pong = nullptr;
  const int extra = misalign*sizeof(long);
  if (mem == 'd') {
    CHECK(hipHostMalloc(&hping,bytes+extra));
    hping += misalign;
    CHECK(hipHostMalloc(&hpong,bytes+extra));
    hpong += misalign;
    CHECK(hipMalloc(&ping,bytes+extra));
    ping += misalign;
    CHECK(hipMalloc(&pong,bytes+extra));
    pong += misalign;
    for (int i = 0; i < n; i ++) hping[i] = i;
    CHECK(hipMemset(pong,0,bytes));
    CHECK(hipMemcpy(ping,hping,bytes,hipMemcpyHostToDevice));
    memset(hpong,0,bytes);
  } else {
    if (mem == 'h') {
      CHECK(hipHostMalloc(&ping,bytes+extra));
      ping += misalign;
      CHECK(hipHostMalloc(&pong,bytes+extra));
      pong += misalign;
    } else {
      ping = reinterpret_cast<long*>(malloc(bytes+extra));
      ping += misalign;
      pong = reinterpret_cast<long*>(malloc(bytes+extra));
      pong += misalign;
    }
    for (int i = 0; i < n; i++) ping[i] = i;
    memset(pong,0,bytes);
  }

  for (int stride = half; stride > 0; stride--) if (size%(2*stride) == 0) {

    const bool lower = ((rank/stride)%2 == 0);
    const int partner = lower ? rank+stride : rank-stride;
    assert(partner >= 0);
    assert(partner < size);
    assert(partner != rank);
    if (rank == 0) {
      printf("\n# stride between partners: %d\n",stride);
      printf("# ping-pongs | count (longs) | max avg min time/msg/pair (us) | min avg max bandwidth (GiB/s) | min avg max rate (msgs/s)\n");
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

    for (int parts = 1; parts <= n; parts += parts) {

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
      double dmax = 0;
      MPI_Allreduce(&delta,&dmax,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
      double dmin = dmax;
      MPI_Reduce(&delta,&dmin,1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);
      double dsum = 0;
      MPI_Reduce(&delta,&dsum,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
      if (rank == 0) {
        const double davg = dsum/double(size);
        const double us = 0.5e6/double(parts);
        const double bw = 2.0*double(end)*double(sizeof(long))/(1024.0*1024.0*1024.0);
        const double rate = 2.0*double(parts);
        printf("%d %d %g %g %g %g %g %g %g %g %g\n",parts,count,dmax*us,davg*us,dmin*us,bw/dmax,bw/davg,bw/dmin,rate/dmax,rate/davg,rate/dmin);
        fflush(stdout);
      }

      if (mem == 'd') {
        CHECK(hipMemcpy(hpong,pong,bytes,hipMemcpyDeviceToHost));
        for (int i = 0; i < end; i++) if (hping[i] != hpong[i]) MPI_Abort(MPI_COMM_WORLD,i);
      } else {
        for (int i = 0; i < end; i++) if (ping[i] != pong[i]) MPI_Abort(MPI_COMM_WORLD,i);
      }
      if (dmax > timeout) {
        if (rank == 0) printf("\n");
        break;
      }
    }
  }
  MPI_Finalize();
  return 0;
}
