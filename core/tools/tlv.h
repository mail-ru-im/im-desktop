#pragma once

#include "binary_stream.h"

namespace core
{
    namespace tools
    {
        class tlv;

        typedef std::vector<std::shared_ptr<tlv>> tlv_list;

        template<class T, bool is_integral>
        struct is_implementaion_defined_fundamental_impl
        {
            enum {value = true};
        };

        template<class T>
        struct is_implementaion_defined_fundamental_impl<T, false>
        {
            enum {value = false};
        };

        template<class T>
        struct is_implementaion_defined_fundamental
        {
            enum {value = is_implementaion_defined_fundamental_impl<T, std::numeric_limits<T>::is_integer>::value};
        };

        template<> struct is_implementaion_defined_fundamental<bool> { enum {value  = false}; };
        template<> struct is_implementaion_defined_fundamental<int32_t> { enum {value  = false}; };
        template<> struct is_implementaion_defined_fundamental<uint32_t> { enum {value  = false}; };
        template<> struct is_implementaion_defined_fundamental<int64_t> { enum {value  = false}; };
        template<> struct is_implementaion_defined_fundamental<uint64_t> { enum {value  = false}; };

        class tlvpack
        {
            tlv_list			tlvlist_;
            tlv_list::iterator	cursor_;

            void copy(const tlvpack& _pack);

        public:
            tlvpack();
            tlvpack(const tlvpack& _pack);
            tlvpack& operator=(const tlvpack& _pack);
            virtual ~tlvpack();

            void serialize(binary_stream& _stream) const;
            bool unserialize(const binary_stream& _stream);

            void push_child(const std::shared_ptr<tlv>& _tlv);
            void push_child(tlv _tlv);

            std::shared_ptr<tlv> get_item(const uint32_t _type) const;
            std::shared_ptr<tlv> get_first();
            std::shared_ptr<tlv> get_next();

            uint32_t size() const;
            bool empty() const;
        };

        struct iserializable_tlv
        {
            virtual ~iserializable_tlv();
            virtual void serialize(tlvpack &_pack) const = 0;

            virtual bool unserialize(const tlvpack &_pack) = 0;
        };

        class tlv
        {
            uint32_t					type_;
            mutable binary_stream		value_stream_;

        public:

            tlv();

            template <class T_>
            tlv(const uint32_t _type, const T_ &_val);

            virtual ~tlv();

            template <class T_>
            void set_value(const T_ &_value);

            template <class T_>
            T_ get_value(const T_& _default_value) const;

            template <class T_>
            T_ get_value() const;

            uint32_t get_type() const;
            void set_type( const uint32_t _type );

            void serialize(binary_stream& _stream) const;
            bool unserialize(const binary_stream& _stream);
            static bool try_get_field_with_type(const binary_stream& _stream, uint32_t _type, uint32_t& _length);

        };

        template<> void tlv::set_value<iserializable_tlv>(const iserializable_tlv& _value);
        template<> void tlv::set_value<std::string>(const std::string& _value);
        template<> void tlv::set_value<core::tools::binary_stream>(const binary_stream& _value);
        template<> void tlv::set_value<core::tools::tlvpack>(const tlvpack& _value);

        template<> std::string tlv::get_value<std::string>(const std::string& _default_value) const;
        template<> std::string tlv::get_value<std::string>() const;
        template<> tlvpack tlv::get_value() const;
        template<> binary_stream tlv::get_value() const;

        template <class T_>
        tlv::tlv(
            const uint32_t _type,
            const T_ &_val
            )
        {
            static_assert(!is_implementaion_defined_fundamental<T_>::value, "Data Structure requires fixed-width types.");

            typedef typename std::conditional<
                std::is_base_of<iserializable_tlv, T_>::value,
                iserializable_tlv,
                T_>::type type;

            set_type(_type);
            set_value<type>(_val);
        }

        template <class T_>
        void tlv::set_value(const T_ &_value)
        {
            value_stream_.reset();
            value_stream_.write(_value);
        }

        template <class T_>
        T_ tlv::get_value(const T_& _default_value) const
        {
            static_assert(std::is_scalar<T_>::value, "value should be of scalar type");

            typename std::remove_const<T_>::type val = _default_value;

            if (value_stream_.available() < sizeof(T_))
            {
                assert(!"bad tlv length");
                return T_();
            }

            memcpy(&val, value_stream_.read(sizeof(T_)), sizeof(T_));
            value_stream_.reset_out();
            return val;
        }

        template <class T_>
        T_ tlv::get_value() const
        {
            static_assert(std::is_scalar<T_>::value, "value should be of scalar type");

            typename std::remove_const<T_>::type val;

            if (value_stream_.available() < sizeof(T_))
            {
                assert(!"bad tlv length");
                return T_();
            }

            memcpy(&val, value_stream_.read(sizeof(T_)), sizeof(T_));
            value_stream_.reset_out();
            return val;
        }
    }
}
