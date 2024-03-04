
#include "gtest/gtest.h"


#include "Wordle.h"
#include "WordQuery.h"
#include "Board.h"
#include "Strategy.h"
#include "WordleBot.h"
#include "ScoredWord.h"
#include "LookaheadStrategy.h"

#include <algorithm>
#ifndef __APPLE__
// Apple CLANG does not have <execution> as of 2/2024
#include <execution>
#endif

#ifdef __APPLE__
std::filesystem::path dictPath("..");
#else
std::filesystem::path dictPath("D:/work/git_repos/wordlebot");
#endif

namespace WordleBotTests
{
TEST(Wordle, GuessOnFirstTry)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    bool loaded = LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    ASSERT_TRUE(loaded);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy, true);

    int guessCount = bot.SolvePuzzle("joker", "joker");
    EXPECT_EQ(guessCount, 1);
}

// NYT wordlebot gets "crane truce price"
TEST(Wordle, Price)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    bool loaded = LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    ASSERT_TRUE(loaded);

    {
        BlendedStrategy strategy(guessingWords, 10);
        Bot bot(guessingWords, solutionWords, strategy, true);

        int guessCount = bot.SolvePuzzle("price", "stale");
        EXPECT_GT(guessCount, 0);
        EXPECT_LE(guessCount, 4);
    }

    {
        EntropyStrategy entropy(guessingWords, 10);
        LookaheadStrategy strategy(entropy, guessingWords, 10);
        Bot bot(guessingWords, solutionWords, strategy, true);

        int guessCount = bot.SolvePuzzle("price", "stale");
        EXPECT_GT(guessCount, 0);
        EXPECT_LE(guessCount, 4);
    }
}

TEST(Wordle, Oozed)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    bool loaded = LoadDictionaries(false, 5, dictPath, solutionWords, guessingWords);
    ASSERT_TRUE(loaded);

    {
        std::cout << "Blended" << std::endl;
        BlendedStrategy strategy(guessingWords, 10);
        Bot bot(guessingWords, solutionWords, strategy, true);

        int guessCount = bot.SolvePuzzle("oozed", "stale");
        EXPECT_GT(guessCount, 0);
        EXPECT_LE(guessCount, 6);
    }

    {
        std::cout << "Lookahead with Entropy as the sub" << std::endl;
        EntropyStrategy entropy(guessingWords, 10);
        LookaheadStrategy strategy(entropy, guessingWords, 10);
        Bot bot(guessingWords, solutionWords, strategy, true);

        int guessCount = bot.SolvePuzzle("oozed", "stale");
        EXPECT_GT(guessCount, 0);
        EXPECT_LE(guessCount, 6);
    }

    {
        std::cout << "Score Grouping" << std::endl;
        ScoreGroupingStrategy grouping(guessingWords, 10);
        Bot bot(guessingWords, solutionWords, grouping, true);

        int guessCount = bot.SolvePuzzle("oozed", "stale");
        EXPECT_GT(guessCount, 0);
        EXPECT_LE(guessCount, 6);
    }
}

TEST(Score, Scram)
{
    auto scoredWord = Score("scram", "trace");
    std::stringstream str;
    print(str, "trace", scoredWord);
    EXPECT_EQ(str.str(), std::string(".RAC."));
}

TEST(RemoveDups, Test1)
{
    std::vector< ScoredGuess > guesses;

    guesses.push_back(ScoredGuess{ "octary", 1.89719f });
    guesses.push_back(ScoredGuess{ "costar", 1.93545f });
    guesses.push_back(ScoredGuess{ "octary", 1.89719f });
    guesses.push_back(ScoredGuess{ "octary", 1.89719f });

    RemoveDuplicateGuesses(guesses);

    EXPECT_EQ(guesses.size(), 2);
}

