#ifndef __IM_LOGIN_H__
#define __IM_LOGIN_H__

#pragma once

namespace core
{
    const int32_t default_im_id = 1;

    enum class login_type
    {
        lt_login_password,
        lt_phone
    };


    class im_login_id
    {
        int32_t id_;
        std::string login_;

    public:

        im_login_id(const std::string& _login, int32_t _id = -1);

        int32_t get_id() const;
        void set_id(int32_t _id);

        const std::string& get_login() const;
    };

    class im_login_list
    {
        std::list<im_login_id> logins_;

        const std::wstring file_name_;

    public:

        im_login_list(const std::wstring& _file_name);

        int32_t get_next_id() const;
        bool update(/*in, out*/ im_login_id& _login);
        bool get_first_login(/*out*/ im_login_id& _login);

        bool load();
        void save() const;
        void replace_uin(im_login_id& old_login, im_login_id& new_login);

        bool empty() const;
    };
}

#endif //__IM_LOGIN_H__
