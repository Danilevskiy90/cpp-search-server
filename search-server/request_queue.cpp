#include "request_queue.h"

#include <numeric>

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) 
{
    // напишите реализацию
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus stat, int rating) { return stat == status; });
}


int RequestQueue::GetNoResultRequests() const 
{
    // напишите реализацию
    return empty_results;
}