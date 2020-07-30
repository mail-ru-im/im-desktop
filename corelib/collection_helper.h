#pragma once

#include "core_face.h"
#include "ifptr.h"
#include "iserializable.h"

namespace core
{
    class coll_helper;

    typedef std::function<void(const coll_helper&)> remote_proc_t;

    class serializable;

    class coll_helper
    {
        mutable ifptr<icollection> collection_;

    public:

        coll_helper(const ifptr<icollection> &_collection)
            : collection_(_collection)
        {
        }

        coll_helper(icollection* _collection, bool auto_release)
            : collection_(_collection, auto_release)
        {
        }

        virtual ~coll_helper()
        {
        }

        coll_helper(const coll_helper& _copy)
            : collection_(_copy.collection_)

        {

        }

        coll_helper& operator=(const coll_helper& _copy)
        {
            collection_ = _copy.collection_;

            return *this;
        }

        icollection* detach()
        {
            return collection_.detach();
        }

        bool is_value_exist(std::string_view _name) const
        {
            return collection_->is_value_exist(_name);
        }

        void set_value_as_int(std::string_view _name, int32_t _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_int(_value);
            collection_->set_value(_name, val.get());
        }

        void set_value_as_uint(std::string_view _name, uint32_t _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_uint(_value);
            collection_->set_value(_name, val.get());
        }

        template<class T>
        void set_value_as_enum(std::string_view _name, const T _value);

        int32_t get_value_as_int(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return 0;

            return val->get_as_int();
        }

        int32_t get_value_as_int(std::string_view _name, int32_t _def_val) const
        {
            if (!is_value_exist(_name))
            {
                return _def_val;
            }

            return get_value_as_int(_name);
        }

        uint32_t get_value_as_uint(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return 0;

            return val->get_as_uint();
        }

        uint32_t get_value_as_uint(std::string_view _name, uint32_t _def_val) const
        {
            if (!is_value_exist(_name))
            {
                return _def_val;
            }

            return get_value_as_uint(_name);
        }

        void set_value_as_int64(std::string_view _name, int64_t _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_int64(_value);
            collection_->set_value(_name, val.get());
        }

        template<class T>
        T get_value_as_enum(std::string_view _name) const;

        template<class T>
        T get_value_as_enum(std::string_view _name, const T _def) const;


        int64_t get_value_as_int64(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return 0;

            return val->get_as_int64();
        }

        int64_t get_value_as_int64(std::string_view _name, int64_t _def_val) const
        {
            if (!is_value_exist(_name))
            {
                return _def_val;
            }

            return get_value_as_int64(_name);
        }

        void set_value_as_double(std::string_view _name, double _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_double(_value);
            collection_->set_value(_name, val.get());
        }

        double get_value_as_double(std::string_view _name, double _def_val = 0)
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return _def_val;

            return val->get_as_double();
        }

        void set_value_as_bool(std::string_view _name, bool _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_bool(_value);
            collection_->set_value(_name, val.get());
        }

        bool get_value_as_bool(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
            {
                return false;
            }

            return val->get_as_bool();
        }

        bool get_value_as_bool(std::string_view _name, const bool _def_val) const
        {
            if (!collection_->is_value_exist(_name))
            {
                return _def_val;
            }

            return get_value_as_bool(_name);
        }