// Double letter scoring. Based on this source.
// https://nerdschalk.com/wordle-same-letter-twice-rules-explained-how-does-it-work/
TEST(Scoring, DoubleLetterScoring)
{
    // Guess has double letter 'e' but solution only has one 'e'
    auto r1 = ScoreString("abbey", "keeps");
    EXPECT_EQ(r1, ".E...");
    auto r2 = ScoreString("keeps", "abbey");
    EXPECT_EQ(r2, "...E.");

    // Guess has double letter 'b' and solution has double letter 'b'. Answer should
    // reflect two b's in the solution word
    auto r3 = ScoreString("abbey", "babes");
    EXPECT_EQ(r3, "BAbe.");
    auto r4 = ScoreString("abbey", "abled");
    EXPECT_EQ(r4, "ab.e.");
    // The first b in the guess is present but in the wrong place. But later in the scan from left
    // to right it should discover a single b in the correct place. This means that scoring is in two
    // passes over the solution.
    auto r5 = ScoreString("aebde", "bibed");
    EXPECT_EQ(r5, "..bED");

    //auto r3 = ScoreString("lully", "able");
    //auto r4 = ScoreString("lully", "lilly");
}

TEST(WordQuery, First)
{
    Board board(5);
    board.PushScoredGuess("slate", ScoredWord(".L..."));
    auto query = board.GenerateQuery();

    // Query should pass words that have an L and no 's', 'a', 't', or 'e'.
    EXPECT_TRUE(query.Satisfies("build"));   // passes. Has an 'l' and no bad chars
    EXPECT_FALSE(query.Satisfies("slots"));  // has an 's' so no
    EXPECT_FALSE(query.Satisfies("apple"));  // has an 'a' so no

    board.PushScoredGuess("grody", ScoredWord("...D."));
    EXPECT_TRUE(query.Satisfies("build"));

    board.PushScoredGuess("caput", ScoredWord("...U."));
    EXPECT_TRUE(query.Satisfies("build"));
    EXPECT_FALSE(query.Satisfies("group")); // no 'g's allowed or r's
}

TEST(WordQuery, DoubleLetter)
{
    Board board(5);
    board.PushScoredGuess("babes", "BAbe.");
    auto query = board.GenerateQuery();

    // Query should pass words that have an L and no 's', 'a', 't', or 'e'.
    EXPECT_TRUE(query.Satisfies("abbey"));   // passes. This is actually the solutions
    EXPECT_FALSE(query.Satisfies("slots"));  // has an 's' so no
    EXPECT_FALSE(query.Satisfies("apple"));   // fails. No 'b's let alone is the right place
    EXPECT_FALSE(query.Satisfies("cable"));   // fails. No 'b' in correct place.
}

TEST(WordQuery, DoubleLetter2)
{
    // slate "SLAt." shuns "S...s"
    Board board(5);
    board.PushScoredGuess("shuns", ScoredWord("S...s"));    // solution was "lasts"
    auto query = board.GenerateQuery();

    EXPECT_TRUE(query.Satisfies("lasts"));
    EXPECT_FALSE(query.Satisfies("malts")); // minimum of two 's's.
}

class LionStudiosFiveLetter : public testing::Test
{
protected:

    void SetUp() override
    {
        LoadDictionaries(false, 5, dictPath, solutionWords, guessingWords);

        scoreGrouping = new ScoreGroupingStrategy(guessingWords, 10);
        search = new SearchStrategy(guessingWords, 10);
        entropy = new EntropyStrategy(guessingWords, 10);
        lookahead = new LookaheadStrategy( *scoreGrouping, guessingWords, 10);

        strategies.push_back(scoreGrouping);
        strategies.push_back(search);
        strategies.push_back(entropy);
    }

    void TearDown() override
    {
        delete scoreGrouping;
        delete search;
        delete entropy;
        delete lookahead;
    }

    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    ScoreGroupingStrategy* scoreGrouping;
    SearchStrategy* search;
    EntropyStrategy* entropy;
    LookaheadStrategy* lookahead;

    std::vector< Strategy*> strategies;
};

