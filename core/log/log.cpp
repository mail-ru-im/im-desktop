#include "stdafx.h"

#include "../tools/threadpool.h"
#include "../tools/system.h"
#include "../utils.h"
#include "log.h"

#define LOG_FILE_EXT_HTML "html"
#define LOG_FILE_EXT_TEXT "txt"
#define LOG_FILE_EXT_HTMLW L"html"
#define LOG_FILE_EXT_TEXTW L"txt"

namespace
{
    using namespace core;
    using namespace tools;

    using namespace std::chrono;

    namespace fs = boost::filesystem;

    typedef time_point<system_clock, milliseconds> ms_time_point;

    enum class record_type
    {
        invalid = 0,
        min,

        trace = min,
        info,
        warn,
        error,
        net,

        max = net
    };

    struct log_record
    {
        log_record(const record_type _type, const std::string &_area, const std::string &_text, const ms_time_point &_ts);

        const record_type type_;
        const std::string area_;
        const std::string text_;
        const ms_time_point ts_;
    };

    typedef std::unique_ptr<log_record> log_record_uptr;

    typedef std::function<void(const log_record&, Out std::stringstream&)> format_record_fn;

    std::map<std::string, FILE*> log_files_;

    std::list<log_record_uptr> log_records_;

    std::mutex logging_thread_mutex_;

    std::atomic<bool> stop_signal_;

    std::unique_ptr<std::thread> logging_thread_;

    std::condition_variable logging_thread_cond_;

    format_record_fn record_formatter_;

    fs::path logs_dir_;

    int32_t log_file_index_ = -1;

    std::wstring log_files_ext_;

    bool trace_data_enabled_ = true;

    std::set<std::string> enabled_log_areas_;

    void enqueue_record(const record_type type_, const std::string &area_, const std::string &text_);

    void format_html(const log_record &_record, Out std::stringstream &_wss);

    void format_plain(const log_record &_record, Out std::stringstream &_wss);

    void logging_thread_proc();

    void read_enabled_log_areas_file();

}

namespace core
{
    namespace log
    {

        void determine_log_index()
        {
            boost::system::error_code error;
            assert(fs::is_directory(logs_dir_, Out error));
            assert(log_file_index_ == -1);

            using namespace boost::xpressive;

            static auto re = sregex::compile("(?P<index>\\d+)\\.\\w+\\.(" LOG_FILE_EXT_HTML "|" LOG_FILE_EXT_TEXT ")");

            auto max_index = -1;

            try
            {
                fs::directory_iterator dir_entry(logs_dir_, error);
                fs::directory_iterator dir_end;
                for (; dir_entry != dir_end && !error; dir_entry.increment(error))
                {
                    const auto file_name_path = dir_entry->path().filename();
                    const auto file_name_raw = file_name_path.c_str();
#ifdef _WIN32
                    const std::string filename = tools::from_utf16(file_name_raw);
#else
                    const std::string filename = file_name_raw;
#endif

                    smatch match;
                    if (!regex_match(filename, match, re))
                        continue;

                    const auto &index_str = match["index"].str();
                    const auto index = std::stoi(index_str);
                    assert(index >= 0);
                    max_index = std::max(max_index, index);
                }
            }
            catch (const std::exception&)
            {

            }

            log_file_index_ = (max_index + 1);
            assert(log_file_index_ >= 0);
        }

        bool create_logs_directory(const fs::wpath &_path)
        {
            boost::system::error_code error;
            if (!fs::is_directory(_path, error))
            {
                return core::tools::system::create_directory(_path);
            }

            return true;
        }

        void enable_trace_data(const bool _is_enabled)
        {
            trace_data_enabled_ = _is_enabled;
        }

        void init(const fs::wpath &_logs_dir, const bool _is_html)
        {
            assert(!logging_thread_);

            log_file_index_ = -1;
            stop_signal_ = false;
            logs_dir_.clear();

            if (_is_html)
            {
                record_formatter_= format_html;
                log_files_ext_ = LOG_FILE_EXT_HTMLW;
            }
            else
            {
                record_formatter_= format_plain;
                log_files_ext_ = LOG_FILE_EXT_TEXTW;
            }

            if (!create_logs_directory(_logs_dir))
            {
                assert(!"cannot create logs directory");
                return;
            }

            logs_dir_ = _logs_dir;

            read_enabled_log_areas_file();

            determine_log_index();

            logging_thread_ = std::make_unique<std::thread>(logging_thread_proc);
        }

