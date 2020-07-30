#include "stdafx.h"

#include "smartreply_suggest.h"

#include "core.h"
#include "../tools/json_helper.h"
#include "../../corelib/collection_helper.h"
#include "../../common.shared/smartreply/smartreply_types.h"
#include "../../common.shared/smartreply/smartreply_config.h"
#include "../../common.shared/config/config.h"
#include "../../libomicron/include/omicron/omicron.h"

namespace
{
    bool get_config_value(const feature::smartreply::omicron_field& _field)
    {
        return omicronlib::_o(_field.name(), _field.def<bool>());
    }
}

namespace core
{
    namespace smartreply
    {
        std::vector<smartreply::type> supported_types_for_fetch()
        {
            if (config::get().is_on(config::features::smartreplies) && get_config_value(feature::smartreply::is_enabled()))
            {
                std::vector<smartreply::type> types;

                if (get_config_value(feature::smartreply::text_enabled()))
                    types.push_back(smartreply::type::text);

                if (get_config_value(feature::smartreply::sticker_enabled()))
                    types.push_back(smartreply::type::sticker);

                return types;
            }

            return {};
        }

        std::vector<suggest> parse_node(const smartreply::type _type, const std::string& _aimid, const int64_t _msgid, const rapidjson::Value& _node, std::string_view _node_name)
        {
            std::vector<suggest> result;
            if (const auto value = _node.FindMember(core::tools::make_string_ref(_node_name)); value != _node.MemberEnd() && value->value.IsArray())
            {
                result.reserve(value->value.Size());
                for (const auto& s : value->value.GetArray())
                {
                    smartreply::suggest suggest;
                    suggest.set_type(_type);
                    suggest.set_aimid(_aimid);
                    suggest.set_msgid(_msgid);
                    suggest.set_data(rapidjson_get_string(s));

                    result.push_back(std::move(suggest));
                }
            }

            return result;
        }

        suggest::suggest()
            : type_(type::invalid)
            , msg_id_(-1)
        {
        }

        void suggest::serialize(coll_helper& _coll) const
        {
            _coll.set_value_as_string("contact", aimid_);
            _coll.set_value_as_int64("msgId", msg_id_);
            _coll.set_value_as_int("type", int(type_));
            _coll.set_value_as_string("data", data_);
        }

        void suggest::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
        {
            _node.AddMember("type", tools::make_string_ref(type_2_string(type_)), _a);
            _node.AddMember("sn", aimid_, _a);

            if (msg_id_ != -1)
                _node.AddMember("msgId", msg_id_, _a);

            rapidjson::Value data_node(rapidjson::Type::kArrayType);
            data_node.Reserve(1, _a);
            data_node.PushBack(tools::make_string_ref(data_), _a);

            if (const auto node_name = array_node_name_for(type_); !node_name.empty())
                _node.AddMember(core::tools::make_string_ref(node_name), std::move(data_node), _a);
        }

        bool suggest::unserialize(const coll_helper& _coll)
        {
            if (!_coll.is_value_exist("type"))
                return false;

            type_ = (core::smartreply::type)_coll.get_value_as_int("type");
            msg_id_ = _coll.get_value_as_int64("msgId");
            aimid_ = _coll.get_value_as_string("contact");
            data_ = _coll.get_value_as_string("data");
            return true;
        }

        std::vector<suggest> unserialize_suggests_node(const rapidjson::Value& _node)
        {
            if (std::string type_str; tools::unserialize_value(_node, "type", type_str))
            {
                const auto type = smartreply::string_2_type(type_str);
                if (type == smartreply::type::invalid)
                    return {};

                std::string aimid;
                if (!tools::unserialize_value(_node, "sn", aimid) || aimid.empty())
                    return {};

                int64_t msgid = -1;
                if (!tools::unserialize_value(_node, "msgId", msgid))
                {
                    //assert("https://jira.mail.ru/browse/ICQHD-8481 not done yet");
                    if (std::string s; tools::unserialize_value(_node, "msgId", s) && !s.empty())
                        msgid = std::stoll(s);
                }

                if (const auto node_name = array_node_name_for(type); !node_name.empty())
                    return parse_node(type, aimid, msgid, _node, node_name);
            }

            return {};
        }

        std::vector<suggest> unserialize_suggests_node(const rapidjson::Value& _node, smartreply::type _type, const std::string& _aimid, int64_t _msgid)
        {
            return parse_node(_type, _aimid, _msgid, _node, smartreply::type_2_string(_type));
        }

        void serialize_smartreply_suggests(const std::vector<smartreply::suggest>& _suggests, coll_helper& _coll)
        {
            ifptr<iarray> suggests_array(_coll->create_array());
            suggests_array->reserve(_suggests.size());
            for (const auto& suggest : _suggests)
            {
                coll_helper suggest_coll(g_core->create_collection(), true);
                suggest.serialize(suggest_coll);
                ifptr<ivalue> val(_coll->create_value());
                val->set_as_collection(suggest_coll.get());
                suggests_array->push_back(val.get());
            }
            _coll.set_value_as_array("suggests", suggests_array.get());
        }
    }
}