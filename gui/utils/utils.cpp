#include "stdafx.h"

#include <rapidjson/writer.h>

#include "exif.h"
#include "utils.h"
#include "features.h"
#include "url_config.h"

#include "gui_coll_helper.h"
#include "InterConnector.h"
#include "SChar.h"
#include "profiling/auto_stop_watch.h"
#include "translit.h"
#include "PhoneFormatter.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../cache/countries.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/stickers/stickers.h"

#include "../controls/DpiAwareImage.h"
#include "../controls/GeneralDialog.h"
#include "../controls/TextEditEx.h"
#include "../controls/CheckboxList.h"
#include "../controls/textrendering/TextRendering.h"
#include "../controls/CallLinkWidget.h"
#include "../controls/UrlEditDialog.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/GroupChatOperations.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/RecentsModel.h"
#include "../main_window/contact_list/UnknownsModel.h"
#include "../main_window/contact_list/IgnoreMembersModel.h"
#include "../main_window/history_control/MessageStyle.h"
#include "../main_window/history_control/MessagesScrollArea.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/containers/LastseenContainer.h"
#include "../main_window/containers/StatusContainer.h"
#include "../styles/ThemesContainer.h"
#include "../styles/ThemeParameters.h"
#include "DrawUtils.h"
#include "../../common.shared/version_info.h"
#include "../../common.shared/config/config.h"
#include "previewer/toast.h"
#include "UrlCommand.h"
#include "log/log.h"
#include "../app_config.h"
#include "../statuses/StatusUtils.h"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <cache/emoji/EmojiDb.h>
    #include "macos/mac_support.h"
    typedef unsigned char byte;
#elif defined(__linux__)
    #define byte uint8_t
    Q_LOGGING_CATEGORY(linuxRunCommand, "linuxRunCommand")
#endif

namespace
{
    const int drag_preview_max_width = 320;
    const int drag_preview_max_height = 240;

    const int delete_messages_hor_offset = 16;
    const int delete_messages_top_offset = 16;
    const int messages_bottom_offset = 24;
    const int delete_messages_bottom_offset = 36;
    const int delete_messages_offset_short = 20;

    const int big_online_size = 20;
    const int online_size = 16;
    const int small_online_size = 12;

    static constexpr std::string_view base64_encoded_image_signature = "data:image/";

    constexpr int getUrlInDialogElideSize() noexcept { return 140; }

    constexpr QColor colorTable3[][3] =
    {
#ifdef __APPLE__
        { { 0xFB, 0xE8, 0x9F }, { 0xF1, 0xA1, 0x6A }, { 0xE9, 0x99, 0x36 } },
        { { 0xFB, 0xE7, 0xB0 }, { 0xE9, 0x62, 0x64 }, { 0xEF, 0x8B, 0x70 } },
        { { 0xFF, 0xBB, 0xAD }, { 0xD9, 0x3B, 0x50 }, { 0xED, 0x61, 0x5B } },
        { { 0xDB, 0xED, 0xB5 }, { 0x48, 0xAB, 0x68 }, { 0x3C, 0x9E, 0x55 } },
        { { 0xD4, 0xC7, 0xFC }, { 0x9E, 0x60, 0xC3 }, { 0xB5, 0x7F, 0xDB } },
        { { 0xE3, 0xAA, 0xEE }, { 0xCB, 0x55, 0x9C }, { 0xE1, 0x7C, 0xD0 } },
        { { 0xFE, 0xC8, 0x94 }, { 0xD3, 0x4F, 0xB6 }, { 0xEF, 0x72, 0x86 } },
        { { 0x95, 0xDF, 0xF7 }, { 0x8C, 0x75, 0xD0 }, { 0x68, 0x86, 0xE6 } },
        { { 0xB6, 0xEE, 0xFF }, { 0x1A, 0xB1, 0xD1 }, { 0x21, 0xA8, 0xCC } },
        { { 0x7F, 0xED, 0x9B }, { 0x3D, 0xB7, 0xC0 }, { 0x13, 0xA9, 0x8A } },
        { { 0xE2, 0xF4, 0x96 }, { 0x69, 0xBA, 0x3C }, { 0x6C, 0xCA, 0x30 } }
#else
        { { 0xFF, 0xEA, 0x90 }, { 0xFF, 0x9E, 0x5C }, { 0xFF, 0x9B, 0x20 } },
        { { 0xFF, 0xEA, 0xA6 }, { 0xFF, 0x5C, 0x60 }, { 0xFF, 0x82, 0x60 } },
        { { 0xFF, 0xBE, 0xAE }, { 0xF2, 0x33, 0x4C }, { 0xFE, 0x51, 0x4A } },
        { { 0xD8, 0xEF, 0xAB }, { 0x37, 0xB0, 0x5F }, { 0x26, 0xB4, 0x4A } },
        { { 0xD5, 0xC8, 0xFD }, { 0xA7, 0x5A, 0xD3 }, { 0xA3, 0x54, 0xDB } },
        { { 0xEB, 0xA6, 0xF8 }, { 0xDC, 0x4D, 0xA2 }, { 0xE1, 0x4E, 0xC8 } },
        { { 0xFF, 0xCA, 0x94 }, { 0xE6, 0x45, 0xC4 }, { 0xFF, 0x62, 0x7B } },
        { { 0x88, 0xE3, 0xFF }, { 0x81, 0x60, 0xDE }, { 0x56, 0x7D, 0xF8 } },
        { { 0xB6, 0xEE, 0xFF }, { 0x1A, 0xB1, 0xD1 }, { 0x0A, 0xB6, 0xE3 } },
        { { 0x70, 0xF2, 0x90 }, { 0x27, 0xBC, 0xC8 }, { 0x00, 0xBC, 0x95 } },
        { { 0xE4, 0xF5, 0x98 }, { 0x5B, 0xBD, 0x25 }, { 0x53, 0xCA, 0x07 } }
#endif
    };
    constexpr auto colorTableSize = sizeof(colorTable3)/sizeof(colorTable3[0]);

    constexpr quint32 CRC32Table[ 256 ] =
    {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
        0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
        0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
        0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
        0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
        0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
        0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
        0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
        0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
        0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
        0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
        0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
        0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
        0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
        0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
        0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
        0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
        0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
        0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
        0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
        0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
        0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
        0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
        0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
        0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
        0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
        0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
        0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
        0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
        0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
        0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    quint32 crc32FromIODevice(QIODevice* _device)
    {
        quint32 crc32 = 0xffffffff;

        std::array<char, std::size(CRC32Table)> buf;
        qint64 n = 0;

        while((n = _device->read(buf.data(), buf.size())) > 0)
            for (qint64 i = 0; i < n; ++i)
                crc32 = (crc32 >> 8) ^ CRC32Table[(crc32 ^ buf[i]) & 0xff];

        crc32 ^= 0xffffffff;
        return crc32;
    }

    quint32 crc32FromByteArray(const QByteArray& _array)
    {
        QBuffer buffer;
        buffer.setData(_array);
        if (!buffer.open(QIODevice::ReadOnly))
            return 0;

        return crc32FromIODevice(&buffer);
    }

    quint32 crc32FromString(QStringView _text)
    {
        QVarLengthArray<char> buf;
        buf.reserve(_text.size());
        for (auto ch : _text)
            buf.push_back(ch.unicode() > 0xff ? '?' : char(ch.unicode()));

        return crc32FromByteArray(QByteArray::fromRawData(buf.constData(), buf.size()));
    }

#ifdef _WIN32
    QChar VKeyToChar( short _vkey, HKL _layout)
    {
        DWORD dwScan=MapVirtualKeyEx(_vkey & 0x00ff, 0, _layout);
        byte KeyStates[256] = {0};
        KeyStates[_vkey & 0x00ff] = 0x80;
        DWORD dwFlag=0;
        if(_vkey & 0x0100)
        {
            KeyStates[VK_SHIFT] = 0x80;
        }
        if(_vkey & 0x0200)
        {
            KeyStates[VK_CONTROL] = 0x80;
        }
        if(_vkey & 0x0400)
        {
            KeyStates[VK_MENU] = 0x80;
            dwFlag = 1;
        }
        wchar_t Result = L' ';
        if (!ToUnicodeEx(_vkey & 0x00ff, dwScan, KeyStates, &Result, 1, dwFlag, _layout) == 1)
            return u' ';
        return QChar(Result);
    }
#endif //_WIN32

#ifndef __APPLE__
    bool isDangerousExtensions(QStringView _ext)
    {
        const static std::vector<QStringView> extensions =
        {
#ifdef _WIN32
            u"scr", u"exe", u"chm", u"cmd", u"pif", u"bat", u"vbs", u"jar", u"ade", u"adp", u"com", u"cpl",
            u"hta", u"ins", u"isp", u"jse", u"lib", u"lnk", u"mde", u"msc", u"msp", u"mst", u"sct", u"shb",
            u"sys", u"vb", u"vbe", u"vxd", u"wsc", u"wsf", u"wsh", u"reg", u"rgs", u"inf", u"msi", u"dll",
            u"cpl", u"job", u"ws", u"vbscript", u"gadget", u"shb", u"shs", u"sct", u"isu", u"inx", u"app", u"ipa",
            u"apk", u"rpm", u"apt", u"deb", u"dmg", u"action", u"command", u"xpi", u"crx", u"js", u"scf", u"url",
            u"ace"
#else
            u"desktop"
#endif
        };

        return std::any_of(extensions.cbegin(), extensions.cend(), [_ext](auto x) { return _ext.compare(x, Qt::CaseInsensitive) == 0; });
    }
#endif

    QString msgIdFromUidl(const QString& uidl)
    {
        if (uidl.size() != 16)
        {
            im_assert(!"wrong uidl");
            return QString();
        }

        byte b[8] = {0};
        bool ok;
        b[0] = uidl.midRef(0, 2).toUInt(&ok, 16);
        b[1] = uidl.midRef(2, 2).toUInt(&ok, 16);
        b[2] = uidl.midRef(4, 2).toUInt(&ok, 16);
        b[3] = uidl.midRef(6, 2).toUInt(&ok, 16);
        b[4] = uidl.midRef(8, 2).toUInt(&ok, 16);
        b[5] = uidl.midRef(10, 2).toUInt(&ok, 16);
        b[6] = uidl.midRef(12, 2).toUInt(&ok, 16);
        b[7] = uidl.midRef(14, 2).toUInt(&ok, 16);

        return QString::asprintf("%u%010u", *(int*)(b), *(int*)(b + 4*sizeof(byte)));
    }

    constexpr auto redirect() noexcept
    {
        return QStringView(u"&noredirecttologin=1");
    }

    QString getPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru  % u":443/cls3564/") % Ui::getUrlConfig().getUrlMailWin();
    }

    QString getFailPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru % u":443/cls3564/") % Ui::getUrlConfig().getUrlMailWin();
    }

    QString getReadMsgPage()
    {
        const QString& r_mail_ru = Ui::getUrlConfig().getUrlMailRedirect();
        return u"https://" % (r_mail_ru.isEmpty() ? QString() : r_mail_ru % u":443/cln8791/") % Ui::getUrlConfig().getUrlMailRead() % u"?id=";
    }

    QString getBaseMailUrl()
    {
        return u"https://" % Ui::getUrlConfig().getUrlMailAuth() % u"?Login=%1&agent=%2&ver=%3&agentlang=%4";
    }

    QString getMailUrl()
    {
        return getBaseMailUrl() % redirect() % u"&page=" % getPage() % u"&FailPage=" % getFailPage();
    }

    QString getMailOpenUrl()
    {
        return getBaseMailUrl() % redirect() % u"&page=" % getReadMsgPage() % u"%5&lang=%4" % u"&FailPage=" % getReadMsgPage() % u"%5&lang=%4";
    }



    QString getCpuId()
    {
#if defined(_WIN32)
        std::array<int, 4> cpui;
        std::vector<std::array<int, 4>> extdata;

        __cpuid(cpui.data(), 0x80000000);

        int nExIds = cpui[0];
        for (int i = 0x80000000; i <= nExIds; ++i)
        {
            __cpuidex(cpui.data(), i, 0);
            extdata.push_back(cpui);
        }

        char brand[0x40];
        memset(brand, 0, sizeof(brand));

        // Interpret CPU brand string if reported
        if (nExIds >= 0x80000004)
        {
            memcpy(brand, extdata[2].data(), sizeof(cpui));
            memcpy(brand + 16, extdata[3].data(), sizeof(cpui));
            memcpy(brand + 32, extdata[4].data(), sizeof(cpui));
            return QString::fromLatin1(brand);
        }
        return QString();
#elif defined(__APPLE__)
        const auto BUFFERLEN = 128;
        char buffer[BUFFERLEN];
        size_t bufferlen = BUFFERLEN;
        sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0);
        return QString::fromLatin1(buffer);
#elif defined(__linux__)
        std::string line;
        std::ifstream finfo("/proc/cpuinfo");
        while (std::getline(finfo, line))
        {
            std::stringstream str(line);
            std::string itype;
            std::string info;
            if (std::getline(str, itype, ':') && std::getline(str, info) && itype.substr(0, 10) == "model name")
                return QString::fromStdString(info);
        }
        return QString();
#endif
    }

    unsigned int getColorTableIndex(const QString& _uin)
    {
        const auto crc32 = crc32FromString(_uin.isEmpty() ? QStringView(u"0") : _uin);
        return (crc32 % colorTableSize);
    }

    struct AvatarBadge
    {
        QSize size_;
        QPoint offset_;
        QPixmap officialIcon_;
        QPixmap muteIcon_;
        QPixmap smallOnlineIcon_;
        QPixmap smallSelectedOnlineIcon_;
        QPixmap onlineIcon_;
        QPixmap onlineSelectedIcon_;
        int cutWidth_;
        int emojiOffsetY_;

        AvatarBadge(const int _size, const int _offsetX, const int _offsetY, const int _cutWidth, const QString& _officialPath, const QString& _mutedPath, const QString& _smallOnlinePath, const QString& _onlinePath, const QString& _onlineSelectedPath)
            : size_(Utils::scale_value(QSize(_size, _size)))
            , offset_(Utils::scale_value(_offsetX), Utils::scale_value(_offsetY))
            , cutWidth_(Utils::scale_value(_cutWidth))
            , emojiOffsetY_(0)
        {
            if (!_officialPath.isEmpty())
                officialIcon_ = Utils::renderSvg(_officialPath, size_);

            if (!_mutedPath.isEmpty())
                muteIcon_ = Utils::renderSvg(_mutedPath, size_);

            if (!_onlinePath.isEmpty())
            {
                onlineIcon_ = Utils::parse_image_name(_onlinePath);
                if (platform::is_windows())
                {
                    if (onlineIcon_.width() != Utils::scale_value(online_size) && onlineIcon_.width() != Utils::scale_value(big_online_size))
                        onlineIcon_ = onlineIcon_.scaledToWidth(Utils::scale_value(online_size));
                }
                Utils::check_pixel_ratio(onlineIcon_);

                onlineSelectedIcon_ = Utils::renderSvg(_onlineSelectedPath, onlineIcon_.size() / Utils::scale_bitmap(1));
            }

            if (!_smallOnlinePath.isEmpty())
            {
                smallOnlineIcon_ = Utils::parse_image_name(_smallOnlinePath);
                if (platform::is_windows())
                {
                    if (smallOnlineIcon_.width() != Utils::scale_value(small_online_size))
                        smallOnlineIcon_ = smallOnlineIcon_.scaledToWidth(Utils::scale_value(small_online_size));
                }
                Utils::check_pixel_ratio(smallOnlineIcon_);

                smallSelectedOnlineIcon_ = Utils::renderSvg(_onlineSelectedPath, smallOnlineIcon_.size() / Utils::scale_bitmap(1));
            }
        }
    };

    using badgeDataMap = std::map<int, AvatarBadge>;
    const badgeDataMap& getBadgesData()
    {
        static const badgeDataMap badges =
        {
            { Utils::scale_bitmap_with_value(24),  AvatarBadge(16, 15 , 15 , 2, qsl(":/avatar_badges/official_32"), qsl(":/avatar_badges/mute_32"), qsl(":/online_small_100"), qsl(":/online_100"), qsl(":/online_selected")) },
            { Utils::scale_bitmap_with_value(32),  AvatarBadge(12, 22 , 20 , 2, qsl(":/avatar_badges/official_32"), qsl(":/avatar_badges/mute_32"), qsl(":/online_small_100"), qsl(":/online_100"), qsl(":/online_selected")) },
            { Utils::scale_bitmap_with_value(44),  AvatarBadge(14, 30 , 30 , 2, qsl(":/avatar_badges/official_44"), QString(), QString(), QString(), QString()) },
            { Utils::scale_bitmap_with_value(52),  AvatarBadge(16, 35 , 35 , 2, qsl(":/avatar_badges/official_52"), qsl(":/avatar_badges/mute_52"), qsl(":/online_small_100"), qsl(":/online_100"), qsl(":/online_selected")) },
            { Utils::scale_bitmap_with_value(56),  AvatarBadge(16, 35 , 35 , 2, qsl(":/avatar_badges/official_52"), qsl(":/avatar_badges/mute_52"), qsl(":/online_small_100"), qsl(":/online_100"), qsl(":/online_selected")) },
            { Utils::scale_bitmap_with_value(60),  AvatarBadge(20, 42 , 40 , 4, qsl(":/avatar_badges/official_60"), qsl(":/avatar_badges/mute_52"), qsl(":/online_small_100"), qsl(":/online_big_100"), qsl(":/online_selected")) },
            { Utils::scale_bitmap_with_value(80),  AvatarBadge(20, 57 , 57 , 2, qsl(":/avatar_badges/official_60"), QString(), QString(), QString(), QString()) },
            { Utils::scale_bitmap_with_value(138), AvatarBadge(24, 106, 106, 2, qsl(":/avatar_badges/official_138"),QString(), QString(), QString(), QString()) },
        };
        return badges;
    };

    constexpr std::chrono::milliseconds getLoaderOverlayDelay()
    {
        return std::chrono::milliseconds(100);
    }
    auto avatarOverlayColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY);
    }
}


namespace Debug
{
    void debugFormattedText(QMimeData* _mime)
    {
        if constexpr (!build::is_debug())
            return;

        if (const auto rawText = _mime->data(MimeData::getRawMimeType()); !rawText.isEmpty())
            qDebug() << qsl("\ntext: %1").arg(QString::fromUtf8(rawText));

        if (_mime->hasFormat(MimeData::getTextFormatMimeType()))
        {
            if (const auto rawText = _mime->data(MimeData::getTextFormatMimeType()); !rawText.isEmpty())
                qDebug() << QString::fromUtf8(rawText);
        }
    }

    void dumpQtEvent(QEvent* _event, QStringView _context)
    {
#ifdef _DEBUG
        switch (_event->type())
        {
            case QEvent::Type::UpdateRequest:
            case QEvent::Type::Polish:
            case QEvent::Type::Paint:

            case QEvent::Type::Timer:

            case QEvent::Type::InputMethodQuery:

            case QEvent::Type::MouseMove:
            case QEvent::Type::NonClientAreaMouseMove:
            case QEvent::Type::CursorChange:
            case QEvent::Type::HoverMove:

            case QEvent::Type::WindowActivate:
            case QEvent::Type::WindowDeactivate:
            case QEvent::Type::ActivationChange:

            case QEvent::Type::FocusIn:
            case QEvent::Type::FocusOut:
            case QEvent::Type::FocusAboutToChange:
            case QEvent::Type::Enter:
            case QEvent::Type::Leave:

            case QEvent::Type::MetaCall:
            case QEvent::Type::HelpRequest:
            case QEvent::Type::ToolTip:
            case QEvent::Type::PaletteChange:
            case QEvent::Type::ActionChanged:
            case QEvent::Type::LayoutRequest:

                return;

            default:
                ;
        }

        auto deb = qDebug();
        deb << QDateTime::currentDateTime().toString(u"mm:ss.zzz");
        if (!_context.isEmpty())
            deb << _context;
        deb << _event;
#else
        Q_UNUSED(_event)
        Q_UNUSED(_context)
#endif
    }
}


namespace Utils
{
    QString getAppTitle()
    {
        const auto str = config::get().translation(config::translations::app_title);
        return QApplication::translate("title", str.data());
    }

    QString getVersionLabel()
    {
#if defined(GIT_COMMIT_HASH) && defined(GIT_BRANCH_NAME) && defined(BUILD_TIME)
        return getAppTitle() % u' ' % QString::fromStdString(core::tools::version_info().get_version_with_patch()) % u" (" % ql1s(GIT_COMMIT_HASH) % u", " % ql1s(GIT_BRANCH_NAME) % u") " % ql1s(BUILD_TIME);
#else
        return getAppTitle() % u" (" % QString::fromStdString(core::tools::version_info().get_version_with_patch()) % u')';
#endif
    }

    QString getVersionPrintable()
    {
        return getAppTitle() % u' ' % QString::fromStdString(core::tools::version_info().get_version_with_patch());
    }

    void drawText(QPainter& _painter, const QPointF& _point, int _flags,
        const QString& _text, QRectF* _boundingRect)
    {
        const qreal size = 32767.0;
        QPointF corner(_point.x(), _point.y() - size);

        if (_flags & Qt::AlignHCenter)
            corner.rx() -= size/2.0;
        else if (_flags & Qt::AlignRight)
            corner.rx() -= size;

        if (_flags & Qt::AlignVCenter)
            corner.ry() += size/2.0;
        else if (_flags & Qt::AlignTop)
            corner.ry() += size;
        else _flags |= Qt::AlignBottom;

        QRectF rect(corner, QSizeF(size, size));
        _painter.drawText(rect, _flags, _text, _boundingRect);
    }

