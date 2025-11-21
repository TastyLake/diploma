#pragma once
#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

struct SearchResult {
    std::string url;
    std::string title;
    int relevance;
};

class SearchDatabase {
private:
    std::unique_ptr<pqxx::connection> conn_;

public:
    SearchDatabase(const std::string& connection_string);
    std::vector<SearchResult> search(const std::string& query);
};
