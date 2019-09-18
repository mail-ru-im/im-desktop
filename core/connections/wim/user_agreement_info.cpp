#include "stdafx.h"
#include "user_agreement_info.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/encodedstream.h>

using namespace core;
using namespace wim;

user_agreement_info::user_agreement_info()
    : need_to_accept_types_()
{

}

int32_t user_agreement_info::unserialize(const rapidjson::Value& _node)
{
    assert(_node.IsArray());

    const auto& types = _node;
    for (rapidjson::SizeType i = 0; i < types.Size(); ++i)
    {
        const auto& type = types[i];
        if (!type.IsString())
        {
            assert(!"not a string in useragreement array");
            continue;
        }

        if (rapidjson_get_string_view(type) == "gdpr_pp")
        {
            need_to_accept_types_.push_back(agreement_type::gdpr_pp);
        }
    }

    return 0;
}

void user_agreement_info::serialize(coll_helper &_coll)
{
    ifptr<iarray> accepted_types_array(_coll->create_array());
    accepted_types_array->reserve(need_to_accept_types_.size());

    for (auto type : need_to_accept_types_)
    {
        ifptr<ivalue> type_val(_coll->create_value());
        type_val->set_as_int(static_cast<int32_t>(type));

        accepted_types_array->push_back(type_val.get());
    }

    _coll.set_value_as_array("userAgreement", accepted_types_array.get());
}

