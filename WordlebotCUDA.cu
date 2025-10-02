// ============================================================================
// WordlebotCUDA.cu - CUDA kernel implementations
// ============================================================================

#include "WordlebotCUDA.h"
#include <stdio.h>

#include "Wordle.h"

#include <cuda_runtime.h>

// Branchless pattern computation (device and host compatible)
__device__ __host__ uint16_t compute_pattern(const char* solution, const char* guess) {
    // Count letters in solution
    int sol_counts[26];
    for (int i = 0; i < 26; i++) sol_counts[i] = 0;

    for (int i = 0; i < 5; i++) {
        sol_counts[solution[i] - 'a']++;
    }

    uint16_t pattern = 0;
    int temp_counts[26];
    for (int i = 0; i < 26; i++) temp_counts[i] = sol_counts[i];

    // First pass: mark greens and reduce counts
    for (int i = 0; i < 5; i++) {
        int is_green = (guess[i] == solution[i]);
        pattern |= (NotPresent * is_green) << (i * 2);
        temp_counts[guess[i] - 'a'] -= is_green;
    }

    // Second pass: mark yellows
    for (int i = 0; i < 5; i++) {
        int is_green = (guess[i] == solution[i]);
        int letter_idx = guess[i] - 'a';
        int has_remaining = (temp_counts[letter_idx] > 0);
        int is_yellow = (!is_green) & has_remaining;

        pattern |= (CorrectNotHere * is_yellow) << (i * 2);
        temp_counts[letter_idx] -= is_yellow;
    }

    return pattern;
}

// CUDA kernel to build the precomputed pattern table
__global__ void build_pattern_table_kernel(
    const char* solutions,
    const char* guesses,
    uint16_t* pattern_table,
    int num_solutions,
    int num_guesses
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = num_solutions * num_guesses;

    if (idx < total) {
        int sol_idx = idx / num_guesses;
        int guess_idx = idx % num_guesses;

        const char* sol = &solutions[sol_idx * 5];
        const char* guess = &guesses[guess_idx * 5];

        pattern_table[idx] = compute_pattern(sol, guess);
    }
}

// CUDA kernel to compute entropy for all guesses using shared memory
__global__ void compute_all_entropies_kernel(
    const uint16_t* pattern_table,
    const int* remaining_indices,
    int num_remaining,
    int num_solutions,
    int num_guesses,
    float* entropies
) {
    int guess_idx = blockIdx.x;

    if (guess_idx >= num_guesses) return;

    // Shared memory for pattern bins (1024 possible patterns)
    __shared__ int bins[1024];

    // Initialize bins to zero
    for (int i = threadIdx.x; i < 1024; i += blockDim.x) {
        bins[i] = 0;
    }
    __syncthreads();

    // Count patterns - each thread processes multiple solutions
    for (int i = threadIdx.x; i < num_remaining; i += blockDim.x) {
        int sol_idx = remaining_indices[i];
        uint16_t pattern = pattern_table[sol_idx * num_guesses + guess_idx];
        atomicAdd(&bins[pattern], 1);
    }
    __syncthreads();

    // Compute entropy using parallel reduction
    __shared__ float partial_entropy[256];

    float local_entropy = 0.0f;
    float total = (float)num_remaining;

    for (int i = threadIdx.x; i < 1024; i += blockDim.x) {
        if (bins[i] > 0) {
            float p = (float)bins[i] / total;
            local_entropy -= p * log2f(p);
        }
    }

    partial_entropy[threadIdx.x] = local_entropy;
    __syncthreads();

    // Reduce partial entropies
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (threadIdx.x < stride) {
            partial_entropy[threadIdx.x] += partial_entropy[threadIdx.x + stride];
        }
        __syncthreads();
    }

    // Write final entropy
    if (threadIdx.x == 0) {
        entropies[guess_idx] = partial_entropy[0];
    }
}

// Host wrapper functions that call the kernels
void launch_build_pattern_table(
    const char* d_solutions,
    const char* d_guesses,
    uint16_t* d_pattern_table,
    int num_solutions,
    int num_guesses
) {
    size_t total_patterns = (size_t)num_solutions * num_guesses;
    int threads = 256;
    int blocks = (total_patterns + threads - 1) / threads;

    build_pattern_table_kernel << <blocks, threads >> > (
        d_solutions, d_guesses, d_pattern_table,
        num_solutions, num_guesses
        );

    cudaDeviceSynchronize();
}

void launch_compute_entropies(
    const uint16_t* d_pattern_table,
    const int* d_remaining_indices,
    int num_remaining,
    int num_solutions,
    int num_guesses,
    float* d_entropies
) {
    int threads = 256;

    compute_all_entropies_kernel << <num_guesses, threads >> > (
        d_pattern_table, d_remaining_indices, num_remaining,
        num_solutions, num_guesses, d_entropies
        );

    cudaDeviceSynchronize();
}

// Host-side pattern computation for filtering
uint16_t compute_pattern_host(const char* solution, const char* guess) {
    return compute_pattern(solution, guess);
}