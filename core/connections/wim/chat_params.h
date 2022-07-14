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
            std::optional<std::string> stamp_;
            std::optional<bool> public_;
            std::optional<bool> approved_;
            std::optional<bool> joiningByLink_;
            std::optional<bool> readOnly_;
            std::optional<bool> isChannel_;
            std::optional<bool> trust_required_;
            std::optional<bool> threads_enabled_;
            std::optional<bool> threads_auto_subscribe_;

        public:
            void set_name(const std::string& _name);
            void set_avatar(const std::string& _avatar);
            void set_about(const std::string& _about);
            void set_rules(const std::string& _rules);
            void set_stamp(const std::string& _stamp);
            void set_public(bool _public);
            void set_join(bool _approved);
            void set_joiningByLink(bool _joiningByLink);
            void set_readOnly(bool _readOnly);
            void set_isChannel(bool _isChannel);
            void set_trust_required(bool _trust_required);
            void set_threads_enabled(bool _threads_enabled);
            void set_threads_auto_subscribe(bool _threads_auto_subscribe);

            inline const std::optional<std::string>& get_name() const { return name_; }
            inline const std::optional<std::string>& get_avatar() const { return avatar_; }
            inline const std::optional<std::string>& get_about() const { return about_; }
            inline const std::optional<std::string>& get_rules() const { return rules_; }
            inline const std::optional<std::string>& get_stamp() const { return stamp_; }
            inline const std::optional<bool>& get_public() const { return public_; }
            inline const std::optional<bool>& get_join() const { return approved_; }
            inline const std::optional<bool>& get_joiningByLink() const { return joiningByLink_; }
            inline const std::optional<bool>& get_readOnly() const { return readOnly_; }
            inline const std::optional<bool>& get_isChannel() const { return isChannel_; }
            inline const std::optional<bool>& get_trust_required() const { return trust_required_; }
            inline const std::optional<bool>& get_threads_enabled() const { return threads_enabled_; }
            inline const std::optional<bool>& get_threads_auto_subscribe() const { return threads_auto_subscribe_; }

            static chat_params create(const core::coll_helper& _params);
        };

    }
}
