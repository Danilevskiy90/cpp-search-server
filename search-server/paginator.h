#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>


template <typename Iterator>
struct IteratorRange
{
    Iterator start;
    Iterator finish;
};

    
template <typename Iterator>
std::ostream& operator << (std::ostream& out, const IteratorRange<Iterator>& page)
{
    for (auto i = page.start; i != page.finish; i++)
    {
        out << *i;
    }
    return out;
}

template <typename Iterator>
class Paginator 
{
public:
    Paginator(Iterator begin, Iterator end, int page_size)
    {
        while(begin < end)
        {
            IteratorRange<Iterator> page;
            page.start = begin;
            for(int i = 0; i < page_size; i++)
            {
                ++begin;
                if (begin == end) break;
            }
            page.finish = begin;
            pages_documents.push_back(page);
            //begin+=page_size;
        }       
    }
    
    auto begin() const
    {
        return pages_documents.begin();
    }

    auto end() const
    {
        return pages_documents.end();
    }

    size_t size() const
    {
        return pages_documents.size();
    }
private:
    std::vector<IteratorRange<Iterator>> pages_documents;
}; 

template <typename Container>
auto Paginate(const Container& c, size_t page_size) 
{
    return Paginator(begin(c), end(c), page_size);
}
