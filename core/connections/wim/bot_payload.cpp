#include "stdafx.h"

#include "../../log/log.h"

#include "bot_payload.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../../../../common.shared/json_helper.h"

using namespace core;
using namespace core::wim;


void bot_payload::unserialize(const rapidjson::Value &_node)
{
    if (const auto iter_payload = _node.FindMember("payload"); iter_payload != _node.MemberEnd() && iter_payload->value.IsObject())
    {
        tools::unserialize_value(iter_payload->value, "text", text_);
        tools::unserialize_value(iter_payload->value, "url", url_);
        tools::unserialize_value(iter_payload->value, "showAlert", show_alert_);
        empty_ = false;
    }
}

void bot_payload::serialize(icollection* _collection) const
{
    if (empty_)
        return;


    coll_helper coll(_collection, false);
    coll_helper coll_payload(coll->create_collection(), true);
    coll_payload.set_value_as_string("text", text_);
    coll_payload.set_value_as_string("url", url_);
    coll_payload.set_value_as_bool("show_alert", show_alert_);

    coll.set_value_as_collection("payload", coll_payload.get());
}