    bool supportUpdates() noexcept
    {
#ifdef UPDATES
        return Ui::GetAppConfig().IsUpdateble();
#else
        return false;
#endif
    }

    ShadowWidgetEventFilter::ShadowWidgetEventFilter(int _shadowWidth)
        : QObject(nullptr)
        , ShadowWidth_(_shadowWidth)
    {
    }

    bool ShadowWidgetEventFilter::eventFilter(QObject* obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Paint)
        {
            QWidget* w = qobject_cast<QWidget*>(obj);

            QRect origin = w->rect();

            QRect right = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1),
                QPoint(origin.width(), origin.height() - ShadowWidth_ - 1)
            );

            QRect left = QRect(
                QPoint(origin.x(), origin.y() + ShadowWidth_),
                QPoint(origin.x() + ShadowWidth_ - 1, origin.height() - ShadowWidth_ - 1)
            );

            QRect top = QRect(
                QPoint(origin.x() + ShadowWidth_ + 1, origin.y()),
                QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_)
            );

            QRect bottom = QRect(
                QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_),
                QPoint(origin.width() - ShadowWidth_ - 1, origin.height())
            );

            QRect topLeft = QRect(
                origin.topLeft(),
                QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_)
            );

            QRect topRight = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.y()),
                QPoint(origin.width(), origin.y() + ShadowWidth_)
            );

            QRect bottomLeft = QRect(
                QPoint(origin.x(), origin.height() - ShadowWidth_),
                QPoint(origin.x() + ShadowWidth_, origin.height())
            );

            QRect bottomRight = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_),
                origin.bottomRight()
            );

            QPainter p(w);

            bool isActive = w->isActiveWindow();

            QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
            setGradientColor(lg, isActive);
            p.fillRect(right, QBrush(lg));

            lg = QLinearGradient(left.topRight(), left.topLeft());
            setGradientColor(lg, isActive);
            p.fillRect(left, QBrush(lg));

            lg = QLinearGradient(top.bottomLeft(), top.topLeft());
            setGradientColor(lg, isActive);
            p.fillRect(top, QBrush(lg));

            lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
            setGradientColor(lg, isActive);
            p.fillRect(bottom, QBrush(lg));

            QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
            setGradientColor(g, isActive);
            p.fillRect(topLeft, QBrush(g));

            g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
            setGradientColor(g, isActive);
            p.fillRect(topRight, QBrush(g));

            g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
            setGradientColor(g, isActive);
            p.fillRect(bottomLeft, QBrush(g));

            g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
            setGradientColor(g, isActive);
            p.fillRect(bottomRight, QBrush(g));
        }

        return QObject::eventFilter(obj, _event);
    }

    void ShadowWidgetEventFilter::setGradientColor(QGradient& _gradient, bool _isActive)
    {
        QColor windowGradientColor(u"#000000");
        windowGradientColor.setAlphaF(0.2);
        _gradient.setColorAt(0, windowGradientColor);
        windowGradientColor.setAlphaF(_isActive ? 0.08 : 0.04);
        _gradient.setColorAt(0.2, windowGradientColor);
        windowGradientColor.setAlphaF(0.02);
        _gradient.setColorAt(0.6, _isActive ? windowGradientColor : Qt::transparent);
        _gradient.setColorAt(1, Qt::transparent);
    }

    bool isUin(const QString& _aimId)
    {
        return !_aimId.isEmpty() && std::all_of(_aimId.begin(), _aimId.end(), [](auto c) { return c.isDigit(); });
    }

    const std::vector<QString>& urlSchemes()
    {
        const static auto schemes = []() {
            const auto scheme_csv = config::get().string(config::values::register_url_scheme_csv); //url1,descr1,url2,desc2,..
            const auto splitted = QString::fromUtf8(scheme_csv.data(), scheme_csv.size()).split(QLatin1Char(','));
            std::vector<QString> res;
            res.reserve(splitted.size() / 2);
            for (int i = 0; i < splitted.size(); i += 2)
                res.push_back(splitted.at(i));
            return res;
        }();

        return schemes;
    }

    bool isInternalScheme(const QString& scheme)
    {
        const auto& schemes = urlSchemes();
        return std::any_of(schemes.begin(), schemes.end(), [&scheme](const auto& x) { return x.compare(scheme, Qt::CaseInsensitive) == 0; });
    }

    QString getCountryNameByCode(const QString& _isoCode)
    {
        const auto &countries = Ui::countries::get();
        const auto i = std::find_if(
            countries.begin(), countries.end(), [&_isoCode](const Ui::countries::country &v)
        {
            return v.iso_code_.compare(_isoCode, Qt::CaseInsensitive) == 0;
        });
        if (i != countries.end())
            return i->name_;
        return QString();
    }

    QString getCountryCodeByName(const QString & _name)
    {
        const auto &countries = Ui::countries::get();
        const auto i = std::find_if(
            countries.begin(), countries.end(), [&_name](const Ui::countries::country &v)
        {
            return v.name_.compare(_name, Qt::CaseInsensitive) == 0;
        });
        if (i != countries.end())
            return i->iso_code_.toString();
        return QString();
    }

    QMap<QString, QString> getCountryCodes()
    {
        QMap<QString, QString> result;
        for (const auto& country : Ui::countries::get())
            result.insert(country.name_, u'+' % QString::number(country.phone_code_));
        return result;
    }

    QString ScaleStyle(const QString& _style, double _scale)
    {
        QString outString;
        QTextStream result(&outString);

        static const QRegularExpression semicolon(
            ql1s("\\;"),
            QRegularExpression::UseUnicodePropertiesOption);
        static const QRegularExpression dip(
            ql1s("[\\-\\d]\\d*dip"),
            QRegularExpression::UseUnicodePropertiesOption);

        const auto tokens = _style.split(semicolon);

        for (const auto& line : tokens)
        {
            const auto lineView = QStringView(line);
            int pos = line.indexOf(dip);

            if (pos != -1)
            {
                result << lineView.left(pos);
                const auto tmp = line.midRef(pos, line.rightRef(pos).size());
                const int size = tmp.left(tmp.indexOf(ql1s("dip"))).toInt() * _scale;
                result << size;
                result << QStringView(u"px");
            }
            else
            {
                const double currentScale = _scale >= 2.0 ? 2.0 : _scale;

                const auto scalePostfix = ql1s("_100");
                pos = line.indexOf(scalePostfix);
                if (pos != -1)
                {
                    result << lineView.left(pos);
                    result << ql1c('_');
                    result << (Utils::is_mac_retina() ? 2 : currentScale) * 100;
                    result << lineView.mid(pos + scalePostfix.size());
                }
                else
                {
                    const auto scaleDir = ql1s("/100/");
                    pos = line.indexOf(scaleDir);
                    if (pos != -1)
                    {
                        result << lineView.left(pos);
                        result << ql1c('/');
                        result << Utils::scale_bitmap(currentScale) * 100;
                        result << ql1c('/');
                        result << lineView.mid(pos + scaleDir.size());
                    }
                    else
                    {
                        result << line;
                    }
                }
            }
            result << ql1c(';');
        }
        if (!outString.isEmpty())
            outString.chop(1);

        return outString;
    }

    void ApplyPropertyParameter(QWidget* _widget, const char* _property, QVariant _parameter)
    {
        if (_widget)
        {
            _widget->setProperty(_property, _parameter);
            _widget->style()->unpolish(_widget);
            _widget->style()->polish(_widget);
            _widget->update();
        }
    }

    void ApplyStyle(QWidget* _widget, const QString& _style)
    {
        if (_widget)
        {
            const QString newStyle = Fonts::SetFont(Utils::ScaleStyle(_style, Utils::getScaleCoefficient()));
            if (newStyle != _widget->styleSheet())
                _widget->setStyleSheet(newStyle);
        }
    }

    QString LoadStyle(const QString& _qssFile)
    {
        QFile file(_qssFile);

        if (!file.open(QIODevice::ReadOnly))
        {
            im_assert(!"open style file error");
            return QString();
        }

        QString qss = QString::fromUtf8(file.readAll());
        if (qss.isEmpty())
            return QString();

        return ScaleStyle(Fonts::SetFont(std::move(qss)), Utils::getScaleCoefficient());
    }

    // COLOR
    QPixmap getDefaultAvatar(const QString& _uin, const QString& _displayName, const int _sizePx)
    {
        const auto antialiasSizeMult = 8;
        const auto bigSizePx = (_sizePx * antialiasSizeMult);
        const auto bigSize = QSize(bigSizePx, bigSizePx);
        const auto isMailIcon = _uin == u"mail";

        QImage bigResult(bigSize, QImage::Format_ARGB32);

        // evaluate colors
        const auto colorIndex = getColorTableIndex(_uin == getDefaultCallAvatarId() ? _displayName.at(0) : _uin);
        QColor color1 = colorTable3[colorIndex][0];
        QColor color2 = colorTable3[colorIndex][1];

        if (_uin.isEmpty() && _displayName.isEmpty())
        {
            color1 = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
            color2 = color1;
        }

        if (isMailIcon)
        {
            color1 = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
            color2 = color1;
        }

        QPainter painter(&bigResult);

        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

        // set pen

        QPen hollowAvatarPen(1);

        const auto hollowPenWidth = scale_value(2 * antialiasSizeMult);
        hollowAvatarPen.setWidth(hollowPenWidth);

        QPen filledAvatarPen(Qt::white);

        QPen &avatarPen = isMailIcon ? hollowAvatarPen : filledAvatarPen;
        painter.setPen(avatarPen);

        // draw

        if (isMailIcon)
        {
            bigResult.fill(Qt::transparent);
            painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

            const auto correction = ((hollowPenWidth / 2) + 1);
            const auto ellipseRadius = (bigSizePx - (correction * 2));
            painter.drawEllipse(correction, correction, ellipseRadius, ellipseRadius);
        }
        else if (!_uin.isEmpty())
        {
            painter.setPen(Qt::NoPen);

            QLinearGradient radialGrad(QPointF(0, 0), QPointF(bigSizePx, bigSizePx));
            radialGrad.setColorAt(0, color1);
            radialGrad.setColorAt(1, color2);

            painter.fillRect(QRect(0, 0, bigSizePx, bigSizePx), radialGrad);
        }

        if (const auto overlay = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_OVERLAY); !_uin.isEmpty() && overlay.isValid() && overlay.alpha() > 0)
        {
            Utils::PainterSaver ps(painter);
            painter.setCompositionMode(QPainter::CompositionMode_Overlay);
            painter.fillRect(QRect(0, 0, bigSizePx, bigSizePx), overlay);
        }

        auto scaledBigResult = bigResult.scaled(QSize(_sizePx, _sizePx), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        if (isMailIcon)
        {
            {
                QPainter scaled(&scaledBigResult);
                scaled.setRenderHint(QPainter::Antialiasing);
                scaled.setRenderHint(QPainter::TextAntialiasing);
                scaled.setRenderHint(QPainter::SmoothPixmapTransform);
                scaled.drawPixmap(0, 0, renderSvg(qsl(":/mail_icon"), QSize(_sizePx, _sizePx)));
            }
            return QPixmap::fromImage(std::move(scaledBigResult));
        }

        auto fillDefault = [_sizePx](QImage& _result)
        {
            _result.fill(Qt::transparent);
            QPainter p(&_result);
            p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

            p.fillRect(QRect(0, 0, _sizePx, _sizePx), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.08));

            QPen pen;
            pen.setWidth(Utils::scale_bitmap(Utils::scale_value(1)));
            pen.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            pen.setStyle(Qt::PenStyle::SolidLine);
            p.setPen(pen);

            auto start = 0;
            auto span = 9;
            while (start < 360)
            {
                p.drawArc(QRect(Utils::scale_value(1), Utils::scale_value(1), _sizePx - Utils::scale_value(2), _sizePx - Utils::scale_value(2)), start * 16, span * 16);
                start += span * 2;
            }
        };

        const auto trimmedDisplayName = QStringView(_displayName).trimmed();
        if (trimmedDisplayName.isEmpty())
        {
            QImage result(QSize(_sizePx, _sizePx), QImage::Format_ARGB32);
            fillDefault(result);
            {
                QPainter p(&result);
                const auto icon_size = Utils::scale_value(32) * Utils::scale_bitmap(1);
                p.drawPixmap((_sizePx - icon_size) / 2, (_sizePx - icon_size) / 2, renderSvg(qsl(":/cam_icon"), QSize(icon_size, icon_size), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)));
            }
            return QPixmap::fromImage(std::move(result));
        }

        if (_uin.isEmpty())
            fillDefault(scaledBigResult);

        const auto percent = _sizePx / scale_bitmap_ratio() <= Ui::MessageStyle::getLastReadAvatarSize() ? 0.64 : 0.54;
        const auto fontSize = _sizePx * percent;

        qsizetype pos = 0;
        if (const auto startEmoji = Emoji::getEmoji(trimmedDisplayName, pos); startEmoji != Emoji::EmojiCode())
        {
            QPainter emojiPainter(&scaledBigResult);
            auto nearestSizeAvailable = _sizePx / 2 * scale_bitmap_ratio();
            const auto emoji = Ui::DpiAwareImage(Emoji::GetEmoji(startEmoji, nearestSizeAvailable));
            emoji.draw(emojiPainter, (_sizePx - emoji.width()) / 2, (_sizePx - emoji.height()) / 2 - (platform::is_apple() ? 0 : 1));
        }
        else
        {
            QFont font = Fonts::appFont(fontSize, Fonts::FontFamily::ROUNDED_MPLUS);

            QPainter letterPainter(&scaledBigResult);
            letterPainter.setRenderHint(QPainter::Antialiasing);
            letterPainter.setFont(font);
            if (_uin.isEmpty())
                avatarPen.setColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            letterPainter.setPen(avatarPen);

            const auto toDisplay = trimmedDisplayName.at(0).toUpper();

            auto rawFont = QRawFont::fromFont(font);
            if constexpr (platform::is_apple())
            {
                if (!rawFont.supportsCharacter(toDisplay))
                    rawFont = QRawFont::fromFont(QFont(qsl("AppleColorEmoji"), fontSize));
            }

            quint32 glyphIndex = 0;
            int numGlyphs = 1;
            const auto glyphSearchSucceed = rawFont.glyphIndexesForChars(&toDisplay, 1, &glyphIndex, &numGlyphs);
            if (!glyphSearchSucceed)
                return QPixmap::fromImage(std::move(scaledBigResult));

            im_assert(numGlyphs == 1);

            auto glyphPath = rawFont.pathForGlyph(glyphIndex);

            const auto rawGlyphBounds = glyphPath.boundingRect();
            glyphPath.translate(-rawGlyphBounds.x(), -rawGlyphBounds.y());
            const auto glyphBounds = glyphPath.boundingRect();
            const auto glyphHeight = glyphBounds.height();
            const auto glyphWidth = glyphBounds.width();

            QFontMetrics m(font);
            QRect pseudoRect(0, 0, _sizePx, _sizePx);
            auto centeredRect = m.boundingRect(pseudoRect, Qt::AlignCenter, QString(toDisplay));

            qreal y = (_sizePx / 2.0);
            y -= glyphHeight / 2.0;

            qreal x = centeredRect.x();
            x += (centeredRect.width() / 2.0);
            x -= (glyphWidth / 2.0);

            letterPainter.translate(x, y);
            letterPainter.fillPath(glyphPath, avatarPen.brush());
        }
        return QPixmap::fromImage(std::move(scaledBigResult));
    }

    void updateBgColor(QWidget* _widget, const QColor& _color)
    {
        im_assert(_widget);
        im_assert(_color.isValid());
        if (_widget)
        {
            im_assert(_widget->autoFillBackground());

            auto pal = _widget->palette();
            pal.setColor(QPalette::Window, _color);
            _widget->setPalette(pal);
            _widget->update();
        }
    }

    void setDefaultBackground(QWidget* _widget)
    {
        im_assert(_widget);
        if (_widget)
        {
            _widget->setAutoFillBackground(true);
            updateBgColor(_widget, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        }
    }

    void drawBackgroundWithBorders(QPainter& _p, const QRect& _rect, const QColor& _bg, const QColor& _border, const Qt::Alignment _borderSides, const int _borderWidth)
    {
        Utils::PainterSaver ps(_p);
        _p.setPen(Qt::NoPen);

        if (_bg.isValid())
            _p.fillRect(_rect, _bg);

        if (_border.isValid())
        {
            const QRect borderHor(0, 0, _rect.width(), _borderWidth);
            if (_borderSides & Qt::AlignTop)
                _p.fillRect(borderHor, _border);
            if (_borderSides & Qt::AlignBottom)
                _p.fillRect(borderHor.translated(0, _rect.height() - borderHor.height()), _border);

            const QRect borderVer(0, 0, _borderWidth, _rect.height());
            if (_borderSides & Qt::AlignLeft)
                _p.fillRect(borderVer, _border);
            if (_borderSides & Qt::AlignRight)
                _p.fillRect(borderVer.translated(_rect.width() - borderVer.width(), 0), _border);
        }
    }

    QColor getNameColor(const QString& _uin)
    {
        im_assert(!_uin.isEmpty());
        if (!_uin.isEmpty())
        {
            const auto colorIndex = getColorTableIndex(_uin);
            return colorTable3[colorIndex][2];
        }

        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    std::vector<std::vector<QString>> GetPossibleStrings(const QString& _text, unsigned& _count)
    {
        _count = 0;
        std::vector<std::vector<QString>> result;
        if (_text.isEmpty())
            return result;
        result.reserve(_text.size());


#if defined(__linux__) || defined(_WIN32)
        for (int i = 0; i < _text.size(); ++i)
            result.push_back({ _text.at(i) });
#endif

#if defined _WIN32
        HKL aLayouts[8] = {0};
        HKL hCurrent = ::GetKeyboardLayout(0);
        _count = ::GetKeyboardLayoutList(8, aLayouts);

        for (int i = 0, size = _text.size(); i < size; ++i)
        {
            const auto ucChar = _text.at(i).unicode();
            const auto scanEx = ::VkKeyScanEx(ucChar, hCurrent);

            if (scanEx == -1)
            {
                const auto found = std::any_of(std::cbegin(aLayouts), std::cend(aLayouts), [ucChar](auto layout) { return ::VkKeyScanEx(ucChar, layout); });

                if (!found)
                    return std::vector<std::vector<QString>>();
            }

            for (unsigned j = 0; j < _count; ++j)
            {
                if (hCurrent == aLayouts[j])
                    continue;

                if (scanEx != -1)
                    result[i].push_back(VKeyToChar(scanEx, aLayouts[j]));
            }
        }

#elif defined __APPLE__
        MacSupport::getPossibleStrings(_text.toLower(), result, _count);
#elif defined __linux__
        _count = 1;
#endif

        const auto translit = Translit::getPossibleStrings(_text);

        if (!translit.empty())
        {
            for (int i = 0, size = std::min(Translit::getMaxSearchTextLength(), _text.size()); i < size; ++i)
            {
                for (const auto& pattern : translit[i])
                    result[i].push_back(pattern);
            }
        }

        for (auto& res : result)
            res.erase(std::remove_if(res.begin(), res.end(), [](const auto& _p) { return _p.isEmpty(); }), res.end());
        result.erase(std::remove_if(result.begin(), result.end(), [](const auto& _v) { return _v.empty(); }), result.end());

        return result;
    }

    QPixmap roundImage(const QPixmap& _img, bool /*isDefault*/, bool _miniIcons)
    {
        const int scale = std::min(_img.height(), _img.width());
        QPixmap imageOut(scale, scale);
        imageOut.fill(Qt::transparent);

        QPainter painter(&imageOut);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);

        QPainterPath path;
        path.addEllipse(0, 0, scale, scale);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, _img);

        Utils::check_pixel_ratio(imageOut);

        return imageOut;
    }

    bool isValidEmailAddress(const QString& _email)
    {
        static const QRegularExpression re(
            qsl("^\\b[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,10}\\b$"),
            QRegularExpression::UseUnicodePropertiesOption);
        return re.match(_email).hasMatch();
    }

    bool foregroundWndIsFullscreened()
    {
#ifdef _WIN32
        const HWND foregroundWindow = ::GetForegroundWindow();

        RECT rcScreen;
        ::GetWindowRect(GetDesktopWindow(), &rcScreen);

        RECT rcForegroundApp;
        GetWindowRect(foregroundWindow, &rcForegroundApp);

        if (foregroundWindow != ::GetDesktopWindow() && foregroundWindow != ::GetShellWindow())
        {
            return rcScreen.left == rcForegroundApp.left &&
                rcScreen.top == rcForegroundApp.top &&
                rcScreen.right == rcForegroundApp.right &&
                rcScreen.bottom == rcForegroundApp.bottom;
        }
        return false;
#elif __APPLE__
        return MacSupport::isFullScreen();
#else
        return false;
#endif
    }

    double fscale_value(const double _px) noexcept
    {
        return  _px * getScaleCoefficient();
    }

    int scale_value(const int _px) noexcept
    {
        return static_cast<int>(getScaleCoefficient() * _px);
    }

    QSize scale_value(const QSize& _px) noexcept
    {
        return _px * getScaleCoefficient();
    }

    QSizeF scale_value(const QSizeF& _px) noexcept
    {
        return _px * getScaleCoefficient();
    }

    QRect scale_value(const QRect& _px) noexcept
    {
        return QRect(_px.topLeft(), scale_value(_px.size()));
    }

    QPoint scale_value(const QPoint& _px) noexcept
    {
        return QPoint(scale_value(_px.x()), scale_value(_px.y()));
    }

    QMargins scale_value(const QMargins& _px) noexcept
    {
        return _px * getScaleCoefficient();
    }

    int unscale_value(const int _px) noexcept
    {
        double scale = getScaleCoefficient();
        return int(_px / double(scale == 0 ? 1.0 : scale));
    }

    QSize unscale_value(const QSize& _px) noexcept
    {
        return _px / getScaleCoefficient();
    }

    QRect unscale_value(const QRect& _px) noexcept
    {
        return QRect(_px.topLeft(), unscale_value(_px.size()));
    }

    QPoint unscale_value(const QPoint& _px) noexcept
    {
        return QPoint(unscale_value(_px.x()), unscale_value(_px.y()));
    }

    int scale_bitmap_ratio() noexcept
    {
        return is_mac_retina() ? 2 : 1;
    }

    int scale_bitmap(const int _px) noexcept
    {
        return _px * scale_bitmap_ratio();
    }

    double fscale_bitmap(const double _px) noexcept
    {
        return _px * scale_bitmap_ratio();
    }

    QSize scale_bitmap(const QSize& _px) noexcept
    {
        return _px * scale_bitmap_ratio();
    }

    QSizeF scale_bitmap(const QSizeF& _px) noexcept
    {
        return _px * scale_bitmap_ratio();
    }

    QRect scale_bitmap(const QRect& _px) noexcept
    {
        return QRect(_px.topLeft(), scale_bitmap(_px.size()));
    }

    int unscale_bitmap(const int _px) noexcept
    {
        return _px / scale_bitmap_ratio();
    }

    QSize unscale_bitmap(const QSize& _px) noexcept
    {
        return _px / scale_bitmap_ratio();
    }

    QRect unscale_bitmap(const QRect& _px) noexcept
    {
        return QRect(_px.topLeft(), unscale_bitmap(_px.size()));
    }

    int scale_bitmap_with_value(const int _px) noexcept
    {
        return scale_value(scale_bitmap(_px));
    }

    double fscale_bitmap_with_value(const double _px) noexcept
    {
        return fscale_value(fscale_bitmap(_px));
    }

    QSize scale_bitmap_with_value(const QSize& _px) noexcept
    {
        return scale_value(scale_bitmap(_px));
    }

    QSizeF scale_bitmap_with_value(const QSizeF& _px) noexcept
    {
        return scale_value(scale_bitmap(_px));
    }

    QRect scale_bitmap_with_value(const QRect& _px) noexcept
    {
        return QRect(_px.topLeft(), scale_bitmap_with_value(_px.size()));
    }

    QRectF scale_bitmap_with_value(const QRectF& _px) noexcept
    {
        return QRectF(_px.topLeft(), scale_bitmap_with_value(_px.size()));
    }

    int getBottomPanelHeight() { return  Utils::scale_value(48); };
    int getTopPanelHeight() { return  Utils::scale_value(56); };

    template <typename _T>
    void check_pixel_ratio(_T& _image)
    {
        if (is_mac_retina())
            _image.setDevicePixelRatio(2);
    }
    template void check_pixel_ratio<QImage>(QImage& _pixmap);
    template void check_pixel_ratio<QPixmap>(QPixmap& _pixmap);

    QString parse_image_name(const QString& _imageName)
    {
        QString result(_imageName);
        if (is_mac_retina())
        {
            result.replace(ql1s("/100/"), ql1s("/200/"));
            result.replace(ql1s("_100"), ql1s("_200"));
            return result;
        }

        int currentScale = Utils::getScaleCoefficient() * 100;
        if (currentScale > 200)
        {
            if (Ui::GetAppConfig().IsFullLogEnabled())
            {
                std::stringstream logData;
                logData << "INVALID ICON SIZE = " << currentScale << ", " << _imageName.toUtf8().constData();
                Log::write_network_log(logData.str());
            }
            currentScale = 200;
        }

        const QString scaleString = QString::number(currentScale);
        result.replace(ql1s("_100"), u'_' % scaleString);
        result.replace(ql1s("/100/"), u'/' % scaleString % u'/');
        return result;
    }

    QPixmap renderSvg(const QString& _filePath, const QSize& _scaledSize, const QColor& _tintColor, const KeepRatio _keepRatio)
    {
        if (Q_UNLIKELY(!QFileInfo::exists(_filePath)))
            return QPixmap();

        QSvgRenderer renderer(_filePath);
        const auto defSize = renderer.defaultSize();

        if (Q_UNLIKELY(_scaledSize.isEmpty() && defSize.isEmpty()))
            return QPixmap();

        QSize resultSize;
        QRect bounds;

        if (!_scaledSize.isEmpty())
        {
            resultSize = scale_bitmap(_scaledSize);
            bounds = QRect(QPoint(), resultSize);

            if (_keepRatio == KeepRatio::yes)
            {
                const double wRatio = defSize.width() / (double)resultSize.width();
                const double hRatio = defSize.height() / (double)resultSize.height();
                constexpr double epsilon = std::numeric_limits<double>::epsilon();

                if (Q_UNLIKELY(std::fabs(wRatio - hRatio) > epsilon))
                {
                    const auto resultCenter = bounds.center();
                    bounds.setSize(defSize.scaled(resultSize, Qt::KeepAspectRatio));
                    bounds.moveCenter(resultCenter);
                }
            }
        }
        else
        {
            resultSize = scale_bitmap_with_value(defSize);
            bounds = QRect(QPoint(), resultSize);
        }

        QPixmap pixmap(resultSize);
        pixmap.fill(Qt::transparent);

        {
            QPainter painter(&pixmap);
            renderer.render(&painter, bounds);

            if (_tintColor.isValid())
            {
                painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                painter.fillRect(bounds, _tintColor);
            }
        }

        check_pixel_ratio(pixmap);

        return pixmap;
    }

    QPixmap renderSvgLayered(const QString& _filePath, const SvgLayers& _layers, const QSize& _scaledSize)
    {
        if (Q_UNLIKELY(!QFileInfo::exists(_filePath)))
            return QPixmap();

        QSvgRenderer renderer(_filePath);
        const auto defSize = scale_bitmap_with_value(renderer.defaultSize());

        if (Q_UNLIKELY(_scaledSize.isEmpty() && defSize.isEmpty()))
            return QPixmap();

        QSize resultSize = _scaledSize.isEmpty() ? defSize : scale_bitmap(_scaledSize);
        QPixmap pixmap(resultSize);
        pixmap.fill(Qt::transparent);

        QMatrix scaleMatrix;
        if (!_scaledSize.isEmpty() && defSize != resultSize)
        {
            const auto s = double(resultSize.width()) / defSize.width();
            scaleMatrix.scale(s, s);
        }

        {
            QPainter painter(&pixmap);

            if (!_layers.empty())
            {
                for (const auto& [layerName, layerColor] : _layers)
                {
                    if (!layerColor.isValid() || layerColor.alpha() == 0)
                        continue;

                    im_assert(renderer.elementExists(layerName));
                    if (!renderer.elementExists(layerName))
                        continue;

                    QPixmap layer(resultSize);
                    layer.fill(Qt::transparent);

                    const auto elMatrix = renderer.matrixForElement(layerName);
                    const auto elBounds = renderer.boundsOnElement(layerName);
                    const auto mappedRect = scale_bitmap_with_value(elMatrix.mapRect(elBounds));
                    auto layerBounds = scaleMatrix.mapRect(mappedRect);
                    layerBounds.moveTopLeft(QPointF(
                        fscale_bitmap_with_value(layerBounds.topLeft().x()),
                        fscale_bitmap_with_value(layerBounds.topLeft().y())
                    ));

                    {
                        QPainter layerPainter(&layer);
                        renderer.render(&layerPainter, layerName, layerBounds);

                        layerPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                        layerPainter.fillRect(layer.rect(), layerColor);
                    }
                    painter.drawPixmap(QPoint(), layer);
                }
            }
            else
            {
                renderer.render(&painter, QRect(QPoint(), resultSize));
            }
        }

        check_pixel_ratio(pixmap);

        return pixmap;
    }

    void addShadowToWidget(QWidget* _target)
    {
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(_target);
        QColor widgetShadowColor(u"#000000");
        widgetShadowColor.setAlphaF(0.3);
        shadow->setColor(widgetShadowColor);
        shadow->setBlurRadius(scale_value(16));
        shadow->setXOffset(scale_value(0));
        shadow->setYOffset(scale_value(2));
        _target->setGraphicsEffect(shadow);
    }

    void addShadowToWindow(QWidget* _target, bool _enabled)
    {
        int shadowWidth = _enabled ? Ui::get_gui_settings()->get_shadow_width() : 0;
        if (_enabled && !_target->testAttribute(Qt::WA_TranslucentBackground))
            _target->setAttribute(Qt::WA_TranslucentBackground);

        auto oldMargins = _target->contentsMargins();
        _target->setContentsMargins(QMargins(oldMargins.left() + shadowWidth, oldMargins.top() + shadowWidth,
            oldMargins.right() + shadowWidth, oldMargins.bottom() + shadowWidth));

        static QPointer<QObject> eventFilter(new ShadowWidgetEventFilter(Ui::get_gui_settings()->get_shadow_width()));

        if (_enabled)
            _target->installEventFilter(eventFilter);
        else
            _target->removeEventFilter(eventFilter);
    }

    void grabTouchWidget(QWidget* _target, bool _topWidget)
    {
#ifdef _WIN32
        im_assert(_target);
        if (!_target)
            return;
        if (_topWidget)
        {
            QScrollerProperties sp;
            sp.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
            sp.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
            sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
            sp.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.4);
            sp.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 1.2);
            sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.2);
            sp.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0);
            sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
            sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0);
            QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
            sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, overshootPolicy);
            sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);

            QScroller* clScroller = QScroller::scroller(_target);
            im_assert(clScroller);
            if (clScroller)
            {
                clScroller->grabGesture(_target);
                clScroller->setScrollerProperties(sp);
            }
        }
        else
        {
            QScroller::grabGesture(_target);
        }
