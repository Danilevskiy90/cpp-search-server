#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer &search_server)
{
    std::set<std::set<std::string>> storage;
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
            storage.insert({storage_key});
    }

    for (int index : indices_for_removal)
    {
        std::cout<<"Found duplicate document " << index << std::endl;
        search_server.RemoveDocument(index);
    }
}