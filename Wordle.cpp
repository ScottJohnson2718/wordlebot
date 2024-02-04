

#include "Wordle.h"

#include "WordQuery.h"

#include <fstream>

void LoadDictionary( const std::filesystem::path &filename, std::vector<std::string> &words)
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

void LoadDictionaries(bool newYorkTimes, int n,
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
            LoadDictionary(words5_long, guessingWords);
            LoadDictionary(words5_short, solutionWords);
            // For Lion Studios, we make one big dictionary and the guessing words and solution
            // words are actually the same list
            std::copy(guessingWords.begin(), guessingWords.end(), std::back_inserter(solutionWords));
            guessingWords = solutionWords;
        }
        else if (n == 6)
        {
            std::filesystem::path words6(dictPath);
            words6 /= "words6";
            LoadDictionary(words6, guessingWords);
            solutionWords = guessingWords;
        }
    }
    else
    {
        // For new york times, again, we keep the solution words and the guessing words separate. The solutions
        // words is a fairly small list.
        LoadDictionary(words5_long, guessingWords);
        LoadDictionary(words5_short, solutionWords);
    }

}