#else
        Q_UNUSED(_target);
        Q_UNUSED(_topWidget);
#endif //WIN32
    }

    void removeLineBreaks(QString& _source)
    {
        if (_source.isEmpty())
            return;

        const bool spaceAtEnd = _source.at(_source.length() - 1) == QChar::Space;
        _source.replace(u'\n', QChar::Space);
        _source.remove(u'\r');
        if (!spaceAtEnd && _source.at(_source.length() - 1) == QChar::Space)
            _source.chop(1);
    }

    static bool macRetina = false;

    bool is_mac_retina() noexcept
    {
        return platform::is_apple() && macRetina;
    }

    void set_mac_retina(bool _val) noexcept
    {
        macRetina = _val;
    }

    static double scaleCoefficient = 1.0;

    double getScaleCoefficient() noexcept
    {
        return scaleCoefficient;
    }

    void setScaleCoefficient(double _coefficient) noexcept
    {
        if constexpr (platform::is_apple())
        {
            scaleCoefficient = 1.0;
            return;
        }

        constexpr double epsilon = std::numeric_limits<double>::epsilon();

        if (
            std::fabs(_coefficient - 1.0) < epsilon ||
            std::fabs(_coefficient - 1.25) < epsilon ||
            std::fabs(_coefficient - 1.5) < epsilon ||
            std::fabs(_coefficient - 2.0) < epsilon ||
            std::fabs(_coefficient - 2.5) < epsilon ||
            std::fabs(_coefficient - 3.0) < epsilon
            )
        {
            scaleCoefficient = _coefficient;
            return;
        }

        im_assert(!"unexpected scale value");
        scaleCoefficient = 1.0;
    }

    namespace { double basicScaleCoefficient = 1.0; }

    double getBasicScaleCoefficient() noexcept
    {
        return basicScaleCoefficient;
    }

    void initBasicScaleCoefficient(double _coefficient) noexcept
    {
        static bool isInitBasicScaleCoefficient = false;
        if (!isInitBasicScaleCoefficient)
        {
            constexpr double epsilon = std::numeric_limits<double>::epsilon();

            if (
                std::fabs(_coefficient - 1.0) < epsilon ||
                std::fabs(_coefficient - 1.25) < epsilon ||
                std::fabs(_coefficient - 1.5) < epsilon ||
                std::fabs(_coefficient - 2.0) < epsilon ||
                std::fabs(_coefficient - 2.5) < epsilon ||
                std::fabs(_coefficient - 3.0) < epsilon
                )
            {
                basicScaleCoefficient = _coefficient;
                return;
            }

            im_assert(!"unexpected scale value");
            basicScaleCoefficient = 1.0;
        }
        else
            im_assert(!"initBasicScaleCoefficient should be called once.");
    }

    void groupTaskbarIcon(bool _enabled)
    {
#ifdef _WIN32
        const auto model = config::get().string(config::values::app_user_model_win);
        SetCurrentProcessExplicitAppUserModelID(_enabled ? QString::fromUtf8(model.data(), model.size()).toStdWString().c_str() : L"");
#endif //_WIN32
    }

    bool isStartOnStartup()
    {
#ifdef _WIN32
        QSettings s(qsl("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
        return s.contains(getProductName());
#endif //_WIN32
        return true;
    }

    void setStartOnStartup(bool _start)
    {
#ifdef _WIN32
        if (bool currentState = isStartOnStartup(); currentState == _start)
            return;

        QSettings s(qsl("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);

        if (_start)
        {
            wchar_t buffer[1025];
            if (!::GetModuleFileName(0, buffer, 1024))
                return;

            s.setValue(getProductName(), QString(u'\"' % QString::fromWCharArray(buffer) % u"\" /startup"));
        }
        else
        {
            s.remove(getProductName());
        }
#endif //_WIN32
    }

#ifdef _WIN32
    HWND createFakeParentWindow()
    {
        HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
        HWND hwnd = 0;

        CAtlString className = L"fake_parent_window";
        WNDCLASSEX wc = {0};

        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = (LPCWSTR)className;
        if (!::RegisterClassEx(&wc))
            return hwnd;

        hwnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, (LPCWSTR)className, 0, WS_POPUP, 0, 0, 0, 0, 0, 0, instance, 0);
        if (!hwnd)
            return hwnd;

        ::SetLayeredWindowAttributes(
            hwnd,
            0,
            0,
            LWA_ALPHA
            );

        return hwnd;
    }
#endif //WIN32

    int calcAge(const QDateTime& _birthdate)
    {
        QDate thisdate = QDateTime::currentDateTime().date();
        QDate birthdate = _birthdate.date();

        int age = thisdate.year() - birthdate.year();
        if (age < 0)
            return 0;

        if ((birthdate.month() > thisdate.month()) || (birthdate.month() == thisdate.month() && birthdate.day() > thisdate.day()))
            return (--age);

        return age;
    }

    QString DefaultDownloadsPath()
    {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    }

    QString UserDownloadsPath()
    {
        auto path = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath()));
        return QFileInfo(path).canonicalFilePath(); // we need canonical path to follow symlinks
    }

    QString UserSaveAsPath()
    {
        QString result = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory_save_as, QString()));
        return (result.isEmpty() ? UserDownloadsPath() : result);
    }

    template<typename T>
    static bool containsCaseInsensitive(T first, T last, QStringView _ext)
    {
        return std::any_of(first, last, [&_ext](auto e) { return _ext.compare(e, Qt::CaseInsensitive) == 0; });
    }

    template<typename T>
    static bool containsCaseInsensitive(T first, T last, const QByteArray& _ext)
    {
        return std::any_of(first, last, [&_ext](auto e) { return _ext.compare(e, Qt::CaseInsensitive) == 0; });
    }

    const std::vector<QStringView>& getImageExtensions()
    {
        const static std::vector<QStringView> ext = { u"jpg", u"jpeg", u"png", u"bmp", u"tif", u"tiff", u"webp" };
        return ext;
    }

    const std::vector<QStringView>& getVideoExtensions()
    {
        const static std::vector<QStringView> ext = { u"avi", u"mkv", u"wmv", u"flv", u"3gp", u"mpeg4", u"mp4", u"webm", u"mov", u"m4v" };
        return ext;
    }

    bool is_image_extension_not_gif(QStringView _ext)
    {
        const auto& imagesExtensions = getImageExtensions();
        return containsCaseInsensitive(imagesExtensions.begin(), imagesExtensions.end(), _ext);
    }

    bool is_image_extension(QStringView _ext)
    {
        const static std::vector<QStringView> gifExt = { u"gif" };
        return is_image_extension_not_gif(_ext) || containsCaseInsensitive(gifExt.begin(), gifExt.end(), _ext);
    }

    bool is_video_extension(QStringView _ext)
    {
        const auto& videoExtensions = getVideoExtensions();
        return containsCaseInsensitive(videoExtensions.begin(), videoExtensions.end(), _ext);
    }

    void copyFileToClipboard(const QString& _path)
    {
        static QMimeDatabase mimeDatabase;

        static const std::unordered_set<QString, QStringHasher> supportedImageMimeTypes = []()
        {
            // These MIME types currently must be supressed
            std::array<const char*, 3> supressedMimes = { "image/gif", "image/svg+xml", "image/svg+xml-compressed" };
            std::unordered_set<QString, QStringHasher> result;
            const QByteArrayList imageMimeTypes = QImageReader::supportedMimeTypes();
            // remove supressed MIME formats
            std::for_each(imageMimeTypes.constBegin(), imageMimeTypes.constEnd(), [&result, &supressedMimes](const QByteArray& t)
            {
                 if (!containsCaseInsensitive(supressedMimes.cbegin(), supressedMimes.cend(), t))
                     result.insert(QString::fromLatin1(t));
            });
            return result;
        }();

        // A QMimeDatabase lookup is normally a better approach than QImageReader
        // for identifying potentially non-image files or data.
        QMimeType mimeType = mimeDatabase.mimeTypeForFile(_path, QMimeDatabase::MatchContent);
        const auto mimeName = mimeType.name();

        QMimeData* mimeData = new QMimeData;
        if (supportedImageMimeTypes.find(mimeName) != supportedImageMimeTypes.cend())
        {
            mimeData->setImageData(QImage(_path, mimeType.preferredSuffix().toLatin1().constData()));
        }
        else
        {
            mimeData->setUrls({ QUrl::fromLocalFile(_path) });
            if (mimeName == u"image/gif")
                mimeData->setData(mimeName, QFileInfo(_path).absoluteFilePath().toUtf8());
        }

        QApplication::clipboard()->setMimeData(mimeData);
    }

    void copyLink(const QString& _link)
    {
        QApplication::clipboard()->setText(_link);
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "Link copied"));
    }

    void saveAs(const QString& _inputFilename, std::function<void (const QString& _filename, const QString& _directory, bool _exportAsPng)> _callback, std::function<void ()> _cancel_callback, bool asSheet)
    {
        static const QRegularExpression re(
            qsl("\\/:*?\"<>\\|\""),
            QRegularExpression::UseUnicodePropertiesOption);

        auto save_to_settings = [](const QString& _lastDirectory) {
            Ui::setSaveAsPath(QDir::toNativeSeparators(_lastDirectory));
        };

        auto lastDirectory = Ui::getSaveAsPath();
        const int dot = _inputFilename.lastIndexOf(u'.');
        const auto ext = dot != -1 ? QStringView(_inputFilename).mid(dot) : QStringView{};
        QString name = (_inputFilename.size() >= 128 || _inputFilename.contains(re)) ? QT_TRANSLATE_NOOP("chat_page", "File") : _inputFilename;
        QString fullName = QDir::toNativeSeparators(QDir(lastDirectory).filePath(name));

        const auto isWebp = ext.compare(u".webp", Qt::CaseInsensitive) == 0;

        const QString filter = u'*' % ext % (isWebp ? u";; *.png" : QStringView{});

#ifdef __APPLE__
        if (asSheet)
        {
            auto callback = [save_to_settings, _callback](const QString& _filename, const QString& _directory, bool _exportAsPng){
                save_to_settings(_directory);

                if (_callback)
                    _callback(_filename, _directory, _exportAsPng);
            };

            MacSupport::saveFileName(QT_TRANSLATE_NOOP("context_menu", "Save as..."), fullName, u'*' % ext, callback, ext.toString(), _cancel_callback);
            return;
        }
#endif //__APPLE__
        const QString destination = QFileDialog::getSaveFileName(nullptr, QT_TRANSLATE_NOOP("context_menu", "Save as..."), fullName, filter);
        if (!destination.isEmpty())
        {
            const QFileInfo info(destination);
            lastDirectory = info.dir().absolutePath();
            const QString directory = info.dir().absolutePath();
            const auto inputExt = !ext.isEmpty() && info.suffix().isEmpty() ? ext : QStringView{};
            const bool exportAsPng = isWebp && info.suffix().compare(u"png", Qt::CaseInsensitive) == 0;
            if (_callback)
                _callback(info.fileName() % inputExt, directory, exportAsPng);

            save_to_settings(directory);
        }
        else if (_cancel_callback)
        {
            _cancel_callback();
        }
    }

    const SendKeysIndex& getSendKeysIndex()
    {
        static SendKeysIndex index;

        if (index.empty())
        {
            index.emplace_back(QT_TRANSLATE_NOOP("settings", "Enter"), Ui::KeyToSendMessage::Enter);
            if constexpr (platform::is_apple())
            {
                index.emplace_back(Ui::KeySymbols::Mac::control + QT_TRANSLATE_NOOP("settings", " + Enter"), Ui::KeyToSendMessage::CtrlMac_Enter);
                index.emplace_back(Ui::KeySymbols::Mac::command + QT_TRANSLATE_NOOP("settings", " + Enter"), Ui::KeyToSendMessage::Ctrl_Enter);
                index.emplace_back(Ui::KeySymbols::Mac::shift + QT_TRANSLATE_NOOP("settings", " + Enter"), Ui::KeyToSendMessage::Shift_Enter);
            }
            else
            {
                index.emplace_back(QT_TRANSLATE_NOOP("settings", "Ctrl + Enter"), Ui::KeyToSendMessage::Ctrl_Enter);
                index.emplace_back(QT_TRANSLATE_NOOP("settings", "Shift + Enter"), Ui::KeyToSendMessage::Shift_Enter);
            }
        }

        return index;
    }

    const ShortcutsCloseActionsList& getShortcutsCloseActionsList()
    {
        static const ShortcutsCloseActionsList closeList
        {
            { QT_TRANSLATE_NOOP("settings", "Minimize the window"), Ui::ShortcutsCloseAction::RollUpWindow },
            { QT_TRANSLATE_NOOP("settings", "Minimize the window and minimize the chat"), Ui::ShortcutsCloseAction::RollUpWindowAndChat },
            { QT_TRANSLATE_NOOP("settings", "Close chat"), Ui::ShortcutsCloseAction::CloseChat },
        };

        return closeList;
    }

    QString getShortcutsCloseActionName(Ui::ShortcutsCloseAction _action)
    {
        const auto& actsList = getShortcutsCloseActionsList();
        for (const auto& [name, code] : actsList)
        {
            if (_action == code)
                return name;
        }

        return QString();
    }

    const ShortcutsSearchActionsList& getShortcutsSearchActionsList()
    {
        static const ShortcutsSearchActionsList searchList
        {
            { QT_TRANSLATE_NOOP("settings", "Search in chat"), Ui::ShortcutsSearchAction::SearchInChat },
            { QT_TRANSLATE_NOOP("settings", "Global search"), Ui::ShortcutsSearchAction::GlobalSearch },
        };

        return searchList;
    }

    const PrivacyAllowVariantsList& getPrivacyAllowVariants()
    {
        static const PrivacyAllowVariantsList variants
        {
            { QT_TRANSLATE_NOOP("settings", "Everybody"), core::privacy_access_right::everybody },
            { QT_TRANSLATE_NOOP("settings", "People you have corresponded with and contacts from your phone book"), core::privacy_access_right::my_contacts },
            { QT_TRANSLATE_NOOP("settings", "Nobody"), core::privacy_access_right::nobody },
        };

        return variants;
    }

    void post_stats_with_settings()
    {
        core::stats::event_props_type props;

        //System settings
        const auto geometry = qApp->desktop()->screenGeometry();
        props.emplace_back("Sys_Screen_Size", std::to_string(geometry.width()) + 'x' + std::to_string(geometry.height()));

        const QString processorInfo = u"Architecture:" % QSysInfo::currentCpuArchitecture() % u" Number of Processors:" % QString::number(QThread::idealThreadCount());
        props.emplace_back("Sys_CPU", processorInfo.toStdString());

        auto processorId = getCpuId().trimmed();
        if (processorId.isEmpty())
            processorId = qsl("Unknown");
        props.emplace_back("Sys_CPUID", processorId.toStdString());

        //General settings
        props.emplace_back("Settings_Startup", std::to_string(Utils::isStartOnStartup()));
        props.emplace_back("Settings_Minimized_Start", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_start_minimazed, true)));
        props.emplace_back("Settings_Taskbar", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true)));
        props.emplace_back("Settings_Sounds", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true)));

        //Chat settings
        props.emplace_back("Settings_Previews", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true)));
        props.emplace_back("Settings_AutoPlay_Video", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_autoplay_video, true)));

        auto currentDownloadDir = Ui::getDownloadPath();
        props.emplace_back(std::make_pair("Settings_Download_Folder", std::to_string(currentDownloadDir == Ui::getDownloadPath())));

        QString keyToSend;
        int currentKey = Ui::get_gui_settings()->get_value<int>(settings_key1_to_send_message, Ui::KeyToSendMessage::Enter);
        for (const auto& key : Utils::getSendKeysIndex())
        {
            if (key.second == currentKey)
                keyToSend = key.first;
        }
        props.emplace_back("Settings_Send_By", keyToSend.toStdString());

        //Interface settings
        props.emplace_back("Settings_Compact_Recents", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default))));
        props.emplace_back("Settings_Scale", std::to_string(Utils::getScaleCoefficient()));
        props.emplace_back("Settings_Language", Ui::get_gui_settings()->get_value(settings_language, QString()).toUpper().toStdString());

        if (const auto id = Styling::getThemesContainer().getGlobalWallpaperId(); id.isValid())
            props.emplace_back("Settings_Wallpaper_Global", id.id_.toStdString());
        else
            props.emplace_back("Settings_Wallpaper_Global", std::to_string(-1));

        const auto& themeCounts = Styling::getThemesContainer().getContactWallpaperCounters();
        for (const auto& [wallId, count] : themeCounts)
            props.emplace_back("Settings_Wallpaper_Local " + wallId.toStdString(), std::to_string(count));

        //Notifications settings
        props.emplace_back("Settings_Sounds_Outgoing", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, false)));
        props.emplace_back("Settings_Alerts", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_notify_new_messages, true)));
        props.emplace_back("Settings_Mail_Alerts", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true)));

        props.emplace_back("Proxy_Type", ProxySettings::proxyTypeStr(Utils::get_proxy_settings()->type_).toStdString());


        props.emplace_back("Settings_Suggest_Emoji", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_suggests_emoji, true)));
        props.emplace_back("Settings_Suggest_Words", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_suggests_words, true)));
        props.emplace_back("Settings_Autoreplace_Emoji", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_autoreplace_emoji, settings_autoreplace_emoji_default())));
        props.emplace_back("Settings_Allow_Big_Emoji", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_allow_big_emoji, settings_allow_big_emoji_default())));
        props.emplace_back("Settings_AutoPlay_Gif", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_autoplay_gif, true)));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::client_settings, props);
    }

    QRect GetMainRect()
    {
        im_assert(!!Utils::InterConnector::instance().getMainWindow() && "Common.cpp (ItemLength)");
        QRect mainRect = GetWindowRect(Utils::InterConnector::instance().getMainWindow());
        im_assert("Couldn't get rect: Common.cpp (ItemLength)");
        return mainRect;
    }

    QRect GetWindowRect(QWidget* window)
    {
        QRect mainRect(0, 0, 0, 0);
        if (window)
        {
            mainRect = window->geometry();
        }
        else if (auto _window = qApp->activeWindow())
        {
            mainRect = _window->geometry();
        }
        return mainRect;
    }

    QPoint GetMainWindowCenter()
    {
        auto mainRect = Utils::GetMainRect();
        auto mainWidth = mainRect.width();
        auto mainHeight = mainRect.height();

        auto x = mainRect.x() + mainWidth / 2;
        auto y = mainRect.y() + mainHeight / 2;
        return QPoint(x, y);
    }

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        core::ifptr<core::iarray> fieldArray(collection->create_array());
        fieldArray->reserve((int)_fields.size());
        for (const auto& field : _fields)
        {
            Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);
            coll.set_value_as_string("field_name", field.first);
            coll.set_value_as_qstring("field_value", field.second);

            core::ifptr<core::ivalue> valField(coll->create_value());
            valField->set_as_collection(coll.get());
            fieldArray->push_back(valField.get());
        }

        collection.set_value_as_array("fields", fieldArray.get());
        Ui::GetDispatcher()->post_message_to_core("update_profile", collection.get());
    }

    Ui::GeneralDialog *NameEditorDialog(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& _resultChatName,
        bool acceptEnter)
    {
        auto mainWidget = new QWidget(_parent);
        mainWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        auto layout = Utils::emptyVLayout(mainWidget);
        layout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16), 0);

        auto textEdit = new Ui::TextEditEx(mainWidget, Fonts::appFontScaled(18), Ui::MessageStyle::getTextColor(), true, true);
        Utils::ApplyStyle(textEdit, Styling::getParameters().getLineEditCommonQss());
        textEdit->setObjectName(qsl("input_edit_control"));
        textEdit->setPlaceholderText(_chatName);
        textEdit->setPlainText(_chatName);
        textEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        textEdit->setAutoFillBackground(false);
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        textEdit->setEnterKeyPolicy(acceptEnter ? Ui::TextEditEx::EnterKeyPolicy::CatchEnter : Ui::TextEditEx::EnterKeyPolicy::None);
        textEdit->setMinimumWidth(Utils::scale_value(300));
        Utils::ApplyStyle(textEdit, qsl("padding: 0; margin: 0;"));
        Testing::setAccessibleName(textEdit, qsl("AS General textEdit"));
        layout->addWidget(textEdit);

        auto horizontalLineWidget = new QWidget(_parent);
        horizontalLineWidget->setFixedHeight(Utils::scale_value(1));
        horizontalLineWidget->setStyleSheet(ql1s("background-color: %1;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::PRIMARY)));
        Testing::setAccessibleName(horizontalLineWidget, qsl("AS General horizontalLineWidget"));
        layout->addWidget(horizontalLineWidget);

        auto generalDialog = new Ui::GeneralDialog(mainWidget, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addLabel(_headerText);
        generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), _buttonText, true);

        if (acceptEnter)
            QObject::connect(textEdit, &Ui::TextEditEx::enter, generalDialog, &Ui::GeneralDialog::accept);

        QTextCursor cursor = textEdit->textCursor();
        cursor.select(QTextCursor::Document);
        textEdit->setTextCursor(cursor);
        textEdit->setFrameStyle(QFrame::NoFrame);

        // to disable link/email highlighting
        textEdit->setFontUnderline(false);
        textEdit->setTextColor(Ui::MessageStyle::getTextColor());

        textEdit->setFocus();

        return generalDialog;
    }

    bool NameEditor(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& _resultChatName,
        bool acceptEnter)
    {
        auto dialog = std::unique_ptr<Ui::GeneralDialog>(NameEditorDialog(_parent, _chatName, _buttonText, _headerText, _resultChatName, acceptEnter));
        const auto result = dialog->showInCenter();
        auto textEdit = dialog->findChild<Ui::TextEditEx*>(qsl("input_edit_control"));
        if (textEdit)
            _resultChatName = textEdit->getPlainText();
        return result;
    }

    bool UrlEditor(QWidget* _parent, QStringView _linkDisplayName, InOut QString& _url)
    {
        auto dialog = Ui::UrlEditDialog::create(Utils::InterConnector::instance().getMainWindow(), _parent, _linkDisplayName, _url);
        const auto result = dialog->showInCenter();
        if (result)
            _url = dialog->getUrl();
        return result;
    }

    static std::unique_ptr<Ui::GeneralDialog> getConfirmationDialogImpl(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow,
        bool _withSemiwindow)
    {
        auto parent = _parent ? _parent : (_mainWindow ? _mainWindow : Utils::InterConnector::instance().getMainWindow());

        Ui::GeneralDialog::Options opt;
        opt.withSemiwindow_ = _withSemiwindow;
        auto dialog = std::make_unique<Ui::GeneralDialog>(nullptr, parent, opt);
        if (!_labelText.isEmpty())
            dialog->addLabel(_labelText);

        dialog->addText(_messageText, Utils::scale_value(12));
        dialog->addButtonsPair(_buttonLeftText, _buttonRightText, true);

        return dialog;
    }

    bool GetConfirmationWithTwoButtons(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow,
        bool _withSemiwindow)
    {
        auto dialog = getConfirmationDialogImpl(_buttonLeftText, _buttonRightText, _messageText, _labelText, _parent, _mainWindow, _withSemiwindow);
        return dialog->showInCenter();
    }

    bool GetDeleteConfirmation(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow,
        bool _withSemiwindow)
    {
        auto dialog = getConfirmationDialogImpl(_buttonLeftText, _buttonRightText, _messageText, _labelText, _parent, _mainWindow, _withSemiwindow);
        if (auto rightBtn = dialog->getAcceptButton())
            rightBtn->changeRole(Ui::DialogButtonRole::CONFIRM_DELETE);
        return dialog->showInCenter();
    }

    bool GetConfirmationWithOneButton(
        const QString& _buttonText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow,
        bool _withSemiwindow)
    {
        auto parent = _parent ? _parent : (_mainWindow ? _mainWindow : Utils::InterConnector::instance().getMainWindow());

        Ui::GeneralDialog::Options opt;
        opt.withSemiwindow_ = _withSemiwindow;
        Ui::GeneralDialog dialog(nullptr, parent, opt);
        if (!_labelText.isEmpty())
            dialog.addLabel(_labelText);

        dialog.addText(_messageText, Utils::scale_value(12));
        dialog.addAcceptButton(_buttonText, true);

        return dialog.showInCenter();
    }

    bool GetErrorWithTwoButtons(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& /*_messageText*/,
        const QString& _labelText,
        const QString& _errorText,
        QWidget* _parent)
    {
        auto parent = _parent ? _parent : Utils::InterConnector::instance().getMainWindow();

        Ui::GeneralDialog dialog(nullptr, parent);
        dialog.addLabel(_labelText);
        dialog.addError(_errorText);
        dialog.addButtonsPair(_buttonLeftText, _buttonRightText, true);
        return dialog.showInCenter();
    }

    void ShowBotAlert(const QString& _alertText)
    {
        Ui::GeneralDialog dialog(nullptr, Utils::InterConnector::instance().getMainWindow());

        dialog.addText(_alertText, Utils::scale_value(12), Fonts::appFontScaled(21), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        dialog.addAcceptButton(QT_TRANSLATE_NOOP("bots", "Ok"), true);

        dialog.showInCenter();
    }

    void showCancelGroupJoinDialog(const QString& _aimId)
    {
        if (_aimId.isEmpty())
            return;

        const QString text = Logic::getContactListModel()->isChannel(_aimId)
            ? QT_TRANSLATE_NOOP("popup_window", "After canceling the request, the channel will disappear from the chat list")
            : QT_TRANSLATE_NOOP("popup_window", "After canceling the request, the group will disappear from the chat list");

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Cancel request?"),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("sn", _aimId);
            Ui::GetDispatcher()->post_message_to_core("livechat/join/cancel", collection.get());
        }
    }

    ProxySettings::ProxySettings(core::proxy_type _type,
                                 core::proxy_auth _authType,
                                 QString _username,
                                 QString _password,
                                 QString _proxyServer,
                                 int _port,
                                 bool _needAuth)
        : type_(_type)
        , authType_(_authType)
        , username_(std::move(_username))
        , needAuth_(_needAuth)
        , password_(std::move(_password))
        , proxyServer_(std::move(_proxyServer))
        , port_(_port)
    {}

    ProxySettings::ProxySettings()
        : type_(core::proxy_type::auto_proxy)
        , authType_(core::proxy_auth::basic)
        , needAuth_(false)
        , port_(invalidPort)
    {}

    void ProxySettings::postToCore()
    {
        Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);

        coll.set_value_as_enum("settings_proxy_type", type_);
        coll.set_value_as_enum("settings_proxy_auth_type", authType_);
        coll.set<QString>("settings_proxy_server", proxyServer_);
        coll.set_value_as_int("settings_proxy_port", port_);
        coll.set<QString>("settings_proxy_username", username_);
        coll.set<QString>("settings_proxy_password", password_);
        coll.set_value_as_bool("settings_proxy_need_auth", needAuth_);

        Ui::GetDispatcher()->post_message_to_core("set_user_proxy_settings", coll.get());
    }

    QString ProxySettings::proxyTypeStr(core::proxy_type _type)
    {
        switch (_type)
        {
            case core::proxy_type::auto_proxy:
                return qsl("Auto");
            case core::proxy_type::http_proxy:
                return qsl("HTTP Proxy");
            case core::proxy_type::socks4:
                return qsl("SOCKS4");
            case core::proxy_type::socks5:
                return qsl("SOCKS5");
            default:
                return QString();
        }
    }

    QString ProxySettings::proxyAuthStr(core::proxy_auth _type)
    {
        switch (_type)
        {
            case core::proxy_auth::basic:
                return qsl("Basic");
            case core::proxy_auth::digest:
                return qsl("Digest");
            case core::proxy_auth::negotiate:
                return qsl("Negotiate");
            case core::proxy_auth::ntlm:
                return qsl("NTLM");
            default:
                return QString();
        }
    }

    QNetworkProxy::ProxyType ProxySettings::proxyType(core::proxy_type _type)
    {
        switch (_type)
        {
        case core::proxy_type::http_proxy:
            return QNetworkProxy::ProxyType::HttpProxy;
        case core::proxy_type::socks4:
        case core::proxy_type::socks5:
            return QNetworkProxy::ProxyType::Socks5Proxy;
        default:
            return QNetworkProxy::ProxyType::NoProxy;
        }
    }

    ProxySettings* get_proxy_settings()
    {
        static auto proxySetting = std::make_unique<ProxySettings>();
        return proxySetting.get();
    }

    QSize getMaxImageSize()
    {
        static QSize sz = QApplication::desktop()->screenGeometry().size();

        return QSize(std::max(sz.width(), sz.height()), std::max(sz.width(), sz.height()));
    }

    QScreen* mainWindowScreen()
    {
        if (auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            if (auto wh = mw->windowHandle())
            {
                if (auto s = wh->screen())
                    return s;
            }
        }
        return QApplication::primaryScreen();
    }


    ImageBase64Format detectBase64ImageFormat(const QByteArray& _data)
    {
        using LookupMap = std::vector<std::pair<QByteArray, QByteArray>>;

        static const LookupMap supportedImageFormats = []()
        {
            LookupMap lookup;
            const QByteArrayList formats = QImageReader::supportedImageFormats();
            lookup.reserve(formats.size());
            for (const auto& format : formats)
            {
                QByteArray key;
                key += base64_encoded_image_signature.data();
                key += format;
                key += ";base64";
                lookup.emplace_back(std::move(key), format);
            }
            return lookup;
        }();

        ImageBase64Format result = { QByteArray(), 0 };
        if (!_data.startsWith(base64_encoded_image_signature.data()))
            return result;

        for (const auto& imageFormat : supportedImageFormats)
        {
            if (_data.startsWith(imageFormat.first))
            {
                result.imageFormat = imageFormat.second;
                result.headerLength = imageFormat.first.size();
                break;
            }
        }
        return result;
    }

    bool isBase64EncodedImage(const QString& _data)
    {
        return _data.startsWith(QLatin1String(base64_encoded_image_signature.data(), base64_encoded_image_signature.size()));
    }

    bool isBase64(const QByteArray& _data)
    {
        static const QRegularExpression re(qsl("([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?"));
        return re.match(QString::fromLatin1(_data)).hasMatch();
    }

    bool loadBase64ImageImpl(QByteArray& _data, const QByteArray& _format, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize);
    bool loadImageScaledImpl(QByteArray& _data, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize);

    QPixmap loadPixmap(const QString& _resource)
    {
        QPixmap p(parse_image_name(_resource));

        check_pixel_ratio(p);

        return p;
    }

    bool loadPixmap(const QString& _path, Out QPixmap& _pixmap)
    {
        im_assert(!_path.isEmpty());

        if (!QFile::exists(_path))
        {
            im_assert(!"file does not exist");
            return false;
        }

        QFile file(_path);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        return loadPixmap(file.readAll(), _pixmap, Utils::Exif::getExifOrientation(_path));
    }

    bool loadPixmap(const QByteArray& _data, Out QPixmap& _pixmap, Exif::ExifOrientation _orientation)
    {
        im_assert(!_data.isEmpty());

        static QMimeDatabase db;
        if (db.mimeTypeForData(_data).preferredSuffix() == u"svg")
        {
            QSvgRenderer renderer;
            if (!renderer.load(_data))
                return false;

            _pixmap = QPixmap(renderer.defaultSize());
            _pixmap.fill(Qt::white);

            QPainter p(&_pixmap);
            renderer.render(&p);
            return !_pixmap.isNull();
        }


        constexpr std::array<const char*, 4> availableFormats = { nullptr, "png", "jpg", "webp" };

        for (auto fmt : availableFormats)
        {
            _pixmap.loadFromData(_data, fmt);

            if (!_pixmap.isNull())
            {
                Exif::applyExifOrientation(_orientation, InOut _pixmap);
                return true;
            }
        }

        return false;
    }

    bool loadPixmapScaled(const QString& _path, const QSize& _maxSize,  Out QPixmap& _pixmap, Out QSize& _originalSize, const PanoramicCheck _checkPanoramic)
    {
        QImage image;
        if (!loadImageScaled(_path, _maxSize, image, _originalSize, _checkPanoramic))
            return false;

        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return false;

        _pixmap = QPixmap::fromImage(std::move(image));

        return true;
    }

    bool loadBase64ImageImpl(QByteArray& _data, const QByteArray& _format, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize)
    {
        if (!_image.loadFromData(_data, _format.constData()))
            return false;

        _originalSize = _image.size();
        if (_maxSize.isValid() && (_maxSize.width() < _originalSize.width() || _maxSize.height() < _originalSize.height()))
            _image = _image.scaled(_maxSize, Qt::KeepAspectRatio);

        return true;
    }

    bool loadBase64Image(const QByteArray& _data, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize)
    {
        const auto format = detectBase64ImageFormat(_data);

        if (format.headerLength == 0)
        {
            return loadImageScaledImpl(const_cast<QByteArray&>(_data), _maxSize, _image, _originalSize);
        }
        else
        {
            QByteArray rawData = std::move(QUrl::fromPercentEncoding(_data)).toLatin1();
            rawData.remove(0, format.headerLength);
            rawData = QByteArray::fromBase64(rawData);

            return loadBase64ImageImpl(rawData, format.imageFormat, _maxSize, _image, _originalSize);
        }
    }

    bool loadImageScaled(const QString& _path, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize, const PanoramicCheck _checkPanoramic)
    {
        QImageReader reader(_path);
        reader.setAutoTransform(true);

        if (!reader.canRead())
        {
            reader.setDecideFormatFromContent(true);
            reader.setFileName(_path);              // needed to reset QImageReader state
            if (!reader.canRead())
                return false;
        }

        QSize imageSize = reader.size();

        _originalSize = imageSize;

        if (const auto tr = reader.transformation(); tr == QImageIOHandler::TransformationRotate90 || tr == QImageIOHandler::TransformationRotate270)
            _originalSize.transpose();

        const auto needRescale = _maxSize.isValid() && (_maxSize.width() < imageSize.width() || _maxSize.height() < imageSize.height());
        const auto isSvg = reader.format() == "svg";

        if (needRescale || isSvg)
        {
            const auto aspectMode = (_checkPanoramic == PanoramicCheck::yes && isPanoramic(_originalSize)) ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio;
            imageSize.scale(_maxSize, aspectMode);

            reader.setScaledSize(imageSize);
        }

        _image = reader.read();

        return true;
    }

    bool loadPixmapScaled(QByteArray& _data, const QSize& _maxSize, Out QPixmap& _pixmap, Out QSize& _originalSize)
    {
        QImage image;
        loadImageScaled(_data, _maxSize, image, _originalSize);

        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return false;
        _pixmap = QPixmap::fromImage(std::move(image));

        return true;
    }

    bool loadImageScaled(QByteArray& _data, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize)
    {
        const auto format = detectBase64ImageFormat(_data);
        if (format.headerLength < 1)
            return loadImageScaledImpl(_data, _maxSize, _image, _originalSize);

        _data = QByteArray::fromBase64(_data);
        return loadBase64ImageImpl(_data.remove(0, format.headerLength), format.imageFormat, _maxSize, _image, _originalSize);
    }

    bool loadImageScaledImpl(QByteArray& _data, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize)
    {
        QBuffer buffer(&_data, nullptr);

        QImageReader reader;
        reader.setDecideFormatFromContent(true);
        reader.setDevice(&buffer);

        if (!reader.canRead())
            return false;

        QSize imageSize = reader.size();

        _originalSize = imageSize;

        if (_maxSize.isValid() && (_maxSize.width() < imageSize.width() || _maxSize.height() < imageSize.height()))
        {
            imageSize.scale(_maxSize, Qt::KeepAspectRatio);

            reader.setScaledSize(imageSize);
        }

        _image = reader.read();

        return true;
    }

    bool canCovertToWebp(const QString& _path)
    {
        const auto format = QImageReader::imageFormat(_path).toLower();
        return format == "png" || format == "bmp";
    }

    bool dragUrl(QObject*, const QPixmap& _preview, const QString& _url)
    {
        QDrag *drag = new QDrag(&(Utils::InterConnector::instance()));

        QMimeData *mimeData = new QMimeData();
        mimeData->setProperty(mimetype_marker(), true);

        mimeData->setUrls({ _url });
        drag->setMimeData(mimeData);

        QPixmap p(_preview);
        if (!p.isNull())
        {
            if (p.width() > Utils::scale_value(drag_preview_max_width))
                p = p.scaledToWidth(Utils::scale_value(drag_preview_max_width), Qt::SmoothTransformation);
            if (p.height() > Utils::scale_value(drag_preview_max_height))
                p = p.scaledToHeight(Utils::scale_value(drag_preview_max_height), Qt::SmoothTransformation);

            QPainter painter;
            painter.begin(&p);
            painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            QColor dragPreviewColor(u"#000000");
            dragPreviewColor.setAlphaF(0.5);
            painter.fillRect(p.rect(), dragPreviewColor);
            painter.end();

            drag->setPixmap(p);
        }

        bool result = drag->exec(Qt::CopyAction) == Qt::CopyAction;
        drag->deleteLater();
        return result;
    }

    StatsSender::StatsSender()
        : guiSettingsReceived_(false)
        , themeSettingsReceived_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::guiSettings, this, &StatsSender::recvGuiSettings);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::themeSettings, this, &StatsSender::recvThemeSettings);
    }

    StatsSender* getStatsSender()
    {
        static auto statsSender = std::make_shared<StatsSender>();
        return statsSender.get();
    }

    void StatsSender::trySendStats() const
    {
        if ((guiSettingsReceived_ || Ui::get_gui_settings()->getIsLoaded()) &&
            (themeSettingsReceived_ || Styling::getThemesContainer().isLoaded()))
        {
            static std::once_flag flag;
            std::call_once(flag, []() { Utils::post_stats_with_settings(); });
        }
    }

    bool haveText(const QMimeData * mimedata)
    {
        if (!mimedata)
            return false;

        if (mimedata->hasFormat(qsl("application/x-qt-windows-mime;value=\"Csv\"")))
            return true;

        if (mimedata->hasUrls())
        {
            const QList<QUrl> urlList = mimedata->urls();
            for (const auto& url : urlList)
            {
                if (url.isValid())
                    return false;
            }
        }

        const auto text = mimedata->text().trimmed();
        QUrl url(text, QUrl::ParsingMode::StrictMode);
        return !text.isEmpty() && (!url.isValid() || url.host().isEmpty());
    }

    QStringView normalizeLink(QStringView _link)
    {
        return _link.trimmed();
    }

    std::string_view get_crossprocess_mutex_name()
    {
        return config::get().string(config::values::main_instance_mutex_win);
    }

    template<typename T>
    static T initLayout(T layout)
    {
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return layout;
    }

    QHBoxLayout* emptyHLayout(QWidget* parent)
    {
        return initLayout(new QHBoxLayout(parent));
    }

    QVBoxLayout* emptyVLayout(QWidget* parent)
    {
        return initLayout(new QVBoxLayout(parent));
    }

    QGridLayout* emptyGridLayout(QWidget* parent)
    {
        return initLayout(new QGridLayout(parent));
    }

    void emptyContentsMargins(QWidget* w)
    {
        w->setContentsMargins(0, 0, 0, 0);
    }

    void transparentBackgroundStylesheet(QWidget* w)
    {
        w->setStyleSheet(qsl("background: transparent;"));
    }

    void openMailBox(const QString& email, const QString& mrimKey, const QString& mailId)
    {
        core::tools::version_info infoCurrent;
        const auto buildStr = QString::number(infoCurrent.get_build());
        const auto langStr = Utils::GetTranslator()->getLang();
        const auto url = mailId.isEmpty()
            ? getMailUrl().arg(email, mrimKey, buildStr, langStr)
            : getMailOpenUrl().arg(email, mrimKey, buildStr, langStr, msgIdFromUidl(mailId));

        QDesktopServices::openUrl(url);

        Q_EMIT Utils::InterConnector::instance().mailBoxOpened();
    }

    void openAttachUrl(const QString& _email, const QString& _mrimKey, bool _canCancel)
    {
        core::tools::version_info infoCurrent;
        const QStringView canCancel = _canCancel ? u"0" : u"1";
        const QString signed_url = getBaseMailUrl() % redirect() % u"&page=" % Features::attachPhoneUrl() % u"?nocancel=" % canCancel
            % u"&client_code=" % getProductName() % u"&back_url=" % urlSchemes().front() % u"://attach_phone_result";
        const QString final_url = signed_url.arg(_email, _mrimKey, QString::number(infoCurrent.get_build()), Utils::GetTranslator()->getLang());

        QDesktopServices::openUrl(final_url);
    }

    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey)
    {
        core::tools::version_info infoCurrent;

        const QString signed_url = getBaseMailUrl() % redirect() % u"&page=" % _url % u"&FailPage=" % _fail_url;

        const QString final_url = signed_url.arg(_email, _mrimKey, QString::number(infoCurrent.get_build()), Utils::GetTranslator()->getLang());

        QDesktopServices::openUrl(final_url);
    }


    QString getProductName()
    {
        const auto product = config::get().string(config::values::product_name_short);
        return QString::fromUtf8(product.data(), product.size()) % u".desktop";
    }

    QString getInstallerName()
    {
        const auto exe = config::get().string(config::values::installer_exe_win);
        return QString::fromUtf8(exe.data(), exe.size());
    }

    QString getUnreadsBadgeStr(const int _unreads)
    {
        QString cnt;
        if (_unreads > 0)
        {
            if (_unreads < 1000)
                cnt = QString::number(_unreads);
            else if (_unreads < 10000)
                cnt = QString::number(_unreads / 1000) % u'k';
            else
                cnt = qsl("9k+");
        }

        return cnt;
    }

    QSize getUnreadsBadgeSize(const int _unreads, const int _height)
    {
        const QString& unreadsString = getUnreadsBadgeStr(_unreads);

        const int symbolsCount = unreadsString.size();

        switch (symbolsCount)
        {
            case 1:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(20)), Utils::scale_bitmap(Utils::scale_value(20)));

                return QSize(_height, _height);
            }
            case 2:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(28)), Utils::scale_bitmap(Utils::scale_value(20)));

                const int badgeWidth = int(double(_height) * (double(baseSize.width()) / double(baseSize.height())));

                return QSize(badgeWidth, _height);
            }
            case 3:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(32)), Utils::scale_bitmap(Utils::scale_value(20)));

                const int badgeWidth = int(double(_height) * (double(baseSize.width()) / double(baseSize.height())));

                return QSize(badgeWidth, _height);

            }
            default:
            {
                im_assert(false);

                return QSize(Utils::scale_bitmap(Utils::scale_value(20)), Utils::scale_bitmap(Utils::scale_value(20)));
            }
        }
    }

    QPixmap getUnreadsBadge(const int _unreads, const QColor _bgColor, const int _height)
    {
        const QString unreadsString = getUnreadsBadgeStr(_unreads);

        const int symbolsCount = unreadsString.size();

        switch (symbolsCount)
        {
            case 1:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(20)), Utils::scale_bitmap(Utils::scale_value(20)));

                return Utils::renderSvg(qsl(":/controls/unreads_x_counter"), QSize(_height, _height), _bgColor);
            }
            case 2:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(28)), Utils::scale_bitmap(Utils::scale_value(20)));

                const int badgeWidth = int(double(_height) * (double(baseSize.width()) / double(baseSize.height())));

                return Utils::renderSvg(qsl(":/controls/unreads_xx_counter"), QSize(badgeWidth, _height), _bgColor);
            }
            case 3:
            {
                const QSize baseSize(Utils::scale_bitmap(Utils::scale_value(32)), Utils::scale_bitmap(Utils::scale_value(20)));

                const int badgeWidth = int(double(_height) * (double(baseSize.width()) / double(baseSize.height())));

                return Utils::renderSvg(qsl(":/controls/unreads_xxx_counter"), QSize(badgeWidth, _height), _bgColor);

            }
            default:
            {
                im_assert(false);

                return QPixmap();
            }
        }
    }

    namespace Badge
    {
        namespace
        {
            QSize getSize(int _textLength)
            {
                switch (_textLength)
                {
                case 1:
                    return Utils::scale_value(QSize(18, 18));
                case 2:
                    return Utils::scale_value(QSize(22, 18));
                case 3:
                    return Utils::scale_value(QSize(26, 18));
                default:
                    im_assert(!"unsupported length");
                    return QSize();
                    break;
                }
            }

            constexpr int minTextSize() noexcept
            {
                return 1;
            }

            constexpr int maxTextSize() noexcept
            {
                return 3;
            }


            std::vector<QPixmap> generatePixmaps(QStringView _basePath, Color _color)
            {
                std::vector<QPixmap> result;
                result.reserve(maxTextSize());


                auto bgColor = Styling::getParameters().getColor((_color == Color::Red) ? Styling::StyleVariable::SECONDARY_ATTENTION : (_color == Color::Gray ? Styling::StyleVariable::BASE_TERTIARY : Styling::StyleVariable::PRIMARY));
                auto borderColor =  Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);


                for (int i = minTextSize(); i <= maxTextSize(); ++i)
                    result.push_back(Utils::renderSvgLayered(_basePath % QString::number(i), {
                                                            { qsl("border"), borderColor },
                                                            { qsl("bg"), bgColor },
                                                        }));
                return result;
            }

            QPixmap getBadgeBackground(int _textLength, Color _color)
            {
                if (_textLength < minTextSize() || _textLength > maxTextSize())
                {
                    im_assert(!"unsupported length");
                    return QPixmap();
                }

                if (_color == Color::Red)
                {
                    static const std::vector<QPixmap> v = generatePixmaps(u":/controls/badge_", Color::Red);
                    return v[_textLength - 1];
                }
                else if (_color == Color::Gray)
                {
                    static const std::vector<QPixmap> v = generatePixmaps(u":/controls/badge_", Color::Gray);
                    return v[_textLength - 1];
                }
                else
                {
                    static const std::vector<QPixmap> v = generatePixmaps(u":/controls/badge_", Color::Green);
                    return v[_textLength - 1];
                }
            }
        }

        int drawBadge(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _x, int _y, Color _color)
        {
            const auto textLength = _textUnit->getSourceText().size();

            if (textLength < minTextSize() || textLength > maxTextSize())
            {
                im_assert(!"unsupported length");
                return 0;
            }

            Utils::PainterSaver saver(_p);
            _textUnit->evaluateDesiredSize();
            const auto desiredWidth = _textUnit->desiredWidth();
            _p.setRenderHint(QPainter::Antialiasing);
            _p.setRenderHint(QPainter::TextAntialiasing);
            _p.setRenderHint(QPainter::SmoothPixmapTransform);
            const auto size = getSize(textLength);
            _textUnit->setOffsets(_x + (size.width() - desiredWidth + 1) / 2,
                                  _y + size.height() / 2);
            _p.drawPixmap(_x, _y, getBadgeBackground(textLength, _color));
            _textUnit->draw(_p, Ui::TextRendering::VerPosition::MIDDLE);
            return size.width();
        }

        int drawBadgeRight(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _x, int _y, Color _color)
        {
            const auto textLength = _textUnit->getSourceText().size();
            if (textLength < minTextSize() || textLength > maxTextSize())
            {
                im_assert(!"unsupported length");
                return 0;
            }

            const auto size = getSize(textLength);
            return drawBadge(_textUnit, _p, _x - size.width(), _y, _color);
        }
    }


    int drawUnreads(
        QPainter& _p,
        const QFont& _font,
        const QColor& _bgColor,
        const QColor& _textColor,
        const int _unreadsCount,
        const int _badgeHeight,
        const int _x,
        const int _y)
    {
        PainterSaver p(_p);

        QFontMetrics m(_font);

        const auto text = getUnreadsBadgeStr(_unreadsCount);

        const auto unreadsRect = m.tightBoundingRect(text);

        struct cachedBage
        {
            QColor color_;
            QSize size_;
            QPixmap badge_;

            cachedBage(const QColor& _color, const QSize& _size, const QPixmap& _badge)
                : color_(_color), size_(_size), badge_(_badge)
            {
            }
        };

        static std::vector<cachedBage> cachedBages;

        const QSize badgeSize = getUnreadsBadgeSize(_unreadsCount, _badgeHeight);

        const QPixmap* badge = nullptr;

        for (const cachedBage& _badge : cachedBages)
        {
            if (_bgColor == _badge.color_ && badgeSize == _badge.size_)
            {
                badge = &_badge.badge_;
            }
        }

        if (!badge)
        {
            cachedBages.emplace_back(_bgColor, badgeSize, getUnreadsBadge(_unreadsCount, _bgColor, _badgeHeight));

            badge = &(cachedBages[cachedBages.size() - 1].badge_);
        }

        _p.drawPixmap(_x, _y, *badge);

        _p.setFont(_font);
        _p.setPen(_textColor);

        const QChar firstChar = text[0];
        const QChar lastChar = text[text.size() - 1];
        const int unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
        const int unreadsHeight = unreadsRect.height();

        if constexpr (platform::is_apple())
        {
            _p.drawText(QRectF(_x, _y, badgeSize.width(), badgeSize.height()), text, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
        }
        else
        {
            const float textX = floorf(float(_x) + (float(badgeSize.width()) - float(unreadsWidth)) / 2.f);
            const float textY = ceilf(float(_y) + (float(badgeSize.height()) + float(unreadsHeight)) / 2.f);

            _p.drawText(textX, textY, text);
        }

        return badgeSize.width();
    }

    QPixmap pixmapWithEllipse(const QPixmap& _original, const QColor& _brushColor, int brushWidth)
    {
        if (!brushWidth)
            return _original;

        QPixmap result(_original.width(),
                       _original.height());
        result.fill(Qt::transparent);

        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing);

        p.drawPixmap(0, 0, _original.width(), _original.height(), _original);

        QPen pen(_brushColor);
        pen.setWidth(brushWidth);
        p.setPen(pen);

        auto rect = result.rect();
        rect.setLeft(rect.left() + brushWidth);
        rect.setTop(rect.top() + brushWidth);
        rect.setWidth(_original.width() - 2 * brushWidth);
        rect.setHeight(_original.height() - 2 * brushWidth);

        p.drawEllipse(rect);

        return result;
    }

    int drawUnreads(QPainter *p, const QFont &font, QColor bgColor, QColor textColor, QColor borderColor, int unreads, int balloonSize, int x, int y, QPainter::CompositionMode borderMode)
    {
        if (!p || unreads <= 0)
            return 0;

        QFontMetrics m(font);

        const auto text = getUnreadsBadgeStr(unreads);
        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
        const auto unreadsHeight = unreadsRect.height();

        auto balloonWidth = unreadsWidth;
        const auto isLongText = (text.length() > 1);
        if (isLongText)
        {
            balloonWidth += m.height();
        }
        else
        {
            balloonWidth = balloonSize;
        }

        const auto balloonRadius = (balloonSize / 2);

        PainterSaver saver(*p);
        p->setPen(Qt::NoPen);
        p->setRenderHint(QPainter::Antialiasing);

        auto mode = p->compositionMode();
        int borderWidth = 0;
        if (borderColor.isValid())
        {
            p->setCompositionMode(borderMode);
            p->setBrush(borderColor);
            p->setPen(borderColor);
            borderWidth = Utils::scale_value(1);
            p->drawRoundedRect(x - borderWidth, y - borderWidth, balloonWidth + borderWidth * 2, balloonSize + borderWidth * 2, balloonRadius, balloonRadius);
        }

        p->setCompositionMode(mode);
        p->setBrush(bgColor);
        p->drawRoundedRect(x, y, balloonWidth, balloonSize, balloonRadius, balloonRadius);

        p->setFont(font);
        p->setPen(textColor);
        if constexpr (platform::is_apple())
        {
            p->drawText(QRectF(x, y, balloonWidth, balloonSize), text, QTextOption(Qt::AlignCenter));
        }
        else
        {
            const float textX = floorf((float)x + ((float)balloonWidth - (float)unreadsWidth) / 2.);
            const float textY = ceilf((float)y + ((float)balloonSize + (float)unreadsHeight) / 2.);
            p->drawText(textX, textY, text);
        }

        return balloonWidth;
    }

    QSize getUnreadsSize(const QFont& _font, const bool _withBorder, const int _unreads, const int _balloonSize)
    {
        const auto text = getUnreadsBadgeStr(_unreads);
        const QFontMetrics m(_font);
        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

        auto balloonWidth = unreadsWidth;
        if (text.length() > 1)
        {
            balloonWidth += m.height();
        }
        else
        {
            balloonWidth = _balloonSize;
        }

        const auto borderWidth = _withBorder ? Utils::scale_value(2) : 0;
        return QSize(
            balloonWidth + borderWidth,
            _balloonSize + borderWidth
        );
    }

    QImage iconWithCounter(int size, int count, QColor bg, QColor fg, QImage back)
    {
        const QString cnt = getUnreadsBadgeStr(count);

        const auto isBackNull = back.isNull();

        QImage result = isBackNull ? QImage(size, size, QImage::Format_ARGB32) : std::move(back);
        int32_t cntSize = cnt.size();
        if (isBackNull)
            result.fill(Qt::transparent);

        {
            QPainter p(&result);
            p.setBrush(bg);
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);

            //don't try to understand it. just walk away. really.
            int32_t top = 0;
            int32_t radius = cntSize > 2 ? 6 : 8;
            int32_t shiftY = 0;
            int32_t shiftX = count > 10000 ? 1 : 0;
            int32_t left = 0;
            int32_t fontSize = 8;

            if (isBackNull)
            {
                fontSize = (cntSize < 3) ? 11 : 8;

                if (size == 16)
                {
                    shiftY = cntSize > 2 ? 2 : 0;
                    top = cntSize > 2 ? 4 : 0;
                }
                else
                {
                    shiftY = cntSize > 2 ? 4 : 0;
                    top = cntSize > 2 ? 8 : 0;
                }
            }
            else
            {
                if (size == 16)
                {
                    left = cntSize > 2 ? 0 : 4;
                    shiftY = 2;
                    top = 4;
                }
                else
                {
                    left = cntSize > 2 ? 0 : 8;
                    shiftY = 4;
                    top = 8;
                }
            }

            if (size > 16)
            {
                radius *= 2;
                fontSize *= 2;
                shiftX *= 2;
            }

            auto rect = QRect(left, top, size - left, size - top);
            p.drawRoundedRect(rect, radius, radius);

            auto f = QSysInfo::WindowsVersion != QSysInfo::WV_WINDOWS10
                      ? Fonts::appFont(fontSize, Fonts::FontFamily::ROUNDED_MPLUS)
                      : Fonts::appFont(fontSize, Fonts::FontWeight::Normal);

            f.setStyleStrategy(QFont::PreferQuality);

            QFontMetrics m(f);
            const auto unreadsRect = m.tightBoundingRect(cnt);
            const auto firstChar = cnt[0];
            const auto lastChar = cnt[cnt.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
            const auto unreadsHeight = unreadsRect.height();
            const auto textX = floorf(float(size - unreadsWidth) / 2. + shiftX + left / 2.);
            const auto textY = ceilf(float(size + unreadsHeight) / 2. + shiftY);

            p.setFont(f);
            p.setPen(fg);
            p.drawText(textX, textY, cnt);
        }
        return result;
    }

    QStringView extractAimIdFromMention(QStringView _mention)
    {
        im_assert(isMentionLink(_mention));
        return _mention.mid(_mention.indexOf(u'[') + 1, _mention.size() - 3);
    }

    static bool openConfirmDialog(const QString& _label, const QString& _infoText, bool _isSelected, const QString& _setting)
    {
        auto widget = new Utils::CheckableInfoWidget(nullptr);
        widget->setCheckBoxText(QT_TRANSLATE_NOOP("popup_window", "Don't ask me again"));
        widget->setCheckBoxChecked(_isSelected);
        widget->setInfoText(_infoText);

        Ui::GeneralDialog generalDialog(widget, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(_label);
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Yes"), true);
        
        const auto isConfirmed = generalDialog.showInCenter();
        if (isConfirmed)
        {
            if (widget->isChecked())
                Ui::get_gui_settings()->set_value<bool>(_setting, true);
        }

        return isConfirmed;
    }

    void openUrl(QStringView _url, OpenUrlConfirm _confirm)
    {
        if (Utils::isMentionLink(_url))
        {
            const auto aimId = extractAimIdFromMention(_url);
            if (!aimId.isEmpty())
            {
                std::string resStr;

                switch (openDialogOrProfile(aimId.toString(), OpenDOPParam::aimid))
                {
                case OpenDOPResult::dialog:
                    resStr = "Chat_Opened";
                    break;
                case OpenDOPResult::profile:
                    resStr = "Profile_Opened";
                    break;
                default:
                    break;
                }
                im_assert(!resStr.empty());
                if (!resStr.empty())
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_click_in_chat, { { "Result", std::move(resStr) } });
                return;
            }
        }

        const auto url = _url.toString();
        if (const auto command = UrlCommandBuilder::makeCommand(url, UrlCommandBuilder::Context::Internal); command->isValid())
        {
            command->execute();
        }
        else
        {
            auto openWithoutWarning = []()
            {
                return Ui::get_gui_settings()->get_value<bool>(settings_open_external_link_without_warning, settings_open_external_link_without_warning_default());
            };

            auto isConfirmed = true;
            if (_confirm == OpenUrlConfirm::Yes && !openWithoutWarning())
            {
                auto elidedUrl = url;
                if (elidedUrl.size() > getUrlInDialogElideSize())
                    elidedUrl = elidedUrl.mid(0, getUrlInDialogElideSize()) % u"...";

                isConfirmed = openConfirmDialog(QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to open an external link?"),
                                                elidedUrl,
                                                settings_open_external_link_without_warning_default(),
                                                qsl(settings_open_external_link_without_warning));
            }

            if (isConfirmed && !QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode)))
                Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "Unable to open invalid url \"%1\"").arg(url));
        }
    }

    void openFileOrFolder(const QString& _chatAimId, QStringView _path, OpenAt _openAt, OpenWithWarning _withWarning)
    {
        const auto isPublic = Logic::getContactListModel()->isPublic(_chatAimId);
        const auto isStranger = _chatAimId.isEmpty() || Logic::getRecentsModel()->isStranger(_chatAimId) || Logic::getRecentsModel()->isSuspicious(_chatAimId);
        const auto openFile = Features::opensOnClick() && (_openAt == OpenAt::Launcher) && !isPublic && !isStranger;

#ifdef __APPLE__
        if (openFile)
            MacSupport::openFile(_path.toString());
        else
            MacSupport::openFinder(_path.toString());
#else
        const auto pathString = _path.toString();
        auto nativePath = QDir::toNativeSeparators(pathString);
        if (openFile)
        {
            auto exeFile = [&nativePath]()
            {
#ifdef _WIN32
                ShellExecute(0, L"open", nativePath.toStdWString().c_str(), 0, 0, SW_SHOWNORMAL);
#else
                QDesktopServices::openUrl(QString(u"file:///" % nativePath));
#endif
            };

            auto isAvailableWithoutWarning = []()
            {
                return Ui::get_gui_settings()->get_value<bool>(settings_exec_files_without_warning, settings_exec_files_without_warning_default());
            };

            auto isConfirmed = true;
            if (_withWarning == OpenWithWarning::Yes && !isAvailableWithoutWarning() && isDangerousExtensions(QFileInfo(pathString).suffix()))
            {
                isConfirmed = openConfirmDialog(QT_TRANSLATE_NOOP("popup_window", "Run this file?"),
                                                QT_TRANSLATE_NOOP("popup_window", "This file may be unsafe"),
                                                settings_exec_files_without_warning_default(),
                                                qsl(settings_exec_files_without_warning));
                
            }

            if (isConfirmed)
                exeFile();
        }
        else
        {
#ifdef _WIN32
            nativePath = nativePath.replace(u'"', ql1s("\"\""));
            ShellExecute(0, 0, L"explorer", ql1s("/select,\"%1\"").arg(nativePath).toStdWString().c_str(), 0, SW_SHOWNORMAL);
#else
            QDir dir(nativePath);
            dir.cdUp();
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
#endif
        }
#endif // __APPLE__
    }

    void convertMentionsToFriendlyPlainText(Data::FString& _source, const Data::MentionMap& _mentions)
    {
        convertMentions(_source, [&_mentions](const auto& _ft, Data::FStringView _view)
        {
            const auto aimId = Utils::extractAimIdFromMention(_view.string());
            if (const auto it = _mentions.find(aimId); it != _mentions.end())
                return _view.replaceByString(it->second, false);
            else
                return _view.toFString();
        });
    }

    static void replaceTextWithinMarkersByFormat(Data::FString& _text, QStringView _marker, core::data::format_type _type)
    {
        using ftype = core::data::format_type;
        const auto markerSize = _marker.size();

        const auto view = Data::FStringView(_text);
        Data::FString::Builder builder;
        auto offset = 0;
        while (offset < view.size())
        {
            const auto start = view.indexOf(_marker, offset);
            if (start == -1)
                break;

            auto end = view.indexOf(_marker, start + markerSize);
            if (-1 == end)
                break;
            end += markerSize;

            const auto viewWithMarkers = view.mid(start, end - start);
            const auto innerView = viewWithMarkers.mid(markerSize, viewWithMarkers.size() - 2 * markerSize);
            if (innerView.isEmpty() || viewWithMarkers.isAnyOf({ftype::monospace, ftype::pre}))
            {
                builder %= view.mid(offset, end - offset);
            }
            else
            {
                if (offset < start)
                    builder %= view.mid(offset, start - offset);

                auto part = innerView.toFString();
                part.addFormat(_type);
                builder %= std::move(part);
            }

            offset = end;
        }
        if (offset > 0 && offset < view.size())
            builder %= Data::FStringView(_text, offset, view.size() - offset);

        if (!builder.isEmpty())
            _text = builder.finalize();
    }

    void convertOldStyleMarkdownToFormats(Data::FString& _text, Ui::ParseBackticksPolicy _backticksPolicy)
    {
        using Policy = Ui::ParseBackticksPolicy;
        // Before formatting was introduced client supported displaying text in single and
        // triple backticks as monospaced (aka partial markdown support). These are to be supported still
        using ftype = core::data::format_type;

        if (_text.isEmpty() || _text.containsAnyFormatExceptFor(ftype::mention))
            return;

        if (_backticksPolicy == Policy::ParseTriples || _backticksPolicy == Policy::ParseSinglesAndTriples)
            replaceTextWithinMarkersByFormat(_text, Data::tripleBackTick(), ftype::pre);
        if (_backticksPolicy == Policy::ParseSingles || _backticksPolicy == Policy::ParseSinglesAndTriples)
            replaceTextWithinMarkersByFormat(_text, QString(Data::singleBackTick()), ftype::monospace);
    }

    void convertMentions(
        Data::FString& _source,
        std::function<Data::FString(const core::data::range_format&, Data::FStringView)> _converter)
    {
        const auto sourceView = Data::FStringView(_source);
        Data::FString::Builder builder;
        int start = 0;
        for (const auto& _ft : _source.formatting().formats())
        {
            if (_ft.type_ != core::data::format_type::mention)
                continue;

            const auto [offset, length] = _ft.range_;

            builder %= sourceView.mid(start, offset - start);
            builder %= _converter(_ft, sourceView.mid(offset, length));

            start = offset + length;
        }
        if (start < sourceView.size())
            builder %= sourceView.mid(start, sourceView.size() - start);

        _source = builder.finalize();
    }

    QString convertMentions(const QString& _source, const Data::MentionMap& _mentions)
    {
        if (_mentions.empty())
            return _source;

        static const auto mentionMarker = ql1s("@[");
        auto ndxStart = _source.indexOf(mentionMarker);
        if (ndxStart == -1)
            return _source;

        QString result;
        result.reserve(_source.size());

        auto ndxEnd = -1;

        while (ndxStart != -1)
        {
            result += _source.midRef(ndxEnd + 1, ndxStart - (ndxEnd + 1));
            ndxEnd = _source.indexOf(u']', ndxStart + 2);
            if (ndxEnd != -1)
            {
                const auto it = _mentions.find(_source.midRef(ndxStart + 2, ndxEnd - ndxStart - 2));
                if (it != _mentions.end())
                    result += it->second;
                else
                    result += _source.midRef(ndxStart, ndxEnd + 1 - ndxStart);

                ndxStart = _source.indexOf(mentionMarker, ndxEnd + 1);
            }
            else
            {
                result += _source.rightRef(_source.size() - ndxStart);
                return result;
            }
        }
        result += _source.rightRef(_source.size() - ndxEnd - 1);

        if (!result.isEmpty())
            return result; // if we ocasionally replace with smth empty

        return _source;
    }

    void convertMentions(Data::FString& _source, const Data::MentionMap& _mentions)
    {
        if (_mentions.empty() || _source.isEmpty())
            return;

        if (!_source.hasFormatting())
            _source = convertMentions(_source.string(), _mentions);

        convertMentions(_source, [&_mentions](const auto& _ft, Data::FStringView _view)
        {
            const auto aimId = Utils::extractAimIdFromMention(_view.string());
            if (const auto it = _mentions.find(aimId); it != _mentions.end())
                return _view.replaceByString(it->second);
            else
                return _view.toFString();
        });
    }

    QString convertFilesPlaceholders(const QStringRef& _source, const Data::FilesPlaceholderMap& _files)
    {
        if (_files.empty())
            return _source.toString();

        constexpr auto filePlaceholderMarker = u'[';
        auto ndxStart = _source.indexOf(filePlaceholderMarker);
        if (ndxStart == -1)
            return _source.toString();

        QString result;
        result.reserve(_source.size());

        auto ndxEnd = -1;

        while (ndxStart != -1)
        {
            result += _source.mid(ndxEnd + 1, ndxStart - (ndxEnd + 1));
            ndxEnd = _source.indexOf(u']', ndxStart + 2);
            if (ndxEnd != -1)
            {
                const auto it = _files.find(_source.mid(ndxStart + 2, ndxEnd - ndxStart - 2));
                if (it != _files.end())
                    result += it->second;
                else
                    result += _source.mid(ndxStart, ndxEnd + 1 - ndxStart);

                ndxStart = _source.indexOf(filePlaceholderMarker, ndxEnd + 1);
            }
            else
            {
                result += _source.right(_source.size() - ndxStart);
                return result;
            }
        }
        result += _source.right(_source.size() - ndxEnd - 1);

        if (!result.isEmpty())
            return result; // if we ocasionally replace with smth empty

        return _source.toString();
    }

    void convertFilesPlaceholders(Data::FString& _source, const Data::FilesPlaceholderMap& _files)
    {
        if (_files.empty() || _source.isEmpty())
            return;

        if (!_source.hasFormatting())
            _source = convertFilesPlaceholders(_source.string(), _files);

        auto ranges = std::vector<core::data::range>();
        {
            int i = 0;
            while (i < _source.size())
            {
                const auto leftBracketPos = _source.indexOf(u'[', i);
                if (leftBracketPos == -1)
                    break;
                const auto rightBracketPos = _source.indexOf(u']', leftBracketPos);
                if (rightBracketPos == -1)
                    break;
                ranges.push_back({ leftBracketPos, rightBracketPos - leftBracketPos + 1 });
                i = rightBracketPos + 1;
            }
        }

        const auto sourceView = Data::FStringView(_source);
        Data::FString::Builder builder;
        int start = 0;
        for (const auto& [offset, length] : ranges)
        {
            const auto placeholder = sourceView.mid(offset, length);
            builder %= sourceView.mid(start, offset - start);
            if (const auto it = _files.find(placeholder.string()); it != _files.end() && !placeholder.hasFormatting())
            {
                const auto hasLineFeedBefore = offset > 0 && sourceView.at(offset - 1) == QChar::LineFeed;
                const auto hasLineFeedAfter = offset + length < sourceView.size() && sourceView.at(offset + length) == QChar::LineFeed;
                if (!hasLineFeedBefore)
                    builder %= qsl("\n");
                builder %= it->second;
                if (!hasLineFeedAfter)
                    builder %= qsl("\n");
            }
            else
            {
                builder %= placeholder;
            }

            start = offset + length;
        }
        if (start < sourceView.size())
            builder %= sourceView.mid(start, sourceView.size() - start);

        _source = builder.finalize();
    }

    QString setFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files)
    {
        for (const auto& p : _files)
            _text.replace(p.first, p.second);
        return _text;
    }

    Data::FString setFilesPlaceholders(const Data::FString& _text, const Data::FilesPlaceholderMap& _files)
    {
        if (_text.hasFormatting())
            return _text;
        else
            return setFilesPlaceholders(_text.string(), _files);
    }

    bool isNick(QStringView _text)
    {
        // match alphanumeric str with @, starting with a-zA-Z
        static const QRegularExpression re(qsl("^@[a-zA-Z0-9][a-zA-Z0-9._]*$"), QRegularExpression::UseUnicodePropertiesOption);
        return re.match(_text).hasMatch();
    }

    QString makeNick(const QString& _text)
    {
        return (_text.isEmpty() || _text.contains(u'@')) ? _text : (u'@' % _text);
    }

    bool isMentionLink(QStringView _url)
    {
        return _url.startsWith(u"@[") && _url.endsWith(u']');
    }

    bool doesContainMentionLink(QStringView _text)
    {
        if (const auto idxStart = _text.indexOf(u"@["); idxStart >= 0)
        {
            const auto idxEnd = _text.indexOf(u']', idxStart + 2);
            return idxEnd != -1;
        }
        return false;
    }

    OpenDOPResult openDialogOrProfile(const QString& _contact, const OpenDOPParam _paramType)
    {
        im_assert(!_contact.isEmpty());

        if (Ui::get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default()))
            Q_EMIT Utils::InterConnector::instance().searchEnd();
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        if (_paramType == OpenDOPParam::stamp)
        {
            if (const auto& aimid = Logic::getContactListModel()->getAimidByStamp(_contact); !aimid.isEmpty())
            {
                openDialogWithContact(aimid, -1, true);
            }
            else
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("stamp", _contact);
                collection.set_value_as_int("limit", 0);
                Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get(), Ui::MainPage::instance(), [](core::icollection* _coll)
                {
                    Ui::gui_coll_helper coll(_coll, false);

                    if (const auto error = coll.get_value_as_int("error"); error != 0)
                    {
                        QString errorText;
                        switch ((core::group_chat_info_errors)error)
                        {
                            case core::group_chat_info_errors::network_error:
                                errorText = QT_TRANSLATE_NOOP("groupchats", "Chat information is unavailable now, please try again later");
                                break;
                            default:
                                errorText = QT_TRANSLATE_NOOP("groupchats", "Chat does not exist or it is hidden by privacy settings");
                                break;
                        }
                        Utils::showTextToastOverContactDialog(errorText);
                    }
                    else if (const auto contact = coll.get<QString>("aimid"); !contact.isEmpty())
                    {
                        const auto info = std::make_shared<Data::ChatInfo>(Data::UnserializeChatInfo(&coll));
                        Logic::getContactListModel()->updateChatInfo(*info);

                        openDialogWithContact(contact, -1, true);
                    }
                });
            }
            return OpenDOPResult::dialog;
        }
        else
        {
            /*We open a dialog with the user if:
            -is this a chat or have we already corresponded with this contact
            - when you click on head, the call comes from the thread from tasks page, and not head from the messenger*/
            const auto* contactModel = Logic::getContactListModel();
            const auto isMessenger = InterConnector::instance().getMainWindow()->isMessengerPageContactDialog();
            if ((isChat(_contact) || contactModel->contains(_contact)) && (!isMessenger || contactModel->selectedContact() != _contact))
            {
                openDialogWithContact(_contact);
                return OpenDOPResult::dialog;
            }
            else
            {
                Q_EMIT Utils::InterConnector::instance().profileSettingsShow(_contact);
                return OpenDOPResult::profile;
            }
        }
    }

    void openDialogWithContact(const QString& _contact, qint64 _id, bool _sel, std::function<void(Ui::PageBase*)> _getPageCallback)
    {
        Logic::GetFriendlyContainer()->updateFriendly(_contact);
        Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(Logic::getContactListModel()->selectedContact());
        Utils::InterConnector::instance().openDialog(_contact, _id, _sel, _getPageCallback);
    }

    bool clicked(const QPoint& _prev, const QPoint& _cur, int dragDistance)
    {
        if (dragDistance == 0)
            dragDistance = 5;

        return std::abs(_cur.x() - _prev.x()) < dragDistance && std::abs(_cur.y() - _prev.y()) < dragDistance;
    }

    void drawBubbleRect(QPainter& _p, const QRectF& _rect, const QColor& _color, int _bWidth, int _bRadious)
    {
        Utils::PainterSaver saver(_p);

        auto pen = QPen(_color, _bWidth);
        _p.setPen(pen);
        auto r = _rect;
        auto diff = Utils::getScaleCoefficient() == 2 ? 1 : 0;
        r = QRect(r.x(), r.y() + 1, r.width(), r.height() - _bWidth - 1 + diff);

        _p.setRenderHint(QPainter::Antialiasing, false);
        _p.setRenderHint(QPainter::TextAntialiasing, false);

        auto add = (Utils::getScaleCoefficient() == 2 || Utils::is_mac_retina()) ? 0 : 1;
        _p.drawLine(QPoint(r.x() + _bRadious + _bWidth, r.y() - add), QPoint(r.x() - _bRadious + r.width() - _bWidth, r.y() - add));
        _p.drawLine(QPoint(r.x() + r.width(), r.y() + _bRadious + _bWidth), QPoint(r.x() + r.width(), r.y() + r.height() - _bRadious - _bWidth));
        _p.drawLine(QPoint(r.x() + _bRadious + _bWidth, r.y() + r.height()), QPoint(r.x() - _bRadious + r.width() - _bWidth, r.y() + r.height()));
        _p.drawLine(QPoint(r.x() - add, r.y() + _bRadious + _bWidth), QPoint(r.x() - add, r.y() + r.height() - _bRadious - _bWidth));

        _p.setRenderHint(QPainter::Antialiasing);
        _p.setRenderHint(QPainter::TextAntialiasing);

        QRectF rectangle(r.x(), r.y(), _bRadious * 2, _bRadious * 2);
        int stA = 90 * 16;
        int spA = 90 * 16;
        _p.drawArc(rectangle, stA, spA);

        rectangle = QRectF(r.x() - _bRadious * 2 + r.width(), r.y(), _bRadious * 2, _bRadious * 2);
        stA = 0 * 16;
        spA = 90 * 16;
        _p.drawArc(rectangle, stA, spA);

        rectangle = QRectF(r.x() + r.width() - _bRadious * 2, r.y() + r.height() - _bRadious * 2, _bRadious * 2, _bRadious * 2);
        stA = 0 * 16;
        spA = -90 * 16;
        _p.drawArc(rectangle, stA, spA);

        rectangle = QRectF(r.x(), r.y() + r.height() - _bRadious * 2, _bRadious * 2, _bRadious * 2);
        stA = 180 * 16;
        spA = 90 * 16;
        _p.drawArc(rectangle, stA, spA);
    }

    int getShadowMargin() noexcept
    {
        return Utils::scale_value(2);
    }

    void drawBubbleShadow(QPainter& _p, const QPainterPath& _bubble, const int _clipLength, const int _shadowMargin, const QColor _shadowColor)
    {
        Utils::PainterSaver saver(_p);

        _p.setRenderHint(QPainter::Antialiasing);

        auto clip = _bubble.boundingRect();
        const auto clipLen = _clipLength == -1 ? Ui::MessageStyle::getBorderRadius() : _clipLength;
        const auto shadowMargin = _shadowMargin == -1 ? Utils::getShadowMargin() : _shadowMargin;
        const auto shadowColor = _shadowColor.isValid() ? _shadowColor : QColor(0, 0, 0, 255 * 0.01);

        clip.adjust(0, clip.height() - clipLen, 0, shadowMargin);

        _p.setClipRect(clip);

        _p.fillPath(_bubble.translated(0, shadowMargin), shadowColor);
        _p.fillPath(_bubble.translated(0, shadowMargin/2), QColor(0, 0, 0, 255 * 0.08));
    }

    QColor plateShadowColor() noexcept
    {
        return QColor(0, 0, 0, 255);
    }

    constexpr double plateShadowColorAlpha()
    {
        return 0.12;
    }

    QColor plateShadowColorWithAlpha(double _opacity)
    {
        auto color = plateShadowColor();
        color.setAlphaF(plateShadowColorAlpha() * _opacity);
        return color;
    }

    QGraphicsDropShadowEffect* initPlateShadowEffect(QWidget* _parent, double _opacity)
    {
        auto shadow = new QGraphicsDropShadowEffect(_parent);
        shadow->setBlurRadius(8);
        shadow->setOffset(0, Utils::scale_value(1));
        shadow->setColor(plateShadowColorWithAlpha(_opacity));
        return shadow;
    }

    void drawPlateSolidShadow(QPainter& _p, const QPainterPath& _path)
    {
        const auto shadowHeight = Utils::scale_value(1);

        Utils::ShadowLayer layer;
        layer.color_ = QColor(17, 32, 55, 255 * 0.05);
        layer.yOffset_ = shadowHeight;

        Utils::drawLayeredShadow(_p, _path, { layer });
    }

    QSize avatarWithBadgeSize(const int _avatarWidthScaled)
    {
        auto& badges = getBadgesData();

        const auto it = badges.find(Utils::scale_bitmap(_avatarWidthScaled));
        if (it == badges.end())
            return QSize(_avatarWidthScaled, _avatarWidthScaled);

        const auto& badge = it->second;

        const auto w = std::max(_avatarWidthScaled, badge.offset_.x() + badge.size_.width());
        const auto h = std::max(_avatarWidthScaled, badge.offset_.y() + badge.size_.height());

        return QSize(w, h);
    }

    const StatusBadge& getStatusBadgeParams(const int _avatarWidthScaled)
    {
        static const std::map<int, StatusBadge> badgesRectMap =
        {
            {Utils::scale_bitmap_with_value(24), StatusBadge(14, 18)},
            {Utils::scale_bitmap_with_value(32), StatusBadge(20, 13)},
            {Utils::scale_bitmap_with_value(52), StatusBadge(32, 19)},
            {Utils::scale_bitmap_with_value(56), StatusBadge(32, 26)},
            {Utils::scale_bitmap_with_value(60), StatusBadge(36, 24)},
            {Utils::scale_bitmap_with_value(80), StatusBadge(54, 28)},
            {Utils::scale_bitmap_with_value(104), StatusBadge(54, 28)},
        };

        const auto it = badgesRectMap.find(_avatarWidthScaled);
        if (it == badgesRectMap.end())
        {
            static const StatusBadge empty;
            return empty;
        }

        return it->second;
    }

    QPixmap getStatusBadge(const QString& _aimid, int _avatarSize)
    {
        if (Statuses::isStatusEnabled())
        {
            const auto badgeSize = getStatusBadgeParams(_avatarSize).size_.width();
            if (const auto status = Logic::GetStatusContainer()->getStatus(_aimid); !status.isEmpty())
                return QPixmap::fromImage(status.getImage(badgeSize));

            if (Logic::GetLastseenContainer()->isBot(_aimid))
                return getBotDefaultBadge(badgeSize);
        }
        return QPixmap();
    }

    QPixmap getBotDefaultBadge(int _badgeSize)
    {
        auto image = Emoji::GetEmoji(Emoji::EmojiCode(0x1F916), Emoji::EmojiSizePx(_badgeSize));
        Utils::check_pixel_ratio(image);
        return QPixmap::fromImage(std::move(image));
    }

    QPixmap getEmptyStatusBadge(StatusBadgeState _state, int _avatarSize)
    {
        const auto badgeSize = getStatusBadgeParams(_avatarSize).size_.width();
        const auto color = (_state == StatusBadgeState::Hovered) ? Styling::StyleVariable::PRIMARY_HOVER
                        : ((_state == StatusBadgeState::Pressed) ? Styling::StyleVariable::PRIMARY_ACTIVE
                        : Styling::StyleVariable::PRIMARY);
        return Utils::renderSvg(qsl(":/avatar_badges/add_status"), QSize(badgeSize, badgeSize), Styling::getParameters().getColor(color));
    }

    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _avatar, const bool _isOfficial, const QPixmap& _status, const bool _isMuted, const bool _isSelected, const bool _isOnline, const bool _small_online, bool _withOverlay)
    {
        im_assert(!_avatar.isNull());

        auto drawOverlay = [&_p, &_topLeft](QSize _scaledSize, QRect _cutRect)
        {
            QPixmap overlay(_scaledSize);
            overlay.setDevicePixelRatio(1);
            overlay.fill(Qt::transparent);
            QPainter p(&overlay);
            p.setPen(Qt::NoPen);
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            p.setBrush(avatarOverlayColor());
            p.drawEllipse(overlay.rect());
            if (_cutRect.isValid())
            {
                p.setBrush(Qt::transparent);
                p.setCompositionMode(QPainter::CompositionMode_Source);
                p.drawEllipse(_cutRect);
            }
            Utils::check_pixel_ratio(overlay);
            _p.drawPixmap(_topLeft, overlay);
        };

        auto& badges = getBadgesData();
        const auto hasStatus = !_status.isNull();
        const auto it = badges.find(_avatar.width());
        if ((!_isMuted && !_isOfficial && !_isOnline && !hasStatus) || it == badges.end())
        {
            if ((_isMuted || _isOfficial || _isOnline) && it == badges.end())
                im_assert(!"unknown size");

            // Draw without badge
            _p.drawPixmap(_topLeft, _avatar);
            if (_withOverlay)
                drawOverlay(_avatar.size(), QRect());
            return;
        }

        auto badge = it->second;
        if (!_isMuted && hasStatus)
        {
            const auto& statusBadgeParams = getStatusBadgeParams(_avatar.width());
            if (statusBadgeParams.isValid())
            {
                badge.offset_ = statusBadgeParams.offset_;
                badge.size_ = statusBadgeParams.size_;
            }
        }
        const QRect badgeRect(badge.offset_, badge.size_);

        const auto w = badge.cutWidth_;
        const auto cutRect = badgeRect.adjusted(-w, -w, w, w);
        QPixmap cut = _avatar;
        {
            QPainter p(&cut);
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::transparent);
            p.setRenderHint(QPainter::Antialiasing);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawEllipse(cutRect);
        }
        _p.drawPixmap(_topLeft, cut);

        if (_withOverlay)
            drawOverlay(_avatar.size(), cutRect);

        Utils::PainterSaver ps(_p);
        _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        _p.setPen(Qt::NoPen);

        static const auto muted = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        static const auto selMuted = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        static const auto official = QColor(u"#2594ff");
        const auto& badgeColor = _isMuted ? (_isSelected ? selMuted : muted) : (_isOfficial && !hasStatus ? official : Qt::transparent);
        _p.setBrush(badgeColor);
        _p.drawEllipse(badgeRect.translated(_topLeft));

        const auto& icon = _isMuted ? badge.muteIcon_
                        : (hasStatus ? _status
                        : (_isOfficial ? badge.officialIcon_
                        : (_small_online ? (_isSelected ? badge.smallSelectedOnlineIcon_ : badge.smallOnlineIcon_)
                        : (_isSelected ? badge.onlineSelectedIcon_ : badge.onlineIcon_))));

        if (!icon.isNull())
        {
            const auto ratio = Utils::scale_bitmap_ratio();
            const auto x = _topLeft.x() + badge.offset_.x() + (badge.size_.width() - icon.width() / ratio) / 2;
            const auto y = _topLeft.y() + badge.offset_.y() + (badge.size_.height() - icon.height() / ratio) / 2 - badge.emojiOffsetY_;

            _p.drawPixmap(x, y, icon);
        }
    }

    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _avatar, const QString& _aimid, const bool _officialOnly, StatusBadgeState _state, const bool _isSelected, const bool _small_online)
    {
        im_assert(!_avatar.isNull());

        const auto noBadge = _aimid.isEmpty() || _state == StatusBadgeState::StatusOnly;
        const auto isMuted = noBadge ? false : !_officialOnly && Logic::getContactListModel()->isMuted(_aimid);
        const auto isOfficial = noBadge ? false : !isMuted && Logic::GetFriendlyContainer()->getOfficial(_aimid);
        const auto isOnline = noBadge ? false : !_officialOnly && Logic::GetLastseenContainer()->isOnline(_aimid);

        QPixmap statusBadge;
        if (_state != StatusBadgeState::BadgeOnly)
        {
            statusBadge = _aimid.isEmpty() ? QPixmap() : getStatusBadge(_aimid, _avatar.width());
            if (statusBadge.isNull() && _state != StatusBadgeState::CanBeOff && _state != StatusBadgeState::StatusOnly)
                statusBadge = getEmptyStatusBadge(_state, _avatar.width());
        }

        drawAvatarWithBadge(_p, _topLeft, _avatar, isOfficial, statusBadge, isMuted, _isSelected, isOnline, _small_online);
    }

    void drawAvatarWithoutBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _avatar, const QPixmap& _status)
    {
        im_assert(!_avatar.isNull());

        drawAvatarWithBadge(_p, _topLeft, _avatar, false, _status, false, false, false, false);
    }

    QPixmap mirrorPixmap(const QPixmap& _pixmap, const MirrorDirection _direction)
    {
        const auto horScale = _direction & MirrorDirection::horizontal ? -1 : 1;
        const auto verScale = _direction & MirrorDirection::vertical ? -1 : 1;
        return _pixmap.transformed(QTransform().scale(horScale, verScale));
    }

    void drawUpdatePoint(QPainter &_p, const QPoint& _center, int _radius, int _borderRadius)
    {
        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);
        _p.setPen(Qt::NoPen);

        if (_borderRadius)
        {
            _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            _p.drawEllipse(_center, _borderRadius, _borderRadius);
        }

        _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        _p.drawEllipse(_center, _radius, _radius);
    }

    void restartApplication()
    {
        qApp->setProperty("restarting", true);
        qApp->quit();
    }

    bool isServiceLink(const QString& _url)
    {
        QUrl url(_url);
        if (url.isEmpty())
            return false;

        const QString host = url.host();
        const QString path = url.path();
        const auto pathView = QStringView(path);

        auto startWithServicePath = [](QStringView _path)
        {
            return !_path.isEmpty()
                && (_path.startsWith(url_commandpath_service_getlogs(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_getlogs_with_rtp(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_clear_avatars(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_clear_cache(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_update(), Qt::CaseInsensitive)
                    || _path.startsWith(url_commandpath_service_debug(), Qt::CaseInsensitive)
                    )
            ;
        };

        return isInternalScheme(url.scheme())
            && host.compare(url_command_service(), Qt::CaseInsensitive) == 0
            && pathView.startsWith(u'/')
            && startWithServicePath(pathView.mid(1));
    }

    void clearContentCache()
    {
        Ui::GetDispatcher()->post_message_to_core("remove_content_cache", nullptr, nullptr,
            [](core::icollection* _coll)
            {

                Ui::gui_coll_helper coll(_coll, false);
                auto last_error = coll.get_value_as_int("last_error");
                im_assert(!last_error);
                Q_UNUSED(last_error);
            }
        );
    }

    void clearAvatarsCache()
    {
        Ui::GetDispatcher()->post_message_to_core("clear_avatars", nullptr, nullptr,
            [](core::icollection* _coll)
            {

                Ui::gui_coll_helper coll(_coll, false);
                auto last_error = coll.get_value_as_int("last_error");
                im_assert(!last_error);
                Q_UNUSED(last_error);
            }
        );
    }

    void removeOmicronFile()
    {
        Ui::GetDispatcher()->post_message_to_core("remove_ominron_stg", nullptr, nullptr,
            [](core::icollection* _coll)
            {

                Ui::gui_coll_helper coll(_coll, false);
                auto last_error = coll.get_value_as_int("last_error");
                im_assert(!last_error);
                Q_UNUSED(last_error);
            }
        );
    }

    void checkForUpdates()
    {
        Ui::GetDispatcher()->post_message_to_core("update/check", nullptr);
    }

    PhoneValidator::PhoneValidator(QWidget* _parent, bool _isCode, bool _isParseFullNumber)
        : QValidator(_parent)
        , isCode_(_isCode)
        , isParseFullNumber_(_isParseFullNumber)
    {
    }

    QValidator::State PhoneValidator::validate(QString& s, int&) const
    {
        QString code;
        QString number;
        if (isParseFullNumber_ && PhoneFormatter::parse(s, code, number))
        {
            Q_EMIT Utils::InterConnector::instance().fullPhoneNumber(code, number);
            return QValidator::Invalid;
        }

        static const QRegularExpression reg(qsl("[\\s\\-\\(\\)]*"), QRegularExpression::UseUnicodePropertiesOption);
        s.remove(reg);
        if (!isCode_)
            s.remove(u'+');

        auto i = 0;
        for (const auto& ch : std::as_const(s))
        {
            if (!ch.isDigit() && !(isCode_ && i == 0 && ch == u'+'))
                return QValidator::Invalid;

            ++i;
        }

        return QValidator::Acceptable;
    }

    DeleteMessagesWidget::DeleteMessagesWidget(QWidget* _parent, bool _showCheckBox, ShowInfoText _showInfoText)
        : QWidget(_parent)
        , showCheckBox_(_showCheckBox)
        , showInfoText_(_showInfoText)
    {
        checkbox_ = new Ui::CheckBox(this);

        checkbox_->move(Utils::scale_value(delete_messages_hor_offset), Utils::scale_value(delete_messages_top_offset));

        checkbox_->setVisible(showCheckBox_);
    }

    void DeleteMessagesWidget::setCheckBoxText(const QString& _text)
    {
        checkbox_->setText(_text);
    }

    void DeleteMessagesWidget::setCheckBoxChecked(bool _value)
    {
        checkbox_->setChecked(_value);
    }

    void DeleteMessagesWidget::setInfoText(const QString& _text)
    {
        label_ = Ui::TextRendering::MakeTextUnit(_text);
        label_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    }

    void DeleteMessagesWidget::paintEvent(QPaintEvent* _event)
    {
        if (!showCheckBox_ && showInfoText_ == ShowInfoText::Yes && label_)
        {
            QPainter p(this);
            label_->setOffsets(Utils::scale_value(delete_messages_hor_offset), Utils::scale_value(delete_messages_top_offset));
            label_->draw(p);
        }

        QWidget::paintEvent(_event);
    }

    bool DeleteMessagesWidget::isChecked() const
    {
        return showCheckBox_ && checkbox_->isChecked();
    }

    void DeleteMessagesWidget::resizeEvent(QResizeEvent* _event)
    {
        const auto maxWidth = _event->size().width() - Utils::scale_value(delete_messages_hor_offset * 2);
        checkbox_->setFixedWidth(maxWidth);
        const auto showInfoText = showInfoText_ == ShowInfoText::Yes;
        auto height = Utils::scale_value(showInfoText ? (delete_messages_top_offset + delete_messages_bottom_offset) : delete_messages_offset_short);
        if (showCheckBox_)
            height += checkbox_->height();
        else if (showInfoText && label_)
            height += label_->getHeight(maxWidth);

        setFixedHeight(height);
        QWidget::resizeEvent(_event);
    }

    CheckableInfoWidget::CheckableInfoWidget(QWidget* _parent)
        : QWidget(_parent)
        , checkbox_(new Ui::CheckBox(this))
        , hovered_(false)
    {
        Testing::setAccessibleName(checkbox_, qsl("AS General checkBox"));
        setMouseTracking(true);
    }

    bool CheckableInfoWidget::isChecked() const
    {
        return checkbox_->isChecked();
    }

    void CheckableInfoWidget::setCheckBoxText(const QString& _text)
    {
        checkbox_->setText(_text);
    }

    void CheckableInfoWidget::setCheckBoxChecked(bool _value)
    {
        checkbox_->setChecked(_value);
    }

    void CheckableInfoWidget::setInfoText(const QString& _text, QColor _color)
    {
        label_ = Ui::TextRendering::MakeTextUnit(_text, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);       
        label_->init(Fonts::appFontScaled(15), _color.isValid() ? _color : Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    void CheckableInfoWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.fillRect(checkboxRect_, Styling::getParameters().getColor(hovered_ ? Styling::StyleVariable::BASE_BRIGHT_INVERSE : Styling::StyleVariable::BASE_GLOBALWHITE));
        label_->setOffsets(Utils::scale_value(delete_messages_hor_offset), Utils::scale_value(delete_messages_top_offset));
        label_->draw(p);

        QWidget::paintEvent(_event);
    }

    void CheckableInfoWidget::enterEvent(QEvent* _event)
    {
        hovered_ = checkboxRect_.contains(QCursor::pos());
        update();
        QWidget::enterEvent(_event);
    }

    void CheckableInfoWidget::leaveEvent(QEvent* _event)
    {
        hovered_ = false;
        update();
        QWidget::leaveEvent(_event);
    }

    void CheckableInfoWidget::mouseMoveEvent(QMouseEvent* _event)
    {
        hovered_ = checkboxRect_.contains(_event->pos());
        update();
        QWidget::mouseMoveEvent(_event);
    }

    void CheckableInfoWidget::resizeEvent(QResizeEvent* _event)
    {
        const auto maxWidth = _event->size().width() - Utils::scale_value(delete_messages_hor_offset * 2);
        checkbox_->setFixedWidth(maxWidth);
        const auto labelHeight = label_->getHeight(maxWidth);
        setFixedHeight(labelHeight + Utils::scale_value(delete_messages_top_offset + messages_bottom_offset) + checkbox_->height());

        checkbox_->move(Utils::scale_value(delete_messages_hor_offset), Utils::scale_value(delete_messages_top_offset * 2) + labelHeight);
        checkboxRect_ = QRect(0, Utils::scale_value(delete_messages_top_offset * 2) + labelHeight, width(), checkbox_->height());

        QWidget::resizeEvent(_event);
    }

    QString formatFileSize(const int64_t size)
    {
        im_assert(size >= 0);

        constexpr auto KiB = 1024;
        constexpr auto MiB = 1024 * KiB;
        constexpr auto GiB = 1024 * MiB;

        if (size >= GiB)
        {
            const auto gibSize = ((double)size / (double)GiB);

            return qsl("%1 GB").arg(gibSize, 0, 'f', 1);
        }

        if (size >= MiB)
        {
            const auto mibSize = ((double)size / (double)MiB);

            return qsl("%1 MB").arg(mibSize, 0, 'f', 1);
        }

        if (size >= KiB)
        {
            const auto kibSize = ((double)size / (double)KiB);

            return qsl("%1 KB").arg(kibSize, 0, 'f', 1);
        }

        return qsl("%1 B").arg(size);
    }

    void SetProxyStyle(QWidget* _widget, QStyle* _style)
    {
        im_assert(_widget);
        if (_widget)
        {
            _style->setParent(_widget);
            _widget->setStyle(_style);
        }
    }

    void logMessage(const QString &_message)
    {
        static QString logPath;
        static const QString folderName = qsl("gui_debug");
        static const QString fileName = qsl("logs.txt");
        if (logPath.isEmpty())
        {
            auto loc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir folder(loc);
            folder.mkdir(folderName);
            folder.cd(folderName);
            logPath = folder.filePath(fileName);
        }

        QFile f(logPath);
        f.open(QIODevice::Append);
        f.write("\n");
        f.write(QDateTime::currentDateTime().toString().toUtf8());
        f.write("\n");
        f.write(_message.toUtf8());
        f.write("\n");
        f.flush();
    }

#ifdef __linux__

    bool runCommand(const QByteArray &command)
    {
        auto result = system(command.constData());
        if (result)
        {
            qCDebug(linuxRunCommand) << QString(qsl("Command failed, code: %1, command (in utf8): %2")).arg(result).arg(QString::fromLatin1(command.constData()));
            return false;
        }
        else
        {
            qCDebug(linuxRunCommand) << QString(qsl("Command succeeded, command (in utf8): %2")).arg(QString::fromLatin1(command.constData()));
            return true;
        }
    }

    QByteArray escapeShell(const QByteArray &content)
    {
        auto result = QByteArray();

        auto b = content.constData(), e = content.constEnd();
        for (auto ch = b; ch != e; ++ch)
        {
            if (*ch == ' ' || *ch == '"' || *ch == '\'' || *ch == '\\')
            {
                if (result.isEmpty())
                    result.reserve(content.size() * 2);
                if (ch > b)
                    result.append(b, ch - b);

                result.append('\\');
                b = ch;
            }
        }
        if (result.isEmpty())
            return content;

        if (e > b)
            result.append(b, e - b);

        return result;
    }

#endif

    void registerCustomScheme()
    {
#ifdef __linux__
        const QString home = QDir::homePath() % u'/';
        const auto executableInfo = QFileInfo(QCoreApplication::applicationFilePath());
        const QString executableName = executableInfo.fileName();
        const QString executableDir = executableInfo.absoluteDir().absolutePath() % u'/';

        const QByteArray urlSchemeName = urlSchemes().front().toUtf8();

        if (QDir(home % u".local/").exists())
        {
            const QString apps = home % u".local/share/applications/";
            const QString icons = home % u".local/share/icons/";

            if (!QDir(apps).exists())
                QDir().mkpath(apps);

            if (!QDir(icons).exists())
                QDir().mkpath(icons);

            const QString desktopFileName =  executableName % u"desktop.desktop";

            const auto tempPath = QDir::tempPath();
            const QString tempFile = tempPath % u'/' % desktopFileName;
            QDir().mkpath(tempPath);
            QFile f(tempFile);
            if (f.open(QIODevice::WriteOnly))
            {
                const QString icon = icons % executableName % u".png";

                QFile(icon).remove();

                const auto iconName = qsl(":/logo/logo");
                const auto image = Utils::renderSvg(iconName, (QSize(256, 256))).toImage();
                image.save(icon);

                QFile(iconName).copy(icon);

                const auto appName = getAppTitle();

                QTextStream s(&f);
                s.setCodec("UTF-8");
                s << "[Desktop Entry]\n";
                s << "Version=1.0\n";
                s << "Name=" << appName << '\n';
                s << "Comment=" << QT_TRANSLATE_NOOP("linux_desktop_file", "Official desktop application for the %1 messaging service").arg(appName) << '\n';
                s << "TryExec=" << escapeShell(QFile::encodeName(executableDir + executableName)) << '\n';
                s << "Exec=" << escapeShell(QFile::encodeName(executableDir + executableName)) << " -urlcommand %u\n";
                s << "Icon=" << escapeShell(QFile::encodeName(icon)) << '\n';
                s << "Terminal=false\n";
                s << "Type=Application\n";
                s << "StartupWMClass=" << executableName << '\n';
                s << "Categories=Network;InstantMessaging;Qt;\n";
                s << "MimeType=x-scheme-handler/" << urlSchemeName << ";\n";
                f.close();

                if (runCommand("desktop-file-install --dir=" % escapeShell(QFile::encodeName(home % u".local/share/applications")) % " --delete-original " % escapeShell(QFile::encodeName(tempFile))))
                {
                    runCommand("update-desktop-database " % escapeShell(QFile::encodeName(home % u".local/share/applications")));
                    runCommand("xdg-mime default " % desktopFileName.toLatin1() % " x-scheme-handler/" % urlSchemeName);
                }
                else // desktop-file-utils not installed, copy by hands
                {
                    if (QFile(tempFile).copy(home % u".local/share/applications/" % desktopFileName))
                        runCommand("xdg-mime default " % desktopFileName.toLatin1() % " x-scheme-handler/" % urlSchemeName);
                    QFile(tempFile).remove();
                }
            }
        }

        if (runCommand("gconftool-2 -t string -s /desktop/gnome/url-handlers/" % urlSchemeName % "/command " % escapeShell(escapeShell(QFile::encodeName(executableDir + executableName)) % " -urlcommand %s")))
        {
            runCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" % urlSchemeName % "/needs_terminal false");
            runCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" % urlSchemeName % "/enabled true");
        }

        QString services;
        if (QDir(home % u".kde4/").exists())
        {
            services = home % u".kde4/share/kde4/services/";
        }
        else if (QDir(home % u".kde/").exists())
        {
            services = home % u".kde/share/kde4/services/";
        }

        if (!services.isEmpty())
        {
            if (!QDir(services).exists())
                QDir().mkpath(services);

            const auto path = services;
            const QString file = path % executableName % u".protocol";
            QFile f(file);
            if (f.open(QIODevice::WriteOnly))
            {
                QTextStream s(&f);
                s.setCodec("UTF-8");
                s << "[Protocol]\n";
                s << "exec=" << QFile::decodeName(escapeShell(QFile::encodeName(executableDir + executableName))) << " -urlcommand %u\n";
                s << "protocol=" << urlSchemeName << '\n';
                s << "input=none\n";
                s << "output=none\n";
                s << "helper=true\n";
                s << "listing=false\n";
                s << "reading=false\n";
                s << "writing=false\n";
                s << "makedir=false\n";
                s << "deleting=false\n";
                f.close();
            }
        }

#endif

    }

    void openFileLocation(const QString& _path)
    {
#ifdef _WIN32
        const QString command = u"explorer /select," % QDir::toNativeSeparators(_path);
        QProcess::startDetached(command);
#else
#ifdef __APPLE__
        MacSupport::openFinder(_path);
#else
        QDir dir(_path);
        dir.cdUp();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
#endif // __APPLE__
#endif //_WIN32
    }

    bool isPanoramic(const QSize& _size)
    {
        if (_size.height() == 0)
            return false;

        const auto ratio = (double)_size.width() / _size.height();
        constexpr double oversizeCoeff = 2.0;
        return ratio >= oversizeCoeff || ratio <= 1. / oversizeCoeff;
    }

    bool isMimeDataWithImageDataURI(const QMimeData* _mimeData)
    {
        return _mimeData->text().startsWith(u"data:image/");
    }

    bool isMimeDataWithImage(const QMimeData* _mimeData)
    {
        return _mimeData->hasImage() || isMimeDataWithImageDataURI(_mimeData);
    }

    QImage getImageFromMimeData(const QMimeData* _mimeData)
    {
        QImage result;

        if (_mimeData->hasImage())
        {
            result = qvariant_cast<QImage>(_mimeData->imageData());
        }
        else if (isMimeDataWithImageDataURI(_mimeData))
        {
            auto plainText = _mimeData->text();

            plainText.remove(0, plainText.indexOf(u',') + 1);
            auto imageData = QByteArray::fromBase64(std::move(plainText).toUtf8());

            result = QImage::fromData(imageData);
        }

        return result;
    }

    bool startsCyrillic(const QString& _str)
    {
        return !_str.isEmpty() && _str[0] >= 0x0400 && _str[0] <= 0x04ff;
    }

    bool startsNotLetter(const QString& _str)
    {
        return !_str.isEmpty() && !_str[0].isLetter();
    }

    QPixmap tintImage(const QPixmap& _source, const QColor& _tint)
    {
        QPixmap res;
        if (_tint.isValid() && _tint.alpha())
        {
            constexpr auto fullyOpaque = 255;
            im_assert(_tint.alpha() > 0 && _tint.alpha() < fullyOpaque);

            QPixmap pm(_source.size());
            QPainter p(&pm);

            if (_tint.alpha() != fullyOpaque)
                p.drawPixmap(QPoint(), _source);

            p.fillRect(pm.rect(), _tint);
            res = std::move(pm);
        }
        else
        {
            res = _source;
        }

        Utils::check_pixel_ratio(res);
        return res;
    }

    bool isChat(QStringView _aimid)
    {
        return _aimid.endsWith(u"@chat.agent") || _aimid == getDefaultCallAvatarId();
    }

    bool isServiceAimId(QStringView _aimId)
    {
        return _aimId.startsWith(u'~') && _aimId.endsWith(u'~');
    }

    QString getDomainUrl()
    {
        const bool useProfileAgent = config::get().is_on(config::features::profile_agent_as_domain_url);
        return useProfileAgent ? Features::getProfileDomainAgent() : Features::getProfileDomain();
    }

    QString getStickerBotAimId()
    {
        return qsl("100500");
    }

    QString msgIdLogStr(qint64 _msgId)
    {
        return qsl("msg id: %1").arg(_msgId);
    }

    bool isUrlVCS(const QString& _url)
    {
        const auto& vcs_urls = Ui::getUrlConfig().getVCSUrls();
        return std::any_of(vcs_urls.begin(), vcs_urls.end(), [&_url](const auto& u) { return _url.startsWith(u); });
    }

    bool canShowSmartreplies(const QString& _aimId)
    {
        if (!Features::isSmartreplyEnabled())
            return false;

        if (Utils::InterConnector::instance().isMultiselect(_aimId))
            return false;

        if (Logic::getIgnoreModel()->contains(_aimId) || (Utils::isChat(_aimId) && (!Logic::getContactListModel()->contains(_aimId) || Logic::getContactListModel()->isReadonly(_aimId))))
            return false;

        return true;
    }

    QString getDefaultCallAvatarId()
    {
        return qsl("~default_call_avatar_id~");
    }

    QString getReadOnlyString(ROAction _action, bool _isChannel)
    {
        if (_isChannel)
        {
            if (_action == ROAction::Ban)
                return QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to ban user to write in this channel?");
            else
                return QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to allow user to write in this channel?");
        }
        else
        {
            if (_action == ROAction::Ban)
                return QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to ban user to write in this chat?");
            else
                return QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to allow user to write in this chat?");
        }
    }

    QString getMembersString(int _number, bool _isChannel)
    {
        if (_isChannel)
        {
            return Utils::GetTranslator()->getNumberString(_number, QT_TRANSLATE_NOOP3("contactlist", "%1 subscriber", "1"),
                                                                    QT_TRANSLATE_NOOP3("contactlist", "%1 subscribers", "2"),
                                                                    QT_TRANSLATE_NOOP3("contactlist", "%1 subscribers", "5"),
                                                                    QT_TRANSLATE_NOOP3("contactlist", "%1 subscribers", "21")).arg(_number);
        }

        return Utils::GetTranslator()->getNumberString(_number, QT_TRANSLATE_NOOP3("contactlist", "%1 member", "1"),
                                                                QT_TRANSLATE_NOOP3("contactlist", "%1 members", "2"),
                                                                QT_TRANSLATE_NOOP3("contactlist", "%1 members", "5"),
                                                                QT_TRANSLATE_NOOP3("contactlist", "%1 members", "21")).arg(_number);
    }

    QString getFormattedTime(std::chrono::milliseconds _duration)
    {
        const auto h = std::chrono::duration_cast<std::chrono::hours>(_duration);
        const auto m = std::chrono::duration_cast<std::chrono::minutes>(_duration) - h;
        const auto s = std::chrono::duration_cast<std::chrono::seconds>(_duration) - h - m;
        const auto ms = _duration - h - m - s;

        QString res;
        QTextStream stream(&res);
        stream.setPadChar(ql1c('0'));
        stream
            << qSetFieldWidth(2) << m.count()
            << qSetFieldWidth(0) << ql1c(':')
            << qSetFieldWidth(2) << s.count()
            << qSetFieldWidth(0) << ql1c(',')
            << qSetFieldWidth(2) << (ms / 10).count();

        return res;
    }

    CallLinkCreator::CallLinkCreator(CallLinkFrom _from)
        : seqRequestWithLoader_(-1)
        , from_(_from)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayCancelled, this, [this]() { seqRequestWithLoader_ = -1; });
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::createConferenceResult, this, &CallLinkCreator::onCreateCallLinkResult);
    }

    void CallLinkCreator::createCallLink(Ui::ConferenceType _type, const QString& _aimId)
    {
        aimId_ = _aimId;
        seqRequestWithLoader_ = Ui::GetDispatcher()->createConference(_type);
        QTimer::singleShot(getLoaderOverlayDelay(), this, [this, seq = seqRequestWithLoader_]()
        {
            if (seqRequestWithLoader_ == seq)
                    Q_EMIT Utils::InterConnector::instance().loaderOverlayShow();
        });
    }

    void CallLinkCreator::onCreateCallLinkResult(int64_t _seq, int _error, bool _isWebinar, const QString& _url, int64_t _expires)
    {
        if (seqRequestWithLoader_ != _seq)
            return;

        seqRequestWithLoader_ = -1;
        Q_EMIT Utils::InterConnector::instance().loaderOverlayHide();

        if (_error)
        {
            const auto text = _isWebinar ? QT_TRANSLATE_NOOP("input_widget", "Error creating webinar") : QT_TRANSLATE_NOOP("input_widget", "Error creating call link");
            Utils::showTextToastOverContactDialog(text);
            return;
        }
        Ui::CallLinkWidget wid(nullptr, from_, _url, _isWebinar ? Ui::ConferenceType::Webinar : Ui::ConferenceType::Call, aimId_);
        if (wid.show())
        {
            if (from_ == CallLinkFrom::CallLog)
            {
                Ui::forwardMessage(_url, QT_TRANSLATE_NOOP("popup_window", "Send link"), QString(), false);
            }
            else
            {
                Ui::GetDispatcher()->sendMessageToContact(aimId_, _url);
                if (!Logic::getContactListModel()->isThread(aimId_))
                    Utils::openDialogWithContact(aimId_);
            }
        }
    }
}

void Logic::updatePlaceholders(const std::vector<Placeholder>& _locations)
{
    const auto recentsModel = Logic::getRecentsModel();
    const auto contactListModel = Logic::getContactListModel();
    const auto unknownsModel = Logic::getUnknownsModel();

    for (auto & location : _locations)
    {
        if (location == Placeholder::Contacts)
        {
            const auto needContactListPlaceholder = contactListModel->contactsCount() == 0;
            if (needContactListPlaceholder)
                Q_EMIT Utils::InterConnector::instance().showContactListPlaceholder();
            else
                Q_EMIT Utils::InterConnector::instance().hideContactListPlaceholder();
        }
        else if (location == Placeholder::Recents)
        {
            const auto needRecentsPlaceholder = recentsModel->dialogsCount() == 0 && unknownsModel->itemsCount() == 0;
            if (needRecentsPlaceholder)
                Q_EMIT Utils::InterConnector::instance().showRecentsPlaceholder();
            else
                Q_EMIT Utils::InterConnector::instance().hideRecentsPlaceholder();
        }
    }
}

//! Append links to the map
static void extractFilesharingLinks(const QString& _text, Data::FilesPlaceholderMap& _files)
{
    const auto& placeholders = MimeData::getFilesPlaceholderList();
    const auto filesHost = QString(u"https://" % Ui::getUrlConfig().getUrlFilesParser() % u'/');
    int idx = 0;
    while (idx != -1)
    {
        const auto next = _text.midRef(idx).indexOf(u'[');
        if (next == -1)
            break;

        idx += next;
        const auto idxSpace = _text.midRef(idx).indexOf(QChar::Space);
        if (idxSpace == -1)
            break;

        const auto idxEnd = _text.midRef(idx + idxSpace).indexOf(u']');
        if (idxEnd == -1)
            break;

        if (std::any_of(placeholders.begin(), placeholders.end(), [tmpText = QStringView(_text).mid(idx, idxSpace + 1)](auto ph){ return ph == tmpText; }))
        {
            auto id = QStringView(_text).mid(idx + idxSpace + 1, idxEnd - 1);
            const auto hasSpaces = std::any_of(id.begin(), id.end(), [](const auto &c){return c.isSpace();});
            if (!hasSpaces && !id.isEmpty())
            {
                if (id.size() == Features::getFsIDLength())
                {
                    auto fileText = QStringView(_text).mid(idx, idxSpace + idxEnd + 1).toString();
                    _files[std::move(fileText)] = filesHost % id;
                }
            }
            idx += idxSpace + idxEnd;
        }
        else
        {
            ++idx;
        }
    }
}

void Utils::replaceFilesPlaceholders(Data::FString& _text, const Data::FilesPlaceholderMap& _files)
{
    if (!_text.hasFormatting())
        _text = replaceFilesPlaceholders(_text.string(), _files);

    Data::FilesPlaceholderMap allFiles;
    for (const auto& [link, ph] : _files)
        allFiles[ph] = link;
    extractFilesharingLinks(_text.string(), allFiles);
    convertFilesPlaceholders(_text, allFiles);
}

QString Utils::replaceFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files)
{
    Data::FilesPlaceholderMap allFiles;
    for (const auto& [link, ph] : _files)
        allFiles[ph] = link;
    extractFilesharingLinks(_text, allFiles);

    for (const auto& [ph, link] : allFiles)
    {
        if (link.isEmpty() || ph.isEmpty())
            continue;

        auto idxPl = _text.indexOf(ph);
        if (idxPl == -1)
            continue;

        while (idxPl != -1 && idxPl < _text.size())
        {
            if (idxPl > 0 && !_text.at(idxPl - 1).isSpace())
            {
                _text.insert(idxPl, QChar::LineFeed);
                ++idxPl;
            }

            _text.replace(idxPl, ph.size(), link);
            if (const auto pos = idxPl + link.size(); pos < _text.size() && !_text.at(pos).isSpace())
                _text.insert(pos, QChar::LineFeed);
            idxPl = _text.indexOf(ph);
        }
    }

    return _text;
}


