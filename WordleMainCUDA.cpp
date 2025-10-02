
#include "WordleSolverCUDA.h"
#include "WordlebotCUDA.h"

#include "Wordle.h"
#include "ScoredWord.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>

int find_word_index(const std::vector<std::string>& guesses, const std::string& target) {
    for (int i = 0; i < guesses.size(); i++) {
        if (guesses[i] == target) {
            return i;
        }
    }
    return -1;  // Not found
}

bool Solve(WordleSolverCUDA &solver, const char* answer, int initialGuessIndex, const  std::vector<std::string>&guessWords, std::vector<std::string>& bestGuesses, bool verbose = false, bool showGuesses = false)
{
    int turn = 1;
    solver.reset();

    // opening guess
    std::string guess = guessWords[initialGuessIndex];
    int best_idx = initialGuessIndex;
   
    uint16_t observed_pattern = compute_pattern_host(answer, guess.c_str());
    solver.update_remaining(best_idx, observed_pattern);
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
        
        if (showGuesses)
        {
            ScoredWord scoredWord(observed_pattern);
            std::cout << scoredWord.ToStringColored(guess) << " ";
        }

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
            if (showGuesses)
            {
                ScoredWord scoredWord;
                for (int j = 0; j < 5; ++j)
                    scoredWord.Set(j, Correct);
                std::cout << scoredWord.ToStringColored(answer) << std::endl;
            }
            bestGuesses.emplace_back(answer);
            solved = true;
            break;
        }

        // Get next best guess
        best_idx = solver.get_best_guess();
        guess = guessWords[best_idx];

        // Update remaining solutions based on the guess
        observed_pattern = compute_pattern_host(answer, guess.c_str());

        solver.update_remaining(best_idx, observed_pattern);
      
        
        //float best_entropy = solver.get_entropy(best_idx);

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

    LoadDictionaries(true, 5, "d:/work/git_repos/wordlebot", solutions, guessWords);

    if (solutions.empty() || guessWords.empty()) {
        std::cerr << "Error: Word lists are empty!\n";
        std::cerr << "Usage: " << argv[0] << " <solutions_file> <guesses_file>\n";
        return 1;
    }

    std::cout << "Solutions: " << solutions.size() << "\n";
   // guessWords = solutions;
    std::cout << "Guesses: " << guessWords.size() << "\n\n";
    
    std::string initialGuess("crane");
    if (argc >= 1)
        initialGuess = argv[1];
    int initialGuessIndex = find_word_index(guessWords, initialGuess);

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
        if (Solve(solver, solution.c_str(), initialGuessIndex, guessWords, bestGuesses, false, true))
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
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Time to solve all words in solution dictionary (msec): " << duration  << "\n\n";

    std::cout << "Average time to solve a single word (msec): " << (double) duration / (double) totalGuesses * 1000.0 << std::endl;
    std::cout << "Average number of guesses with opening word of : " << initialGuess << " : " << (double)totalGuesses / (double)solutions.size() << std::endl;
    return 0;
}