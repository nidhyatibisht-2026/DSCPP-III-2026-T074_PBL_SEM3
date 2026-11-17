#include "Database.hpp"
#include <iostream>

Database::Database(const std::string& databasePath) : db(nullptr) {
    int result = sqlite3_open(databasePath.c_str(), &db);

    if (result != SQLITE_OK) {
        std::cerr << "[Database Error] "
                  << sqlite3_errmsg(db) << '\n';

        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
        return;
    }

    // Wait up to 20 seconds if another operation holds a database lock.
int timeoutResult = sqlite3_busy_timeout(db, 20000);

if (timeoutResult != SQLITE_OK) {
    std::cerr << "[Database Error] Failed to configure busy timeout.\n";
    sqlite3_close(db);
    db = nullptr;
    return;
}

// Enforce foreign-key constraints.
if (!execute("PRAGMA foreign_keys = ON;")) {
    std::cerr << "[Database Error] Failed to enable foreign keys.\n";
    sqlite3_close(db);
    db = nullptr;
    return;
}

std::cout << "[Database] Connected successfully.\n";
}

Database::~Database() {
    if (db != nullptr) {
        int result = sqlite3_close(db);

        if (result != SQLITE_OK) {
            std::cerr << "[Database Error] Failed to close database: "
                      << sqlite3_errmsg(db) << '\n';
        } else {
            db = nullptr;
        }
    }
}

sqlite3* Database::getConnection() const {
    return db;
}
bool Database::isConnected() const {
    return db != nullptr;
}
bool Database::execute(const std::string& sql) {
    if (db == nullptr) {
        std::cerr << "[Database Error] No active connection.\n";
        return false;
    }

    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        std::cerr << "[Database Error] "
                  << (errorMessage ? errorMessage : "Unknown error")
                  << '\n';

        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}