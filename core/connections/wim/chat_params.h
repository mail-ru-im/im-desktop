#pragma once

#include "../../../corelib/collection_helper.h"

namespace core
{
    namespace wim
    {
        class chat_params final
        {
        private:
            std::optional<std::string> name_;
            std::optional<std::string> avatar_;
            std::optional<std::string> about_;
            std::optional<std::string> rules_;
            std::optional<bool> public_;
            std::optional<bool> approved_;
            std::optional<bool> joiningByLink_;
            std::optional<bool> readOnly_;

        public:
            chat_params();
            ~chat_params();

            chat_params(chat_params&&) = default;
            chat_params& operator=(chat_params&&) = default;

            void set_name(const std::string& _name);
            void set_avatar(const std::string& _avatar);
            void set_about(const std::string& _about);
            void set_rules(const std::string& _rules);
            void set_public(bool _public);
            void set_join(bool _approved);
            void set_joiningByLink(bool _joiningByLink);
            void set_readOnly(bool _readOnly);

            inline const std::optional<std::string> &get_name() const { return name_; }
            inline const std::optional<std::string> &get_avatar() const { return avatar_; }
            inline const std::optional<std::string> &get_about() const { return about_; }
            inline const std::optional<std::string> &get_rules() const { return rules_; }
            inline const std::optional<bool> &get_public() const { return public_; }
            inline const std::optional<bool> &get_join() const { return approved_; }
            inline const std::optional<bool> &get_joiningByLink() const { return joiningByLink_; }
            inline const std::optional<bool> &get_readOnly() const { return readOnly_; }

            static chat_params create(const core::coll_helper& _params);
        };

    }
}
