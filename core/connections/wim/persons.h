#pragma once

#include "../../tools/string_comparator.h"

namespace core
{
    struct icollection;

    namespace archive
    {
        struct person
        {
            std::string friendly_;
            std::string nick_;
            std::optional<bool> official_;
        };

        class history_message;
        using history_block = std::vector<std::shared_ptr<history_message>>;

        using persons_map = std::map<std::string, person, tools::string_comparator>;
    }

    namespace wim
    {
        archive::persons_map parse_persons(const rapidjson::Value &_node);
        bool is_official(const rapidjson::Value &_node);

        void serialize_persons(const archive::persons_map& _persons, icollection* _collection);
    }
}