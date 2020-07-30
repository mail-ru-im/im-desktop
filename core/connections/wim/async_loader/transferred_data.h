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

            transferred_data(long _response_code, data_stream_ptr _header, data_stream_ptr _content)
                : response_code_(_response_code)
                , header_(std::move(_header))
                , content_(std::move(_content))
            {
            }

            transferred_data(long _response_code, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , additional_data_(std::move(_additional_data))
            {
            }

            transferred_data(long _response_code, data_stream_ptr _header, data_stream_ptr _content, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , header_(std::move(_header))
                , content_(std::move(_content))
                , additional_data_(std::move(_additional_data))
            {
            }

            template <typename Y>
            transferred_data(const transferred_data<Y>& _copied, additional_data_ptr _additional_data)
                : response_code_(_copied.response_code_)
                , header_(_copied.header_)
                , content_(_copied.content_)
                , additional_data_(std::move(_additional_data))
            {
            }

            explicit transferred_data(additional_data_ptr _additional_data)
                : additional_data_(std::move(_additional_data))
            {
            }

            const data_stream_ptr& get_content() const
            {
                if (!content_)
                {
                    content_ = std::make_shared<tools::binary_stream>();
                    if (auto file = tools::system::open_file_for_read(additional_data_->local_path_, std::ios::binary); file.good())
                        content_->write(file);
                }

                return content_;
            }

            ifptr<istream> get_content_as_core_istream(coll_helper& _helper) const
            {
                auto stream = _helper->create_stream();

                const auto& content = get_content();
                if (const auto size = content->available(); size > 0)
                    stream->write(reinterpret_cast<uint8_t*>(content->read(size)), size);

                return stream;
            }

            long response_code_ = 0;

            data_stream_ptr header_;
            mutable data_stream_ptr content_;

            additional_data_ptr additional_data_;
        };
    }
}
