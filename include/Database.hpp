#ifndef FOODFLOW_DATABASE_HPP
#define FOODFLOW_DATABASE_HPP

#include <sqlite3.h>
#include <string>

class Database {
private:
    sqlite3* db;

public:
    explicit Database(const std::string& databasePath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* getConnection() const;
    bool isConnected() const;
    bool execute(const std::string& sql);
};

#endif