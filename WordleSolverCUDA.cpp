
#include "WordleSolverCUDA.h"
#include "WordlebotCUDA.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include <cuda_runtime.h>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


WordleSolverCUDA::WordleSolverCUDA(const std::vector<std::string>& solutions,
    const std::vector<std::string>& guesses)
    : num_solutions(static_cast<int>(solutions.size())),
    num_guesses(static_cast<int>(guesses.size())),
    d_solutions(nullptr),
    d_guesses(nullptr),
    d_pattern_table(nullptr),
    d_remaining_indices(nullptr),
    d_temp_indices(nullptr),
    d_entropies(nullptr) {

    // Allocate and copy solutions
    std::vector<char> h_solutions(num_solutions * 5);
    for (int i = 0; i < num_solutions; i++) {
        memcpy(&h_solutions[i * 5], solutions[i].c_str(), 5);
    }
    CUDA_CHECK(cudaMalloc(&d_solutions, num_solutions * 5));
    CUDA_CHECK(cudaMemcpy(d_solutions, h_solutions.data(),
        num_solutions * 5, cudaMemcpyHostToDevice));

    // Allocate and copy guesses
    std::vector<char> h_guesses(num_guesses * 5);
    for (int i = 0; i < num_guesses; i++) {
        memcpy(&h_guesses[i * 5], guesses[i].c_str(), 5);
    }
    CUDA_CHECK(cudaMalloc(&d_guesses, num_guesses * 5));
    CUDA_CHECK(cudaMemcpy(d_guesses, h_guesses.data(),
        num_guesses * 5, cudaMemcpyHostToDevice));

    // Allocate pattern table
    size_t table_size = static_cast<size_t>(num_solutions) * num_guesses * sizeof(uint16_t);
    CUDA_CHECK(cudaMalloc(&d_pattern_table, table_size));

    // Allocate remaining indices and entropies
    CUDA_CHECK(cudaMalloc(&d_remaining_indices, num_solutions * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_temp_indices, num_solutions * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_entropies, num_guesses * sizeof(float)));

    h_entropies.resize(num_guesses);

    // Initialize all solutions as remaining
    remaining_indices.resize(num_solutions);
    for (int i = 0; i < num_solutions; i++) {
        remaining_indices[i] = i;
    }

    printf("Building pattern table...\n");
    build_pattern_table_impl();
    printf("Pattern table built successfully!\n");
}

WordleSolverCUDA::~WordleSolverCUDA() {
    if (d_solutions) cudaFree(d_solutions);
    if (d_guesses) cudaFree(d_guesses);
    if (d_pattern_table) cudaFree(d_pattern_table);
    if (d_remaining_indices) cudaFree(d_remaining_indices);
    if (d_temp_indices) cudaFree(d_temp_indices);
    if (d_entropies) cudaFree(d_entropies);
}

void WordleSolverCUDA::build_pattern_table_impl() {
    launch_build_pattern_table(
        d_solutions, d_guesses, d_pattern_table,
        num_solutions, num_guesses
    );
}

int WordleSolverCUDA::get_best_guess() {
    int num_remaining = static_cast<int>(remaining_indices.size());

    // Copy remaining indices to device
    CUDA_CHECK(cudaMemcpy(d_remaining_indices, remaining_indices.data(),
        num_remaining * sizeof(int), cudaMemcpyHostToDevice));

    // Launch entropy computation kernel
    launch_compute_entropies(
        d_pattern_table, d_remaining_indices, num_remaining,
        num_solutions, num_guesses, d_entropies
    );

    // Copy entropies back to host
    CUDA_CHECK(cudaMemcpy(h_entropies.data(), d_entropies,
        num_guesses * sizeof(float), cudaMemcpyDeviceToHost));

    // Find best guess on CPU
    return find_best_guess();
}

