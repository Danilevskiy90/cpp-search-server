#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() 
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() 
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

//разделить на слова
vector<string> SplitIntoWords(const string& text) 
{
    vector<string> words;
    string word;
    for (const char c : text) 
    {
        if (c == ' ') 
        {
            words.push_back(word);
            word = "";
        } 
        else 
        {
            word += c;
        }
    }
    words.push_back(word);
    
    return words;
}
    
struct Document 
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct StatusAndRating
    {
        DocumentStatus status;
        int rating;
    };

class SearchServer 
{
public:
    //установить стоп-слова
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text)) 
        {
            stop_words_.insert(word);
        }
    }    
    
    //добавить документ
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) 
    {
        document_info[document_id].status = status;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) 
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        //document_ratings_.emplace(document_id, ComputeAverageRating(ratings));
        document_info[document_id].rating = ComputeAverageRating(ratings);
    }
    //найти лучшие документы
    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const 
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) 
             {
                return lhs.relevance > rhs.relevance ||
                (abs(lhs.relevance - rhs.relevance) 
                < EPSILON && (lhs.rating>rhs.rating));//хотел бы узнать, в данном месте насколько будет грамотно так оформлять проверку
             });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const 
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) { return stat == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const
    {
        return document_info.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> document_words;

        for (const string& word : query.plus_words) 
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
        [](const string& plus_word1, const string& plus_word2)
        {
            return plus_word1 < plus_word2;
        });

        for (const string& word : query.minus_words) 
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
    
private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    //map<int, int> document_ratings_;
    //map<int, DocumentStatus> document_status; 

    map<int, StatusAndRating> document_info;
    
    struct QueryWord 
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query 
    {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    bool IsStopWord(const string& word) const 
    {
        return stop_words_.count(word) > 0;
    }

    //Разделить на слова без стоп-слов
    vector<string> SplitIntoWordsNoStop(const string& text) const 
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) 
        {
            if (!IsStopWord(word)) 
            {
                words.push_back(word);
            }
        }
        return words;
    }
    
    //Вычислить средний рейтинг
    static int ComputeAverageRating(const vector<int>& ratings) 
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
    QueryWord ParseQueryWord(string text) const 
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return 
        {
            text,
            is_minus,
            IsStopWord(text)
        };
    }
    
    //разобрать запрос
    Query ParseQuery(const string& text) const 
    {
        Query query;
        for (const string& word : SplitIntoWords(text)) 
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
    double ComputeWordInverseDocumentFreq(const string& word) const 
    {
        return log(document_info.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    //найти все документы
    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const 
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) 
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
        
        for (const string& word : query.minus_words) 
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

        vector<Document> matched_documents;
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

void PrintDocument(const Document& document) 
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() 
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}