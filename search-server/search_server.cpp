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
        words_frequency_by_documents_[document_id][word] += inv_word_count;
    }
    document_info[document_id].rating = ComputeAverageRating(ratings);

    document_indexes.insert(document_id);

}

int SearchServer::GetDocumentCount() const
{
    return document_info.size();
}

// int SearchServer::GetDocumentId(int index) const 
// {
//     //int ind = index - 1;
//     document_info.at(index);
//     return index;
// }


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

std::set<int>::const_iterator SearchServer::begin() 
{
    return document_indexes.cbegin();

    // std::set<int> s;
    // std::transform(document_info.begin(), document_info.end(), 
    //                                 std::inserter(s, s.begin()),
    //                                 [](auto pair){ return pair.first; });
    // return s.cbegin();
}

std::set<int>::const_iterator SearchServer::end()
{
    return document_indexes.cend();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string, double> words_in_document;

    if (words_frequency_by_documents_.count(document_id) > 0)
        return words_frequency_by_documents_.at(document_id);

    return words_in_document;
}

void SearchServer::RemoveDocument(int document_id)
{
    document_info.erase(document_id);
    document_indexes.erase(document_id);
    for (const auto& [word, _] : words_frequency_by_documents_.at(document_id))
    {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    words_frequency_by_documents_.erase(document_id);
}

void RemoveDuplicates(SearchServer &search_server)
{
    std::map<std::set<std::string>, int> storage;
    std::vector<int> indices_for_removal;

    for (const int index : search_server) 
    {
        const auto &word_frequencies = search_server.GetWordFrequencies(index);
        std::set<std::string> storage_key;
        std::transform(word_frequencies.begin(), word_frequencies.end(),
                       std::inserter(storage_key, storage_key.begin()), [](const auto &item) { return item.first; });

        
        if (storage.count(storage_key) > 0)
            indices_for_removal.emplace_back(index);
        else
            storage.insert({storage_key, index});
    }

    for (int index : indices_for_removal)
    {
        std::cout<<"Found duplicate document " << index << std::endl;
        search_server.RemoveDocument(index);
    }
}
