
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

// Filter solutions based on observed pattern (returns number of remaining solutions)
int launch_filter_solutions(
    const uint16_t* d_pattern_table,
    const int* d_input_indices,
    int* d_output_indices,
    int num_input,
    int num_guesses,
    int guess_idx,
    uint16_t target_pattern
);

// Host-side pattern computation
uint16_t compute_pattern_host(const char* solution, const char* guess);

#endif 