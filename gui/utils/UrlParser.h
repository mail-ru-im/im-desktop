#pragma once

namespace Utils
{
    struct FileSharingId;

    class UrlParser
    {
    public:
        enum class UrlCategory
        {
            Undefined,
            Image,
            Video,
            FileSharing,
            Site,
            Email,
            Ftp,
            ImProtocol,
            Profile
        };

        UrlParser();
        ~UrlParser();

        template<typename T>
        void operator()(T&& _text)
        {
            process(std::forward<T>(_text));
        }

        template<typename T>
        void process(T&& _text)
        {
            constexpr size_t MAX_LOCAL_SIZE = 1024;
            if (_text.isEmpty())
                return;

            wchar_t* buf;
            wchar_t local_buf[MAX_LOCAL_SIZE];
            if (_text.size() >= MAX_LOCAL_SIZE)
                buf = new wchar_t[_text.size() + 1];
            else
                buf = local_buf;

            std::copy(_text.utf16(), _text.utf16() + _text.size(), buf);
            processImpl(buf, _text.size());

            if (_text.size() >= MAX_LOCAL_SIZE)
                delete[] buf;
        }

        void setUrl(int32_t _scheme_hint, std::wstring_view _text);

        void registerUrlPattern(const QString& _pattern, UrlCategory category = UrlCategory::FileSharing);

        template<class _InIt>
        inline void registerUrlPatterns(_InIt _first, _InIt _last, UrlCategory _category = UrlCategory::FileSharing)
        {
            for (; _first != _last; ++_first)
                registerUrlPattern(*_first, _category);
        }

        QString fileSuffix() const;

        UrlCategory category() const;
        bool isFileSharing() const;
        bool isMedia() const;
        bool isImage() const;
        bool isVideo() const;
        bool isSite() const;
        bool isEmail() const;
        bool isFtp() const;
        bool isProfile() const;

        bool hasScheme() const;
        bool hasUrl() const;
        QString rawUrlString() const;
        QString formattedUrl() const;

        void reset();

        FileSharingId getFilesharingId() const;

    private:
        void processImpl(const wchar_t* _unicode, size_t _length);

    private:
        std::unique_ptr<class UrlParserPrivate> d;
    };
}
