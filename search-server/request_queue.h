//Реализация класса RequestQueue - очередь запросов.
//Сохраняет запросы в максимально допустимом количестве (по умолчанию - запросы за день)
//Удаляет старые запросы
//Дает возможность узнать, сколько запросов было с пустым результатом


#pragma once

#include <deque>

#include "document.h"
#include "search_server.h"



class RequestQueue 
{    
public:
    explicit RequestQueue(const SearchServer& search_server)
    :search_server(search_server)
    { }
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL);

    int GetNoResultRequests() const;
    
private:
//храним сам запрос и результат поиска среди документов:
//true - что-то нашлось, false - не нашлось
    struct QueryResult 
    {
        bool empty_result;
        std::string query;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    
    int16_t total_results = 0;
    int16_t empty_results = 0;
    const SearchServer& search_server;
}; 


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) 
{
    //кол-во поступивших запросов не должно превышать кол-во минут в дне,
    //иначе старые запросы удаляются
    auto matched_documents = search_server.FindTopDocuments(raw_query, document_predicate);
    if (total_results < min_in_day_)
    {
        if(!matched_documents.empty())
        {
            requests_.push_back({true, raw_query});
        }
        else
        {
            requests_.push_back({false, raw_query});
            ++empty_results;
        }
    }
    else
    {
        if (requests_.front().empty_result)
        {
            requests_.pop_front();
        }
        else
        {
            --empty_results;
            requests_.pop_front();
        }
        ++total_results;

        if(!matched_documents.empty())
        {
            requests_.push_back({true, raw_query});
        }
        else
        {
            requests_.push_back({false, raw_query});
            ++empty_results;
        }
    }
    ++total_results;

    return matched_documents;
}