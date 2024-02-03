
#include "Entropy.h"

std::array<float, 26> charFrequency(const std::vector<std::string> &words)
{
    std::array<int, 26> counts;
    counts.fill(0);
    int totalChars = 0;
    for (auto const &word : words)
    {
        for (char ch : word)
        {
            char lower = tolower(ch);
            counts[ lower - 'a']++;
        }
        totalChars += word.size();
    }
    std::array<float, 26> freq;
    for (size_t i = 0; i < counts.size(); ++i)
    {
        freq[i] = ((double) counts[i]) / (double) totalChars;
    }
    return freq;
}

std::array<float, 26> removeKnownChars(const std::array<float, 26> &freq, const std::string &contains)
{
    std::array<float, 26> f(freq);
    for (char ch : contains)
    {
        if (ch != '.')
        {
            int charIndex = tolower(ch) - 'a';
            f[charIndex] = 0.0f;
        }
    }
    return f;
}

float entropy( const std::string &word, const std::array<float, 26> &freqs )
{
    float e = 0.0f;
    uint32_t charMask = 0;

    // Only count a character in the word once
    for (char ch : word)
    {
        int charIndex = tolower(ch) - 'a';

        if ((charMask & (1 << charIndex)) == 0) {
            float Pi = freqs[charIndex];
            if (Pi > 0.0f)
            {
                e += Pi * std::log2(Pi);
            }
            charMask |= 1 << charIndex;
        }
    }
    return -e;
}