        void set_value_as_string(std::string_view _name, const char* _value, int32_t _len)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_string(_value, _len);
            collection_->set_value(_name, val.get());
        }

        void set_value_as_string(std::string_view _name, std::string_view _value)
        {
            set_value_as_string(_name, _value.data(), int32_t(_value.size()));
        }

        const char* get_value_as_string(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return "";

            return val->get_as_string();
        }

        const char* get_value_as_string(std::string_view _name, const char* _def_val) const
        {
            if (!collection_->is_value_exist(_name))
            {
                return _def_val;
            }

            return get_value_as_string(_name);
        }

        void set_value_as_collection(std::string_view _name, icollection* _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_collection(_value);
            collection_->set_value(_name, val.get());
        }

        icollection* get_value_as_collection(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return nullptr;

            return val->get_as_collection();
        }

        void set_value_as_array(std::string_view _name, iarray* _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_array(_value);
            collection_->set_value(_name, val.get());
        }

        iarray* get_value_as_array(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return nullptr;

            return val->get_as_array();
        }

        void set_value_as_stream(std::string_view _name, istream* _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_stream(_value);
            collection_->set_value(_name, val.get());
        }


        istream* get_value_as_stream(std::string_view _name) const
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return nullptr;

            return val->get_as_stream();
        }

        void set_value_as_hheaders(std::string_view _name, ihheaders_list* _value)
        {
            ifptr<ivalue> val(collection_->create_value());
            val->set_as_hheaders(_value);
            collection_->set_value(_name, val.get());
        }

        ihheaders_list* get_value_as_hheaders(std::string_view _name)
        {
            auto val = collection_->get_value(_name);
            if (!val)
                return nullptr;

            return val->get_as_hheaders();
        }

        void set_value_as_proc(std::string_view _name, const remote_proc_t _proc);

        ivalue* first()
        {
            return collection_->first();
        }

        ivalue* next()
        {
            return collection_->next();
        }

        virtual int32_t count()
        {
            return collection_->count();
        }

        virtual bool empty()
        {
            return collection_->empty();
        }

        icollection* get() const
        {
            return collection_.get();
        }

        icollection* operator ->() const
        {
            return get();
        }

        // helpers for our good-for-nothing compiler

#define REMOVE_R(T) std::remove_reference_t<T>

#define REMOVE_CVR(T) REMOVE_R(std::remove_cv_t<T>)

#define ENABLE_IF_NOT_ENUM_T(T) std::enable_if_t<!std::is_enum_v<REMOVE_CVR(T)>, T>

#define ENABLE_IF_ENUM_T(T) std::enable_if_t<std::is_enum_v<REMOVE_CVR(T)>, T>

#define IS_SCALAR(T) std::is_scalar_v<REMOVE_CVR(T)>

#define REMOVE_REF_IF_SCALAR(T) std::conditional_t<IS_SCALAR(T), REMOVE_R(T), T>

// getters

        template<class T>
        ENABLE_IF_NOT_ENUM_T(T) get(std::string_view _name) const;

        template<class T, class U>
        ENABLE_IF_NOT_ENUM_T(T) get(std::string_view _name, U _def) const;

        template<class T>
        ENABLE_IF_ENUM_T(T) get(std::string_view _name) const;

        template<class T>
        ENABLE_IF_ENUM_T(T) get(std::string_view _name, const T _def) const;

        // setters

        template<class T>
        void set(std::string_view _name, ENABLE_IF_ENUM_T(const T) _value);

#ifdef _WIN32
        template<class T>
        void set(std::string_view _name, ENABLE_IF_NOT_ENUM_T(REMOVE_REF_IF_SCALAR(const T&)) _value);
#else
        template<class T>
        void set(std::string_view _name, ENABLE_IF_NOT_ENUM_T(const T&) _value);
