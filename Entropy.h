
#include <vector>
#include <string>

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
            totalChars++;
        }
    }
    std::array<float, 26> freq;
    for (size_t i = 0; i < counts.size(); ++i)
    {
        freq[i] = ((double) counts[i]) / (double) totalChars;
    }
    return freq;
}

float entropy( const std::string &word, const std::array<float, 26> &freqs )
{
    float e = 0.0f;
    uint32_t charMask = 0;

    // Only count a character in the word once
    for (char ch : word)
    {
        int charIndex = ch - 'a';

        if ((charMask & (1 << charIndex)) == 0) {
            float Pi = freqs[ch - 'a'];
            e += Pi * std::log2(Pi);
            charMask |= 1 << charIndex;
        }
    }
    return -e;
}