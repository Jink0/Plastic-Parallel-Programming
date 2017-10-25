#include "kernels.hpp"

#include <vector>
#include <limits>
#include <asm_kernels.hpp>

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
} local;

uint64_t addpd(uint64_t loops) {

	std::vector<double> vecta(1024);

	// should be around 2ms per call of mulpd_kernel
    static const std::size_t repeat = 4096 * 1024;

	for (std::size_t i = 0; i < 16; ++i) {
        vecta[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    uint64_t loop = 0;

    uint64_t output;

    do {
    	output = addpd_kernel(vecta.data(), repeat);
    	loop++;

    } while (loop < loops);

    return output;
}

uint64_t mulpd(uint64_t loops) {

    std::vector<double> vecta(1024);

    // should be around 2ms per call of mulpd_kernel
    static const std::size_t repeat = 4096 * 1024;

    for (std::size_t i = 0; i < 16; ++i) {
        vecta[i] = 1. + std::numeric_limits<double>::epsilon();
    }

    uint64_t loop = 0;

    uint64_t output;

    do {
        output = mulpd_kernel(vecta.data(), repeat);
        loop++;

    } while (loop < loops);

    return output;
}

uint64_t sqrt(uint64_t max_loops) {

    static const std::size_t repeat = 1024; // 4096;
    static const std::size_t type = 2;

    auto& comp_A = local.vec_A;
    auto& comp_F = local.vec_F;

    std::size_t loops = 0;

    uint64_t output = 0;

    do
    {
        // SCOREP_USER_REGION("sqrt_kernel_loop", SCOREP_USER_REGION_TYPE_FUNCTION)
        switch (type)
        {
        case 0:
            output = sqrtss_kernel(comp_F.data(), comp_F.size(), repeat);
            break;
        case 1:
            output = sqrtps_kernel(comp_F.data(), comp_F.size(), repeat);
            break;
        case 2:
            output = sqrtsd_kernel(comp_A.data(), comp_A.size(), repeat);
            break;
        case 3:
            output = sqrtpd_kernel(comp_A.data(), comp_A.size(), repeat);
            break;
        }

        loops++;
    } while (loops < max_loops);

    return output;
}

template <typename Container>
void compute_kernel(Container& A, Container& B, Container& C, std::size_t repeat)
{
    double m = C[0];
    const auto size = thread_local_memory().vec_size;

    for (std::size_t i = 0; i < repeat; i++)
    {
        for (uint64_t i = 0; i < size; i++)
        {
            m += B[i] * A[i];
        }
        C[0] = m;
    }
}

void compute(uint64_t max_loops) {

    auto& vec_A = local.vec_A;
    auto& vec_B = local.vec_B;
    auto& vec_C = local.vec_C;

    std::size_t loops = 0;

    while (loops < max_loops)
    {
        if (vec_C[0] == 123.12345)
            vec_A[0] += 1.0;

        compute_kernel(vec_A, vec_B, vec_C, 32);

        loops++;
    }

    // just as a data dependency
    volatile int dd = 0;
    if (vec_C[0] == 42.0)
        dd++;
}

#include <cmath>

void sinus(uint64_t max_loops) {

    static const std::size_t sinus_loop = 200000;

    double m = 0.0;

    std::size_t loops = 0;

    do
    {
        for (std::size_t i = 0; i < sinus_loop; i++)
        {
            m += sin((double)i);
        }

        loops++;
    } while (loops < max_loops);

    // just as a data dependency
    volatile int dd = 0;
    if (m == 42.0)
        dd++;
}

#include <unistd.h>

void idle(uint64_t micros) {
    usleep(micros);
}