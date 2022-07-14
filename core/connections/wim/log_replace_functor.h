#pragma once

namespace core
{
    class log_replace_functor
    {
    public:
        using ranges = std::vector<std::pair<std::ptrdiff_t, std::ptrdiff_t>>;
    private:
        using ranges_evaluator = std::function<ranges(std::string_view s)>;
        using marker_item = std::pair<std::string, ranges_evaluator>;
    public:
        void add_marker(std::string_view _marker, ranges_evaluator _re = nullptr);
        void add_url_marker(std::string_view _marker, ranges_evaluator _re = nullptr);
        void add_json_marker(std::string_view _marker, ranges_evaluator _re = nullptr);

        //! It's able to find url-encoded array but doesn't mask it unless ranges_evaluator is specified
        void add_json_array_marker(std::string_view _marker, ranges_evaluator _re = nullptr);

        void add_message_markers();

        void operator()(tools::binary_stream& _bs) const;

        bool is_null() const;

    private:
        static void mask_char(char* c);

        std::vector<marker_item> markers_;
        std::vector<marker_item> markers_json_;
        std::vector<marker_item> markers_json_encoded_;
    };

    class aimsid_range_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class aimsid_single_range_evaluator : public aimsid_range_evaluator
    {
    public:
        std::pair<std::ptrdiff_t, std::ptrdiff_t> operator()(std::string_view s) const;
    };

    class speechtotext_fileid_range_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class poll_responses_ranges_evaluator
    {
    public:
        log_replace_functor::ranges operator()(std::string_view s) const;
    };

    class tail_from_last_range_evaluator
    {
    public:
        tail_from_last_range_evaluator(const char _chr);

        log_replace_functor::ranges operator()(std::string_view _str) const;

    private:
        const char chr_;
    };

    class distance_range_evaluator
    {
    public:
        distance_range_evaluator(std::ptrdiff_t _distance);

        log_replace_functor::ranges operator()(std::string_view _str) const;

    private:
        const std::ptrdiff_t distance_;
    };

}