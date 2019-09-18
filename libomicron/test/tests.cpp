#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <omicron/omicron.h>

bool download(const std::string& url, std::string& data, long& code)
{
    static bool trigger = true;

    std::cout << "GET request: " << url << "\n\n";

    if (trigger)
    {
        data = "{\"config_v\" : 4, \"cond_s\" : \"44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a\","
            " \"segments\" : { \"_segment1\" : \"experimentA\", \"_segment2\" : \"experimentB\" },"
            " \"config\" : { "
            "     \"icq_cool_feature\" : \"bar\", \"icq_bool\" : true, \"icq_float\" : 1.25, "
            "     \"icq_json\" : { "
            "         \"icq_cool_feature\" : \"bar\", \"icq_bool\" : true, \"icq_float\" : 1.25 } } }";
    }
    else
    {
        data = "{\"config_v\" : 8, \"cond_s\" : \"a8ffaac16f060c0138e77ef12cf4bf49e9468e7f61da6411a8763b553af63144\","
            " \"config\" : {   } }";
    }

    code = 200;
    trigger = !trigger;

    return true;
}

using namespace omicronlib;

int main(int argc, char *argv[])
{
    omicron_config conf("https://e.mail.ru/api/v1/omicron/get", "icq_desktop_ic1nmMjqg7Yu-0hL", 10);
    conf.add_fingerprint("app_version", "10.0");
    conf.add_fingerprint("app_build", "1999");
    conf.add_fingerprint("os_version", "Windows 10");
    conf.add_fingerprint("device_id", "b9277c01-e6a2-4eaa-acd7-38be6dc0dd71");
    conf.add_fingerprint("account", "12345678");
    conf.add_fingerprint("lang", "ru");
    conf.add_fingerprint("", "");
#ifndef OMICRON_USE_LIBCURL
    conf.set_json_downloader(download);
#endif

    omicron_init(conf, L"1.dat");

    for (auto i = 0; i < 60; ++i)
    {
        auto tb = _o("icq_bool", false);
        auto ti = _o("int", 123);
        auto td = _o("icq_float", -0.001);
        auto ts = _o("icq_cool_feature", "unknow_feature");
        auto tj = _o("icq_json", json_string("{}"));

        std::cout << "tb = " << tb << '\n';
        std::cout << "ti = " << ti << '\n';
        std::cout << "td = " << td << '\n';
        std::cout << "ts = " << ts << '\n';
        std::cout << "tj = " << tj << '\n';

#ifdef _WIN32
        Sleep(5000);
#else
        sleep(5);
#endif
    }

    omicron_cleanup();

    return 0;
}
