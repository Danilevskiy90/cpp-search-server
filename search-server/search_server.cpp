#include "search_server.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

//добавить документ
void SearchServer::AddDocument(int document_id, const std::string& document, 
                    DocumentStatus status, const std::vector<int>& ratings) 
{
    if ((document_id < 0) || (document_info.count(document_id)) || (!IsValidWord(document)))
    {
        throw std::invalid_argument{"Невозможно добавить документ"};
    }

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    document_info[document_id].status = status;
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) 
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    //document_ratings_.emplace(document_id, ComputeAverageRating(ratings));
    document_info[document_id].rating = ComputeAverageRating(ratings);
}

int SearchServer::GetDocumentCount() const
{
    return document_info.size();
}

int SearchServer::GetDocumentId(int index) const 
{
    //int ind = index - 1;
    document_info.at(index);
    return index;
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> document_words;

    for (const std::string& word : query.plus_words) 
    {
        if (word_to_document_freqs_.count(word) == 0) 
        {
            continue;
        }
        for (const auto [id, _] : word_to_document_freqs_.at(word)) 
        {
            if (id == document_id)
            {
                document_words.push_back(word);
            }
        }
    }
    sort(document_words.begin(), document_words.end(),
    [](const std::string& plus_word1, const std::string& plus_word2)
    {
        return plus_word1 < plus_word2;
    });

    for (const std::string& word : query.minus_words) 
    {
        if (word_to_document_freqs_.count(word) == 0) 
        {
            continue;
        }
        for (const auto [id, _] : word_to_document_freqs_.at(word)) 
        {
            if (id == document_id)
            {
                document_words.clear();
            }
        }
    }
    return {document_words, document_info.at(document_id).status};
}