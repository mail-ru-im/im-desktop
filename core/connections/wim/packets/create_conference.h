#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
        class http_request_simple;
    }

    namespace wim
    {
    	enum class conference_type
    	{
    		equitable = 0,
    		webinar = 1
    	};

        constexpr std::string_view get_conference_type_name(conference_type _type) noexcept
        {
            switch (_type)
            {
            case conference_type::equitable: return std::string_view("equitable");
            case conference_type::webinar: return std::string_view("webinar");
            default:
                return std::string_view("unknown");
            }
        }

        class create_conference : public robusto_packet
        {
            std::string name_;
            conference_type type_;
            std::vector<std::string> participants_;
            bool call_participants_;
            std::string conference_url_;
            int64_t expires_on_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _data) override;

        public:
            create_conference(wim_packet_params _params, std::string_view _name, conference_type _type, std::vector<std::string> _participants, bool _call_participants);
            virtual ~create_conference() = default;

            const std::string& get_conference_url() const { return conference_url_; }
            int64_t get_expires_on_() const { return expires_on_; }

            virtual std::string_view get_method() const override;
        };
    }
}
