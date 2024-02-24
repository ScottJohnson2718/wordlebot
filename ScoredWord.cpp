
#include "ScoredWord.h"
#include <sstream>
#include <algorithm>
#include <array>
#include <unordered_map>

// LetterCount is the count of each letter in the alphabet for a given word
using LetterCount = std::array<int, 26>;

void CreateLetterTable(const std::string &word, LetterCount &letterTable)
{
    std::fill(letterTable.begin(), letterTable.end(), 0);
    for (int i = 0; i < word.size(); ++i)
    {
        int idx = tolower(word[i]) - 'a';
        letterTable[idx]++;
    }
}

ScoredWord Score(const std::string& solution, const std::string& guess)
{
    LetterCount solutionLT;
    CreateLetterTable(solution, solutionLT);

    ScoredWord score;
    for (int i = 0; i < solution.size(); ++i)
    {
        if (guess[i] == solution[i])
        {
            int idx = guess[i] - 'a';
            score.Set(i, Correct);
            solutionLT[idx]--;
        }
    }
    for (int i = 0; i < solution.size(); ++i)
    {
        int idx = guess[i] - 'a';

        if (score.Get(i) != Correct)
        {
            if (solutionLT[idx] > 0)
            {
                score.Set(i, CorrectNotHere);
                solutionLT[idx]--;
            }
            else
            {
                score.Set(i, NotPresent);
            }
        }
    }
    return score;
}

//ScoredWord Score(const std::string& solution, const std::string& guess)
//{
//    ScoredWord score;
//    uint32_t solutionMask = ComputeMask(solution);
//
//    for (int i = 0; i < solution.size(); ++i)
//    {
//        if (guess[i] == solution[i])
//        {
//            score.Set(i, Correct);
//        }
//        else
//        {
//            uint32_t charMask = 1 << (guess[i] - 'a');
//
//            if (charMask & solutionMask)
//            {
//                score.Set(i, CorrectNotHere);
//            }
//            else
//            {
//                score.Set(i, NotPresent);
//            }
//        }
//    }
//    return score;
//}

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

size_t ScoreGroupCount(const std::string& guessWord, std::vector<std::string>& solutionWords)
{
    // This guess separates the remaining solutions into groups according to a common board score
    std::unordered_map<ScoredWord, size_t, ScoredWordHash, ScoredWordEqual> groups;
    for (auto const& possibleSolution : solutionWords)
    {
        ScoredWord s = Score(possibleSolution, guessWord);
        std::string str = s.ToString(guessWord);
        groups[s]++;
    }

    return groups.size();
}

std::vector<std::string> ScoreGroup(
    const std::string& guessWord,
    const ScoredWord& score,
    std::vector < std::string>& solutionWords)
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