namespace MimeData
{
    QByteArray convertMapToArray(const std::map<QString, QString, Utils::StringComparator>& _map)
    {
        QByteArray res;
        if (!_map.empty())
        {
            res.reserve(512);
            for (const auto& [key, value] : _map)
            {
                res += key.toUtf8();
                res += ": ";
                res += value.toUtf8();
                res += '\0';
            }
            res.chop(1);
        }

        return res;
    }

    std::map<QString, QString, Utils::StringComparator> convertArrayToMap(const QByteArray& _array)
    {
        std::map<QString, QString, Utils::StringComparator> res;

        if (!_array.isEmpty())
        {
            if (const auto items = _array.split('\0'); !items.isEmpty())
            {
                for (const auto& item : items)
                {
                    const auto ndx = item.indexOf(": ");
                    if (ndx != -1)
                    {
                        auto key = QString::fromUtf8(item.mid(0, ndx));
                        auto value = QString::fromUtf8(item.mid(ndx + 1).trimmed());

                        res.emplace(std::move(key), std::move(value));
                    }
                }
            }
        }

        return res;
    }

    void copyMimeData(const Ui::MessagesScrollArea& _area)
    {
        if (auto text = _area.getSelectedText(Ui::MessagesScrollArea::TextFormat::Raw); !text.isEmpty())
        {
            const auto placeholders = _area.getFilesPlaceholders();
            const auto mentions = _area.getMentions();

            QApplication::clipboard()->setMimeData(toMimeData(std::move(text), mentions, placeholders));
            Utils::showCopiedToast();
        }
    }