class NewYorkTimesFiveLetter : public testing::Test
{
protected:
    void SetUp() override
    {
        LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    }

    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;
};

TEST(WordQuery, TempsBug)
{
    std::string actualSolution = "temps";
    std::string guess = "feels";
    std::vector<std::string> solutions = { "teems", "temes", "temps" };

    Board board(5);
    ScoredWord sw = Score(actualSolution, guess);
    std::string str = sw.ToString(guess); // ".e..s"

    // "feels" partitions the solutions into 3 groups. That is good.
    // "teems" -> ".ee.s"
    // "temes" -> ".eE.s"
    // "temps" -> ".e..s"
    size_t largestGroup(0);
    size_t groups = ScoreGroupCount(guess, solutions, largestGroup);
    auto remaining = ScoreGroup(guess, sw, solutions);

    // But guessing 'feels' doesn't reduce the search space to 1. It just
    // reduces it to 2.
    board.PushScoredGuess("feels", sw);
    auto query = board.GenerateQuery();
    remaining = PruneSearchSpace(query, solutions);

    EXPECT_FALSE(query.Satisfies("teems"));  // position 2 can't be an 'e'
    EXPECT_FALSE(query.Satisfies("temes"));  // must have exactly one 'e'
    EXPECT_TRUE(query.Satisfies("temps"));   // passes 

    EXPECT_LT(remaining.size(), 3);
}

TEST(Score, Local)
{
    auto scoredWord = Score("vocal", "local");
    std::string str = scoredWord.ToString("local");
    Board board(5);
    board.PushScoredGuess("local", scoredWord);
    auto query = board.GenerateQuery();
    EXPECT_FALSE(query.Satisfies("local"));
}

// At one point, this interactive session was returning the guess "eager" 
// over and over
TEST_F(LionStudiosFiveLetter, Temps)
{
    Board board(5);

    board.PushScoredGuess("slate", ScoredWord("S..TE"));
    board.PushScoredGuess("meres", ScoredWord("Me..s"));

    WordQuery query = board.GenerateQuery();

    std::vector<std::string>  remaining = PruneSearchSpace(query, solutionWords);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (int idx = 0; idx < remaining.size(); ++idx)
    {
        std::cout << remaining[idx] << " ";
        if ((idx % 15) == 0 && (idx > 0))
        {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    std::cout << "Best guesses " << std::endl;
    auto bestGuesses = scoreGrouping->BestGuesses(board, remaining);
    for (const auto& g : bestGuesses)
    {
        std::cout << g.first << " : " << g.second << std::endl;
    }

    auto bestGuess = scoreGrouping->BestGuess(board, remaining);
    

    query = board.GenerateQuery();
    remaining = PruneSearchSpace(query, remaining);
    int foo = 4;
}

TEST_F(LionStudiosFiveLetter, TempsSinglePuzzle)
{
    Bot bot(guessingWords, solutionWords, *scoreGrouping, true);
    int guessCount = bot.SolvePuzzle("temps", "slate");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);
}


TEST_F(LionStudiosFiveLetter, SinglePuzzles)
{
    std::vector<std::string> words = { "agony", "votes", "abode", "joker", "globe"};

    for (Strategy* strat : strategies)
    {
        for (auto word : words)
        {
            Bot bot(guessingWords, solutionWords, *strat, true);
            int guessCount = bot.SolvePuzzle(word, "slate");
            EXPECT_NE(guessCount, 0);
            EXPECT_LE(guessCount, 6);
        }
    }
}

TEST_F(LionStudiosFiveLetter, Votes)
{
    Bot bot(guessingWords, solutionWords, *scoreGrouping, true);
    int guessCount = bot.SolvePuzzle("votes", "slate");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);
}

TEST_F(LionStudiosFiveLetter, Votes_Lookahead)
{
    Bot bot(guessingWords, solutionWords, *lookahead, true);
    int guessCount = bot.SolvePuzzle("votes", "slate");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);
}

