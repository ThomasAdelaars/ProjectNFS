#include "DatabaseConnection.h"

#include <stdexcept>
#include <string>

DatabaseConnection::DatabaseConnection(
    const std::string& host,
    const std::string& database,
    const std::string& username,
    const std::string& password,
    unsigned int port
)
    : host(host),
    database(database),
    username(username),
    password(password),
    port(port)
{
}

DatabaseConnection::~DatabaseConnection()
{
    disconnect();
}

void DatabaseConnection::connect()
{
    if (connection)
    {
        return;
    }

    const std::string url =
        "jdbc:mariadb://" + host + ":" +
        std::to_string(port) + "/" + database;

    connection.reset(
        sql::DriverManager::getConnection(
            sql::SQLString(url),
            sql::SQLString(username),
            sql::SQLString(password)
        )
    );
}

void DatabaseConnection::disconnect()
{
    connection.reset();
}

bool DatabaseConnection::isConnected() const
{
    return connection != nullptr;
}

sql::Connection& DatabaseConnection::getConnection()
{
    if (!connection)
    {
        throw std::runtime_error(
            "Geen databaseverbinding. Roep eerst connect() aan."
        );
    }

    return *connection;
}