#ifndef __VERSION_INFO_H_
#define __VERSION_INFO_H_

#pragma once

#include "version_info_constants.h"

namespace core
{
    namespace tools
    {
        class version_info
        {
        public:
            const std::string& get_version() const
            {
                static auto version = get_major_version() + '.' + get_minor_version() + '.' + get_build_version();

                return version;
            }

            const std::string& get_ua_version() const
            {
                static auto ua_version = get_major_version() + '.' + get_minor_version() + ".0(" + get_build_version() + ')';

                return ua_version;
            }

            bool operator < (const version_info& _version_info) const
            {
                if (major_ != _version_info.major_)
                    return major_ < _version_info.major_;

                if (minor_ != _version_info.minor_)
                    return minor_ < _version_info.minor_;

                return build_ < _version_info.build_;
            }

            bool operator != (const version_info& _version_info) const
            {
                return major_ != _version_info.major_ || minor_ != _version_info.minor_ || build_ != _version_info.build_;
            }

            std::string get_major_version() const
            {
                return std::to_string(major_);
            }

            std::string get_minor_version() const
            {
                return std::to_string(minor_);
            }

            std::string get_build_version() const
            {
                return std::to_string(build_);
            }

            int32_t get_major() const
            {
                return major_;
            }

            int32_t get_minor() const
            {
                return minor_;
            }

            int32_t get_build() const
            {
                return build_;
            }


            version_info()
            {
                load_version(VERSION_INFO_STR);
            }

            version_info(const std::string& _version)
            {
                load_version(_version);
            }

            version_info(int32_t _major, int32_t _minor, int32_t _build)
                :   major_(_major),
                minor_(_minor),
                build_(_build)
            {

            }

        private:

            std::vector<std::string> split(const std::string& _str, char _delimiter)
            {
                std::vector<std::string> internal;
                std::stringstream ss(_str); // Turn the string into a stream.
                std::string tok;

                while(std::getline(ss, tok, _delimiter)) {
                    internal.push_back(tok);
                }

                return internal;
            }

            void load_version(const std::string& _version)
            {
                auto words = split(_version, '.');
                major_ = std::stoi(*(words.begin())) ;
                minor_ = std::stoi(*(words.begin() + 1));
                build_ = std::stoi(*(words.begin() + 2));
            }

            int32_t major_;
            int32_t minor_;
            int32_t build_;
        };


    }
}

#endif // __VERSION_INFO_H_
