
#include "ScoredWord.h"
#include <sstream>
#include <algorithm>
#include <array>
#include <unordered_map>

// LetterCount is the count of each letter in the alphabet for a given word
//using LetterCount = std::array<int, 26>;

//void CreateLetterTable(const std::string &word, LetterCount &letterTable)
//{
//    std::fill(letterTable.begin(), letterTable.end(), 0);
//    for (int i = 0; i < word.size(); ++i)
//    {
//        int idx = tolower(word[i]) - 'a';
//        letterTable[idx]++;
//    }
//}

bool Contains(const std::string &str, const char ch, int &foundIndex)
{
    bool found = false;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == ch)
        {
            foundIndex = i;
            return true;
        }
    }
    return false;
}

ScoredWord Score(const std::string& solution, const std::string& guess)
{
    std::string mutableSolution(solution);
    std::string mutableGuess(guess);
    const char *pSolution = mutableSolution.c_str();
    const char *pGuess = guess.c_str();
    ScoredWord score;
    for (int i = 0; i < solution.size(); ++i)
    {
        if (pGuess[i] == pSolution[i])
        {
            score.Set(i, Correct);
            mutableSolution[i] = '.';
            mutableGuess[i] = '.';
        }
    }
    for (int i = 0; i < solution.size(); ++i)
    {
        if (mutableGuess[i] == '.')
            continue;
        int foundIndex;
        if (Contains(mutableSolution, guess[i], foundIndex))
        {
            score.Set(i, CorrectNotHere);
            mutableSolution[foundIndex] = '.';
        }
        else
        {
            score.Set(i, NotPresent);
        }
    }
    return score;
}

std::string ScoredWord::ToString(const std::string &guess) const
{
    std::stringstream str;
    print( str, guess, *this);
    return str.str();
}

std::string ScoreString(const std::string &solution, const std::string &guess)
{
    ScoredWord sw = Score(solution, guess);
    return sw.ToString(guess);
}

std::ostream& print(std::ostream &str, const std::string &guess, const ScoredWord &scored)
{
    for (int i = 0; i < guess.size(); ++i)
    {
        CharScore cs = scored.Get(i);
        char ch = guess[i];

        switch (cs)
        {
            case NotPresent:
                str << ".";
                break;
            case Correct :
                str << ch;
                break;
            case CorrectNotHere :
                str << (char) toupper(ch);
                break;
        }
    }
    return str;
}

size_t ScoreGroupCount(const std::string& guessWord,
                       const std::vector<std::string>& solutionWords,
                       size_t &largestGroup)
{
    largestGroup = 0;
    // This guess separates the remaining solutions into groups according to a common board score
    std::unordered_map<ScoredWord, size_t, ScoredWordHash, ScoredWordEqual> groups;
    for (auto const& possibleSolution : solutionWords)
    {
        ScoredWord s = Score(possibleSolution, guessWord);
        std::string str = s.ToString(guessWord);
        groups[s]++;
    }
    for (const auto &group : groups)
    {
        if (group.second > largestGroup)
        {
            largestGroup = group.second;
        }
    }

    return groups.size();
}

std::vector<std::string> ScoreGroup(
    const std::string& guessWord,
    const ScoredWord& score,
    const std::vector < std::string>& solutionWords)
{
    std::vector<std::string> remaining;

    for (auto const& possibleSolution : solutionWords)
    {
        ScoredWord s = Score(possibleSolution, guessWord);
        std::string str = s.ToString(guessWord);      // useful for debugging
        if (score == s)
        {
            remaining.push_back(possibleSolution);
        }
    }
    return remaining;
}

std::set<ScoredWord> ScoresByGuess(const std::string& guessWord,
                                      const std::vector < std::string>& solutionWords )
{
    std::set< ScoredWord > wordScores;

    for (auto const& possibleSolution : solutionWords)
    {
        ScoredWord s = Score(possibleSolution, guessWord);
        wordScores.insert(s);
    }
    return wordScores;
}