    QByteArray serializeTextFormatAsJson(const core::data::format& _format)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        auto value = _format.serialize(a);
        doc.AddMember("format", value, a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return QByteArray(buffer.GetString(), buffer.GetSize());
    }

    const std::vector<QStringView>& getFilesPlaceholderList()
    {
        static const std::vector<QStringView> placeholders =
        {
            u"[GIF: ",
            u"[Video: ",
            u"[Photo: ",
            u"[Voice: ",
            u"[Sticker: ",
            u"[Audio: ",
            u"[Word: ",
            u"[Excel: ",
            u"[Presentation: ",
            u"[APK: ",
            u"[PDF: ",
            u"[Numbers: ",
            u"[Pages: ",
            u"[File: "
        };
        return placeholders;
    }

    QMimeData* toMimeData(Data::FString&& _text, const Data::MentionMap& _mentions, const Data::FilesPlaceholderMap& _files)
    {
        auto dt = new QMimeData();

        {
            auto plainText = Utils::convertMentions(_text.string(), _mentions);
            plainText = Utils::convertFilesPlaceholders(plainText, _files);
            dt->setText(plainText);
        }

        dt->setData(getRawMimeType(), _text.string().toUtf8());
        if (_text.hasFormatting())
            dt->setData(getTextFormatMimeType(), MimeData::serializeTextFormatAsJson(_text.formatting()));

        if (!_mentions.empty())
            dt->setData(getMentionMimeType(), MimeData::convertMapToArray(_mentions));

        if (!_files.empty())
            dt->setData(MimeData::getFileMimeType(), MimeData::convertMapToArray(_files));

        return dt;
    }

