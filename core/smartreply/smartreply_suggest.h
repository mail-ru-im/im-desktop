#pragma once

namespace core
{
    class coll_helper;

    namespace smartreply
    {
        enum class type;

        std::vector<smartreply::type> supported_types_for_fetch();

        class suggest
        {
        private:
            std::string aimid_;
            smartreply::type type_;
            int64_t msg_id_;
            std::string data_;

        public:
            suggest();

            void serialize(coll_helper& _coll) const;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            bool unserialize(const coll_helper& _coll);

            void set_msgid(const int64_t _msg_id) { msg_id_ = _msg_id; }
            const int64_t get_msgid() const { return msg_id_; }

            void set_aimid(std::string _aimid) { aimid_ = std::move(_aimid); }
            const std::string& get_aimid() const { return aimid_; }

            void set_type(const smartreply::type _type) { type_ = _type; }
            const smartreply::type get_type() const { return type_; }

            void set_data(std::string _data) { data_ = std::move(_data); }
            const std::string& get_data() const { return data_; }
        };

        std::vector<suggest> unserialize_suggests_node(const rapidjson::Value& _node);
        std::vector<suggest> unserialize_suggests_node(const rapidjson::Value& _node, smartreply::type _type, const std::string& _aimid, int64_t _msgid);
        void serialize_smartreply_suggests(const std::vector<smartreply::suggest>& _suggests, coll_helper& _coll);
    }
}