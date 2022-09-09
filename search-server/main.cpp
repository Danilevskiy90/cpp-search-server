#include <algorithm>
#include <iostream>
#include <map>
#include <set>
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
	int result;
	cin >> result;
	ReadLine();
	return result;
}

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
	int relevance;
};



bool HasDocumentGreaterRelevance(const Document& lhs, const Document& rhs) 
{
	return lhs.relevance > rhs.relevance;
}

class SearchServer 
{
public:
    vector<Document> FindTopDocuments(const string& query) 
    {	
        auto matched_documents = FindAllDocuments(query);
        sort(matched_documents.begin(), matched_documents.end(), HasDocumentGreaterRelevance);
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    void AddDocument(int document_id, const string& document) 
    {
        for (const string& word : SplitIntoWordsNoStop(document)) 
        {
            word_to_documents_[word].insert(document_id);
        }
    }
    set<string> SetStopWords(const string& text) 
    {
        set<string> stop_words;
        for (const string& word : SplitIntoWords(text)) 
        {
            stop_words_.insert(word);
        }
        return stop_words;
    }

private:
    map<string, set<int>> word_to_documents_;
    set<string> stop_words_;
    vector<string> SplitIntoWordsNoStop(const string& text) 
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) 
        {
            if (stop_words_.count(word) == 0) 
            {
                words.push_back(word);
            }
        }
        return words;
    }
    vector<Document> FindAllDocuments(const string& query) 
    {
        const vector<string> query_words = SplitIntoWordsNoStop(query);
        map<int, int> document_to_relevance;
        for (const string& word : query_words) 
        {
            if (word_to_documents_.count(word) == 0)
            {
                continue;
            }
            for (const int document_id : word_to_documents_.at(word)) 
            {
                ++document_to_relevance[document_id];
            }
        }
        vector<Document> matched_documents;
        for (auto [document_id, relevance] : document_to_relevance) 
        {
            matched_documents.push_back({document_id, relevance});
        }
	
        return matched_documents;
    }
}; 

SearchServer CreateSearchServer ()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int r=0;r<document_count;r++)
    {
        search_server.AddDocument(r, ReadLine());
    }
    return search_server;
}

int main() 
{
    SearchServer search_server=CreateSearchServer();
    //search_server.FindTopDocuments(ReadLine());
    for (auto [document_id, relevance] : search_server.FindTopDocuments(ReadLine()))
    {
        cout << "{ document_id = " << document_id << ", relevance = " << relevance << " }" << endl;
    }
}