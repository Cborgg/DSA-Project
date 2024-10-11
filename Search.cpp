#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

// Structure to hold book information
struct Book {
    std::string ISBN;
    std::string title;
    std::string author;
    std::string genre;
    int publicationYear;
};

// Levenshtein Distance for typo handling (used in BK-Tree)
int levenshteinDistance(const std::string& s1, const std::string& s2) {
    const size_t m = s1.size(), n = s2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[m][n];
}

// BK-Tree Node Structure
class BKTreeNode {
public:
    std::string word;
    std::unordered_map<int, BKTreeNode*> children;

    BKTreeNode(const std::string& w) : word(w) {}
};

// BK-Tree for typo-tolerant title search
class BKTree {
public:
    BKTree() : root(nullptr) {}

    void insert(const std::string& word) {
        if (!root) {
            root = new BKTreeNode(word);
            return;
        }
        BKTreeNode* node = root;
        int dist = levenshteinDistance(word, node->word);
        while (node->children.count(dist)) {
            node = node->children[dist];
            dist = levenshteinDistance(word, node->word);
        }
        node->children[dist] = new BKTreeNode(word);
    }

    std::unordered_set<std::string> search(const std::string& word, int maxDist) {
        std::unordered_set<std::string> results;
        searchHelper(root, word, maxDist, results);
        return results;
    }

private:
    BKTreeNode* root;

    void searchHelper(BKTreeNode* node, const std::string& word, int maxDist, std::unordered_set<std::string>& results) {
        if (!node) return;
        int dist = levenshteinDistance(word, node->word);
        if (dist <= maxDist) {
            results.insert(node->word);
        }
        for (const auto& [key, child] : node->children) {
            if (key >= dist - maxDist && key <= dist + maxDist) {
                searchHelper(child, word, maxDist, results);
            }
        }
    }
};

// Library System with Inverted Index and BM25 Ranking
class LibrarySystem {
public:
    void addBook(const Book& book) {
        books[book.ISBN] = book;
        insertToBKTree(book.title);
        indexBook(book);
    }

    std::vector<Book> searchBooks(const std::string& query, int maxDist = 1) {
        std::unordered_set<std::string> candidateTitles = bkTree.search(query, maxDist);
        std::vector<std::pair<double, Book>> rankedResults;

        for (const auto& title : candidateTitles) {
            for (const auto& [ISBN, book] : books) {
                if (book.title == title) {
                    double score = calculateBM25(query, book);
                    rankedResults.emplace_back(score, book);
                }
            }
        }

        std::sort(rankedResults.begin(), rankedResults.end(), [](auto& a, auto& b) {
            return a.first > b.first;
        });

        std::vector<Book> results;
        for (const auto& [score, book] : rankedResults) {
            results.push_back(book);
        }
        return results;
    }

private:
    std::unordered_map<std::string, Book> books;
    BKTree bkTree;
    std::unordered_map<std::string, std::unordered_set<std::string>> invertedIndex;

    void insertToBKTree(const std::string& title) {
        bkTree.insert(title);
    }

    void indexBook(const Book& book) {
        auto words = splitIntoWords(book.title + " " + book.author + " " + book.genre);
        for (const auto& word : words) {
            invertedIndex[word].insert(book.ISBN);
        }
    }

    std::vector<std::string> splitIntoWords(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        for (char c : text) {
            if (std::isalnum(c)) {
                word += std::tolower(c);
            } else if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }
        return words;
    }

    double calculateBM25(const std::string& query, const Book& book) {
        const double k = 1.5;
        const double b = 0.75;
        double score = 0.0;

        auto queryWords = splitIntoWords(query);
        double bookWordCount = splitIntoWords(book.title + " " + book.author + " " + book.genre).size();
        double avgDocLength = calculateAverageDocLength();
        
        for (const auto& term : queryWords) {
            if (invertedIndex.count(term)) {
                double termFrequency = invertedIndex[term].count(book.ISBN);
                double docFrequency = invertedIndex[term].size();
                double idf = std::log((books.size() - docFrequency + 0.5) / (docFrequency + 0.5) + 1);
                double tf = (termFrequency * (k + 1)) / (termFrequency + k * (1 - b + b * (bookWordCount / avgDocLength)));
                score += idf * tf;
            }
        }
        return score;
    }

    double calculateAverageDocLength() {
        double totalLength = 0.0;
        for (const auto& [ISBN, book] : books) {
            totalLength += splitIntoWords(book.title + " " + book.author + " " + book.genre).size();
        }
        return totalLength / books.size();
    }
};

// Example usage
int main() {
    LibrarySystem library;
    library.addBook({"12345", "C++ Programming", "Bjarne Stroustrup", "Programming", 1997});
    library.addBook({"67890", "The Pragmatic Programmer", "Andrew Hunt", "Software Engineering", 1999});
    library.addBook({"54321", "Clean Code", "Robert Martin", "Software Engineering", 2008});

    auto results = library.searchBooks("Code", 1);
    for (const auto& book : results) {
        std::cout << "Found Book: " << book.title << " by " << book.author << std::endl;
    }

    return 0;
}
