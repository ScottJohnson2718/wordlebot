#pragma once

#include <vector>
#include <string>

#include "WordQuery.h"

class WordTree
{
public:
    struct Node
    {
        uint8_t ch;     // the character that this node represents such as 'a' or 'z'
        uint32_t childMask;     // a set of bits that represent the chars of the children
        std::vector<Node*> children;    // the child nodes

        Node* Child(char ch) const
        {
            uint32_t charMask = 1 << (ch - 'a');
            if (childMask & charMask)
            {
                for (auto child: children)
                {
                    if (child->ch == ch) {
                        return child;
                    }
                }
            }
            return nullptr;
        }
    };

    WordTree()
    : root_(new Node())
    {
        // There is at least one node in the tree always
    }
    WordTree(const std::vector<std::string> &words)
            : root_(new Node())
    {
        InsertWords(words);
    }
    ~WordTree()
    {
        delete root_;
        root_ = nullptr;
    }
    void InsertWord( const std::string &word);
    void InsertWords(const std::vector<std::string>& words);

    std::vector<std::string> Satisfies( const WordQuery &query) const;
    size_t SatisfiesCount( const WordQuery &query) const;
    void SatisfiesRecurse( const Node *node,
                           const WordQuery& query,
                           int depth,
                           uint32_t wordMask,
                           std::string& charStack,
                           std::vector<std::string> &satisfyingWords) const;

    Node* root_;
};

void WordTree::SatisfiesRecurse(
        const Node *node,
        const WordQuery& query,
        int depth,
        uint32_t wordMask,
        std::string& charStack,
        std::vector<std::string> &satisfyingWords) const
{
    if (depth >= 0) {
        charStack.push_back(node->ch);
        wordMask |= 1 << (node->ch - 'a');
    }
    else {
        depth++;
    }

    if (depth == query.n || node->children.empty())
    {
        // leaf node
        // we have a word that matches so far. Check that it has the required
        // characters somewhere in it.
        if ((wordMask & query.mustContain) == query.mustContain)
            satisfyingWords.push_back(charStack);
    }
    else
    {
        // Not a leaf
        if (query.correct[depth] != '.' )
        {
            // This char is constrained to be the exact given correct character
            auto child = node->Child(query.correct[depth]);
            if (child != nullptr)
                SatisfiesRecurse( child, query, depth + 1, wordMask, charStack, satisfyingWords);
        }
        else {
            for (auto child: node->children) {
                char ch = child->ch;
                uint32_t childMask = 1 << (ch - 'a');

                 if ((query.cantContain[depth] & childMask) == 0) {
                    SatisfiesRecurse(child, query, depth + 1, wordMask,
                                     charStack, satisfyingWords);
                }
            }
        }
    }
    charStack.pop_back();
}

std::vector<std::string> WordTree::Satisfies(const WordQuery& query) const
{
    std::string charStack;
    std::vector<std::string> satisfyingWords;

    SatisfiesRecurse( root_, query, -1, 0, charStack, satisfyingWords);
    return satisfyingWords;
}

size_t WordTree::SatisfiesCount(const WordQuery& query) const
{
    std::string charStack;
    std::vector<std::string> satisfyingWords;

    SatisfiesRecurse( root_, query, -1, 0, charStack, satisfyingWords);
    return satisfyingWords.size();
}

void WordTree::InsertWord(const std::string &word)
{
    Node *n = root_;

    for (int depth = 0; depth < word.size(); ++depth)
    {
        char ch = tolower(word[depth]);
        bool found = false;
        uint32_t charMask = 1 << (ch - 'a');
        if (n->childMask & charMask)
        {
            for (auto child: n->children)
            {
                if (child->ch == ch) {
                    n = child;
                    found = true;
                }
            }
        }
        if (!found)
        {
            auto newChild = new Node();
            newChild->ch = ch;
            newChild->childMask = 0;
            n->childMask |= charMask;
            n->children.push_back(newChild);
            n = newChild;
        }
    }
}

void WordTree::InsertWords(const std::vector<std::string> &words)
{
    for (const auto &w : words)
    {
        InsertWord(w);
    }
}