TEST_F(NewYorkTimesFiveLetter, SinglePuzzlesEntropyStrategy)
{
    EntropyStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy, true);

    int guessCount = bot.SolvePuzzle("agony", "slate");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("votes", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("abode", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("joker", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);
}

TEST_F(NewYorkTimesFiveLetter, SinglePuzzlesScoreGroupingStrategy)
{
    ScoreGroupingStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy, true);

    int guessCount = bot.SolvePuzzle("agony", "slate");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("votes", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("abode", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);

    guessCount = bot.SolvePuzzle("joker", "stale");
    EXPECT_NE(guessCount, 0);
    EXPECT_LE(guessCount, 6);
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
    float e2 = entropy("hikes", freqs);
    std::cout << "Entropy for guess bahil: " << e0 << std::endl;
    std::cout << "Entropy for guess bikes: " << e1 << std::endl;
    std::cout << "Entropy for guess hikes: " << e2 << std::endl;

    EXPECT_GT(e0, e1);
    EXPECT_GT(e0, e2);
}

float TestWords(std::vector<std::string>& solutionWords, const std::vector < std::string>& guessingWords,
    const std::string& openingGuess, Strategy& strategy)
{
    Bot bot(guessingWords, solutionWords, strategy, true);

    int guesses = 0;
#ifndef __APPLE__
    auto policy = std::execution::par_unseq;
    std::for_each(policy, solutionWords.begin(), solutionWords.end(),
        [&guesses, &bot, &openingGuess](const std::string& word)
        {
            guesses += bot.SolvePuzzle(word, openingGuess);
        });
#else
    std::for_each(solutionWords.begin(), solutionWords.end(),
        [&guesses, &bot, &openingGuess](const std::string& word)
        {
            guesses += bot.SolvePuzzle(word, openingGuess);
        });
#endif

    std::cout << "Total guesses for : " << openingGuess << " " << guesses << std::endl;
    std::cout << "Ave guesses : " << openingGuess << " " << (double)guesses / (double)solutionWords.size() << std::endl;
    return (double)guesses / (double)solutionWords.size();
}

TEST(BatchSolve, Strategy)
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
    //LookaheadStrategy lookahead(entropy, guessingWords, 10);
    // LookaheadStrategy lookahead(search, guessingWords, 10);
    ScoreGroupingStrategy groups(guessingWords, 10);

    std::string opening = "slate";
    float aveGuesses0 = TestWords(solutionWords, guessingWords, opening, entropy);
    float aveGuesses1 = TestWords(solutionWords, guessingWords, opening, blended);
    //float aveGuesses2 = TestWords(solutionWords, guessingWords, opening, search);
    float aveGuesses3 = TestWords(solutionWords, guessingWords, opening, groups);


    std::cout << "Ave guesses for entropy strategy: " << aveGuesses0 << std::endl;
    std::cout << "Ave guesses for blended strategy: " << aveGuesses1 << std::endl;
    //std::cout << "Ave guesses for search strategy: " << aveGuesses2 << std::endl;
    std::cout << "Ave guesses for groups strategy: " << aveGuesses3 << std::endl;

    // With NYT dictionary and slate as the starting word...
    // Ave guesses for entropy only strategy: 3.8013
    // Ave guesses for blended strategy: 3.65788
    // Ave guesses for search strategy: 3.65313

    // After handling a remaining size of 2 differently...
    // Ave guesses for entropy only strategy: 3.65616
    // Ave guesses for blended strategy: 3.53045
    // Ave guesses for search strategy: 3.527
    // NYT with arose as starting word
    // Ave guesses for entropy only strategy: 3.68812
    // Ave guesses for blended strategy: 3.58445
    // Ave guesses for search strategy: 3.57754

    // Feb 18 2024 . Just wrote lookahead strategy.
    // It uses entropy strategy to prune so it is no better than entropy.
    // Ave guesses for entropy only strategy : 3.66453
    // Ave guesses for blended strategy : 3.53573
    // Ave guesses for search strategy : 3.53316
    // Ave guesses for lookahead strategy : 3.60847

    // I adjusted the pruning level from the top 100 entropy words to the top 500 and the "oozed" solution droppped to 6 from 7.
    // Then I changed the lookahead strategy to return its substrategy (entropy) if the remaining solutions are greater than 100
    // Ave guesses : slate 3.53958
    // Very close to the SearchStrategy without looking ahead all the way to the end.

    // Wrote strategy ScoreGrouping. It is winning by a tiny bit. But I changed the dictionaries so the previous data is out of date.
    // Rerun
    // Ave guesses for entropy only strategy: 3.6564
    // Ave guesses for blended strategy : 3.52803
    // Ave guesses for search strategy : 3.5246
    // Ave guesses for groups strategy : 3.4908

    // 2/24/2024 - It took 85 minutes to run
    // Ave guesses for entropy strategy : 3.65783
    // Ave guesses for blended strategy : 3.53636
    // Ave guesses for groups strategy : 3.52267
}

