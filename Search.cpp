#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

// Structure to hold book information
struct Book {
    string ISBN;
    string title;
    string author;
    string genre;
    int publicationYear;
};

// Levenshtein Distance for typo handling (used in BK-Tree)
int levenshteinDistance(const string& s1, const string& s2) {
    const size_t m = s1.size(), n = s2.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[m][n];
}

// BK-Tree Node Structure
class BKTreeNode {
public:
    string word;
    unordered_map<int, BKTreeNode*> children;

    BKTreeNode(const string& w) : word(w) {}
};

// BK-Tree for typo-tolerant word search
class BKTree {
public:
    BKTree() : root(nullptr) {}

    void insert(const string& word) {
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

    vector<string> search(const string& word, int maxDist) {
        vector<string> results;
        searchHelper(root, word, maxDist, results);
        return results;
    }

private:
    BKTreeNode* root;

    void searchHelper(BKTreeNode* node, const string& word, int maxDist, vector<string>& results) {
        if (!node) return;
        int dist = levenshteinDistance(word, node->word);
        if (dist <= maxDist) {
            results.push_back(node->word);
        }
        for (const auto& entry : node->children) {
            int key = entry.first;
            BKTreeNode* child = entry.second;
            if (key >= dist - maxDist && key <= dist + maxDist) {
                searchHelper(child, word, maxDist, results);
            }
        }
    }
};

// Library System with word-based BK-Tree search
class LibrarySystem {
public:
    void addBook(const Book& book) {
        books[book.ISBN] = book;
        indexWordsInTitle(book);
    }

    vector<Book> searchBooks(const string& query, int maxDist = 1) {
        vector<string> matchedWords = bkTree.search(query, maxDist);
        unordered_set<string> matchedISBNs;

        for (const auto& word : matchedWords) {
            if (wordToISBNs.count(word)) {
                matchedISBNs.insert(wordToISBNs[word].begin(), wordToISBNs[word].end());
            }
        }

        vector<Book> results;
        for (const auto& ISBN : matchedISBNs) {
            results.push_back(books[ISBN]);
        }
        return results;
    }

private:
    unordered_map<string, Book> books;
    BKTree bkTree;
    unordered_map<string, unordered_set<string>> wordToISBNs;

    void indexWordsInTitle(const Book& book) {
        vector<string> words = splitIntoWords(book.title);
        for (const auto& word : words) {
            bkTree.insert(word);
            wordToISBNs[word].insert(book.ISBN);
        }
    }

    vector<string> splitIntoWords(const string& text) {
        vector<string> words;
        string word;
        for (char c : text) {
            if (isalnum(c)) {
                word += tolower(c);
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
};

// Example usage
int main() {
    LibrarySystem library;
    library.addBook({"12345", "C++ The Programmer", "Bjarne Stroustrup", "Programming", 1997});
    library.addBook({"67890", "The Pragmatic The Programmer", "Andrew Hunt", "Software Engineering", 1999});
    library.addBook({"54321", "Clean Code", "Robert Martin", "Software Engineering", 2008});

    string query;
    cout << "Enter a book title to search: ";
    getline(cin, query);

    auto results = library.searchBooks(query, 5);
    if (results.empty()) {
        cout << "No books found for the query: " << query << endl;
    } else {
        for (const auto& book : results) {
            cout << "Found Book: " << book.title << " by " << book.author << endl;
        }
    }

    return 0;
}
