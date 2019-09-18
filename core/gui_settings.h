#pragma once

#include "tools/settings.h"
#include "tools/string_comparator.h"

namespace core
{
    class coll_helper;

    class gui_settings : public std::enable_shared_from_this<gui_settings>
    {
    protected:
        std::map<std::string, tools::binary_stream, tools::string_comparator> values_;
        std::wstring file_name_;
        std::wstring file_name_exported_;

        bool changed_;
        uint32_t timer_;

        bool load_exported();

    public:

        gui_settings(
            std::wstring _file_name,
            std::wstring _file_name_exported);
        virtual ~gui_settings();

        bool load();

        void start_save();
        void clear_values();
        void clear_personal_values();
        void clear_value(const std::string_view _value);

        virtual void set_value(std::string_view _name, tools::binary_stream&& _data);
        virtual bool get_value(std::string_view _name, tools::binary_stream& _data) const;

        void serialize(core::coll_helper _collection) const;
        void serialize(tools::binary_stream& _bs) const;
        bool unserialize(tools::binary_stream& _bs);
        bool unserialize(const rapidjson::Value& _node);

        void save_if_needed();
    };

}