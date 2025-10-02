
#pragma once

#ifndef WORDLEBOT_CUDA_H
#define WORDLEBOT_CUDA_H

#include <stdint.h>

// Launch the pattern table building kernel
void launch_build_pattern_table(
    const char* d_solutions,
    const char* d_guesses,
    uint16_t* d_pattern_table,
    int num_solutions,
    int num_guesses
);

// Launch the entropy computation kernel
void launch_compute_entropies(
    const uint16_t* d_pattern_table,
    const int* d_remaining_indices,
    int num_remaining,
    int num_solutions,
    int num_guesses,
    float* d_entropies
);

// Host-side pattern computation
uint16_t compute_pattern_host(const char* solution, const char* guess);

#endif 