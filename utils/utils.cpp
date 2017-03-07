#include <utils.hpp>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

int stick_this_thread_to_cpu(uint32_t core_id) {
   uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);

   if (core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    

   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}