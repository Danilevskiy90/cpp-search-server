#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {

            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    int relevance;
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& s : words)
        {
            documents_[s].insert(document_id);
        }
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:


    struct Query
    {
        set<string> minus_slova;
        set<string> plus_slova;
    };

    map<string,set<int>> documents_;

    set<string> stop_words_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWordsNoStop(text))
        {
            if (word[0] == '-')
            {
                query.minus_slova.insert(word.substr(1));
                continue;
            }
            query.plus_slova.insert(word);
        }
        return query;
    }

    vector<Document> FindAllDocuments(const Query &query) const
    {
        map<int,int> document_to_relevance;
        for (const string& plus_slova : query.plus_slova)
        {
            if (documents_.count(plus_slova)==0)
            {
                continue;
            }
            for (int r : documents_.at(plus_slova))
            {
                document_to_relevance[r]=document_to_relevance[r]+1;
            }
        }
        for (const string& minus_slova : query.minus_slova)
        {
            if (documents_.count(minus_slova)==0)
            {
                continue;
            }
            for (int r : documents_.at(minus_slova))
            {
                document_to_relevance.erase(r);
            }
        }
        vector<Document> document_to;
        for (auto paIr: document_to_relevance)
        {
            document_to.push_back({paIr.first, paIr.second});
        }
        return document_to;
    }
    /*static int MatchDocument(const string &content, const Query &query)
    {
        if (query.plus_slova.empty())
        {
            return 0;
        }
        set<string> matched_words;
        for (const string &word : content.words)
        {
            // обработка минус-слов
            if (matched_words.count(word) != 0)
            {
                continue;
            }
            if (query.plus_slova.count(word) != 0)
            {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }*/
};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{

    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto &[document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}