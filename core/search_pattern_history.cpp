#include "stdafx.h"

#include "search_pattern_history.h"
#include "tools/binary_stream.h"
#include "tools/system.h"
#include "../common.shared/string_utils.h"

using namespace core;
using namespace search;

search_pattern_history::search_pattern_history(std::wstring _path)
    : root_path_(std::move(_path))
{
}

void search_pattern_history::add_pattern(std::string_view _pattern, std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer)
{
    _executer->run_async_function([wr_this = weak_from_this(), contact = std::string(_contact), pattern = std::string(_pattern)]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        ptr_this->get_contact(contact).add_pattern(pattern);
        return 0;
    });
}

void search_pattern_history::remove_pattern(std::string_view _pattern, std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer)
{
    _executer->run_async_function([wr_this = weak_from_this(), contact = std::string(_contact), pattern = std::string(_pattern)]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        ptr_this->get_contact(contact).remove_pattern(pattern);
        return 0;
    });
}

std::shared_ptr<get_patterns_handler> search_pattern_history::get_patterns(std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer)
{
    auto handler = std::make_shared<get_patterns_handler>();
    auto patterns = std::make_shared<search_patterns>();

    auto wr_this = weak_from_this();

    _executer->run_async_function([wr_this, contact = std::string(_contact), patterns]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return -1;

        *patterns = ptr_this->get_contact(contact).get_patterns();
        return 0;

    })->on_result_ = [wr_this, handler, patterns](int32_t _err)
    {
        if (_err != 0)
            return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(*patterns);
    };

    return handler;
}

void search_pattern_history::free_dialog(const std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer)
{
    _executer->run_async_function([wr_this = weak_from_this(), contact = std::string(_contact)]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        ptr_this->contact_patterns_.erase(contact);
        return 0;
    });
}

void search_pattern_history::save_all(const std::shared_ptr<core::async_executer>& _executer)
{
    _executer->run_async_function([shared_this = shared_from_this()]()
    {
        for (auto& [_, pat] : shared_this->contact_patterns_)
            if (pat.save_needed())
                pat.save();

        return 0;
    });
}

std::wstring search_pattern_history::get_contact_path(std::string_view _contact) const
{
    std::wstring contact_filename = core::tools::from_utf8(_contact);
    std::replace(contact_filename.begin(), contact_filename.end(), L'|', L'_');

    return su::wconcat(root_path_, L'/', contact_filename, L"/hst");
}

contact_last_patterns& search_pattern_history::get_contact(std::string_view _contact)
{
    auto it = contact_patterns_.find(_contact);
    if (it == contact_patterns_.end())
    {
        contact_last_patterns clp(get_contact_path(_contact));
        it = contact_patterns_.emplace(_contact, std::move(clp)).first;
    }

    return it->second;
}


contact_last_patterns::contact_last_patterns(std::wstring _path)
    : path_(std::move(_path))
{
    load();
}

void contact_last_patterns::save()
{
    if (!save_needed_)
        return;

    std::string output = su::join(patterns_.cbegin(), patterns_.cend(), "\n");

    if (tools::binary_stream::save_2_file(output, path_))
        save_needed_ = false;
}

void contact_last_patterns::load()
{
    tools::binary_stream input;
    if (!input.load_from_file(path_))
        return;

    tools::binary_stream_reader reader(input);
    while (!reader.eof())
    {
        auto line = reader.readline();
        boost::trim(line);

        if (line.empty())
            continue;

        patterns_.push_back(std::move(line));
    }

    resize_if_needed();
}

void contact_last_patterns::add_pattern(std::string_view _pattern)
{
    if (_pattern.empty())
        return;

    const auto pattern_lower = ::tools::system::to_lower(_pattern);
    patterns_.remove_if([&pattern_lower](const auto& p) { return pattern_lower == tools::system::to_lower(p); });
    patterns_.push_front(std::string(_pattern));

    resize_if_needed();

    save_needed_ = true;
}

void contact_last_patterns::remove_pattern(std::string_view _pattern)
{
    if (patterns_.empty())
        return;

    const auto prev_size = patterns_.size();
    const auto pattern_lower = ::tools::system::to_lower(_pattern);
    patterns_.remove_if([&pattern_lower](const auto& p) { return pattern_lower == tools::system::to_lower(p); });

    if (patterns_.size() != prev_size) //TODO: refactor when c++20 arrives and use return value of remove_if
        save_needed_ = true;
}

void contact_last_patterns::resize_if_needed()
{
    if (patterns_.size() > core::search::max_patterns_count())
        patterns_.resize(core::search::max_patterns_count());
}
