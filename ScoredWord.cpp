
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

ScoredWord Score(const char* pSolution, const char* pGuess, int n = 5)
{
    ScoredWord score;

    // Track which positions have been processed using bit flags
    unsigned int solutionUsed = 0;
    unsigned int guessProcessed = 0;

    // First pass: mark exact matches (green)
    for (int i = 0; i < n; ++i)
    {
        if (pGuess[i] == pSolution[i])
        {
            score.Set(i, Correct);
            solutionUsed |= (1u << i);
            guessProcessed |= (1u << i);
        }
    }

    // Second pass: check for letters in wrong positions (yellow/gray)
    for (int i = 0; i < n; ++i)
    {
        if (guessProcessed & (1u << i))
            continue;

        // Look for this guess letter in unused solution positions
        bool found = false;
        for (int j = 0; j < n; ++j)
        {
            if (!(solutionUsed & (1u << j)) && (pGuess[i] == pSolution[j]))
            {
                score.Set(i, CorrectNotHere);
                solutionUsed |= (1u << j);
                found = true;
                break;
            }
        }

        if (!found)
        {
            score.Set(i, NotPresent);
        }
    }

    return score;
}

ScoredWord Score(const std::string& solution, const std::string& guess)
{
    return Score(solution.c_str(), guess.c_str());
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

std::ostream& printColored(std::ostream &str, const std::string &guess, const ScoredWord &scored)
{
    for (int i = 0; i < guess.size(); ++i)
    {
        CharScore cs = scored.Get(i);
        char ch = guess[i];

        switch (cs)
        {
        case NotPresent:
            // White text on gray background
            str << "\033[37;100m" << ch;
            break;
        case Correct:
            // White text on green background
            str << "\033[37;42m" << ch;
            break;
        case CorrectNotHere:
            // White text on yellow background
            str << "\033[37;43m" << ch;
            break;
        }

        // Reset formatting
        str << "\033[0m";
    }
    return str;
}

std::string ScoredWord::ToStringColored(const std::string& guess) const
{
    std::stringstream str;
    printColored(str, guess, *this);
    return str.str();
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
        //std::string str = s.ToString(guessWord);
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
        //std::string str = s.ToString(guessWord);      // useful for debugging
        if (score == s)
        {
            remaining.push_back(possibleSolution);
        }
    }
    return remaining;
}

std::vector<std::string> PruneSearchSpace(
    const std::string& guessWord,
    const ScoredWord& score,
    const std::vector < std::string>& solutionWords)
{
    return ScoreGroup(guessWord, score, solutionWords);
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

WordScorer::WordScorer(const std::vector<std::string>& solutionWords, const std::vector<std::string>& guessWords)
{
    for (const auto &solutionWord : solutionWords)
    {
        for (const auto &guessWord : guessWords)
        {
            std::string joined = solutionWord + guessWord;
            _map[joined] = Score(solutionWord, guessWord);
        }
    }
}

ScoredWord WordScorer::Score(const std::string &solutionWord, const std::string &guessWord) const
{
    std::string joined = solutionWord + guessWord;
    auto iter = _map.find(joined);
    if (iter != _map.end())
        return iter->second;
    return ScoredWord();
}