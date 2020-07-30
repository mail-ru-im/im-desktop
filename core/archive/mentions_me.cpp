#include "stdafx.h"

#include "archive_index.h"
#include "contact_archive.h"
#include "history_message.h"
#include "messages_data.h"
#include "storage.h"

#include "mentions_me.h"

#include <boost/range/adaptor/map.hpp>

using namespace core;
using namespace archive;


mentions_me::mentions_me(std::wstring _file_name)
    : storage_(std::make_unique<storage>(std::move(_file_name)))
{

}

mentions_me::~mentions_me()
{

}

void mentions_me::serialize(const std::shared_ptr<archive::history_message>& _mention, core::tools::binary_stream& _data) const
{
    _mention->serialize(_data);
}

bool mentions_me::unserialize(core::tools::binary_stream& _data)
{
    auto message = std::make_shared<archive::history_message>();

    if (message->unserialize(_data) != 0)
        return false;

    messages_[message->get_msgid()] = std::move(message);
    return true;
}

history_block mentions_me::get_messages() const
{
    const auto values = boost::adaptors::values(messages_);
    return history_block(values.begin(), values.end());
}

bool core::archive::mentions_me::load_from_local()
{
    if (loaded_from_local_)
        return true;

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage_->open(mode))
        return false;

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage] { p_storage->close(); });

    auto tmp_messages = std::move(messages_);
    messages_.clear();

    core::tools::binary_stream data_stream;
    while (storage_->read_data_block(-1, data_stream))
    {
        if (!unserialize(data_stream))
        {
            for (auto &x : tmp_messages)
                messages_.insert({x.first, x.second});

            return false;
        }

        data_stream.reset();
    }

    for (auto &x : tmp_messages)
        messages_.insert({ x.first, x.second });

    if (storage_->get_last_error() != archive::error::end_of_file)
        return false;

    loaded_from_local_ = true;
    return true;
}

void mentions_me::delete_up_to(int64_t _id)
{
    if (auto it = messages_.lower_bound(_id); it != messages_.end())
    {
        if (it->first == _id)
            ++it;
        messages_.erase(messages_.begin(), it);
        save_all();
    }
    else if (!messages_.empty())
    {
        messages_.clear();
        save_all();
    }
}

std::optional<int64_t> mentions_me::first_id() const
{
    if (messages_.empty())
        return {};
    return messages_.begin()->first;
}

void mentions_me::add(const std::shared_ptr<archive::history_message>& _message)
{
    if (_message->has_msgid())
    {
        if (auto[it, res] = messages_.insert({ _message->get_msgid(), _message }); res)
            save(_message);
    }
    else
    {
        assert(!"mentions_me: message has not id");
    }
}

storage::result_type mentions_me::save(const std::shared_ptr<archive::history_message>& _message)
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return { false, 0 };

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage] { p_storage->close(); });

    core::tools::binary_stream data;
    serialize(_message, data);

    int64_t offset = 0;
    return storage_->write_data_block(data, offset);
}

bool mentions_me::save_all()
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage_->open(mode))
        return false;

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage] { p_storage->close(); });

    if (messages_.empty())
        return true;

    core::tools::binary_stream block_data;
    for (const auto& [_, message] : messages_)
    {
        serialize(message, block_data);
        int64_t offset = 0;
        storage_->write_data_block(block_data, offset);
        block_data.reset();
    }
    return true;
}
