
#include "gtest/gtest.h"


#include "Wordle.h"
#include "WordQuery.h"
#include "Board.h"
#include "Strategy.h"
#include "WordleBot.h"

#include <algorithm>
#include <execution>

TEST( Wordle, Joker)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    bot.SolvePuzzle("joker", "stale", true);
}

TEST( Wordle, Globe)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(false, 5, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    bot.SolvePuzzle("globe", "stale", true);
}

// Word was votes
// wordlebot guessed " stale tents boric vapid votes
// Could it do better? tents doesn't seem like a good guess to me.
TEST( Wordle, Solve)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    bot.SolvePuzzle("votes", "stale", true);
}

TEST(Wordle, Entropy)
{
    std::vector<std::string> solutionWords;

    solutionWords.push_back("bikes");
    solutionWords.push_back("hikes");
    solutionWords.push_back("likes");

    auto freqs = charFrequency(solutionWords);

    freqs = removeKnownChars(freqs, ".ikes");

    // The conclusion was that entropy alone does not make the correct choice.
    // The best guess is "bahil" because it differentiates between the choices
    // but it has less entropy than the others because the 'a' counts for zero.
    std::cout << "Entropy for guess bahil: " << entropy("bahil", freqs) << std::endl;
    std::cout << "Entropy for guess bikes: " << entropy("bikes", freqs) << std::endl;
    std::cout << "Entropy for guess hikes: " << entropy("hikes", freqs) << std::endl;
}

float TestWords(std::vector<std::string> &solutionWords, const std::vector < std::string>& guessingWords,
                const std::string &openingGuess, Strategy &strategy)
{
    Bot bot(guessingWords, solutionWords, strategy);

    int guesses = 0;
#ifdef WIN32
    std::for_each(std::execution::par_unseq, solutionWords.begin(), solutionWords.end(),
                  [&guesses, &bot, &openingGuess](const std::string& word)
                  {
                      guesses += bot.SolvePuzzle(word, openingGuess, false);
                  });
#else
    for (auto const& word : solutionWords)
    {
        guesses += bot.SolvePuzzle( word, openingGuess, false );
    }
#endif
    std::cout << "Total guesses for : "<< openingGuess << " " << guesses << std::endl;
    std::cout << "Ave guesses : " << openingGuess << " " << (double) guesses / (double) solutionWords.size() << std::endl;
    return (double)guesses / (double)solutionWords.size();
}

TEST( Wordle, Strategy)
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy blended(guessingWords, 10);
    EntropyStrategy entropy(guessingWords, 10);

    float aveGuessesToSolve;

    aveGuessesToSolve = TestWords(solutionWords, guessingWords, "slate", entropy);
    std::cout << "Ave guesses for entropy only strategy: " << aveGuessesToSolve << std::endl;
    aveGuessesToSolve = TestWords(solutionWords, guessingWords, "slate", blended);
    std::cout << "Ave guesses for blended strategy: " << aveGuessesToSolve << std::endl;
}

TEST( Wordle, TestOpeningWords)
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy strategy(guessingWords, 10);

    TestWords(solutionWords, guessingWords, "slate", strategy);  // 9659
    //TestWords(solutionWords, "stoae");
    //TestWords(solutionWords, "arose");  // 9640
    //TestWords(solutionWords, "limen");  // 9825
    //TestWords(solutionWords, "daisy");  // 9822
}

//void TestWordTree()
//{
//    std::vector< std::string> words;
//    words.push_back("slate");
//    words.push_back("daisy");
//    words.push_back("adieu");
//
//    WordTree tree(words);
//    WordQuery query(5);
//    query.SetMustContain('a');
//    query.SetCantContain(0,'d');
//
//    auto w = tree.Satisfies(query);
//    for (auto ww : w)
//    {
//        std::cout << ww << std::endl;
//    }
//}