#include "html_parser.h"
#include <regex>
#include <algorithm>
#include <cctype>

std::string HtmlParser::extractText(const std::string& html) {
    std::string text = html;

    // Remove script and style tags
    std::regex scriptRegex("<script[^>]*>[\\s\\S]*?</script>", std::regex::icase);
    text = std::regex_replace(text, scriptRegex, " ");

    std::regex styleRegex("<style[^>]*>[\\s\\S]*?</style>", std::regex::icase);
    text = std::regex_replace(text, styleRegex, " ");

    // Remove HTML tags
    std::regex tagRegex("<[^>]*>");
    text = std::regex_replace(text, tagRegex, " ");

    // Replace multiple whitespace with single space
    std::regex whitespaceRegex("\\s+");
    text = std::regex_replace(text, whitespaceRegex, " ");

    // Return text with spaces for word splitting
    return text;
}

std::vector<Link> HtmlParser::extractLinks(const Link& baseLink, const std::string& html) {
    std::vector<Link> links;
    std::regex linkRegex("<a\\s+[^>]*href=\"([^\"]*)\"[^>]*>", std::regex::icase);

    auto words_begin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string href = match[1].str();

        Link resolvedLink = resolveLink(baseLink, href);
        if (!resolvedLink.hostName.empty()) {
            links.push_back(resolvedLink);
        }
    }

    return links;
}

std::unordered_map<std::string, int> HtmlParser::countWords(const std::string& text) {
    std::unordered_map<std::string, int> wordCount;

    std::string word;
    for (unsigned char c : text) {
        if (std::isalnum(c)) {
            word += static_cast<char>(std::tolower(c));
        } else if (!word.empty()) {
            word = cleanText(word);
            if (isValidWord(word)) {
                wordCount[word]++;
            }
            word.clear();
        }
    }

    if (!word.empty()) {
        word = cleanText(word);
        if (isValidWord(word)) {
            wordCount[word]++;
        }
    }

    return wordCount;
}

std::string HtmlParser::extractTitle(const std::string& html) {
    std::regex titleRegex("<title>([\\s\\S]*?)</title>", std::regex::icase);
    std::smatch match;

    if (std::regex_search(html, match, titleRegex) && match.size() > 1) {
        std::string title = match[1].str();
        // Normalize whitespace
        std::regex ws("\\s+");
        title = std::regex_replace(title, ws, " ");
        return title;
    }

    return "Untitled";
}

std::string HtmlParser::cleanText(const std::string& text) {
    std::string cleaned;
    for (unsigned char c : text) {
        if (std::isalnum(c)) {
            cleaned += static_cast<char>(std::tolower(c));
        }
    }
    return cleaned;
}

bool HtmlParser::isValidWord(const std::string& word) {
    return word.length() >= 3 && word.length() <= 32;
}

Link HtmlParser::resolveLink(const Link& baseLink, const std::string& href) {
    Link result;

    // Skip empty links and javascript links
    if (href.empty() || href.find("javascript:") == 0 || href.find("#") == 0) {
        return result;
    }

    // Handle absolute URLs
    if (href.find("http://") == 0) {
        result.protocol = ProtocolType::HTTP;
        size_t start = 7; // "http://"
        size_t end = href.find("/", start);
        if (end == std::string::npos) {
            result.hostName = href.substr(start);
            result.query = "/";
        } else {
            result.hostName = href.substr(start, end - start);
            result.query = href.substr(end);
        }
    }
    else if (href.find("https://") == 0) {
        result.protocol = ProtocolType::HTTPS;
        size_t start = 8; // "https://"
        size_t end = href.find("/", start);
        if (end == std::string::npos) {
            result.hostName = href.substr(start);
            result.query = "/";
        } else {
            result.hostName = href.substr(start, end - start);
            result.query = href.substr(end);
        }
    }
    // Handle relative URLs
    else if (!href.empty() && href[0] == '/') {
        result.protocol = baseLink.protocol;
        result.hostName = baseLink.hostName;
        result.query = href;
    }

    return result;
}