    QString getMentionMimeType()
    {
        return qsl("icq/mentions");
    }

    QString getRawMimeType()
    {
        return qsl("icq/rawText");
    }

    QString getFileMimeType()
    {
        return qsl("icq/files");
    }

    QString getTextFormatMimeType()
    {
        return qsl("icq/textFormat");
    }
}

namespace FileSharing
{
    FileType getFSType(const QString & _filename)
    {
        static const std::map<QStringView, FileType, Utils::StringComparator> knownTypes
        {
            { u"zip", FileType::archive },
            { u"rar", FileType::archive },
            { u"tar", FileType::archive },
            { u"xz",  FileType::archive },
            { u"gz",  FileType::archive },
            { u"7z",  FileType::archive },
            { u"bz2",  FileType::archive },

            { u"xls",  FileType::xls },
            { u"xlsx", FileType::xls },
            { u"xlsm", FileType::xls },
            { u"csv",  FileType::xls },
            { u"xltx",  FileType::xls },
            { u"ods",  FileType::xls },

            { u"html", FileType::html },
            { u"htm",  FileType::html },

            { u"key",  FileType::keynote },

            { u"numbers",  FileType::numbers },

            { u"pages",  FileType::pages },

            { u"pdf",  FileType::pdf },

            { u"ppt",  FileType::ppt },
            { u"pptx", FileType::ppt },
            { u"odp", FileType::ppt },

            { u"wav",  FileType::audio },
            { u"mp3",  FileType::audio },
            { u"ogg",  FileType::audio },
            { u"flac", FileType::audio },
            { u"aac",  FileType::audio },
            { u"m4a",  FileType::audio },
            { u"aiff", FileType::audio },
            { u"ape",  FileType::audio },
            { u"midi",  FileType::audio },

            { u"log", FileType::txt },
            { u"txt", FileType::txt },
            { u"md", FileType::txt },

            { u"doc",  FileType::doc },
            { u"docx", FileType::doc },
            { u"rtf",  FileType::doc },
            { u"odf",  FileType::doc },

            { u"apk",  FileType::apk },
        };

        if (const auto ext = QFileInfo(_filename).suffix(); !ext.isEmpty())
        {
            if (const auto it = knownTypes.find(ext); it != knownTypes.end())
                return it->second;
        }

        return FileType::unknown;
    }

