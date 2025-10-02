
#include "WordleSolverCUDA.h"

#include "Wordle.h"

#include <iostream>
#include <fstream>
#include <chrono>

bool Solve(WordleSolverCUDA &solver, const char* answer, const char* initialGuess, const  std::vector<std::string>&guessWords, std::vector<std::string>& bestGuesses, bool verbose = false)
{
    int turn = 1;
    solver.reset();

    // opening guess
    std::string guess(initialGuess);
    solver.update_remaining(guess, answer);
    if (verbose)
    {
        std::cout << "Initial guess: " << guess << std::endl;
        std::cout << "Remaining solutions: " << solver.get_num_remaining() << "\n\n";
    }

    bool solved = false;

    while (turn <= 6)
    {
        if (verbose)
            std::cout << "Turn " << turn << ": Guessing '" << guess << "'\n";
        bestGuesses.emplace_back(guess);

        // Check if we guessed correctly
        if (guess == answer) {
            if (verbose)
            {
                std::cout << "Success! Found the answer '" << answer
                    << "' in " << turn << " turns!\n";
            }
            solved = true;
            break;
        }

        // Update remaining solutions based on the guess
        solver.update_remaining(guess, answer);
        if (verbose)
            std::cout << "Remaining solutions: " << solver.get_num_remaining() << "\n";

        if (solver.get_num_remaining() == 0) {
            if (verbose)
                std::cout << "Error: No solutions remaining!\n";
            break;
        }

        if (solver.get_num_remaining() == 1) {
            if (verbose)
                std::cout << "Only one solution left!\n";
            // The last remaining solution must be the answer
            turn++;
            if (verbose)
            {
                std::cout << "Turn " << turn << ": Guessing the final solution\n";
                std::cout << "Success! Found the answer '" << answer
                    << "' in " << turn << " turns!\n";
            }
            bestGuesses.emplace_back(answer);
            solved = true;
            break;
        }

        // Get next best guess
        int best_idx = solver.get_best_guess();
      
        guess = guessWords[best_idx];
        float best_entropy = solver.get_entropy(best_idx);

        turn++;
    }

    return solved;
}

int main(int argc, char* argv[]) {

    std::cout << "Wordle CUDA Solver\n";
    std::cout << "==================\n\n";

    // Load word lists
    // You can replace these with your actual file paths
    std::vector<std::string> solutions;
    std::vector<std::string> guessWords;

    std::string initialGuess("crane");

    LoadDictionaries(true, 5, "d:/work/git_repos/wordlebot", solutions, guessWords);

    if (solutions.empty() || guessWords.empty()) {
        std::cerr << "Error: Word lists are empty!\n";
        std::cerr << "Usage: " << argv[0] << " <solutions_file> <guesses_file>\n";
        return 1;
    }

    std::cout << "Solutions: " << solutions.size() << "\n";
   // guessWords = solutions;
    std::cout << "Guesses: " << guessWords.size() << "\n\n";

    // Initialize solver
    auto start = std::chrono::high_resolution_clock::now();
    WordleSolverCUDA solver(solutions, guessWords);
    auto end = std::chrono::high_resolution_clock::now();
    auto init_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Initialization time: " << init_time << " ms\n\n";

    start = std::chrono::high_resolution_clock::now();
    size_t totalGuesses = 0;
    for (const auto solution : solutions)
    {
        std::vector<std::string> bestGuesses;
        if (Solve(solver, solution.c_str(), initialGuess.c_str(), guessWords, bestGuesses))
        {
            /*for (auto const& best : bestGuesses)
            {
                std::cout << best << " ";
            }
            std::cout << std::endl;*/
            totalGuesses += bestGuesses.size();
        }
        else
        {
            std::cout << "Failed to solve : " << solution << std::endl;
        }

    }
    end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << "Time to solve all words in solution dictionary (sec): " << duration << "\n\n";

    std::cout << "Average time to solve a single word (sec): " << (double) duration / (double) totalGuesses << std::endl;
    std::cout << "Average number of guesses with opening word of : " << initialGuess << " : " << (double)totalGuesses / (double)solutions.size() << std::endl;
    return 0;
}