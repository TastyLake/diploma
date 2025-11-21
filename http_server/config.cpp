#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace {
    inline void trim(std::string& s) {
        if (s.empty()) return;
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) {
            s.clear();
            return;
        }
        size_t end = s.find_last_not_of(" \t");
        s = s.substr(start, end - start + 1);
    }
}

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            trim(currentSection);
        }
        else if (!currentSection.empty()) {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);
                trim(key);
                trim(value);
                if (!key.empty()) {
                    data_[currentSection][key] = value;
                }
            }
        }
    }

    file.close();
    return true;
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& defaultValue) {
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultValue) {
    std::string value = getString(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}