    using FileTypesArray = std::array<std::pair<FileType, QString>, static_cast<size_t>(FileType::max_val)>;
    static FileTypesArray getTypes()
    {
        FileTypesArray types =
        {
            std::pair(FileType::audio,   qsl("Audio")),
            std::pair(FileType::doc,     qsl("Word")),
            std::pair(FileType::xls,     qsl("Excel")),
            std::pair(FileType::ppt,     qsl("Presentation")),
            std::pair(FileType::apk,     qsl("APK")),
            std::pair(FileType::pdf,     qsl("PDF")),
            std::pair(FileType::numbers, qsl("Numbers")),
            std::pair(FileType::pages,   qsl("Pages")),

            std::pair(FileType::archive, qsl("File")),
            std::pair(FileType::html,    qsl("File")),
            std::pair(FileType::keynote, qsl("File")),
            std::pair(FileType::txt,     qsl("File")),

            std::pair(FileType::unknown, qsl("File")),
        };

        if (!std::is_sorted(std::cbegin(types), std::cend(types), Utils::is_less_by_first<FileType, QString>))
            std::sort(std::begin(types), std::end(types), Utils::is_less_by_first<FileType, QString>);

        return types;
    }

    QString getPlaceholderForType(FileType _fileType)
    {
        static const auto types = getTypes();
        return types[static_cast<size_t>(_fileType)].second;
    }
}

