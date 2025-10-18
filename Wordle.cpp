

#include "Wordle.h"
#include "Strategy.h"
#include "WordleBot.h"

#include <iostream>
#include <fstream>
#include <thread>

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
        // We allow the guessing of the solution words
        std::copy( solutionWords.begin(), solutionWords.end(), std::back_inserter(guessingWords));
    }
    std::sort( solutionWords.begin(), solutionWords.end());
    // Remove duplicates in the solution words
    solutionWords.erase( unique( solutionWords.begin(), solutionWords.end()), solutionWords.end());
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

float TestWords(std::vector<std::string>& solutionWords, const std::vector < std::string>& guessingWords,
    const std::string& openingGuess, Strategy& strategy, bool verbose)
{
    auto start = std::chrono::high_resolution_clock::now();

    Bot bot(guessingWords, solutionWords, strategy, verbose);

    std::atomic<size_t> guesses_atomic(0);
    const size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    const size_t total_words = solutionWords.size();
    const size_t chunk_size = (total_words + num_threads - 1) / num_threads;

    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            const size_t start = t * chunk_size;
            const size_t end = std::min(start + chunk_size, total_words);

            size_t local_guesses = 0;
            for (size_t i = start; i < end; ++i) {
                local_guesses += bot.SolvePuzzle(solutionWords[i], openingGuess);
            }
            guesses_atomic += local_guesses;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    size_t guesses = guesses_atomic.load();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Total guesses for : " << openingGuess << " " << guesses << std::endl;
    std::cout << "Ave guesses : " << openingGuess << " " << (double)guesses / (double)solutionWords.size() << std::endl;
    std::cout << "Time to solve all words in solution dictionary (msec): " << duration << "\n\n";

    return (double)guesses / (double)solutionWords.size();
}
