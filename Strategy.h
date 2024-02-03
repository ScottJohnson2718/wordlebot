
#pragma once

#include "Wordle.h"
#include "ScoredWord.h"
#include "Board.h"
#include "Entropy.h"

#include <vector>

struct Strategy
{

    // Return the best single guess for the given board
    virtual ScoredGuess BestGuess(Board& board,
                                  const std::vector<std::string>& solutionWords) const = 0;

    // Return a list of the best guesses for the given board.
    // This is useful in interactive mode when the guessed word
    // is not in the dictionary of the Wordle app. In that case,
    // there are alternative words to chose from.
    virtual std::vector<ScoredGuess> BestGuesses(Board& board,
                                                 const std::vector<std::string>& solutionWords) const = 0;
};

// This strategy employs only entropy to chose the next guess given a board.
struct EntropyStrategy : public Strategy
{
    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;

    EntropyStrategy(const std::vector<std::string>& guessingWords, size_t maxGuessesReturned);
    ScoredGuess BestGuess(Board& board, const std::vector<std::string>& solutionWords) const;
    std::vector<ScoredGuess> BestGuesses(Board& board, const std::vector<std::string>& solutionWords) const;
};

///////////////

// This strategy picks the word that reduces the total size of the search space remaining
struct SearchStrategy : public Strategy
{
    SearchStrategy(
            const std::vector<std::string>& guessingWords, size_t maxGuessesReturned);

    ScoredGuess BestGuess(Board& board,
                          const std::vector<std::string>& solutionWords) const;


    std::vector<ScoredGuess> BestGuesses(Board& board,
                                         const std::vector<std::string>& solutionWords) const;


    static size_t  SearchSpaceSize(const WordQuery& query, const std::vector<std::string>& words);

    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;
};

///////////////

// This strategy picks the word that reduces the total size of the search space remaining
struct BlendedStrategy : public Strategy
{
    EntropyStrategy entropyStrategy_;
    SearchStrategy searchStrategy_;

    BlendedStrategy(
            const std::vector<std::string>& guessingWords, size_t maxGuessesReturned);

    ScoredGuess BestGuess(Board& board,
                          const std::vector<std::string>& solutionWords) const;


    std::vector<ScoredGuess> BestGuesses(Board& board,
                                         const std::vector<std::string>& solutionWords) const;

};


