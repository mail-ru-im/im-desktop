#pragma once

namespace core
{
    namespace tools
    {

        class binary_stream;

        class binary_stream_reader final : boost::noncopyable
        {
        public:
            binary_stream_reader(binary_stream &_bs);

            std::string readline();

            bool eof() const;

        private:
            binary_stream &bs_;

        };

    }
}