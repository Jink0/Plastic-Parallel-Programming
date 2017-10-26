#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <stdint.h>
#include <vector>
#include <limits>
#include <memory>
#include <chrono>
#include <cmath>
#include <thread>

#include <asm_kernels.hpp>



struct thread_local_memory {
    thread_local_memory()
    : vec_A(vec_size), vec_B(vec_size), vec_C(vec_size), vec_F(vec_size),
      mat_A(mat_size * mat_size), mat_B(mat_size * mat_size),
      mat_C(mat_size * mat_size), mem_buffer(mem_size)
      // firestarter_buffer(firestarter_size)
    {
        for (std::size_t i = 0; i < vec_A.size(); ++i) {
            vec_A[i] = static_cast<double>(i) * 0.3;
            vec_B[i] = static_cast<double>(i) * 0.2;
            vec_C[i] = static_cast<double>(i) * 0.7;
            vec_F[i] = static_cast<float>(i) * 1.42f;
        }

        for (std::size_t i = 0; i < mat_A.size(); i++) {
            mat_A[i] = static_cast<double>(i + 1);
            mat_B[i] = static_cast<double>(i + 1);
            mat_C[i] = static_cast<double>(i + 1);
        }

        for (std::size_t i = 0; i < mem_buffer.size(); i++) {
            mem_buffer[i] = i * 23 + 42;
        }

        // for (std::size_t i = 0; i < firestarter_buffer.size(); ++i) {
        //     firestarter_buffer[i] = 0.25 + static_cast<double>(i % 9267) * 0.24738995982e-4;
        // }
        
        // std::cout) << "Memory allocated and touched.";
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

    // Size of mem_buffer equals 64MB
    const static std::size_t mem_size = 64 * 1024 * 1024 / sizeof(mem_buffer[0]);

    // Size of mem_buffer equals 160MB
    // const static std::size_t firestarter_size = 160 * 1024 * 1024 / sizeof(firestarter_buffer[0]);
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

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    // Set repeat so we should be around 0.5ms per call of addpd kernel
    static const std::size_t repeat = 92500;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    do {
        addpd_kernel(comp_A.data(), repeat);
    
    } while (std::chrono::high_resolution_clock::now() < until);
}

template<typename rep, typename period>
void mulpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    // Set repeat so we should be around 0.5ms per call of mulpd kernel
    static const std::size_t repeat = 92500;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    do {
        mulpd_kernel(comp_A.data(), repeat);

    } while (std::chrono::high_resolution_clock::now() < until);
}

template<typename rep, typename period>
void sqrt(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    static const std::size_t repeat = 256;
    static const std::size_t type = 2;

    // Get thread local vectors
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;
    auto& comp_F = thread_local_memory(my_id, max_num_threads).vec_F;

    do {
        switch (type) {
        case 0:
            sqrtss_kernel(comp_F.data(), comp_F.size(), repeat);
            break;
        case 1:
            sqrtps_kernel(comp_F.data(), comp_F.size(), repeat);
            break;
        case 2:
            sqrtsd_kernel(comp_A.data(), comp_A.size(), repeat);
            break;
        case 3:
            sqrtpd_kernel(comp_A.data(), comp_A.size(), repeat);
            break;
        }
    } while (std::chrono::high_resolution_clock::now() < until);
}

template <typename Container>
void compute_kernel(Container& A, Container& B, Container& C, std::size_t repeat, uint32_t my_id, uint32_t max_num_threads) {
    
    double m = C[0];
    const auto size = thread_local_memory(my_id, max_num_threads).vec_size;

    for (std::size_t i = 0; i < repeat; i++) {
        for (uint64_t i = 0; i < size; i++) {
            m += B[i] * A[i];
        }
        C[0] = m;
    }
}

template<typename rep, typename period>
void compute(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    auto& vec_A = thread_local_memory(my_id, max_num_threads).vec_A;
    auto& vec_B = thread_local_memory(my_id, max_num_threads).vec_B;
    auto& vec_C = thread_local_memory(my_id, max_num_threads).vec_C;

    while (std::chrono::high_resolution_clock::now() <= until) {
        if (vec_C[0] == 123.12345) {
            vec_A[0] += 1.0;
        }

        compute_kernel(vec_A, vec_B, vec_C, 32, my_id, max_num_threads);
    }
}

template<typename rep, typename period>
void sinus(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    static const std::size_t sinus_loop = 200000;

    double m = 0.0;

    do {
        for (std::size_t i = 0; i < sinus_loop; i++) {
            m += sin((double) i);
        }

    } while (std::chrono::high_resolution_clock::now() < until);
}

template<typename rep, typename period>
void idle(std::chrono::duration<rep, period> duration) {
    std::this_thread::sleep_for(duration);
}

template <std::size_t ChunkSize = 64 * 1024 * 1024, typename rep, typename period>
void memory_read(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    uint64_t m = 0;

    do {
        for (std::size_t i = 0; i < chunksize; i++) {
            m += my_mem_buffer[i];
        }
    } while (std::chrono::high_resolution_clock::now() < until);
}

template <std::size_t ChunkSize = 64 * 1024 * 1024, typename rep, typename period>
void memory_copy(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    do {
        for (std::size_t i = 0; i < chunksize; i++) {
            my_mem_buffer[i] += my_mem_buffer[i];
        }

    } while (std::chrono::high_resolution_clock::now() < until);
}

template <std::size_t ChunkSize = 64 * 1024 * 1024, typename rep, typename period>
void memory_write(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    do {
        for (std::size_t i = 0; i < chunksize; i++) {
            my_mem_buffer[i] = i;
        }

    } while (std::chrono::high_resolution_clock::now() < until);
}

// void matmul(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads);
// void firestarter(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads);

#endif // KERNELS_HPP