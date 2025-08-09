//
// Created by Henrique on 8/9/2025.
//

#pragma once

#include <memory>
#include <string>
#include <pqxx/pqxx>
#include <pqxx/connection.hxx>

namespace beacon {
    class StorageAdapter {
    public:
        StorageAdapter();

        void store(const std::string &key, const std::string &value);
    private:
        std::string get_connection_string();

        std::unique_ptr<pqxx::connection> conn;
    };
} // namespace beacon
