

#include "Wordle.h"

#include "WordQuery.h"

#include <iostream>
#include <fstream>

bool LoadDictionary( const std::filesystem::path &filename, std::vector<std::string> &words)
{
    std::fstream newfile;
    newfile.open(filename,std::ios::in);
    if (newfile.is_open())
    {
        std::string tp;
        while(getline(newfile, tp))
        {
            words.push_back(tp);
        }
        newfile.close();
    }
    else
    {
        std::cerr << "Failed to load dictionary " << filename << std::endl;
        return false;
    }
    return true;
}

std::vector<std::string>  PruneSearchSpace(const WordQuery& query, const std::vector<std::string>& words)
{
    std::vector<std::string> remaining;

    // Apply the query to each word
    for (size_t i = 0; i < words.size(); ++i)
    {
        if (query.Satisfies(words[i]))
        {
            remaining.push_back(words[i]);
        }
    }

    return remaining;
}

uint32_t ComputeMask( const std::string &word)
{
    uint32_t mask(0);

    for (int i = 0; i < word.size(); ++i)
    {
        // This has to handle a '.' as a char that is skipped
        char ch = tolower(word[i]);
        if (ch >= 'a' && ch <= 'z')
            mask |= 1 << (ch - 'a');
    }
    return mask;
}

bool LoadDictionaries(bool newYorkTimes, int n,
                      const std::filesystem::path& dictPath,
                      std::vector<std::string>& solutionWords,
                      std::vector<std::string>& guessingWords)
{
    std::filesystem::path words5_long(dictPath);
    std::filesystem::path words5_short(dictPath);
    words5_long /= "words5_long";
    words5_short /= "words5_short";

    if (!newYorkTimes)
    {
        if (n == 5)
        {
            if (!LoadDictionary(words5_long, guessingWords))
                return false;
            if (!LoadDictionary(words5_short, solutionWords))
                return false;
            // For Lion Studios, we make one big dictionary and the guessing words and solution
            // words are actually the same list
            std::copy(guessingWords.begin(), guessingWords.end(), std::back_inserter(solutionWords));

            // sort and remove duplicates
            sort( solutionWords.begin(), solutionWords.end() );
            solutionWords.erase( unique( solutionWords.begin(), solutionWords.end() ), solutionWords.end() );

            guessingWords = solutionWords;
        }
        else if (n == 6)
        {
            std::filesystem::path words6(dictPath);
            words6 /= "words6";
            if (!LoadDictionary(words6, guessingWords))
                return false;
            solutionWords = guessingWords;
        }
    }
    else
    {
        // For new york times, again, we keep the solution words and the guessing words separate. The solutions
        // words is a fairly small list.
        if (!LoadDictionary(words5_long, guessingWords))
           return false;
        if (!LoadDictionary(words5_short, solutionWords))
            return false;
    }
    std::sort( solutionWords.begin(), solutionWords.end());
    return true;
}

void RemoveDuplicateGuesses( std::vector<ScoredGuess> &scoredGuesses)
{
    // Remove duplicate guesses by string
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.first < b.first;
              });
    scoredGuesses.erase( unique( scoredGuesses.begin(), scoredGuesses.end(),
                                 [](const ScoredGuess& a, const ScoredGuess& b)
                                 {
                                     return a.first == b.first;
                                 }
    ), scoredGuesses.end() );

}

