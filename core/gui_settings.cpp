#include "stdafx.h"
#include "gui_settings.h"

#include "core.h"

#include "tools/system.h"
#include "tools/json_helper.h"
#include "../../../corelib/collection_helper.h"

using namespace core;

enum gui_settings_types
{
    gst_name = 0,
    gst_value = 1
};

gui_settings::gui_settings(
    std::wstring _file_name,
    std::wstring _file_name_exported)
    : file_name_(std::move(_file_name)),
        file_name_exported_(std::move(_file_name_exported)),
        changed_(false)
{
}


gui_settings::~gui_settings()
{
    if (timer_ > 0)
        g_core->stop_timer(timer_);

    save_if_needed();
}

void gui_settings::start_save()
{
    timer_ = g_core->add_timer([wr_this = weak_from_this()]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->save_if_needed();
    }, std::chrono::seconds(10));
}

bool gui_settings::load()
{
    core::tools::binary_stream bstream;
    if (bstream.load_from_file(file_name_))
        return unserialize(bstream);

    return load_exported();
}

bool gui_settings::load_exported()
{
    core::tools::binary_stream bstream_exported;
    if (bstream_exported.load_from_file(file_name_exported_))
    {
        bstream_exported.write<char>('\0');

        rapidjson::Document doc;
        if (!doc.Parse(bstream_exported.read_available()).HasParseError())
        {
            if (unserialize(doc))
            {
                changed_ = true;
                return true;
            }
        }
    }

    return false;
}

void gui_settings::set_value(std::string_view _name, tools::binary_stream&& _data)
{
    values_[std::string(_name)] = std::move(_data);
    changed_ = true;
}

bool gui_settings::get_value(std::string_view _name, tools::binary_stream& _data) const
{
    if (const auto it = values_.find(_name); it != values_.end())
    {
        _data = it->second;
        return true;
    }

    return false;
}

void gui_settings::serialize(tools::binary_stream& _bs) const
{
    tools::tlvpack pack;

    int32_t counter = 0;

    for (auto iter = values_.begin(); iter != values_.end(); ++iter)
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(gui_settings_types::gst_name, iter->first));
        value_tlv.push_child(tools::tlv(gui_settings_types::gst_value, iter->second));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    pack.serialize(_bs);
}

bool gui_settings::unserialize(tools::binary_stream& _bs)
{
    if (!_bs.available())
    {
        return false;
    }

    tools::tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    for (auto tlv_val = pack.get_first(); tlv_val; tlv_val = pack.get_next())
    {
        tools::binary_stream val_data = tlv_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
            return false;

        auto tlv_name = pack_val.get_item(gui_settings_types::gst_name);
        auto tlv_value_data = pack_val.get_item(gui_settings_types::gst_value);

        if (!tlv_name || !tlv_value_data)
        {
            assert(false);
            return false;
        }

        values_[tlv_name->get_value<std::string>()] = tlv_value_data->get_value<tools::binary_stream>();
    }

    return true;
}

bool gui_settings::unserialize(const rapidjson::Value& _node)
{
    if (bool show_in_taskbar; tools::unserialize_value(_node, settings_show_in_taskbar, show_in_taskbar))
    {
        tools::binary_stream bs;
        bs.write<bool>(show_in_taskbar);
        set_value(settings_show_in_taskbar, std::move(bs));
    }

    if (bool enable_sounds; tools::unserialize_value(_node, settings_sounds_enabled, enable_sounds))
    {
        tools::binary_stream bs;
        bs.write<bool>(enable_sounds);
        set_value(settings_sounds_enabled, std::move(bs));
    }

    if (bool autosave_files; tools::unserialize_value(_node, settings_download_files_automatically, autosave_files))
    {
        tools::binary_stream bs;
        bs.write<bool>(autosave_files);
        set_value(settings_download_files_automatically, std::move(bs));
    }

    if (std::string download_folder; tools::unserialize_value(_node, settings_download_directory, download_folder))
    {
        tools::binary_stream bs;
        bs.write(download_folder.c_str(), download_folder.length() + 1);
        set_value(settings_download_directory, std::move(bs));
    }

    if (std::string download_folder_save_as; tools::unserialize_value(_node, settings_download_directory_save_as, download_folder_save_as))
    {
        tools::binary_stream bs;
        bs.write(download_folder_save_as.c_str(), download_folder_save_as.length() + 1);
        set_value(settings_download_directory_save_as, std::move(bs));
    }

    if (int32_t send_hotkey; tools::unserialize_value(_node, settings_key1_to_send_message, send_hotkey))
    {
        tools::binary_stream bs;
        bs.write<int32_t>(send_hotkey);
        set_value(settings_key1_to_send_message, std::move(bs));
    }

    if (bool enable_preview; tools::unserialize_value(_node, settings_show_video_and_images, enable_preview))
    {
        tools::binary_stream bs;
        bs.write<bool>(enable_preview);
        set_value(settings_show_video_and_images, std::move(bs));
    }

    if (std::string_view lang; tools::unserialize_value(_node, settings_language, lang))
    {
        tools::binary_stream bs;
        bs.write(lang);
        set_value(settings_language, std::move(bs));
    }

    if (bool notify; tools::unserialize_value(_node, settings_notify_new_messages, notify))
    {
        tools::binary_stream bs;
        bs.write<bool>(notify);
        set_value(settings_notify_new_messages, std::move(bs));
    }

    if (int shortcuts_close; tools::unserialize_value(_node, settings_shortcuts_close_action, shortcuts_close))
    {
        tools::binary_stream bs;
        bs.write<int>(shortcuts_close);
        set_value(settings_shortcuts_close_action, std::move(bs));
    }

    if (int shortcuts_search; tools::unserialize_value(_node, settings_shortcuts_search_action, shortcuts_search))
    {
        tools::binary_stream bs;
        bs.write<int>(shortcuts_search);
        set_value(settings_shortcuts_search_action, std::move(bs));
    }

    return true;
}

void gui_settings::serialize(core::coll_helper _collection) const
{
    ifptr<iarray> values_array(_collection->create_array(), true);

    for (const auto [name, value_data] : values_)
    {
        if (const int32_t len = value_data.available())
        {
            coll_helper coll_value(_collection->create_collection(), true);

            ifptr<istream> value_data_stream(_collection->create_stream(), true);
            value_data_stream->write((const uint8_t*)value_data.read(len), len);

            coll_value.set_value_as_string("name", name);
            coll_value.set_value_as_stream("value", value_data_stream.get());

            ifptr<ivalue> ival(_collection->create_value(), true);
            ival->set_as_collection(coll_value.get());

            values_array->push_back(ival.get());
        }
    }
    _collection.set_value_as_array("values", values_array.get());
}

void gui_settings::save_if_needed()
{
    if (changed_)
    {
        changed_ = false;

        auto bs_data = std::make_shared<tools::binary_stream>();
        serialize(*bs_data);

        g_core->run_async([bs_data, file_name = file_name_]
        {
            bs_data->save_2_file(file_name);

            return 0;
        });
    }
}

void gui_settings::clear_values()
{
    values_.clear();
    changed_ = true;
}

void gui_settings::clear_personal_values()
{
    clear_value(login_page_last_entered_phone);
    clear_value(login_page_last_entered_uin);
    clear_value(settings_recents_fs_stickers);
    clear_value(settings_old_recents_stickers);
}

void gui_settings::clear_value(const std::string_view _value)
{
    if (const auto it = values_.find(_value); it != values_.end())
    {
        values_.erase(it);
        changed_ = true;
    }
}
