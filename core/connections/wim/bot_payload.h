#pragma once

namespace core
{
    struct icollection;
    namespace wim
    {
        struct bot_payload
        {
            std::string text_;
            std::string url_;
            bool show_alert_ = false;
            bool empty_ = true;

            void unserialize(const rapidjson::Value &_node);
            void serialize(icollection* _collection) const;
        };
    }
}