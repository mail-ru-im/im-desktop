#pragma once

#include <cstdint>
#include <vector>
#include "../../../corelib/collection_helper.h"

namespace core
{
    namespace wim
    {
        class user_agreement_info
        {
        public:
            enum class agreement_type
            {
                gdpr_pp
            };

            user_agreement_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper& _coll);

//            void serialize(core::tools::binary_stream& _data) const;
//            bool unserialize(core::tools::binary_stream& _data);

            std::vector<agreement_type> need_to_accept_types_;
        };
    }
}