#endif //_WIN32B

    };

    template<class T>
    T coll_helper::get_value_as_enum(std::string_view _name) const
    {
        static_assert(std::is_enum<T>::value, "get_value_as_enum should be used for enumerations only");
        return (T)get_value_as_int(_name);
    }

    template<class T>
    T coll_helper::get_value_as_enum(std::string_view _name, const T _def) const
    {
        static_assert(std::is_enum<T>::value, "get_value_as_enum should be used for enumerations only");
        return (T)get_value_as_int(_name, (int32_t)_def);
    }

    template<class T>
    void coll_helper::set_value_as_enum(std::string_view _name, const T _value)
    {
        static_assert(std::is_enum<T>::value, "set_value_as_enum should be used for enumerations only");
        set_value_as_int(_name, (int)_value);
    }

    template<class T>
    inline ENABLE_IF_ENUM_T(T) coll_helper::get(std::string_view _name) const
    {
        return get_value_as_enum<T>(_name);
    }

    template<class T>
    inline ENABLE_IF_ENUM_T(T) coll_helper::get(std::string_view _name, const T _def) const
    {
        return get_value_as_enum<T>(_name, _def);
    }

    template<>
    inline std::string coll_helper::get<std::string>(std::string_view _name) const
    {
        return get_value_as_string(_name);
    }

    template<>
    inline std::string coll_helper::get<std::string>(std::string_view _name, const char *_def) const
    {
        return get_value_as_string(_name, _def);
    }

    template<>
    inline int32_t coll_helper::get<int32_t>(std::string_view _name) const
    {
        return get_value_as_int(_name);
    }

    template<>
    inline int32_t coll_helper::get<int32_t>(std::string_view _name, const int32_t _def) const
    {
        return get_value_as_int(_name, _def);
    }

    template<>
    inline int64_t coll_helper::get<int64_t>(std::string_view _name) const
    {
        return get_value_as_int64(_name);
    }

    template<>
    inline int64_t coll_helper::get<int64_t>(std::string_view _name, const int64_t _def) const
    {
        return get_value_as_int64(_name, _def);
    }

    template<>
    inline int64_t coll_helper::get<int64_t>(std::string_view _name, const int32_t _def) const
    {
        return get_value_as_int64(_name, _def);
    }

    template<>
    inline uint64_t coll_helper::get<uint64_t>(std::string_view _name) const
    {
        return (uint64_t)get_value_as_int64(_name);
    }

    template<>
    inline uint64_t coll_helper::get<uint64_t>(std::string_view _name, const uint64_t _def) const
    {
        return (uint64_t)get_value_as_int64(_name, _def);
    }

    template<>
    inline uint64_t coll_helper::get<uint64_t>(std::string_view _name, const int32_t _def) const
    {
        return (uint64_t)get_value_as_int64(_name, _def);
    }

    template<>
    inline bool coll_helper::get<bool>(std::string_view _name) const
    {
        return get_value_as_bool(_name);
    }

    template<>
    inline bool coll_helper::get<bool>(std::string_view _name, const bool _def) const
    {
        return get_value_as_bool(_name, _def);
    }

    template<class T>
    inline void coll_helper::set(std::string_view _name, ENABLE_IF_ENUM_T(const T) _value)
    {
        set_value_as_enum<T>(_name, _value);
    }

    template<>
    inline void coll_helper::set<std::string>(std::string_view _name, const std::string &_value)
    {
        set_value_as_string(_name, _value);
    }

    template<>
    inline void coll_helper::set<std::string_view>(std::string_view _name, const std::string_view &_value)
    {
        set_value_as_string(_name, _value);
    }

    template<>
    inline void coll_helper::set<remote_proc_t>(std::string_view _name, const remote_proc_t &_proc)
    {
        set_value_as_proc(_name, _proc);
    }


#ifdef _WIN32
    template<>
    inline void coll_helper::set<int32_t>(std::string_view _name, const int32_t _value)
    {
        set_value_as_int(_name, _value);
    }

    template<>
    inline void coll_helper::set<uint32_t>(std::string_view _name, const uint32_t _value)
    {
        set_value_as_uint(_name, _value);
    }

    template<>
    inline void coll_helper::set<int64_t>(std::string_view _name, const int64_t _value)
    {
        set_value_as_int64(_name, _value);
    }

    template<>
    inline void coll_helper::set<uint64_t>(std::string_view _name, const uint64_t _value)
    {
        set_value_as_int64(_name, (int64_t)_value);
    }

    template<>
    inline void coll_helper::set<bool>(std::string_view _name, const bool _value)
    {
        set_value_as_bool(_name, _value);
    }
#else
    template<>
    inline void coll_helper::set<int32_t>(std::string_view _name, const int32_t &_value)
    {
        set_value_as_int(_name, _value);
    }

    template<>
    inline void coll_helper::set<uint32_t>(std::string_view _name, const uint32_t &_value)
    {
        set_value_as_uint(_name, _value);
    }

    template<>
    inline void coll_helper::set<int64_t>(std::string_view _name, const int64_t &_value)
    {
        set_value_as_int64(_name, _value);
    }

    template<>
    inline void coll_helper::set<uint64_t>(std::string_view _name, const uint64_t &_value)
    {
        set_value_as_int64(_name, (int64_t)_value);
    }

    template<>
    inline void coll_helper::set<bool>(std::string_view _name, const bool &_value)
    {
        set_value_as_bool(_name, _value);
    }
#endif // _WIN32

    template<>
    inline void coll_helper::set<iserializable>(std::string_view _name, const iserializable &_value)
    {
        coll_helper coll(collection_->create_collection(), true);
        _value.serialize(Out coll);

        set_value_as_collection(_name, coll.get());
    }

#undef ENABLE_IF_NOT_ENUM_T
#undef ENABLE_IF_ENUM_T
#undef REMOVE_R
#undef REMOVE_CVR
#undef IS_SCALAR
#undef REMOVE_REF_IF_SCALAR
}
