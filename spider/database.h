#pragma once
#include <string>
#include <memory>
#include <pqxx/pqxx>

class Database {
private:
    std::unique_ptr<pqxx::connection> conn_;

public:
    Database(const std::string& connection_string);
    ~Database();

    void initializeDatabase();
    int addDocument(const std::string& url, const std::string& title);
    int addWord(const std::string& word);
    void addWordFrequency(int document_id, int word_id, int frequency);
    bool documentExists(const std::string& url);
};
