#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <unordered_set>
#include <regex>
#include <chrono>

#include "http_utils.h"
#include "html_parser.h"
#include "database.h"
#include "config.h"

std::mutex mtx;
std::condition_variable cv;
std::queue<std::pair<Link, int>> tasks;
std::unordered_set<std::string> visitedUrls;
std::atomic<bool> exitThreadPool{false};
std::unique_ptr<Database> database;

void threadPoolWorker() {
    std::unique_lock<std::mutex> lock(mtx);
    while (!exitThreadPool || !tasks.empty()) {
        if (tasks.empty()) {
            cv.wait(lock, [] { return exitThreadPool || !tasks.empty(); });
            if (exitThreadPool && tasks.empty())
                break;
        } else {
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();

            Link link = task.first;
            int depth = task.second;

            std::string url = (link.protocol == ProtocolType::HTTPS ? "https://" : "http://")
                            + link.hostName + link.query;

            {
                std::lock_guard<std::mutex> visitedLock(mtx);
                if (visitedUrls.count(url) > 0) {
                    lock.lock();
                    continue;
                }
                visitedUrls.insert(url);
            }

            try {
                std::cout << "🌐 Fetching: " << url << std::endl;

                std::string html = getHtmlContent(link);

                if (html.empty()) {
                    std::cout << "❌ Failed to get HTML content from: " << url << std::endl;
                    lock.lock();
                    continue;
                }

                std::string text = HtmlParser::extractText(html);
                std::string title = HtmlParser::extractTitle(html);
                auto wordCounts = HtmlParser::countWords(text);

                int documentId = database->addDocument(url, title);

                for (const auto& [word, count] : wordCounts) {
                    int wordId = database->addWord(word);
                    database->addWordFrequency(documentId, wordId, count);
                }

                std::cout << "✅ Indexed: " << url << " (unique words: " << wordCounts.size() << ")" << std::endl;

                if (depth > 0) {
                    auto links = HtmlParser::extractLinks(link, html);

                    std::lock_guard<std::mutex> taskLock(mtx);
                    for (const auto& newLink : links) {
                        std::string newUrl = (newLink.protocol == ProtocolType::HTTPS ? "https://" : "http://")
                                           + newLink.hostName + newLink.query;

                        if (visitedUrls.count(newUrl) == 0) {
                            tasks.push({newLink, depth - 1});
                        }
                    }
                    cv.notify_all();
                }
            } catch (const std::exception& e) {
                std::cout << "❌ Error processing " << url << ": " << e.what() << std::endl;
            }

            lock.lock();
        }
    }
}

int main() {
    try {
        // Load configuration
        Config& config = Config::getInstance();
        if (!config.load("../config.ini")) {
            std::cerr << "❌ Failed to load config.ini" << std::endl;
            return 1;
        }

        // Initialize database
        std::string dbConnection =
            "host=" + config.getString("database", "host") +
            " port=" + std::to_string(config.getInt("database", "port")) +
            " dbname=" + config.getString("database", "name") +
            " user=" + config.getString("database", "username") +
            " password=" + config.getString("database", "password");

        database = std::make_unique<Database>(dbConnection);
        database->initializeDatabase();

        // Parse start URL
        std::string startUrl = config.getString("spider", "start_url");
        std::regex urlRegex("(https?)://([^/]+)(/.*)?");
        std::smatch match;

        if (!std::regex_match(startUrl, match, urlRegex)) {
            std::cerr << "❌ Invalid start URL: " << startUrl << std::endl;
            return 1;
        }

        Link startLink;
        startLink.protocol = (match[1] == "https") ? ProtocolType::HTTPS : ProtocolType::HTTP;
        startLink.hostName = match[2];
        startLink.query = match[3].matched ? match[3].str() : "/";

        int maxDepth = config.getInt("spider", "max_depth", 1);
        int threadCount = config.getInt("spider", "thread_count", 2);

        std::cout << "🚀 Starting Spider with:" << std::endl;
        std::cout << "   Start URL: " << startUrl << std::endl;
        std::cout << "   Max depth: " << maxDepth << std::endl;
        std::cout << "   Threads: " << threadCount << std::endl;
        std::cout << std::endl;

        // Start thread pool
        std::vector<std::thread> threadPool;
        for (int i = 0; i < threadCount; ++i) {
            threadPool.emplace_back(threadPoolWorker);
        }

        // Add initial task
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push({startLink, maxDepth});
            cv.notify_all();
        }

        // Simple variant: let spider work for fixed time
        std::this_thread::sleep_for(std::chrono::seconds(30));

        // Shutdown
        {
            std::lock_guard<std::mutex> lock(mtx);
            exitThreadPool = true;
            cv.notify_all();
        }

        for (auto& t : threadPool) {
            t.join();
        }

        std::cout << std::endl;
        std::cout << "✅ Spider completed. Indexed " << visitedUrls.size() << " pages." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "💥 Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
