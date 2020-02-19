#pragma once

namespace core
{
    struct icollection;

    namespace archive
    {

    struct poll_data
    {
        struct answer
        {
            int64_t votes_ = 0;
            std::string text_;
            std::string answer_id_;
        };

        std::string id_;
        std::string type_;
        std::string my_answer_id_;
        std::vector<answer> answers_;

        bool closed_ = false;
        bool can_stop_ = false;

        void serialize(icollection* _collection) const;
        bool unserialize(icollection* _coll);
        bool unserialize(const rapidjson::Value& _node);
    };
    using poll = std::optional<poll_data>;

    }

}
