#ifndef KERNELS_HPP
#define KERNELS_HPP



#include <stdint.h>
#include <vector>
#include <limits>
#include <memory>
#include <chrono>
#include <cmath>
#include <thread>

#include <thread_local.hpp>
#include <asm_kernels.hpp>

// Experiment data
extern std::vector<std::vector<double>> grid1, grid2;



template<typename rep, typename period>
void addpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    // Set repeat so we should be around 0.5 microseconds per call of addpd kernel
    static const std::size_t repeat = 100;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    do {
        addpd_kernel(comp_A.data(), repeat);
    
    } while (std::chrono::high_resolution_clock::now() < until);
}



void addpd(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    addpd_kernel(comp_A.data(), repeats);
}



template<typename rep, typename period>
void mulpd(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;

    // Set repeat so we should be around 0.5 microseconds per call of mulpd kernel
    static const std::size_t repeat = 100;

    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    do {
        mulpd_kernel(comp_A.data(), repeat);

    } while (std::chrono::high_resolution_clock::now() < until);
}



void mulpd(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    // Get thread local vector
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;


    for (std::size_t i = 0; i < 16; ++i) {
        comp_A[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    mulpd_kernel(comp_A.data(), repeats);
}



template<typename rep, typename period>
void sqroot(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {

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



void sqroot(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    static const std::size_t type = 2;

    // Get thread local vectors
    auto& comp_A = thread_local_memory(my_id, max_num_threads).vec_A;
    auto& comp_F = thread_local_memory(my_id, max_num_threads).vec_F;

    switch (type) {
    case 0:
        sqrtss_kernel(comp_F.data(), comp_F.size(), repeats);
        break;
    case 1:
        sqrtps_kernel(comp_F.data(), comp_F.size(), repeats);
        break;
    case 2:
        sqrtsd_kernel(comp_A.data(), comp_A.size(), repeats);
        break;
    case 3:
        sqrtpd_kernel(comp_A.data(), comp_A.size(), repeats);
        break;
    }
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



void compute(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    auto& vec_A = thread_local_memory(my_id, max_num_threads).vec_A;
    auto& vec_B = thread_local_memory(my_id, max_num_threads).vec_B;
    auto& vec_C = thread_local_memory(my_id, max_num_threads).vec_C;

    for (uint32_t i = 0; i < repeats; i++) {
        if (vec_C[0] == 123.12345) {
            vec_A[0] += 1.0;
        }

        compute_kernel(vec_A, vec_B, vec_C, 1, my_id, max_num_threads);
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



void sinus(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    static const std::size_t sinus_loop = 200000;

    double m = 0.0;

    for (std::size_t i = 0; i < sinus_loop * repeats; i++) {
        m += sin((double) i);
    }
}



template<typename rep, typename period>
void idle(std::chrono::duration<rep, period> duration) {
    std::this_thread::sleep_for(duration);
}

template<typename rep, typename period>
void idle(std::chrono::duration<rep, period> duration, uint32_t my_id, uint32_t max_num_threads) {
    idle(duration);
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



template <std::size_t ChunkSize = 64 * 1024 * 1024>
void memory_read(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    uint64_t m = 0;

    for (uint32_t i = 0; i < repeats; i++) {
        for (std::size_t j = 0; j < chunksize; j++) {
            m += my_mem_buffer[j];
        }
    }
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



template <std::size_t ChunkSize = 64 * 1024 * 1024>
void memory_copy(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    for (uint32_t i = 0; i < repeats; i++) {
        for (std::size_t j = 0; j < chunksize; j++) {
            my_mem_buffer[i] += my_mem_buffer[i];
        }
    }
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



template <std::size_t ChunkSize = 64 * 1024 * 1024>
void memory_write(uint32_t repeats, uint32_t my_id, uint32_t max_num_threads) {

    auto& my_mem_buffer = thread_local_memory(my_id, max_num_threads).mem_buffer;

    constexpr std::size_t chunksize = ChunkSize / sizeof(my_mem_buffer[0]);

    static_assert(ChunkSize % sizeof(my_mem_buffer[0]) == 0,
                  "Given ChunkSize paramter should be divisible by sizeof(uint64_t)");

    static_assert(chunksize <= thread_local_memory::mem_size,
                  "Given ChunkSize parameter is to big.");

    for (uint32_t i = 0; i < repeats; i++) {
        for (std::size_t j = 0; j < chunksize; j++) {
            my_mem_buffer[i] = i;
        }
    }
}



template <std::size_t ChunkSize = 64 * 1024 * 1024, typename rep, typename period>
void shared_memory_read_small(std::chrono::duration<rep, period> duration, uint32_t i, uint32_t j) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    uint32_t granularity = 100;

    do {
        for (std::size_t k = 0; k < granularity; k++) {
            grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;
        }

    } while (std::chrono::high_resolution_clock::now() < until);
}



void shared_memory_read_small(uint32_t repeats, uint32_t i, uint32_t j) {

    uint32_t granularity = 100;

    for (std::size_t k = 0; k < repeats * granularity; k++) {
        grid2[i][j] = (grid1[i - 1][j] + grid1[i + 1][j] + grid1[i][j - 1] + grid1[i][j + 1]) * 0.25;
    }
}



template <std::size_t ChunkSize = 64 * 1024 * 1024, typename rep, typename period>
void shared_memory_read_large(std::chrono::duration<rep, period> duration, uint32_t i, uint32_t j) {

    // Calculate time limit
    std::chrono::high_resolution_clock::time_point until = std::chrono::high_resolution_clock::now() + duration;

    uint32_t granularity = 10;

    do {
        for (std::size_t k = 0; k < granularity; k++) {
            grid2[i][j] =  0;
            grid2[i][j] += (grid1[i - 2][j + 2] + grid1[i - 1][j + 2] + grid1[i][j + 2] + grid1[i + 1][j + 2] + grid1[i + 2][j + 2]);
            grid2[i][j] += (grid1[i - 2][j + 1] + grid1[i - 1][j + 1] + grid1[i][j + 1] + grid1[i + 1][j + 1] + grid1[i + 2][j + 1]);
            grid2[i][j] += (grid1[i - 2][j] + grid1[i - 1][j] + grid1[i + 1][j] + grid1[i + 2][j]);
            grid2[i][j] += (grid1[i - 2][j - 1] + grid1[i - 1][j - 1] + grid1[i][j - 1] + grid1[i + 1][j - 1] + grid1[i + 2][j - 1]);
            grid2[i][j] += (grid1[i - 2][j - 2] + grid1[i - 1][j - 2] + grid1[i][j - 2] + grid1[i + 1][j - 2] + grid1[i + 2][j - 2]);
            grid2[i][j] =  grid2[i][j] / 24;
        }

    } while (std::chrono::high_resolution_clock::now() < until);
}



void shared_memory_read_large(uint32_t repeats, uint32_t i, uint32_t j) {

    uint32_t granularity = 10;

    for (std::size_t k = 0; k < repeats * granularity; k++) {
        grid2[i][j] =  0;
        grid2[i][j] += (grid1[i - 2][j + 2] + grid1[i - 1][j + 2] + grid1[i][j + 2] + grid1[i + 1][j + 2] + grid1[i + 2][j + 2]);
        grid2[i][j] += (grid1[i - 2][j + 1] + grid1[i - 1][j + 1] + grid1[i][j + 1] + grid1[i + 1][j + 1] + grid1[i + 2][j + 1]);
        grid2[i][j] += (grid1[i - 2][j] + grid1[i - 1][j] + grid1[i + 1][j] + grid1[i + 2][j]);
        grid2[i][j] += (grid1[i - 2][j - 1] + grid1[i - 1][j - 1] + grid1[i][j - 1] + grid1[i + 1][j - 1] + grid1[i + 2][j - 1]);
        grid2[i][j] += (grid1[i - 2][j - 2] + grid1[i - 1][j - 2] + grid1[i][j - 2] + grid1[i + 1][j - 2] + grid1[i + 2][j - 2]);
        grid2[i][j] =  grid2[i][j] / 24;
    }
}

#endif // KERNELS_HPP