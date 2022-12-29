#include "string_processing.h"
#include <algorithm>


std::vector<std::string> SplitIntoWords(const std::string &text) 
{
    std::vector<std::string> words;
    std::string word;
    for (const char symbol : text) {
        if (symbol == ' ') {
            words.push_back(word);
            word = "";
        } else
            word += symbol;
    }

    words.emplace_back(word);
    return words;
}