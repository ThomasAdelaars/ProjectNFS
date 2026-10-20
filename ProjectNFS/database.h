#pragma once
#include <mariadb/conncpp.hpp>   // of <conncpp.hpp> als er geen submap mariadb is
#include <memory>
#include <string>
#include <vector>

struct Db {
    std::unique_ptr<sql::Connection> conn;
    std::string error;

    bool Connect(const std::string& host, const std::string& db,
        const std::string& user, const std::string& pass) {
        try {
            sql::SQLString url("jdbc:mariadb://" + host + ":3306/" + db);
            sql::Properties props({ {"user", user}, {"password", pass} });
            conn.reset(sql::DriverManager::getConnection(url, props));
            error.clear();
            return true;
        }
        catch (sql::SQLException& e) {
            error = e.what();
            return false;
        }
    }

    bool Query(const std::string& q,
        std::vector<std::string>& cols,
        std::vector<std::vector<std::string>>& rows) {
        try {
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(q.c_str()));
            sql::ResultSetMetaData* meta = res->getMetaData();
            int n = meta->getColumnCount();
            cols.clear(); rows.clear();
            for (int i = 1; i <= n; i++) cols.push_back(meta->getColumnLabel(i).c_str());
            while (res->next()) {
                std::vector<std::string> row;
                for (int i = 1; i <= n; i++) row.push_back(res->getString(i).c_str());
                rows.push_back(row);
            }
            return true;
        }
        catch (sql::SQLException& e) {
            error = e.what();
            return false;
        }
    }
};