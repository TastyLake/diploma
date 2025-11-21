#include "database.h"
#include <iostream>

Database::Database(const std::string& connection_string) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        std::cout << "✅ Connected to database: " << conn_->dbname() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Database connection error: " << e.what() << std::endl;
        throw;
    }
}

Database::~Database() {
    if (conn_ && conn_->is_open()) {
        conn_->close();
    }
}

void Database::initializeDatabase() {
    try {
        pqxx::work txn(*conn_);

        // Check if tables exist, create if not
        pqxx::result r = txn.exec(
            "SELECT table_name FROM information_schema.tables "
            "WHERE table_schema = 'public' AND "
            "table_name IN ('documents', 'words', 'word_frequencies')"
        );

        if (r.size() < 3) {
            std::cout << "📝 Creating database tables..." << std::endl;

            txn.exec(
                "CREATE TABLE IF NOT EXISTS documents ("
                "id SERIAL PRIMARY KEY, "
                "url TEXT UNIQUE NOT NULL, "
                "title TEXT, "
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                ")"
            );

            txn.exec(
                "CREATE TABLE IF NOT EXISTS words ("
                "id SERIAL PRIMARY KEY, "
                "word TEXT UNIQUE NOT NULL"
                ")"
            );

            txn.exec(
                "CREATE TABLE IF NOT EXISTS word_frequencies ("
                "document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE, "
                "word_id INTEGER REFERENCES words(id) ON DELETE CASCADE, "
                "frequency INTEGER NOT NULL, "
                "PRIMARY KEY (document_id, word_id)"
                ")"
            );

            // Create indexes
            txn.exec("CREATE INDEX IF NOT EXISTS idx_documents_url ON documents(url)");
            txn.exec("CREATE INDEX IF NOT EXISTS idx_words_word ON words(word)");
            txn.exec("CREATE INDEX IF NOT EXISTS idx_word_freq_word_id ON word_frequencies(word_id)");
            txn.exec("CREATE INDEX IF NOT EXISTS idx_word_freq_doc_id ON word_frequencies(document_id)");
        }

        txn.commit();
        std::cout << "✅ Database initialized successfully" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Database initialization error: " << e.what() << std::endl;
        throw;
    }
}

int Database::addDocument(const std::string& url, const std::string& title) {
    try {
        pqxx::work txn(*conn_);

        pqxx::result r = txn.exec_params(
            "INSERT INTO documents (url, title) VALUES ($1, $2) "
            "ON CONFLICT (url) DO UPDATE SET title = EXCLUDED.title "
            "RETURNING id",
            url, title
        );

        txn.commit();
        return r[0][0].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error adding document: " << e.what() << std::endl;
        throw;
    }
}

int Database::addWord(const std::string& word) {
    try {
        pqxx::work txn(*conn_);

        pqxx::result r = txn.exec_params(
            "INSERT INTO words (word) VALUES ($1) "
            "ON CONFLICT (word) DO UPDATE SET word = EXCLUDED.word "
            "RETURNING id",
            word
        );

        txn.commit();
        return r[0][0].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error adding word: " << e.what() << std::endl;
        throw;
    }
}

void Database::addWordFrequency(int document_id, int word_id, int frequency) {
    try {
        pqxx::work txn(*conn_);

        txn.exec_params(
            "INSERT INTO word_frequencies (document_id, word_id, frequency) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = EXCLUDED.frequency",
            document_id, word_id, frequency
        );

        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error adding word frequency: " << e.what() << std::endl;
        throw;
    }
}

bool Database::documentExists(const std::string& url) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT 1 FROM documents WHERE url = $1",
            url
        );
        return !r.empty();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error checking document existence: " << e.what() << std::endl;
        return false;
    }
}
