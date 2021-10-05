#pragma once

namespace core
{
    namespace threads
    {
        namespace parent_topic
        {
            enum class type
            {
                invalid,

                message,
                task,
            };

            type string_to_type(std::string_view _str);
            std::string_view type_to_string(type _type);
        }
    }
}
