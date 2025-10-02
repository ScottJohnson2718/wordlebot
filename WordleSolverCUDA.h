// ============================================================================
// WordleSolver.h - C++ wrapper class for CUDA Wordle solver
// ============================================================================

#ifndef WORDLE_SOLVER_CUDA_H
#define WORDLE_SOLVER_CUDA_H

#include <vector>
#include <string>
#include <stdint.h>

// Uses CUDA to solve Wordle puzzles in about 12ms on an NVidia Geforce RTX 4060
// It is not thread safe.
class WordleSolverCUDA {
public:
    // Constructor: Initialize with solution and guess word lists
    WordleSolverCUDA(const std::vector<std::string>& solutions,
        const std::vector<std::string>& guesses);

    // Destructor: Clean up GPU memory
    ~WordleSolverCUDA();

    // Get the best guess based on maximum entropy
    int get_best_guess();

    // Update remaining solutions after a guess with the observed pattern
    //void update_remaining(const std::string& guess, uint16_t observed_pattern);

    // Update remaining solutions using guess index (faster - no search needed)
    void update_remaining(int guess_idx, uint16_t observed_pattern);

    // Update remaining solutions when you know the actual solution (for testing)
    //void update_remaining_with_solution(const std::string& guess, const std::string& actual_solution);

    // Get number of remaining possible solutions
    int get_num_remaining() const;

    // Get entropy value for a specific guess index
    float get_entropy(int guess_idx) const;

    // Reset to initial state (all solutions remaining) without rebuilding pattern table
    void reset();

private:
    int num_solutions;
    int num_guesses;

    // Device memory pointers
    char* d_solutions;
    char* d_guesses;
    uint16_t* d_pattern_table;
    int* d_remaining_indices;
    int* d_temp_indices;  // Temporary buffer for filtering
    float* d_entropies;

    // Host memory
    std::vector<int> remaining_indices;
    std::vector<float> h_entropies;

    // Helper methods
    void build_pattern_table_impl();
    int find_best_guess() const;
};

#endif // WORDLE_SOLVER_CUDA_H