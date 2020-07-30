#include "stdafx.h"

#include "login_info.h"
#include "im_login.h"
#include "../corelib/enumerations.h"

using namespace core;

//////////////////////////////////////////////////////////////////////////
// login_info
//////////////////////////////////////////////////////////////////////////
login_info::login_info()
    : login_type_(login_type::lt_phone)
    , token_type_(token_type::basic)
    , save_auth_data_(false)
{
}


login_info::~login_info()
{
}

void core::login_info::set_password(const std::string& _password)
{
    password_ = _password;
}

const std::string& core::login_info::get_password() const
{
    return password_;
}

void core::login_info::set_login(const std::string& _login)
{
    login_ = _login;
}

const std::string& core::login_info::get_login() const
{
    return login_;
}

void core::login_info::set_login_type(login_type _type)
{
    login_type_ = _type;
}

core::login_type core::login_info::get_login_type() const
{
    return login_type_;
}

void login_info::set_token_type(token_type _type)
{
    token_type_ = _type;
}

token_type login_info::get_token_type() const
{
    return token_type_;
}

void core::login_info::set_save_auth_data(bool save)
{
    save_auth_data_ = save;
}

bool core::login_info::get_save_auth_data() const
{
    return save_auth_data_;
}



//////////////////////////////////////////////////////////////////////////
// phone_info
//////////////////////////////////////////////////////////////////////////
void phone_info::set_phone(const std::string& _phone)
{
    phone_ = _phone;
}

const std::string& phone_info::get_phone() const
{
    return phone_;
}

void phone_info::set_locale(const std::string& _locale)
{
    locale_ = _locale;
}

const std::string& phone_info::get_locale() const
{
    return locale_;
}

void phone_info::set_existing(bool _existing)
{
    existing_ = _existing;
}

bool phone_info::get_existing() const
{
    return existing_;
}

void phone_info::set_trans_id(const std::string& _trans_id)
{
    trans_id_ = _trans_id;
}

const std::string& phone_info::get_trans_id() const
{
    return trans_id_;
}

void phone_info::set_sms_code(const std::string& _sms_code)
{
    sms_code_ = _sms_code;
}

const std::string& phone_info::get_sms_code() const
{
    return sms_code_;
}
