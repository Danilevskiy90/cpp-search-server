// Класс сервера
// Вся работа по поиску документов, фильтрация запроса происходит в нем

#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <math.h>

#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;


class SearchServer
{
public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    SearchServer(const std::string &stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
        if (!IsValidWord(stop_words_text))
            throw std::invalid_argument("Ошибка инициализации");
    }

    // добавить документ
    void AddDocument(int document_id, const std::string &document,
                     DocumentStatus status, const std::vector<int> &ratings);

    // найти лучшие документы
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string &raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string &raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating)
                                { return stat == status; });
    }

    std::vector<Document> FindTopDocuments(const std::string &raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    // int GetDocumentId(int index) const;

    //Методы добавленные в 5 спринте
    std::set<int>::const_iterator begin();

    std::set<int>::const_iterator end();

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    ///////////////////////////////
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string &raw_query, int document_id) const;

private:
    struct StatusAndRating
    {
        DocumentStatus status;
        int rating;
    };

    std::set<std::string> stop_words_;

    //обработанные слова документов - слово из документа и соответствующий ему словарь 
    //индексов документов, где встречается, и TF
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, StatusAndRating> document_info;

    //множество индексов документов, присутствующих в сервере
    std::set<int> document_indexes;

    //содержит индексы всех документов, присутствующих в сервере, и само содержание соответствующего документа и его TF
    //заполняется при вызове функции AddDocument
    std::map<int, std::map<std::string, double>> words_frequency_by_documents_;

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

    bool IsStopWord(const std::string &word) const;

    static bool IsValidWord(const std::string &word);

    // Разделить на слова без стоп-слов
    std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;

    // Вычислить средний рейтинг
    static int ComputeAverageRating(const std::vector<int> &ratings);

    // Разобрать слово запроса
    QueryWord ParseQueryWord(std::string text) const;

    // разобрать запрос
    Query ParseQuery(const std::string &text) const;

    // Вычислить Word Inverse DocumentFreq
    double ComputeWordInverseDocumentFreq(const std::string &word) const;

    // найти все документы
    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query &query, Predicate predicate) const;
};


void RemoveDuplicates(SearchServer& search_server);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
{
    std::set<std::string> non_empty_strings;
    for (const std::string &str : stop_words)
    {
        if (!IsValidWord(str))
            throw std::invalid_argument("Ошибка инициализации");

        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    stop_words_ = non_empty_strings;
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query, Predicate predicate) const
{
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs)
         {
             return lhs.relevance > rhs.relevance ||
                    (abs(lhs.relevance - rhs.relevance) < EPSILON && (lhs.rating > rhs.rating));
         });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, Predicate predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string &word : query.plus_words)
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

    for (const std::string &word : query.minus_words)
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
            {document_id,
             relevance,
             document_info.at(document_id).rating});
    }
    return matched_documents;
}