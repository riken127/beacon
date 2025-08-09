//
// Created by José Henrique Noronha Oliveira e Silva on 8/9/2025.
//
// Licensed under the MIT License.
// You are free to use, modify, and distribute this code.
//
// A simple modern C++ wrapper for PostgreSQL using libpqxx.
// Provides easy query execution with parameter binding,
// result mapping via lambdas, and unified error handling.
//

#pragma once

#include <iostream>
#include <pqxx/pqxx>

namespace beacon::db {
    /**
     * Exception type thrown for database errors.
     */
    class DbError : public std::runtime_error {
    public:
        explicit DbError(const std::string &msg) : std::runtime_error(msg) {
        }
    };

    /**
     * Function signature to map pqxx::row -> user-defined type T.
     */
    template<typename T>
    using RowMapper = std::function<T(const pqxx::row &)>;

    /**
     * Database connection wrapper.
     * Simplifies query execution, parameter binding,
     * and maps results using user-provided mappers.
     */
    class Db {
        std::unique_ptr<pqxx::connection> conn;

    public:
        /**
         * Opens a database connection using the given connection string.
         * Throws std::runtime_error or DbError if connection fails.
         *
         * @param conn_info PostgreSQL connection string
         */
        explicit Db(const std::string &conn_info) {
            try {
                conn = std::make_unique<pqxx::connection>(conn_info);

                if (!conn->is_open()) {
                    throw std::runtime_error("Failed to open database connection");
                }
            } catch (const std::exception &e) {
                std::cerr << "Database connection error: " << e.what() << std::endl;
                throw;
            }
        }

        /**
         * Executes a parameterized SQL query and maps each row using the given mapper lambda.
         *
         * @tparam T Result type of each row after mapping
         * @tparam Args Types of parameters to bind
         * @param sql SQL query string with placeholders ($1, $2, ...)
         * @param mapper Lambda/function converting pqxx::row -> T
         * @param args Arguments to bind to query placeholders
         * @return Vector of mapped results
         *
         * @throws DbError on query or mapping errors
         */
        template<typename T, typename... Args>
        std::vector<T> query(const std::string &sql, RowMapper<T> mapper, Args &&... args) {
            try {
                pqxx::work txn{(*conn)};
                pqxx::result res = txn.exec_params(sql, std::forward<Args>(args)...);
                txn.commit();

                std::vector<T> results;
                for (const auto &row: res) {
                    results.push_back(mapper(row));
                }
                return results;
            } catch (const pqxx::sql_error &e) {
                std::ostringstream oss;
                oss << "SQL error:" << e.what() << "\nHad query: " << e.query();
                throw DbError(oss.str());
            } catch (const std::exception &e) {
                throw DbError(e.what());
            }
        }

        /**
         * Executes a scalar query (single value result).
         *
         * @tparam T Expected scalar type
         * @tparam Args Parameter types
         * @param sql SQL query string with placeholders
         * @param args Arguments to bind
         * @return Single value result of type T
         *
         * @throws DbError if no result or error occurs
         */
        template<typename T, typename... Args>
        T exec_scalar(const std::string &sql, Args &&... args) {
            try {
                pqxx::work txn{(*conn)};
                pqxx::result res = txn.exec_params(sql, std::forward<Args>(args)...);
                txn.commit();

                if (res.empty() || res[0].size() == 0) {
                    throw DbError("No scalar result returned");
                }

                return res[0][0].as<T>();
            } catch (const pqxx::sql_error &e) {
                std::ostringstream oss;
                oss << "SQL error: " << e.what() << "\nHad query: " << e.query();
                throw DbError(oss.str());
            } catch (const std::exception &e) {
                throw DbError(e.what());
            }
        }

        /**
         * Executes a non-returning query (e.g., INSERT, UPDATE).
         *
         * @tparam Args Parameter types
         * @param sql SQL query string with placeholders
         * @param args Arguments to bind
         *
         * @throws DbError on failure
         */
        template<typename... Args>
        void exec(const std::string &sql, Args &&... args) {
            try {
                pqxx::work txn{(*conn)};
                txn.exec_params(sql, std::forward<Args>(args)...);
                txn.commit();
            } catch (const pqxx::sql_error &e) {
                std::ostringstream oss;
                oss << "SQL error: " << e.what() << "\nHad query: " << e.query();
                throw DbError(oss.str());
            } catch (const std::exception &e) {
                throw DbError(e.what());
            }
        }
    };
} // beacon:db
