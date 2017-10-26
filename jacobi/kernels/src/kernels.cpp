#include "kernels.hpp"

#include <vector>
#include <limits>
#include <asm_kernels.hpp>


// template<typename rep, typename period>
// uint64_t addpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

//     std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

// 	// std::vector<double> vecta(1024);
//     auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

// 	// should be around 2ms per call of mulpd_kernel
//     static const std::size_t repeat = 4096 * 1024;

// 	for (std::size_t i = 0; i < 16; ++i) {
//         comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
//     }

//     uint64_t loop = 0;

//     uint64_t output;

//     do {
//     	output = addpd_kernel(comp_A.data(), repeat);
//     	loop++;

//     } while (std::chrono::high_resolution_clock::now() < until);

//     return output;
// }

// uint64_t mulpd(uint64_t loops) {

//     std::vector<double> vecta(1024);

//     // should be around 2ms per call of mulpd_kernel
//     static const std::size_t repeat = 4096 * 1024;

//     for (std::size_t i = 0; i < 16; ++i) {
//         vecta[i] = 1. + std::numeric_limits<double>::epsilon();
//     }

//     uint64_t loop = 0;

//     uint64_t output;

//     do {
//         output = mulpd_kernel(vecta.data(), repeat);
//         loop++;

//     } while (loop < loops);

//     return output;
// }

// uint64_t sqrt(uint64_t max_loops) {

//     static const std::size_t repeat = 1024; // 4096;
//     static const std::size_t type = 2;

//     auto& comp_A = local.vec_A;
//     auto& comp_F = local.vec_F;

//     std::size_t loops = 0;

//     uint64_t output = 0;

//     do
//     {
//         // SCOREP_USER_REGION("sqrt_kernel_loop", SCOREP_USER_REGION_TYPE_FUNCTION)
//         switch (type)
//         {
//         case 0:
//             output = sqrtss_kernel(comp_F.data(), comp_F.size(), repeat);
//             break;
//         case 1:
//             output = sqrtps_kernel(comp_F.data(), comp_F.size(), repeat);
//             break;
//         case 2:
//             output = sqrtsd_kernel(comp_A.data(), comp_A.size(), repeat);
//             break;
//         case 3:
//             output = sqrtpd_kernel(comp_A.data(), comp_A.size(), repeat);
//             break;
//         }

//         loops++;
//     } while (loops < max_loops);

//     return output;
// }

// template <typename Container>
// void compute_kernel(Container& A, Container& B, Container& C, std::size_t repeat)
// {
//     double m = C[0];
//     const auto size = thread_local_memory().vec_size;

//     for (std::size_t i = 0; i < repeat; i++)
//     {
//         for (uint64_t i = 0; i < size; i++)
//         {
//             m += B[i] * A[i];
//         }
//         C[0] = m;
//     }
// }

// void compute(uint64_t max_loops) {

//     auto& vec_A = local.vec_A;
//     auto& vec_B = local.vec_B;
//     auto& vec_C = local.vec_C;

//     std::size_t loops = 0;

//     while (loops < max_loops)
//     {
//         if (vec_C[0] == 123.12345)
//             vec_A[0] += 1.0;

//         compute_kernel(vec_A, vec_B, vec_C, 32);

//         loops++;
//     }

//     // just as a data dependency
//     volatile int dd = 0;
//     if (vec_C[0] == 42.0)
//         dd++;
// }

// #include <cmath>

// void sinus(uint64_t max_loops) {

//     static const std::size_t sinus_loop = 200000;

//     double m = 0.0;

//     std::size_t loops = 0;

//     do
//     {
//         for (std::size_t i = 0; i < sinus_loop; i++)
//         {
//             m += sin((double)i);
//         }

//         loops++;
//     } while (loops < max_loops);

//     // just as a data dependency
//     volatile int dd = 0;
//     if (m == 42.0)
//         dd++;
// }

// #include <unistd.h>

// void idle(uint64_t micros) {
//     usleep(micros);
// }