#pragma once

#include <string>
#include <string_view>
#include <algorithm>
#include <utility>

namespace common
{
    namespace tools
    {
        class patch_version
        {
        public:
            explicit patch_version() noexcept = default;

            explicit patch_version(std::string str) noexcept
                : version_string_(std::move(str))
            {
            }

            explicit patch_version(std::string_view str)
                : version_string_(str)
            {
            }

            patch_version(patch_version&&) = default;
            patch_version(const patch_version&) = default;
            patch_version& operator=(patch_version&& other) = default;
            patch_version& operator=(const patch_version& other) = default;

            const std::string& as_string() const noexcept
            {
                return version_string_;
            }

            void set_version(const std::string_view _version)
            {
                version_string_ = _version;
            }

            void set_offline_version(const int32_t _version)
            {
                offline_version_ = _version;
            }

            int32_t get_offline_version() const noexcept
            {
                return offline_version_;
            }

            bool is_empty() const noexcept
            {
                return version_string_.empty();
            }

            void increment_offline()
            {
                ++offline_version_;
            }

             bool operator==(const patch_version &rhs) const noexcept
             {
                 return version_string_ == rhs.version_string_;
             }

             bool operator!=(const patch_version &rhs) const noexcept
             {
                 return version_string_ != rhs.version_string_;
             }

            bool operator<(const patch_version &rhs) const noexcept
            {
                return less_than(version_string_, rhs.version_string_);
            }

            bool operator>(const patch_version &rhs) const noexcept
            {
                return rhs < *this;
            }

            bool operator>=(const patch_version &rhs) const noexcept
            {
                return !(*this < rhs);
            }

        private:
            static bool less_than(std::string_view lhs, std::string_view rhs) noexcept
            {
                if (lhs.size() == rhs.size())
                    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());

                return lhs.size() < rhs.size();
            }

        private:
            std::string version_string_;

            int32_t offline_version_ = 0;
        };
    }
}