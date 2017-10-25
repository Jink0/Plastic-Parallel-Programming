#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <stdint.h>

uint64_t addpd(uint64_t loops);
uint64_t mulpd(uint64_t loops);
uint64_t sqrt(uint64_t loops);

void compute(uint64_t max_loops);
void sinus(uint64_t max_loops);
void idle(uint64_t micros);

// void memory_read<>(uint64_t loops);
// void memory_copy<>(uint64_t loops);
// void memory_write<>(uint64_t loops);

// void matmul(uint64_t loops);

#endif // KERNELS_HPP