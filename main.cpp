#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <cassert>
#include <optional>
#include <deque>
#include <stdint.h>

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

// разделить на слова
vector<string> SplitIntoWords(const string &text)
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
    Document() = default;

    Document(int id_, double relevance_, int rating_)
        : id(id_), relevance(relevance_), rating(rating_)
    {
    }
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words)
    //: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        set<string> non_empty_strings;
        for (const string &str : stop_words)
        {
            if (!IsValidWord(str))
                throw invalid_argument("Ошибка инициализации");

            if (!str.empty())
            {
                non_empty_strings.insert(str);
            }
        }
        stop_words_ = non_empty_strings;
    }

    explicit SearchServer(const string &stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
        if (!IsValidWord(stop_words_text))
            throw invalid_argument("Ошибка инициализации");
    }

    // добавить документ
    void AddDocument(int document_id, const string &document,
                     DocumentStatus status, const vector<int> &ratings)
    {
        if ((document_id < 0) || (document_info.count(document_id)) || (!IsValidWord(document)))
        {
            throw invalid_argument{"Невозможно добавить документ"};
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        document_info[document_id].status = status;
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        // document_ratings_.emplace(document_id, ComputeAverageRating(ratings));
        document_info[document_id].rating = ComputeAverageRating(ratings);
    }

    // найти лучшие документы
    template <typename Predicate>
    vector<Document> FindTopDocuments(const string &raw_query, Predicate predicate) const
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

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating)
                                { return stat == status; });
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const
    {
        return document_info.size();
    }

    int GetDocumentId(int index) const
    {
        // int ind = index - 1;
        document_info.at(index);
        return index;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> document_words;

        for (const string &word : query.plus_words)
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
             [](const string &plus_word1, const string &plus_word2)
             {
                 return plus_word1 < plus_word2;
             });

        for (const string &word : query.minus_words)
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
    struct StatusAndRating
    {
        DocumentStatus status;
        int rating;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    // map<int, int> document_ratings_;
    // map<int, DocumentStatus> document_status;

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

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string &word)
    {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c)
                       { return c >= '\0' && c < ' '; });
    }
    // Разделить на слова без стоп-слов
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

    // Вычислить средний рейтинг
    static int ComputeAverageRating(const vector<int> &ratings)
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

    // Разобрать слово запроса
    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            if ((text[1] == '-') || (text == "-"s) || !IsValidWord(text))
            {
                throw invalid_argument("Ошибка в запросе");
            }

            is_minus = true;
            text = text.substr(1);
        }
        if (!IsValidWord(text))
        {
            throw invalid_argument("Ошибка в запросе");
        }
        return {
            text,
            is_minus,
            IsStopWord(text)};
    }

    // разобрать запрос
    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
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
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(document_info.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // найти все документы
    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query &query, Predicate predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
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

        for (const string &word : query.minus_words)
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
                {document_id,
                 relevance,
                 document_info.at(document_id).rating});
        }
        return matched_documents;
    }
};

void AssertImpl(bool value, const string &expr_str,
                const string &file, const string &func,
                unsigned line, const string &hint)
{
    if (!value)
    {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename predicat>
void RunTestImpl(predicat func, string function_name)
{
    if (func)
    {
        cerr << function_name << " OK" << endl;
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document &doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

// Удаление документов с минус-словами
void TestDeletingDocumentsWithNegativeeywords()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in -the"s).empty());
    }
}

// Матчинг документов
// Сверяем релевантность возвращаемых документов с ожидаемой, совпадение результатов гарантирует
// что система нашла все слова из запроса.

void TestDocumentMatching()
{
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(12, "colorful parrot with green wings and red tail is lost", DocumentStatus::ACTUAL, {1, 2, 3});
        const auto found_docs = server.FindTopDocuments("in the city"s);
        double relevance = 3 * (1.0 / 4) * log(2.0);
        ASSERT(found_docs[0].relevance == relevance);
    }

    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(12, "colorful parrot with green wings and red tail is lost", DocumentStatus::ACTUAL, {1, 2, 3});
        const auto found_docs = server.FindTopDocuments("in -the city"s);
        ASSERT(server.FindTopDocuments("in -the"s).empty());
    }
}

