#pragma once
#include <string>
#include <unordered_map>

class Config {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;

public:
    bool load(const std::string& filename);
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "");
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0);

    static Config& getInstance() {
        static Config instance;
        return instance;
    }
};
