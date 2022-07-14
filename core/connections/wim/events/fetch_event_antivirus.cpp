#include "stdafx.h"

#include "fetch_event_antivirus.h"

#include "../../wim/wim_im.h"
#include "../common.shared/json_helper.h"

namespace core::wim
{

    int32_t fetch_event_antivirus::parse(const rapidjson::Value& _node_event_data)
    {
        if (std::string_view check_result; core::tools::unserialize_value(_node_event_data, "fileState", check_result))
            result_ = core::antivirus::check::result_from_string(check_result);
        else
            return wpie_error_parse_response;

        if (std::string_view file_hash; core::tools::unserialize_value(_node_event_data, "fileHash", file_hash))
        {
            const auto hash_pos = file_hash.find('#');
            const auto file_id = file_hash.substr(0, hash_pos);
            const auto source_id = std::string_view::npos != hash_pos ? file_hash.substr(hash_pos + 1) : std::string_view{};
            file_hash_ = core::tools::filesharing_id(file_id, source_id);
        }
        else
        {
            return wpie_error_parse_response;
        }

        return 0;
    }

    void fetch_event_antivirus::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
    {
        _im->on_event_antivirus(this, _on_complete);
    }

}
