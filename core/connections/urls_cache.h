#pragma once

namespace core::urls
{
    enum class url_type
    {
        wim_host,
        rapi_host,
        stickers_store_host,
        files_host,
        files_preview,
        smsreg_host,
        wapi_host,
        misc_www_host,
        preview_host,
        account_operations_host,
        smsapi_host,
        webrtc_host,
        oauth2_login,
        oauth2_token,
        oauth2_redirect,
        mytracker,
        files_info,
        avatars,
        url_content,
        sticker,
        wallpapers,
        tasks_host
    };

    enum class with_https
    {
        no,
        yes
    };

    std::string get_url(url_type _type, with_https _https = with_https::yes);

    bool is_one_domain_url(std::string_view _url) noexcept;

    struct api_version
    {
        static api_version& instance();
        int server() const noexcept;
        int client() const noexcept;
        constexpr static int minimal_supported() noexcept { return 71; }
        bool update();
        int preferred() const noexcept;
        std::string prefix() const;

    private:
        api_version();
        static int evaluate_client_version();
        int evaluate_server_version() const;
        int evaluate_preferred_version() const;

    private:
        int client_;
        int server_;
        int preferred_;
    };

} // namespace core::urls
