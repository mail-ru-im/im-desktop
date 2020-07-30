#ifndef __LOGIN_INFO_H_
#define __LOGIN_INFO_H_

#pragma once


namespace core
{
    enum class login_type;

    enum class token_type;

    class login_info
    {
        std::string		login_;
        std::string		password_;
        login_type		login_type_;
        token_type      token_type_;
        bool            save_auth_data_;

    public:

        void set_password(const std::string& _password);
        const std::string& get_password() const;

        void set_login(const std::string& _login);
        const std::string& get_login() const;

        void set_login_type(login_type _type);
        login_type get_login_type() const;

        void set_token_type(token_type _type);
        token_type get_token_type() const;

        void set_save_auth_data(bool save);
        bool get_save_auth_data() const;

        login_info();
        virtual ~login_info();
    };

    class phone_info
    {
        std::string		phone_;
        std::string		trans_id_;
        bool			existing_;
        std::string		sms_code_;
        std::string     locale_;

    public:

        virtual ~phone_info() {}

        void set_phone(const std::string& _phone);
        const std::string& get_phone() const;

        void set_locale(const std::string& _locale);
        const std::string& get_locale() const;

        void set_existing(bool _existing);
        bool get_existing() const;

        void set_trans_id(const std::string&);
        const std::string& get_trans_id() const;

        void set_sms_code(const std::string&);
        const std::string& get_sms_code() const;
    };

}

#endif //__LOGIN_INFO_H_
