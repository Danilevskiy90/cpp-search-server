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


bool SearchServer::IsStopWord(const std::string& word) const 
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) 
{
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) 
    {
        return c >= '\0' && c < ' ';
    });
}


std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const 
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) 
    {
        if (!IsStopWord(word)) 
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) 
{
    if (ratings.empty()) 
    {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) 
    {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const 
{
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') 
    {
        if ((text[1] == '-') || (text == "-") || !IsValidWord(text))
        {
            throw std::invalid_argument("Ошибка в запросе");
        }

        is_minus = true;
        text = text.substr(1);
    }
    if (!IsValidWord(text))
    {
        throw std::invalid_argument("Ошибка в запросе");
    }
    return 
    {
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const 
{
    Query query;
    for (const std::string& word : SplitIntoWords(text)) 
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) 
        {
            if (query_word.is_minus) 
            {
                query.minus_words.insert(query_word.data);
            } 
            else 
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const 
{
    return log(document_info.size() * 1.0 / word_to_document_freqs_.at(word).size());
}