        void shutdown()
        {
            stop_signal_ = true;
            logging_thread_cond_.notify_all();

            logging_thread_->join();
            logging_thread_.reset();

            for (auto &pair : log_files_)
            {
                auto file = pair.second;

                assert(file);
                ::fclose(file);
            }

            log_files_.clear();
        }
    }
}

namespace
{

    log_record::log_record(const record_type _type, const std::string &_area, const std::string &_text, const ms_time_point &_ts)
        : type_(_type)
        , area_(_area)
        , text_(_text)
        , ts_(_ts)
    {
        assert(type_ >= record_type::min);
        assert(type_ <= record_type::max);
        assert(!area_.empty());
        assert(!text_.empty());
    }

    void enqueue_record(const record_type _type, const std::string &_area, const std::string &_text)
    {
        assert(_type >= record_type::min);
        assert(_type <= record_type::max);
        assert(!_area.empty());
        assert(!_text.empty());

        const auto skip_trace_record = ((_type == record_type::trace) && !trace_data_enabled_);
        if (skip_trace_record)
        {
            return;
        }

        const auto is_net_record_type = (_type == record_type::net);
        const auto is_log_area_enabled = (
            is_net_record_type ||
            (enabled_log_areas_.count(_area) > 0)
            );
        if (!is_log_area_enabled)
        {
            return;
        }

        const auto now = time_point_cast<milliseconds>(system_clock::now());
        auto new_record = std::make_unique<log_record>(_type, _area, _text, now);

        {
            std::lock_guard<std::mutex> lock(logging_thread_mutex_);
            log_records_.push_back(std::move(new_record));
        }

        logging_thread_cond_.notify_one();
    }

    void format_footer_html(const log_record &_record, std::stringstream &_wss)
    {
        _wss << "<br><br>\n";
    }

    void format_header_html(const log_record &_record, std::stringstream &_wss)
    {
        switch (_record.type_)
        {
        case record_type::trace:
            _wss << "<font color=grey>";
            break;

        case record_type::info:
            _wss << "<font color=black>";
            break;

        case record_type::warn:
            _wss << "<font color=orange>";
            break;

        case record_type::error:
            _wss << "<font color=red>";
            break;

        default:
            assert(!"unknown record type");
        }


        const auto now_c = std::chrono::system_clock::to_time_t(_record.ts_);

        tm now_tm = { 0 };
#ifdef _WIN32
        localtime_s(&now_tm, &now_c);
#else
        localtime_r(&now_c, &now_tm);
#endif

        const auto fraction = (_record.ts_.time_since_epoch().count() % 1000);

        _wss << '[';
        _wss << std::put_time<char>(&now_tm, "%c");
        _wss << '.';
        _wss << fraction;
        _wss << "] --- ";
    }

    void format_body_html(const log_record &_record, std::stringstream &_wss)
    {
        assert(!_record.text_.empty());

        const auto &text = _record.text_;

        for (const auto ch : text)
        {
            if (ch == '<')
            {
                _wss << "&lt;";
            }
            else if (ch == '>')
            {
                _wss << "&gt;";
            }
            else if (ch == '\n')
            {
                _wss << "<br>\n";
            }
            else if (ch == '\t')
            {
                _wss << "&nbsp;&nbsp;&nbsp;&nbsp;";
            }
            else
            {
                _wss << ch;
            }
        }
    }

    void format_html(const log_record &_record, Out std::stringstream &_wss)
    {
        format_header_html(_record, Out _wss);
        format_body_html(_record, Out _wss);
        format_footer_html(_record, Out _wss);
    }

    void format_footer_plain(const log_record &_record, std::stringstream &_wss)
    {
        _wss << "\n\n";
    }

    void format_header_plain(const log_record &_record, std::stringstream &_wss)
    {
        _wss << '[';

        switch (_record.type_)
        {
        case record_type::trace:
            _wss << "TRACE";
            break;

        case record_type::info:
            _wss << "INFO";
            break;

        case record_type::warn:
            _wss << "WARN";
            break;

        case record_type::error:
            _wss << "ERROR";
            break;

        case record_type::net:
            _wss << "NETWORK";
            break;

        default:
            assert(!"unknown record type");
        }

        _wss << "] ";

        const auto now_c = std::chrono::system_clock::to_time_t(_record.ts_);

        tm now_tm = { 0 };
#ifdef _WIN32
        localtime_s(&now_tm, &now_c);
#else
        localtime_r(&now_c, &now_tm);
#endif

        const auto fraction = (_record.ts_.time_since_epoch().count() % 1000);

        _wss << std::put_time<char>(&now_tm, "%c")
            << '.'
            << fraction
            << std::endl;
    }

    void format_body_plain(const log_record &_record, std::stringstream &_wss)
    {
        assert(!_record.text_.empty());

        _wss << _record.text_;
    }

