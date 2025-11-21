#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "link.h"

class HtmlParser {
public:
    static std::string extractText(const std::string& html);
    static std::vector<Link> extractLinks(const Link& baseLink, const std::string& html);
    static std::unordered_map<std::string, int> countWords(const std::string& text);
    static std::string extractTitle(const std::string& html);

private:
    static std::string cleanText(const std::string& text);
    static bool isValidWord(const std::string& word);
    static Link resolveLink(const Link& baseLink, const std::string& href);
};
