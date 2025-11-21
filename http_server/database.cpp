#include "database.h"
#include <sstream>
#include <iostream>
#include <cctype>

SearchDatabase::SearchDatabase(const std::string& connection_string) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        std::cout << "✅ Database connected: " << conn_->dbname() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Database connection error: " << e.what() << std::endl;
        throw;
    }
}

std::vector<SearchResult> SearchDatabase::search(const std::string& query) {
    std::vector<SearchResult> results;

    try {
        // Split query into words
        std::vector<std::string> words;
        std::string word;
        for (unsigned char c : query) {
            if (std::isalnum(c)) {
                word += static_cast<char>(std::tolower(c));
            } else if (!word.empty()) {
                if (word.length() >= 3 && word.length() <= 32) {
                    words.push_back(word);
                }
                word.clear();
            }
        }
        if (!word.empty() && word.length() >= 3 && word.length() <= 32) {
            words.push_back(word);
        }

        if (words.empty()) {
            return results;
        }

        if (words.size() > 4) {
            words.resize(4);
        }

        std::stringstream sql;
        sql << "SELECT d.url, d.title, SUM(wf.frequency) as relevance "
            << "FROM documents d "
            << "JOIN word_frequencies wf ON d.id = wf.document_id "
            << "JOIN words w ON wf.word_id = w.id "
            << "WHERE w.word IN (";

        for (size_t i = 0; i < words.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << "$" << i + 1;
        }

        sql << ") "
            << "GROUP BY d.url, d.title "
            << "HAVING COUNT(DISTINCT w.word) = " << words.size() << " "
            << "ORDER BY relevance DESC "
            << "LIMIT 10";

        pqxx::work txn(*conn_);

        pqxx::params params;
        for (const auto& w : words) {
            params.append(w);
        }

        pqxx::result r = txn.exec_params(sql.str(), params);

        for (const auto& row : r) {
            SearchResult result;
            result.url = row["url"].c_str();
            result.title = row["title"].c_str();
            result.relevance = row["relevance"].as<int>();
            results.push_back(result);
        }

        txn.commit();

        std::cout << "🔍 Search for '" << query << "' found " << results.size() << " results" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Search error: " << e.what() << std::endl;
    }

    return results;
}
