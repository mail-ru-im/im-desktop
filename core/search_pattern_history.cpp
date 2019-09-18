#include "stdafx.h"

#include "search_pattern_history.h"
#include "tools/binary_stream.h"
#include "tools/system.h"

using namespace core;
using namespace search;

search_pattern_history::search_pattern_history(const std::wstring & _path)
    : thread_(nullptr)
    , root_path_(_path)
{
}

void search_pattern_history::add_pattern(const std::string_view _pattern, const std::string_view _contact)
{
    auto wr_this = weak_from_this();

    get_thread()->run_async_function([wr_this, contact = std::string(_contact), pattern = std::string(_pattern)]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        ptr_this->get_contact(contact).add_pattern(pattern);

        return 0;
    });
}

void search_pattern_history::remove_pattern(const std::string_view _pattern, const std::string_view _contact)
{
    auto wr_this = weak_from_this();

    get_thread()->run_async_function([wr_this, contact = std::string(_contact), pattern = std::string(_pattern)]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        ptr_this->get_contact(contact).remove_pattern(pattern);

        return 0;
    });
}

std::shared_ptr<get_patterns_handler> search_pattern_history::get_patterns(const std::string_view _contact)
{
    auto handler = std::make_shared<get_patterns_handler>();
    auto patterns = std::make_shared<search_patterns>();

    auto wr_this = weak_from_this();

    get_thread()->run_async_function([wr_this, contact = std::string(_contact), patterns]()->int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 0;

        *patterns = ptr_this->get_contact(contact).get_patterns();
        return 0;

    })->on_result_ = [wr_this, handler, patterns](int32_t)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(*patterns);
    };

    return handler;
}

void search_pattern_history::save_all()
{
    for (auto& [_, pat] : contact_patterns_)
        pat.save();
}

std::wstring search_pattern_history::get_contact_path(const std::string_view _contact)
{
    std::wstring contact_filename = core::tools::from_utf8(_contact);
    std::replace(contact_filename.begin(), contact_filename.end(), L'|', L'_');

    return (root_path_ + L'/' + contact_filename + L"/hst");
}

contact_last_patterns& search_pattern_history::get_contact(const std::string_view _contact)
{
    auto it = contact_patterns_.find(_contact);
    if (it == contact_patterns_.end())
    {
        contact_last_patterns clp(get_contact_path(_contact));
        it = contact_patterns_.emplace(_contact, clp).first;
    }

    return it->second;
}

std::shared_ptr<async_executer> search_pattern_history::get_thread()
{
    if (!thread_)
        thread_ = std::make_shared<core::async_executer>("srchPatHst");

    return thread_;
}

contact_last_patterns::contact_last_patterns(const std::wstring& _path)
    : save_needed_(false)
    , path_(_path)
{
    load();
}

void contact_last_patterns::save()
{
    if (!save_needed_)
        return;

    tools::binary_stream output;
    for (const auto& p : patterns_)
        output.write(p + '\n');

    if (output.save_2_file(path_))
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

void core::search::contact_last_patterns::replace_last_pattern(std::string_view _pattern)
{
    if (!patterns_.empty())
        patterns_.pop_front();

    add_pattern(_pattern);
}

void contact_last_patterns::remove_pattern(std::string_view _pattern)
{
    if (patterns_.empty())
        return;

    const auto pattern_lower = ::tools::system::to_lower(_pattern);
    patterns_.remove_if([&pattern_lower](const auto& p) { return pattern_lower == tools::system::to_lower(p); });

    save_needed_ = true;
}

const search_patterns& contact_last_patterns::get_patterns() const
{
    return patterns_;
}

void contact_last_patterns::resize_if_needed()
{
    if (patterns_.size() > core::search::max_patterns_count())
        patterns_.resize(core::search::max_patterns_count());
}
