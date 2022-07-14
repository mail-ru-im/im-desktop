#pragma once

using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

namespace core
{
    namespace antivirus
    {
        struct check
        {
            enum class mode
            {
                async,
                sync
            };
            enum class result
            {
                unchecked,
                safe,
                unsafe,
                unknown
            };
            mode mode_ = mode::async;
            result result_ = result::unchecked;

            static std::string_view mode_to_string(mode _mode);
            static mode mode_from_string(std::string_view _mode);
            static std::string_view result_to_string(result _result);
            static result result_from_string(std::string_view _result);

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void unserialize(const rapidjson::Value& _node);

        private:
            friend bool operator==(const check& _lhs, const check& _rhs)
            {
                return (_lhs.mode_ == _rhs.mode_) && (_lhs.result_ == _rhs.result_);
            }
            friend bool operator!=(const check& _lhs, const check& _rhs)
            {
                return !operator==(_lhs, _rhs);
            }
        };
    } // namespace antivirus
} // namespace core