    void format_plain(const log_record &_record, Out std::stringstream &_wss)
    {
        format_header_plain(_record, Out _wss);
        format_body_plain(_record, Out _wss);
        format_footer_plain(_record, Out _wss);
    }

    fs::wpath get_area_file_path(std::string_view _area)
    {
        assert(!_area.empty());
        assert(!log_files_ext_.empty());

        std::wstring w_area = from_utf8(_area);

        boost::wformat area_filename(L"%06d.%s.%s");
        area_filename % log_file_index_ % w_area % log_files_ext_;

        fs::wpath path = logs_dir_;
        path.append(area_filename.str());

        return path;
    }

    FILE *get_area_file(const std::string &_area)
    {
        assert(!_area.empty());

        auto iter = log_files_.find(_area);
        if (iter != log_files_.end())
        {
            assert(iter->second);
            return iter->second;
        }

        auto path = get_area_file_path(_area);

#ifdef _WIN32
        auto file = ::_wfsopen(path.c_str(), L"wt", _SH_DENYWR);
        assert(file);

        log_files_.emplace(_area, file);

        return file;
#else
        auto file = ::fopen(path.c_str(), "wt, ccs=UTF-8");
        assert(file);

        log_files_.emplace(_area, file);

        return file;
#endif
    }

    void flush_records(const std::list<log_record_uptr> &records)
    {
        std::map<std::string, std::stringstream> to_write;

        for (const auto &record : records)
        {
            assert(record);

            auto iter = to_write.find(record->area_);
            if (iter == to_write.end())
            {
                const auto inserted = to_write.emplace(record->area_, std::stringstream());
                assert(inserted.second);
                iter = inserted.first;
            }

            auto &text = iter->second;

            assert(record_formatter_);
            record_formatter_(*record, text);
        }

        for (const auto &pair : to_write)
        {
            const auto &area = pair.first;
            const auto &text = pair.second;

            auto file = get_area_file(area);
            ::fputs(text.str().c_str(), file);
            ::fflush(file);
        }
    }

    bool process_records(std::unique_lock<std::mutex> &_lock)
    {
        std::list<log_record_uptr> records;
        records.swap(log_records_);

        _lock.unlock();

        flush_records(records);

        return !records.empty();
    }

    void logging_thread_proc()
    {
        core::utils::set_this_thread_name("logging");

        for (;;)
        {
            std::unique_lock<std::mutex> lock(logging_thread_mutex_);

            const auto stop_or_record = []
            {
                return (stop_signal_ || !log_records_.empty());
            };

            logging_thread_cond_.wait(lock, stop_or_record);

            for (;;)
            {
                if (!lock.owns_lock())
                {
                    lock.lock();
                }

                const auto has_records = process_records(lock);
                if (!has_records)
                {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(0));
            }

            if (stop_signal_)
            {
                return;
            }
        }
    }

    void read_enabled_log_areas_file()
    {
        using namespace core::tools;

        assert(!logs_dir_.empty());

        static const auto log_areas_file_name = L"!enabled";

        fs::wpath path(logs_dir_);
        path /= log_areas_file_name;

        binary_stream input;
        if (!input.load_from_file(path.wstring()))
        {
            return;
        }

        binary_stream_reader reader(input);

        while (!reader.eof())
        {
            auto line = reader.readline();

            boost::trim(line);

            if (line.empty())
            {
                continue;
            }

            const auto is_commented = (line[0] == '#');
            if (is_commented)
            {
                continue;
            }

            enabled_log_areas_.emplace(std::move(line));
        }
    }
}

#define LOG_IMPL(id, type)                                                    \
    void id(const std::string &_area,const std::string &_str)                \
{                                                                        \
    assert(!_area.empty());                                                \
    assert(!_str.empty());                                              \
    enqueue_record(type, _area, _str);                                    \
}                                                                        \
    \
    void id(const std::string &_area,const boost::format &_format)            \
{                                                                        \
    id(_area, _format.str());                                            \
}

namespace core
{

    namespace log
    {

        using namespace core;
        using namespace tools;

        LOG_IMPL(trace, record_type::trace);
        LOG_IMPL(info, record_type::info);
        LOG_IMPL(warn, record_type::warn);
        LOG_IMPL(error, record_type::error);

        void net(const boost::format &_format)
        {
            static const std::string net_area("net");

            enqueue_record(
                record_type::net,
                net_area,
                _format.str()
                );
        }

        boost::filesystem::wpath get_net_file_path()
        {
            return get_area_file_path(std::string_view("net"));
        }

    }

}