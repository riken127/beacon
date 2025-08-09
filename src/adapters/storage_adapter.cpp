//
// Created by Henrique on 8/9/2025.
//

#include <beacon/storage_adapter.h>
#include <iostream>

#define CREATE_SCHEMA_TABLE_IF_NOT_EXISTS R"(
CREATE TABLE IF NOT EXISTS schemas (
id SERIAL PRIMARY KEY,
name TEXT NOT NULL,
version INTEGER NOT NULL DEFAULT 1,
definition JSONB NOT NULL,
created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
UNIQUE(name, version)
);
)"

#define CREATE_EVENTS_TABLE_IF_NOT_EXISTS R"(
CREATE TABLE IF NOT EXISTS events (
id BIGSERIAL PRIMARY KEY,
schema_name TEXT NOT NULL,
schema_version INTEGER NOT NULL,
entity_id TEXT,
payload JSONB NOT NULL,
event_type TEXT,
created_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);
)"

namespace beacon {
    StorageAdapter::StorageAdapter() {
        try {
            this->_queryBuilder = std::make_unique<QueryBuilder>(get_connection_string());
        } catch (const std::exception &e) {
            std::cerr << "Database connection error: " << e.what() << std::endl;
            throw;
        }

        // Execute both create schemas functions to load them in if necessary
        [&]() -> void {
            this->create_schema_table();
            this->create_events_table();
        }();
    }

    std::optional<Schema> StorageAdapter::get_schema(const std::string &name, int version) {
        const std::string sql = R"(
            SELECT id, name, version, definition, created_at
            FROM schemas
            WHERE name = $1 AND version = $2
            LIMIT 1
        )";

        try {
            auto results =
                    this->_queryBuilder->query<Schema>
                    (sql,
                     [](const pqxx::row &row) {
                         Schema s;
                         s.id = row["id"].as<int>();
                         s.name = row["name"].as<std::string>();
                         s.version = row["version"].as<int>();
                         s.definition = row["definition"].as<std::string>();
                         s.created_at = row["created_at"].as<std::string>();
                         return s;
                     },
                     name, version);

            if (results.empty()) {
                return std::nullopt;
            }
            return results[0];
        } catch (const db::DbError &e) {
            std::cerr << "getSchema error: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    int StorageAdapter::add_schema(const Schema &schema) {
        const std::string sql = R"(
            INSERT INTO schemas (name, version, definition)
            VALUES ($1, $2, $3)
            RETURNING id
        )";

        try {
            return this->_queryBuilder->exec_scalar<int>(sql, schema.name, schema.version, schema.definition.dump());
        } catch (const db::DbError &e) {
            std::cerr << "addSchema error: " << e.what() << std::endl;
            throw;
        }
    }

    std::int64_t StorageAdapter::store_event(const Event &event) {
        const std::string sql = R"(
            INSERT INTO events (schema_name, schema_version, entity_id, payload, event_type)
            VALUES ($1, $2, $3, $4, $5)
            RETURNING id
        )";

        try {
            return this->_queryBuilder->exec_scalar<std::int64_t>(sql, event.schema_name, event.schema_version,
                                                                  event.entity_id, event.payload.dump(),
                                                                  event.event_type);
        } catch (const db::DbError &e) {
            std::cerr << "storeEvent error: " << e.what() << std::endl;
            throw;
        }
    }

    std::vector<Event> StorageAdapter::query_events_by_entity(const std::string &entity_id) {
        const std::string sql = R"(
            SELECT id, schema_name, schema_version, entity_id, payload, event_type, created_at
            FROM events
            WHERE entity_id = $1
            ORDER BY created_at ASC
        )";

        try {
            return this->_queryBuilder->query<Event>(
                sql,
                [](const pqxx::row &row) {
                    Event e;
                    e.id = row["id"].as<int64_t>();
                    e.schema_name = row["schema_name"].as<std::string>();
                    e.schema_version = row["schema_version"].as<int>();
                    e.entity_id = row["entity_id"].is_null() ? "" : row["entity_id"].as<std::string>();
                    e.payload = row["payload"].as<std::string>();
                    e.event_type = row["event_type"].is_null() ? "" : row["event_type"].as<std::string>();
                    e.created_at = row["created_at"].as<std::string>();
                    return e;
                },
                entity_id
            );
        } catch (const db::DbError &e) {
            std::cerr << "queryEventsByEntity error: " << e.what() << std::endl;
            return {};
        }
    }


    /**
     * The create_schema_table creates a schema table in the respective postgresql database if it doesn't already exist.
     */
    void StorageAdapter::create_schema_table() {
        try {
            this->_queryBuilder->exec(CREATE_SCHEMA_TABLE_IF_NOT_EXISTS);
        } catch (const db::DbError &e) {
            std::cerr << "create_schema_table error: " << e.what() << std::endl;
            throw;
        }
    }

    void StorageAdapter::create_events_table() {
        try {
            this->_queryBuilder->exec(CREATE_EVENTS_TABLE_IF_NOT_EXISTS);
        } catch (const db::DbError &e) {
            std::cerr << "create_events_table error: " << e.what() << std::endl;
            throw;
        }
    }

    /**
     * The get_connection_string function is resposible to check for database environment variables,
     * such as postgreSQL's user, password or database. Based on this information it will retrieve a string
     * for production or development environments.
     * @return A connection string.
     */
    std::string StorageAdapter::get_connection_string() {
        // won't crasgh
        auto safe_getenv = [](const char *var) -> std::string {
            const char *val = std::getenv(var);
            return val ? std::string(val) : "";
        };

        auto is_non_empty_and_not_whitespace = [](const std::string &conn_str) -> bool {
            for (char ch: conn_str) {
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