// Сортировка найденных документов по релевантности.
void TestSortingOfFoundDocumentsByRelevance()
{
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT((found_docs[0].relevance > found_docs[1].relevance) && (found_docs[1].relevance > found_docs[2].relevance));
}

// Вычисление рейтинга документов.
void TestCalculationOfDocumentRating()
{
    SearchServer server;
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    const auto found_docs = server.FindTopDocuments("ухоженный пёс выразительные глаза"s);
    ASSERT(found_docs[0].rating == (5 + 1 + 2 - 12) / 4);
}

// Фильтрация результатов поиска с использованием предиката
void TestFilteringSearchResultsUsingPredicate()
{
    SearchServer server("и в на"s);

    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});

    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT(found_docs[0].id == 1);
    ASSERT(found_docs[1].id == 0);
    ASSERT(found_docs[2].id == 2);

    const auto found_docs1 = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT(found_docs1[0].id == 3);

    const auto found_docs2 = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                     { return document_id % 2 == 0; });
    ASSERT(found_docs2[0].id == 0);
    ASSERT(found_docs2[1].id == 2);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestDeletingDocumentsWithNegativeeywords);
    RUN_TEST(TestDocumentMatching);
    RUN_TEST(TestSortingOfFoundDocumentsByRelevance);
    RUN_TEST(TestCalculationOfDocumentRating);
    RUN_TEST(TestFilteringSearchResultsUsingPredicate);
}

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

template <typename Iterator>
struct IteratorRange
{
    Iterator start;
    Iterator finish;
};

ostream &operator<<(ostream &out, const Document &document)
{
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

template <typename Iterator>
ostream &operator<<(ostream &out, const IteratorRange<Iterator> &page)
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
        while (begin < end)
        {
            IteratorRange<Iterator> page;
            page.start = begin;
            for (int i = 0; i < page_size; i++)
            {
                ++begin;
                if (begin == end)
                    break;
            }
            page.finish = begin;
            pages_documents.push_back(page);
            // begin+=page_size;
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
    vector<IteratorRange<Iterator>> pages_documents;
};

template <typename Container>
auto Paginate(const Container &c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server)
        : search_server(search_server)
    {
    }

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string &raw_query, DocumentPredicate document_predicate)
    {
        // напишите реализацию

        // кол-во поступивших запросов не должно превышать кол-во минут  в дне,
        // иначе старые запросы удаляются
        auto matched_documents = search_server.FindTopDocuments(raw_query, document_predicate);
        if (total_results < min_in_day_)
        {
            if (!matched_documents.empty())
            {
                requests_.push_back({true, raw_query});
            }
            else
            {
                requests_.push_back({false, raw_query});
                empty_results += 1;
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
                empty_results -= 1;
                requests_.pop_front();
            }
            total_results -= 1;

            if (!matched_documents.empty())
            {
                requests_.push_back({true, raw_query});
            }
            else
            {
                requests_.push_back({false, raw_query});
                empty_results += 1;
            }
        }
        total_results += 1;

        return matched_documents;
    }

    vector<Document> AddFindRequest(const string &raw_query, DocumentStatus status)
    {
        // напишите реализацию
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus stat, int rating)
                              { return stat == status; });
    }

    vector<Document> AddFindRequest(const string &raw_query)
    {
        // напишите реализацию
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const
    {
        // напишите реализацию
        return empty_results;
    }

private:
    // храним сам запрос и результат поиска среди документов:
    // true - что-то нашлось, false - поиск дал пустой результат
    struct QueryResult
    {
        // определите, что должно быть в структуре
        bool empty_result;
        string query;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
    int16_t total_results = 0;
    int16_t empty_results = 0;
    const SearchServer &search_server;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i)
    {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);

    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);

    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);

    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}
