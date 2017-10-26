#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <stdint.h>


#include <vector>
#include <limits>
#include <asm_kernels.hpp>

#include <memory>
#include <chrono>

struct thread_local_memory
{
    thread_local_memory()
    : vec_A(vec_size), vec_B(vec_size), vec_C(vec_size), vec_F(vec_size),
      mat_A(mat_size * mat_size), mat_B(mat_size * mat_size),
      mat_C(mat_size * mat_size), mem_buffer(mem_size)
      // firestarter_buffer(firestarter_size)
    {
        for (std::size_t i = 0; i < vec_A.size(); ++i)
        {
            vec_A[i] = static_cast<double>(i) * 0.3;
            vec_B[i] = static_cast<double>(i) * 0.2;
            vec_C[i] = static_cast<double>(i) * 0.7;
            vec_F[i] = static_cast<float>(i) * 1.42f;
        }

        for (std::size_t i = 0; i < mat_A.size(); i++)
        {
            mat_A[i] = static_cast<double>(i + 1);
            mat_B[i] = static_cast<double>(i + 1);
            mat_C[i] = static_cast<double>(i + 1);
        }

        for (std::size_t i = 0; i < mem_buffer.size(); i++)
        {
            mem_buffer[i] = i * 23 + 42;
        }

        // for (std::size_t i = 0; i < firestarter_buffer.size(); ++i)
        // {
        //     firestarter_buffer[i] =
        //         0.25 + static_cast<double>(i % 9267) * 0.24738995982e-4;
        // }
        
        // log::info() << "Memory allocated and touched.";
    }

    std::vector<double> vec_A;
    std::vector<double> vec_B;
    std::vector<double> vec_C;
    std::vector<float> vec_F;

    std::vector<double> mat_A;
    std::vector<double> mat_B;
    std::vector<double> mat_C;

    std::vector<std::uint64_t> mem_buffer;
    // std::vector<double, AlignmentAllocator<double, 32>> firestarter_buffer;

    const static std::size_t vec_size = 1024;
    const static std::size_t mat_size = 512;

    // size of mem_buffer equals 64MB
    const static std::size_t mem_size =
        64 * 1024 * 1024 / sizeof(mem_buffer[0]);

    // size of mem_buffer equals 160MB
    // const static std::size_t firestarter_size =
    //     160 * 1024 * 1024 / sizeof(firestarter_buffer[0]);
};

static struct thread_local_memory& thread_local_memory(uint32_t my_id, uint32_t max_num_threads)
{
    static std::vector<std::unique_ptr<struct thread_local_memory>> memory(max_num_threads);

    auto& tld = memory[my_id];

    if (!tld)
    {
        tld.reset(new struct thread_local_memory());
    }

    return *tld;
}

template<typename rep, typename period>
void addpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    // std::vector<double> vecta(1024);
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    // should be around 2ms per call of mulpd_kernel
    // static const std::size_t repeat = 4096 * 1024;
    static const std::size_t repeat = 1024;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    uint64_t loop = 0;

    do {
        addpd_kernel(comp_A.data(), repeat);
        loop++;

    } while (std::chrono::high_resolution_clock::now() < until);
}

// template<typename rep, typename period>
// uint64_t addpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads);
// uint64_t mulpd(uint64_t loops);
// uint64_t sqrt(uint64_t loops);

// void compute(uint64_t max_loops);
// void sinus(uint64_t max_loops);
// void idle(uint64_t micros);

// void matmul(uint64_t loops);

// template <std::size_t ChunkSize = 64 * 1024 * 1024>
// void memory_read(uint64_t max_loops) {

//     auto& my_mem_buffer = local.mem_buffer;

//     constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

//     static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
//                   "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

//     // static_assert(chunksize <= roco2::detail::thread_local_memory::mem_size,
//     //               "Given ChunkSize parameter is to big.");

//     uint64_t m = 0;
//     std::size_t loops = 0;

//     do
//     {
//         for (std::size_t i = 0; i < chunksize; i++)
//         {
//             m += my_mem_buffer[i];
//         }

//         loops++;
//     } while (loops < max_loops);

//     // just as a data dependency
//     volatile int dd = 0;
//     if (m == 42)
//         dd++;
// }

// template <std::size_t ChunkSize = 64 * 1024 * 1024>
// void memory_copy(uint64_t max_loops) {

//     auto& my_mem_buffer = local.mem_buffer;

//     constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

//     static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
//                   "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

//     // static_assert(chunksize <= roco2::detail::thread_local_memory::mem_size,
//     //               "Given ChunkSize parameter is to big.");

//     std::size_t loops = 0;
//     do
//     {
//         // SCOREP_USER_REGION("memory_kernel_loop", SCOREP_USER_REGION_TYPE_FUNCTION)
//         for (std::size_t i = 0; i < chunksize; i++)
//         {
//             my_mem_buffer[i] += my_mem_buffer[i];
//         }

//         loops++;
//     } while (loops < max_loops);
// }

// template <std::size_t ChunkSize = 64 * 1024 * 1024>
// void memory_write(uint64_t max_loops) {

//     auto& my_mem_buffer = local.mem_buffer;

//     constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

//     static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
//                   "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

//     // static_assert(chunksize <= roco2::detail::thread_local_memory::mem_size,
//     //               "Given ChunkSize parameter is to big.");

//     std::size_t loops = 0;
//     do
//     {
//         // SCOREP_USER_REGION("memory_kernel_loop", SCOREP_USER_REGION_TYPE_FUNCTION)
//         for (std::size_t i = 0; i < chunksize; i++)
//         {
//             my_mem_buffer[i] = loops;
//         }

//         loops++;
//     } while (loops < max_loops);
// }

#endif // KERNELS_HPP