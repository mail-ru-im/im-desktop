#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/ifptr.h"

#include "../../../tools/system.h"

namespace core
{
    namespace wim
    {
        template <typename T>
        struct transferred_data
        {
            using data_stream_ptr = std::shared_ptr<tools::binary_stream>;
            using additional_data_ptr = std::shared_ptr<T>;

            transferred_data() = default;

            transferred_data(transferred_data<T>&&) = default;
            transferred_data& operator=(transferred_data<T>&&) = default;
            transferred_data(const transferred_data<T>&) = default;
            transferred_data& operator=(const transferred_data<T>&) = default;

            transferred_data(long _response_code)
                : response_code_(_response_code)
            {
            }

            transferred_data(long _response_code, data_stream_ptr _header)
                : response_code_(_response_code)
                , header_(std::move(_header))
            {
            }

            transferred_data(long _response_code, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , additional_data_(std::move(_additional_data))
            {
            }

            transferred_data(long _response_code, data_stream_ptr _header, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , header_(std::move(_header))
                , additional_data_(std::move(_additional_data))
            {
            }

            template <typename Y>
            transferred_data(const transferred_data<Y>& _copied, additional_data_ptr _additional_data)
                : response_code_(_copied.response_code_)
                , header_(_copied.header_)
                , additional_data_(std::move(_additional_data))
            {
            }

            explicit transferred_data(additional_data_ptr _additional_data)
                : additional_data_(std::move(_additional_data))
            {
            }

            long response_code_ = 0;

            data_stream_ptr header_;

            additional_data_ptr additional_data_;
        };
    }
}
