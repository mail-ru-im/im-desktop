#pragma once
#include <cstdint>
#include <string_view>

enum scheme_type : int16_t
{
    undefined = -1,
    https = 0,
    http,
    email,
    sftp,
    tftp,
    ftps,
    ftp,
    ssh,
    smb,
    vnc,
    rdp,
    icq,
    agent,
    biz,
    biz_old,
    dit,
    max_scheme_type
};

static constexpr size_t SCHEME_OFFSETS[] =
{
    5, // https
    4, // http
    6, // mailto
    4, // sftp
    4, // tftp
    4, // ftps
    3, // ftp
    3, // ssh
    3, // smb
    3, // vnc
    3, // rdp
    3, // icq
    6, // magent
    7, // vkteams
    16, // myteam-messenger
    13  // itd-messenger
};
static_assert (std::size(SCHEME_OFFSETS) == max_scheme_type, "mismatch number of offsets with schemes enumeration");

static constexpr std::pair<std::string_view, scheme_type>  SCHEMES[] =
{
    { "https://", scheme_type::https },
    { "http://",  scheme_type::http },
    { "mailto:",  scheme_type::email },
    { "sftp://",  scheme_type::sftp },
    { "tftp://",  scheme_type::tftp },
    { "ftps://",  scheme_type::ftps },
    { "ftp://",   scheme_type::ftp },
    { "ssh://",   scheme_type::ssh },
    { "smb://",   scheme_type::smb },
    { "vnc://",   scheme_type::vnc },
    { "rdp://",   scheme_type::rdp },
    { "icq://",   scheme_type::icq },
    { "magent://", scheme_type::agent },
    { "vkteams://", scheme_type::biz },
    { "myteam-messenger://", scheme_type::biz_old },
    { "itd-messenger://", scheme_type::dit }
};
static_assert (std::size(SCHEMES) == max_scheme_type, "mismatch number of string representations of schemes enumeration");


static constexpr std::pair<std::u16string_view, scheme_type>  U16SCHEMES[] =
{
    { u"https://", scheme_type::https },
    { u"http://",  scheme_type::http },
    { u"mailto:",  scheme_type::email },
    { u"sftp://",  scheme_type::sftp },
    { u"tftp://",  scheme_type::tftp },
    { u"ftps://",  scheme_type::ftps },
    { u"ftp://",   scheme_type::ftp },
    { u"ssh://",   scheme_type::ssh },
    { u"smb://",   scheme_type::smb },
    { u"vnc://",   scheme_type::vnc },
    { u"rdp://",   scheme_type::rdp },
    { u"icq://",   scheme_type::icq },
    { u"magent://", scheme_type::agent },
    { u"vkteams://", scheme_type::biz },
    { u"myteam-messenger://", scheme_type::biz_old },
    { u"itd-messenger://", scheme_type::dit }
};

static_assert (std::size(U16SCHEMES) == max_scheme_type, "mismatch number of string representations of schemes enumeration");

static const std::pair<std::wstring_view, scheme_type> WSCHEMES[] =
{
    { L"https://", scheme_type::https },
    { L"http://",  scheme_type::http },
    { L"mailto:",  scheme_type::email },
    { L"sftp://",  scheme_type::sftp },
    { L"tftp://",  scheme_type::tftp },
    { L"ftps://",  scheme_type::ftps },
    { L"ftp://",   scheme_type::ftp },
    { L"ssh://",   scheme_type::ssh },
    { L"smb://",   scheme_type::smb },
    { L"vnc://",   scheme_type::vnc },
    { L"rdp://",   scheme_type::rdp },
    { L"icq://",   scheme_type::icq },
    { L"magent://", scheme_type::agent },
    { L"vkteams://", scheme_type::biz },
    { L"myteam-messenger://", scheme_type::biz_old },
    { L"itd-messenger://", scheme_type::dit }
};

static_assert (std::size(WSCHEMES) == max_scheme_type, "mismatch number of string representations of schemes enumeration");

