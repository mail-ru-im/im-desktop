#include "stdafx.h"

#include "binary_stream.h"
#include "binary_stream_reader.h"

namespace core
{
    namespace tools
    {

        binary_stream_reader::binary_stream_reader(binary_stream &_bs)
            : bs_(_bs)
        {
        }

        std::string binary_stream_reader::readline()
        {
            std::string result;

            while (bs_.available() > 0)
            {
                const auto ch = *bs_.read(1);

                if (ch == '\r')
                {
                    continue;
                }

                if (ch == '\n')
                {
                    break;
                }

                result += char(ch);
            }

            return result;
        }

        bool binary_stream_reader::eof() const
        {
            return (bs_.available() == 0);
        }

    }
}