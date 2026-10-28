#pragma once

#include <mariadb/conncpp.hpp>
#include <memory>
#include <string>

class DatabaseConnection
{
public:
    DatabaseConnection(
        const std::string& host,
        const std::string& database,
        const std::string& username,
        const std::string& password,
        unsigned int port = 3306
    );

    ~DatabaseConnection();

    void connect();
    void disconnect();

    bool isConnected() const;

    sql::Connection& getConnection();

private:
    std::string host;
    std::string database;
    std::string username;
    std::string password;
    unsigned int port;

    std::unique_ptr<sql::Connection> connection;
};