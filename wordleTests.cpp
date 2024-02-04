
#include "gtest/gtest.h"


#include "Wordle.h"
#include "WordQuery.h"
#include "Board.h"
#include "Strategy.h"
#include "WordleBot.h"

#include <algorithm>
#include <execution>

std::filesystem::path dictPath("../..");

TEST( Wordle, Joker)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    int guessCount = bot.SolvePuzzle("joker", "stale", true);
    EXPECT_LE( guessCount, 6);
}

TEST( Wordle, Globe)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(false, 5, dictPath, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    int guessCount = bot.SolvePuzzle("globe", "stale", true);
    EXPECT_NE( guessCount, 0);
    EXPECT_LE( guessCount, 6);
}

TEST( Wordle, Abode)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);

    EntropyStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    int guessCount = bot.SolvePuzzle("abode", "stale", true);
    EXPECT_NE( guessCount, 0);
    EXPECT_LE( guessCount, 6);
}

// Word was votes
// wordlebot guessed " stale tents boric vapid votes
// Could it do better? tents doesn't seem like a good guess to me.
TEST( Wordle, Votes)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(false, 5, dictPath, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    int guessCount = bot.SolvePuzzle("votes", "stale", true);
    EXPECT_NE( guessCount, 0);
    EXPECT_LE( guessCount, 6);
}

TEST(Wordle, Entropy)
{
    std::vector<std::string> solutionWords;

    solutionWords.push_back("bikes");
    solutionWords.push_back("hikes");
    solutionWords.push_back("likes");

    WordQuery query(5);
    query.SetMustContain('i');
    query.SetMustContain('k');
    query.SetMustContain('e');
    query.SetMustContain('s');

    auto freqs = charFrequency(solutionWords, query);

    // The conclusion was that entropy alone does not make the correct choice.
    // The best guess is "bahil" because it differentiates between the choices
    // but it has less entropy than the others because the 'a' counts for zero.
    float e0 = entropy("bahil", freqs);
    float e1 = entropy("bikes", freqs);
    float e2 = entropy( "hikes", freqs);
    std::cout << "Entropy for guess bahil: " << e0 << std::endl;
    std::cout << "Entropy for guess bikes: " << e1 << std::endl;
    std::cout << "Entropy for guess hikes: " << e2 << std::endl;

    EXPECT_GT( e0, e1);
    EXPECT_GT( e0, e2);
}

float TestWords(std::vector<std::string> &solutionWords, const std::vector < std::string>& guessingWords,
                const std::string &openingGuess, Strategy &strategy)
{
    Bot bot(guessingWords, solutionWords, strategy);

    int guesses = 0;
#ifdef WIN32
    auto policy = std::execution::par_unseq;
#else
    auto policy = std::execution::seq;
#endif
    std::for_each(policy, solutionWords.begin(), solutionWords.end(),
                  [&guesses, &bot, &openingGuess](const std::string& word)
                  {
                      guesses += bot.SolvePuzzle(word, openingGuess, true);
                  });

    std::cout << "Total guesses for : "<< openingGuess << " " << guesses << std::endl;
    std::cout << "Ave guesses : " << openingGuess << " " << (double) guesses / (double) solutionWords.size() << std::endl;
    return (double)guesses / (double)solutionWords.size();
}

TEST( Wordle, Strategy)
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy blended(guessingWords, 10);
    EntropyStrategy entropy(guessingWords, 10);
    SearchStrategy search(guessingWords, 10);

    float aveGuesses0 = TestWords(solutionWords, guessingWords, "arose", entropy);
    float aveGuesses1 = TestWords(solutionWords, guessingWords, "arose", blended);
    float aveGuesses2 = TestWords(solutionWords, guessingWords, "arose", search);

    std::cout << "Ave guesses for entropy only strategy: " << aveGuesses0 << std::endl;
    std::cout << "Ave guesses for blended strategy: " << aveGuesses1 << std::endl;
    std::cout << "Ave guesses for search strategy: " << aveGuesses2 << std::endl;

    // With NYT dictionary and slate as the starting word...
    // Ave guesses for entropy only strategy: 3.8013
    // Ave guesses for blended strategy: 3.65788
    // Ave guesses for search strategy: 3.65313

    // NYT with arose as starting word
    // Ave guesses for entropy only strategy: 3.8216
    // Ave guesses for blended strategy: 3.68251
    // Ave guesses for search strategy: 3.67732
}

TEST( Wordle, TestOpeningWords)
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy strategy(guessingWords, 10);

    TestWords(solutionWords, guessingWords, "slate", strategy);  // 3.65 guesses per word
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