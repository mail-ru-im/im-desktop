#ifndef __WIM_ACTIVE_DILOGS
#define __WIM_ACTIVE_DILOGS

#pragma once

namespace core
{
    struct icollection;

    namespace wim
    {
        class active_dialog
        {
        public:
            active_dialog();
            active_dialog(std::string _aimid);
            active_dialog(active_dialog&&) = default;
            active_dialog(const active_dialog&) = default;
            active_dialog& operator=(const active_dialog&) = default;
            active_dialog& operator=(active_dialog&&) = default;

            const std::string& get_aimid() const { return aimid_; }

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void serialize(icollection* _coll) const;

        private:
            std::string aimid_;
        };

        class active_dialogs
        {
            bool	changed_;

            std::list<active_dialog>	dialogs_;

        public:

            void enumerate(const std::function<void(const active_dialog&)>& _callback);
            size_t size() const noexcept;

            bool is_changed() const noexcept { return changed_; }
            void set_changed(bool _changed) noexcept { changed_ = _changed; }

            active_dialogs();
            virtual ~active_dialogs();

            void update(active_dialog _dialog);
            void remove(const std::string& _aimid);
            bool contains(const std::string& _aimId) const;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void serialize(icollection* _coll) const;
        };

    }
}


#endif //__WIM_ACTIVE_DILOGS