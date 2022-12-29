#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <math.h>

#include "document.h"
#include "string_processing.h"


class SearchServer 
{
public:  

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        //: stop_words_(MakeUniqueNonEmptyStrings(stop_words)) 
    {   
        std::set<std::string> non_empty_strings;
        for (const std::string& str : stop_words) 
        {
            if (!IsValidWord(str)) throw std::invalid_argument("Ошибка инициализации");

            if (!str.empty()) 
            {
                non_empty_strings.insert(str);
            }
        }
        stop_words_ = non_empty_strings;
    }


    SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  
    {   
        if (!IsValidWord(stop_words_text)) throw std::invalid_argument("Ошибка инициализации");
    }

    //добавить документ
    void AddDocument(int document_id, const std::string& document, 
                    DocumentStatus status, const std::vector<int>& ratings);
                    
    //найти лучшие документы
    template<typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Predicate predicate) const 
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) 
             {
                return lhs.relevance > rhs.relevance ||
                (abs(lhs.relevance - rhs.relevance) < EPSILON && (lhs.rating>rhs.rating));
             });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const 
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) { return stat == status; });
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    
private:

    struct StatusAndRating
    {
        DocumentStatus status;
        int rating;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    //map<int, int> document_ratings_;
    //map<int, DocumentStatus> document_status; 

    std::map<int, StatusAndRating> document_info;
    
    struct QueryWord 
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query 
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    
    bool IsStopWord(const std::string& word) const 
    {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const std::string& word) 
    {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) 
        {
            return c >= '\0' && c < ' ';
        });
    }
    //Разделить на слова без стоп-слов
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const 
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
    
    //Вычислить средний рейтинг
    static int ComputeAverageRating(const std::vector<int>& ratings) 
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
 
    //Разобрать слово запроса
    QueryWord ParseQueryWord(std::string text) const 
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
    
    //разобрать запрос
    Query ParseQuery(const std::string& text) const 
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
    
    // Вычислить Word Inverse DocumentFreq
    double ComputeWordInverseDocumentFreq(const std::string& word) const 
    {
        return log(document_info.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    //найти все документы
    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const 
    {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) 
            {
                if (!predicate(document_id, document_info.at(document_id).status, document_info.at(document_id).rating))
                {
                    continue;
                }
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
        
        for (const std::string& word : query.minus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) 
            {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) 
        {
            matched_documents.push_back(
            {
                document_id,
                relevance,
                document_info.at(document_id).rating
            });
        }
        return matched_documents;
    }
};