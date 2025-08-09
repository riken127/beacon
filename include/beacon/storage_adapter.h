//
// Created by Henrique on 8/9/2025.
//

#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "abstract_schema_validator.h"
#include "query_builder.h"

namespace beacon {
    using QueryBuilder = db::Db;

    struct Schema {
        int id;
        std::string name;
        int version;
        nlohmann::json definition;
        std::string created_at;
    };

    struct Event {
        int64_t id;
        std::string schema_name;
        int schema_version;
        std::optional<std::string> entity_id;
        nlohmann::json payload;
        std::optional<std::string> event_type;
        std::string created_at;
    };

    class StorageAdapter {
    public:
        StorageAdapter();

        int add_schema(const Schema &schema);

        std::optional<Schema> get_schema(const std::string &name, int version);

        std::int64_t store_event(const Event &event);

        std::vector<Event> query_events_by_entity(const std::string &entity_id);

    private:
        std::string get_connection_string();

        void create_schema_table();

        void create_events_table();

        std::unique_ptr<QueryBuilder> _queryBuilder;
    };
} // namespace beacon
