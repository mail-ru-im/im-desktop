#include "stdafx.h"
#include "im_login.h"

namespace core
{
    im_login_id::im_login_id(const std::string& _login, int32_t _id)
        :   id_(_id),
            login_(_login)
    {
    }



    int32_t im_login_id::get_id() const
    {
        return id_;
    }

    void im_login_id::set_id(int32_t _id)
    {
        id_ = _id;
    }

    const std::string& im_login_id::get_login() const
    {
        return login_;
    }

    im_login_list::im_login_list(const std::wstring& _file_name)
        :   file_name_(_file_name)
    {

    }

    enum login_params
    {
        lp_id = 0,
        lp_login = 1
    };

    bool im_login_list::load()
    {
        tools::binary_stream bs_file;
        if (!bs_file.load_from_file(file_name_))
            return false;

        tools::tlvpack pack;
        if (!pack.unserialize(bs_file))
            return false;

        auto on_fail = [this]()
        {
            assert(false);
            logins_.clear();
        };

        for (auto tlv_login = pack.get_first(); tlv_login; tlv_login = pack.get_next())
        {
            tools::binary_stream login_data = tlv_login->get_value<tools::binary_stream>();

            tools::tlvpack pack_login;
            if (!pack_login.unserialize(login_data))
                return false;

            auto tlv_id = pack_login.get_item(login_params::lp_id);
            auto tlv_login_name = pack_login.get_item(login_params::lp_login);

            if (!tlv_id || !tlv_login_name)
            {
                on_fail();
                return false;
            }

            im_login_id login(tlv_login_name->get_value<std::string>());
            login.set_id(tlv_id->get_value<int32_t>());
            logins_.push_back(std::move(login));
        }

        return (!logins_.empty());
    }

    void im_login_list::save() const
    {
        tools::tlvpack pack;

        int32_t counter = 0;

        for (const auto& login : logins_)
        {
            tools::tlvpack tlv_login;
            tlv_login.push_child(tools::tlv(login_params::lp_id, login.get_id()));
            tlv_login.push_child(tools::tlv(login_params::lp_login, login.get_login()));

            tools::binary_stream bs_login;
            tlv_login.serialize(bs_login);
            pack.push_child(tools::tlv(++counter, bs_login));
        }

        tools::binary_stream bs;
        pack.serialize(bs);

        bs.save_2_file(file_name_);
    }

    int32_t im_login_list::get_next_id() const
    {
        if (logins_.empty())
            return default_im_id;

        int32_t max_id = -1;
        for (const auto& login : logins_)
        {
            if (login.get_id() > max_id)
                max_id = login.get_id();
        }

        return (++max_id);
    }

    bool im_login_list::update(/*in, out*/ im_login_id& _login)
    {
        bool found = false;

        for (auto iter = logins_.begin(); iter != logins_.end(); ++iter)
        {
            if (iter->get_login() == _login.get_login())
            {
                found = true;

                _login = *iter;

                if (iter == logins_.begin())
                    return true;

                logins_.erase(iter);
                break;
            }
        }

        if (!found)
            _login.set_id(get_next_id());

        logins_.push_front(_login);

        save();
        return found;
    }

    bool im_login_list::get_first_login(/*out*/ im_login_id& _login)
    {
        if (logins_.empty())
            return false;

        _login = *logins_.begin();

        return true;
    }

    void im_login_list::replace_uin(im_login_id& _old_login, im_login_id& _new_login)
    {
        assert(logins_.begin()->get_login() == _old_login.get_login());

        if (logins_.begin()->get_login() == _old_login.get_login())
        {
            _new_login.set_id(_old_login.get_id());
            logins_.front() = _new_login;
            save();
        }
    }

    bool im_login_list::empty() const
    {
        return logins_.empty();
    }
}
