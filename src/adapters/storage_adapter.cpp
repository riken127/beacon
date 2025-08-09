//
// Created by Henrique on 8/9/2025.
//

#include <beacon/storage_adapter.h>
#include <iostream>
#include <memory>

namespace beacon {


    StorageAdapter::StorageAdapter() {
        try {
            conn = std::make_unique<pqxx::connection>(get_connection_string());

            if (!conn->is_open()) {
                throw std::runtime_error("Failed to open database connection");
            }
        } catch (const std::exception &e) {
            std::cerr << "Database connection error: " << e.what() << std::endl;
            throw;
        }
    }

    void StorageAdapter::store(const std::string &key, const std::string &value) {
        std::cout << "Storing key: " << key << " with value: " << value << std::endl;
    }

    /**
     * The get_connection_string function is resposible to check for database environment variables,
     * such as postgreSQL's user, password or database. Based on this information it will retrieve a string
     * for production or development environments.
     * @return A connection string.
     */
    std::string StorageAdapter::get_connection_string() {
        // won't crasgh
        auto safe_getenv = [](const char* var) -> std::string {
            const char* val = std::getenv(var);
            return val ? std::string(val) : "";
        };

        auto is_non_empty_and_not_whitespace = [](const std::string &conn_str) -> bool {
            for (char ch : conn_str) {
                if (!std::isspace(static_cast<unsigned char>(ch))) {
                    return true; // At least one non-whitespace char
                }
            }
            return false; // All whitespace or empty
        };


        std::string connection_string =
            "host=" + safe_getenv("PG_HOST") + " " +
            "port=" + safe_getenv("PG_PORT") + " " +
            "dbname=" + safe_getenv("PG_DBNAME") + " " +
            "user=" + safe_getenv("PG_USER") + " " +
            "password=" + safe_getenv("PG_PASSWORD");

        if (is_non_empty_and_not_whitespace(connection_string)) {
            return connection_string;
        }

        // returns a default development connection string.
        return "host=localhost port=5432 dbname=schema user=beacon password=beacon";
    }

} // namespace beacon