int WordleSolverCUDA::find_best_guess() const {
    int best_idx = 0;
    float best_entropy = h_entropies[0];

    for (int i = 1; i < num_guesses; i++) {
        if (h_entropies[i] > best_entropy) {
            best_entropy = h_entropies[i];
            best_idx = i;
        }
    }

    return best_idx;
}

void WordleSolverCUDA::update_remaining(int guess_idx, uint16_t observed_pattern)
{
    if (guess_idx < 0 || guess_idx >= num_guesses) {
        fprintf(stderr, "Error: Invalid guess index %d\n", guess_idx);
        return;
    }

    int num_remaining = static_cast<int>(remaining_indices.size());

    // Copy current remaining indices to device
    CUDA_CHECK(cudaMemcpy(d_remaining_indices, remaining_indices.data(),
        num_remaining * sizeof(int), cudaMemcpyHostToDevice));

    // Filter solutions on GPU
    int new_count = launch_filter_solutions(
        d_pattern_table, d_remaining_indices, d_temp_indices,
        num_remaining, num_guesses, guess_idx, observed_pattern
    );

    // Copy filtered results back
    remaining_indices.resize(new_count);
    if (new_count > 0) {
        CUDA_CHECK(cudaMemcpy(remaining_indices.data(), d_temp_indices,
            new_count * sizeof(int), cudaMemcpyDeviceToHost));
    }
}

//void WordleSolverCUDA::update_remaining(const std::string& guess, uint16_t observed_pattern) {
//    // Find the guess index
//    int guess_idx = -1;
//    char guess_chars[6];
//    memcpy(guess_chars, guess.c_str(), 5);
//    guess_chars[5] = '\0';
//
//    // Search for the guess in device memory
//    std::vector<char> h_guesses_check(5);
//    for (int i = 0; i < num_guesses; i++) {
//        CUDA_CHECK(cudaMemcpy(h_guesses_check.data(), &d_guesses[i * 5], 5, cudaMemcpyDeviceToHost));
//        if (memcmp(h_guesses_check.data(), guess_chars, 5) == 0) {
//            guess_idx = i;
//            break;
//        }
//    }
//
//    if (guess_idx == -1) {
//        fprintf(stderr, "Error: Guess '%s' not found in guess list\n", guess.c_str());
//        return;
//    }
//
//    int num_remaining = static_cast<int>(remaining_indices.size());
//
//    // Copy current remaining indices to device
//    CUDA_CHECK(cudaMemcpy(d_remaining_indices, remaining_indices.data(),
//        num_remaining * sizeof(int), cudaMemcpyHostToDevice));
//
//    // Filter solutions on GPU
//    int new_count = launch_filter_solutions(
//        d_pattern_table, d_remaining_indices, d_temp_indices,
//        num_remaining, num_guesses, guess_idx, observed_pattern
//    );
//
//    // Copy filtered results back
//    remaining_indices.resize(new_count);
//    if (new_count > 0) {
//        CUDA_CHECK(cudaMemcpy(remaining_indices.data(), d_temp_indices,
//            new_count * sizeof(int), cudaMemcpyDeviceToHost));
//    }
//}


//void WordleSolverCUDA::update_remaining_with_solution(const std::string& guess, const std::string& actual_solution) {
//    // Compute the pattern that the actual solution produces for this guess
//    uint16_t observed_pattern = compute_pattern_host(actual_solution.c_str(), guess.c_str());
//    update_remaining(guess, observed_pattern);
//}

int WordleSolverCUDA::get_num_remaining() const {
    return static_cast<int>(remaining_indices.size());
}

float WordleSolverCUDA::get_entropy(int guess_idx) const {
    if (guess_idx >= 0 && guess_idx < static_cast<int>(h_entropies.size())) {
        return h_entropies[guess_idx];
    }
    return 0.0f;
}

void WordleSolverCUDA::reset() {
    // Reset remaining indices to include all solutions
    remaining_indices.resize(num_solutions);
    for (int i = 0; i < num_solutions; i++) {
        remaining_indices[i] = i;
    }
}