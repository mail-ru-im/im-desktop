#pragma once

namespace core
{
    namespace tools
    {
        class fast_binary_stream
        {
            typedef std::vector<char>					data_buffer;
            typedef std::shared_ptr<data_buffer>		data_buffer_pointer;
            typedef std::list<data_buffer_pointer>		data_buffer_list;

            data_buffer_pointer		return_buffer_;
            uint32_t				size_;
            uint32_t				cursor_;
            data_buffer_list		data_;

            data_buffer_pointer get_return_data_buffer(uint32_t _size);

        public:

            fast_binary_stream();
            fast_binary_stream(const fast_binary_stream&);
            virtual ~fast_binary_stream();
            void copy(const fast_binary_stream& _stream);
            fast_binary_stream& operator=(const fast_binary_stream&);

            uint32_t available() const;
            void seek(uint32_t _pos);

            void write(const char* _lpData, uint32_t _size);
            template <class T_>	void write(const T_& _val);

            char* read(uint32_t _size);
            template <class T_> T_ read();

            void clear();

            bool save_2_file(const std::wstring& _file_name) const;
            bool load_from_file(const std::wstring& _file_name);
        };

        template <class T_>
        void core::tools::fast_binary_stream::write(const T_& _val)
        {
            write((const char*)&_val, (int32_t) sizeof(_val));
        }

        template <>
        void core::tools::fast_binary_stream::write<std::string>(const std::string& _val)
        {
            write(_val.c_str(), (int32_t)_val.length());
        }

        template <class T_>
        T_ core::tools::fast_binary_stream::read()
        {
            return  *((T_*) read(sizeof(T_)));
        }

        template <>
        std::string core::tools::fast_binary_stream::read<std::string>()
        {
            std::string val;

            auto sz = available();
            if (!sz)
                return val;

            val.assign((const char*) read(sz), sz);

            return val;
        }

    }
}

template <>	void core::tools::fast_binary_stream::write<std::string>(const std::string& _val);
template <> std::string core::tools::fast_binary_stream::read<std::string>();

