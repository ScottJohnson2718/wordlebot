#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <vector>

// Pattern encoding: 2 bits per position
// 00 = Gray, 01 = Yellow, 10 = Green
#define GRAY 0
#define YELLOW 1
#define GREEN 2

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// Branchless pattern computation
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
        pattern |= (GREEN * is_green) << (i * 2);
        temp_counts[guess[i] - 'a'] -= is_green;
    }

    // Second pass: mark yellows
    for (int i = 0; i < 5; i++) {
        int is_green = (guess[i] == solution[i]);
        int letter_idx = guess[i] - 'a';
        int has_remaining = (temp_counts[letter_idx] > 0);
        int is_yellow = (!is_green) & has_remaining;

        pattern |= (YELLOW * is_yellow) << (i * 2);
        temp_counts[letter_idx] -= is_yellow;
    }

    return pattern;
}

// CUDA kernel to build the precomputed pattern table
__global__ void build_pattern_table(
        const char* solutions,     // [num_solutions][5]
        const char* guesses,       // [num_guesses][5]
        uint16_t* pattern_table,   // [num_solutions * num_guesses]
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
__global__ void compute_all_entropies(
        const uint16_t* pattern_table,  // [num_solutions * num_guesses]
        const int* remaining_indices,    // indices of remaining solutions
        int num_remaining,
        int num_solutions,
        int num_guesses,
        float* entropies                 // [num_guesses] output
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

// Host function to find the best guess
int find_best_guess(const float* entropies, int num_guesses) {
    int best_idx = 0;
    float best_entropy = entropies[0];

    for (int i = 1; i < num_guesses; i++) {
        if (entropies[i] > best_entropy) {
            best_entropy = entropies[i];
            best_idx = i;
        }
    }

    return best_idx;
}

// C++ wrapper class for easy usage
class WordleSolver {
private:
    int num_solutions;
    int num_guesses;

    // Device memory
    char* d_solutions;
    char* d_guesses;
    uint16_t* d_pattern_table;
    int* d_remaining_indices;
    float* d_entropies;

    // Host memory
    std::vector<int> remaining_indices;
    std::vector<float> h_entropies;

public:
    WordleSolver(const std::vector<std::string>& solutions,
                 const std::vector<std::string>& guesses)
            : num_solutions(solutions.size()),
              num_guesses(guesses.size()) {

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
        size_t table_size = (size_t)num_solutions * num_guesses * sizeof(uint16_t);
        CUDA_CHECK(cudaMalloc(&d_pattern_table, table_size));

        // Allocate remaining indices and entropies
        CUDA_CHECK(cudaMalloc(&d_remaining_indices, num_solutions * sizeof(int)));
        CUDA_CHECK(cudaMalloc(&d_entropies, num_guesses * sizeof(float)));

        h_entropies.resize(num_guesses);

        // Initialize all solutions as remaining
        remaining_indices.resize(num_solutions);
        for (int i = 0; i < num_solutions; i++) {
            remaining_indices[i] = i;
        }

        printf("Building pattern table...\n");
        build_pattern_table_host();
        printf("Pattern table built successfully!\n");
    }

    ~WordleSolver() {
        cudaFree(d_solutions);
        cudaFree(d_guesses);
        cudaFree(d_pattern_table);
        cudaFree(d_remaining_indices);
        cudaFree(d_entropies);
    }

    void build_pattern_table_host() {
        size_t total_patterns = (size_t)num_solutions * num_guesses;
        int threads = 256;
        int blocks = (total_patterns + threads - 1) / threads;

        build_pattern_table<<<blocks, threads>>>(
                d_solutions, d_guesses, d_pattern_table,
                        num_solutions, num_guesses
        );

        CUDA_CHECK(cudaDeviceSynchronize());
    }

    int get_best_guess() {
        int num_remaining = remaining_indices.size();

        // Copy remaining indices to device
        CUDA_CHECK(cudaMemcpy(d_remaining_indices, remaining_indices.data(),
                              num_remaining * sizeof(int), cudaMemcpyHostToDevice));

        // Launch kernel: one block per guess, 256 threads per block
        int threads = 256;
        compute_all_entropies<<<num_guesses, threads>>>(
                d_pattern_table, d_remaining_indices, num_remaining,
                        num_solutions, num_guesses, d_entropies
        );

        CUDA_CHECK(cudaDeviceSynchronize());

        // Copy entropies back to host
        CUDA_CHECK(cudaMemcpy(h_entropies.data(), d_entropies,
                              num_guesses * sizeof(float), cudaMemcpyDeviceToHost));

        // Find best guess on CPU
        return find_best_guess(h_entropies.data(), num_guesses);
    }

    void update_remaining(const std::string& guess, const std::string& solution) {
        // Filter remaining solutions based on the pattern
        uint16_t target_pattern = compute_pattern(solution.c_str(), guess.c_str());

        std::vector<int> new_remaining;
        for (int idx : remaining_indices) {
            // Compute what pattern this solution would give
            char sol_word[6];
            CUDA_CHECK(cudaMemcpy(sol_word, &d_solutions[idx * 5], 5, cudaMemcpyDeviceToHost));
            sol_word[5] = '\0';

            uint16_t pattern = compute_pattern(sol_word, guess.c_str());
            if (pattern == target_pattern) {
                new_remaining.push_back(idx);
            }
        }

        remaining_indices = new_remaining;
    }

    int get_num_remaining() const {
        return remaining_indices.size();
    }

    float get_entropy(int guess_idx) const {
        return h_entropies[guess_idx];
    }
};

// Example usage
int main() {
    // Load your word lists here
    std::vector<std::string> solutions = {"slate", "crane", "trace", "crate"}; // 2309 words
    std::vector<std::string> guesses = {"slate", "crane", "trace", "crate"};   // 12000 words

    printf("Initializing Wordle Solver...\n");
    printf("Solutions: %zu, Guesses: %zu\n", solutions.size(), guesses.size());

    WordleSolver solver(solutions, guesses);

    printf("\nFinding best opening guess...\n");
    int best_idx = solver.get_best_guess();
    float best_entropy = solver.get_entropy(best_idx);

    printf("Best guess: %s (entropy: %.4f)\n", guesses[best_idx].c_str(), best_entropy);
    printf("Remaining solutions: %d\n", solver.get_num_remaining());

    return 0;
}