TEST(BatchSolve, TestOpeningWords)
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    //SearchStrategy search(guessingWords, 10);
    //LookaheadStrategy lookahead(search, guessingWords, 10);
    EntropyStrategy entropy(guessingWords, 10);
    LookaheadStrategy lookahead(entropy, guessingWords, 10);
    ScoreGroupingStrategy grouping(guessingWords, 10);

    TestWords(solutionWords, guessingWords, "slate", grouping);
    //TestWords(solutionWords, guessingWords, "slate", lookahead);  
    //TestWords(solutionWords, "stoae");
    //TestWords(solutionWords, "arose");  // 9640
    //TestWords(solutionWords, "limen");  // 9825
    //TestWords(solutionWords, "daisy");  // 9822
}

TEST(Wordle, SimpleSearch)
{
    // Guessing words and solution words are the same for this test
    std::vector<std::string> solutionWords;

    solutionWords.push_back("bikes");
    solutionWords.push_back("hikes");
    solutionWords.push_back("likes");
    solutionWords.push_back("slate");

    EntropyStrategy entropy(solutionWords, 10);
    Bot bot(solutionWords, solutionWords, entropy, true);
    Bot::SearchResult result;
    Board board(5);
    bot.Search(board, solutionWords, result);

}

TEST(Wordle, SimpleLookahead)
{
    // Guessing words and solution words are the same for this test
    std::vector<std::string> solutionWords;

    solutionWords.push_back("bikes");
    solutionWords.push_back("hikes");
    solutionWords.push_back("likes");
    solutionWords.push_back("slate");

    EntropyStrategy subStrategy(solutionWords, 10);
    LookaheadStrategy lookahead(subStrategy, solutionWords, 10);
    Board board;

    ScoredGuess bestGuess = lookahead.BestGuess(board, solutionWords);
}

TEST(Wordle, SolveUsingLookahead)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    bool loaded = LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    ASSERT_TRUE(loaded);

    EntropyStrategy subStrat(guessingWords, 200);
    LookaheadStrategy lookaheadStrat(subStrat, guessingWords, 10);
    Bot bot(guessingWords, solutionWords, lookaheadStrat, true);

    int guessCount = bot.SolvePuzzle("ridge", "slate");
    EXPECT_GT(guessCount, 0);
    EXPECT_LE(guessCount, 6);
}

TEST(Wordle, SolveUsingScoreGrouping)
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    bool loaded = LoadDictionaries(true, 5, dictPath, solutionWords, guessingWords);
    ASSERT_TRUE(loaded);

    ScoreGroupingStrategy grouping(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, grouping, true);

    int guessCount = bot.SolvePuzzle("ridge", "slate");
    EXPECT_GT(guessCount, 0);
    EXPECT_LE(guessCount, 6);
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

} // namespace