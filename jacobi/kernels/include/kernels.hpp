#ifndef KERNELS_HPP
#define KERNELS_HPP

enum kernels_enum {none = 0, cpu = 1, io = 2, vm = 3, hdd = 4};



// Workload kernel parameters
static long long const vm_bytes  = 1024 * 1024;
static long long const vm_stride = 4096;
static long long const vm_hang   = -1;
static int       const vm_keep   = 1;

static long long const hdd_bytes = 1024;



// Generates cpu load, repeats for given amount
volatile int hogcpu(long long repeats);

// Generates io load, repeats for given amount
int hogio(long long repeats);

// Generates vm load by allocating memory, touching certain bytes, and optionally hanging. Repeats for given amount
int hogvm(long long repeats);

// Generates hdd load by writing random data to the hdd, repeats for given amount
int hoghdd(long long repeats);

#endif // KERNELS_HPP