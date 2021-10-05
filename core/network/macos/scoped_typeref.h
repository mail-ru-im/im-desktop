// Copyright (c) 2014 The Chromium Authors. All rights reserved.

#pragma once

namespace core
{
    namespace internal
    {

        // scoped_type_ref<> is patterned after std::unique_ptr<>, but maintains ownership
        // of a reference to any type that is maintained by Retain and Release methods.
        //
        // The Traits structure must provide the Retain and Release methods for type T.
        // A default scoped_type_ref_traits is used but not defined, and should be defined
        // for each type to use this interface. For example, an appropriate definition
        // of scoped_type_ref_traits for CGLContextObj would be:
        //
        //   template<>
        //   struct scoped_type_ref_traits<CGLContextObj> {
        //     static CGLContextObj InvalidValue() { return nullptr; }
        //     static CGLContextObj Retain(CGLContextObj object) {
        //       CGLContextRetain(object);
        //       return object;
        //     }
        //     static void Release(CGLContextObj object) { CGLContextRelease(object); }
        //   };
        //
        // For the many types that have pass-by-pointer create functions, the function
        // InitializeInto() is provided to allow direct initialization and assumption
        // of ownership of the object. For example, continuing to use the above
        // CGLContextObj specialization:
        //
        //   base::scoped_type_ref<CGLContextObj> context;
        //   CGLCreateContext(pixel_format, share_group, context.InitializeInto());
        //
        // For initialization with an existing object, the caller may specify whether
        // the scoped_type_ref<> being initialized is assuming the caller's existing
        // ownership of the object (and should not call Retain in initialization) or if
        // it should not assume this ownership and must create its own (by calling
        // Retain in initialization). This behavior is based on the |policy| parameter,
        // with |ASSUME| for the former and |RETAIN| for the latter. The default policy
        // is to |ASSUME|.

        namespace scoped_policy 
        {
            // Defines the ownership policy for a scoped object.
            enum ownership_policy {
                // The scoped object takes ownership of an object by taking over an existing
                // ownership claim.
                ASSUME,

                // The scoped object will retain the object and any initial ownership is
                // not changed.
                RETAIN
            };
        }  // namespace scoped_policy

        template<typename T>
        struct scoped_type_ref_traits;

        template<typename T, typename traits = scoped_type_ref_traits<T>>
        class scoped_type_ref
        {
        public:
            using element_type = T;

            explicit constexpr scoped_type_ref(element_type object = traits::invalid_value(), scoped_policy::ownership_policy policy = scoped_policy::ASSUME)
                : object_(object)
            {
                if (object_ && policy == scoped_policy::RETAIN)
                    object_ = traits::retain(object_);
            }

            scoped_type_ref(const scoped_type_ref<T, traits>& other)
                : object_(other.object_) 
            {
                if (object_)
                    object_ = traits::retain(object_);
            }

            // This allows passing an object to a function that takes its superclass.
            template <typename R, typename r_traits>
            explicit scoped_type_ref(const scoped_type_ref<R, r_traits>& _that_as_subclass)
                : object_(_that_as_subclass.get())
            {
                if (object_)
                    object_ = traits::retain(object_);
            }

            scoped_type_ref(scoped_type_ref<T, traits>&& that) : object_(that.object_)
            {
                that.object_ = traits::invalid_value();
            }

            ~scoped_type_ref() 
            {
                if (object_)
                    traits::release(object_);
            }

            scoped_type_ref& operator=(const scoped_type_ref<T, traits>& _other)
            {
                reset(_other.get(), scoped_policy::RETAIN);
                return *this;
            }

            // This is to be used only to take ownership of objects that are created
            // by pass-by-pointer create functions. To enforce this, require that the
            // object be reset to NULL before this may be used.
            element_type* initialize_into()
            {
                im_assert(!object_);
                return &object_;
            }

            void reset(const scoped_type_ref<T, traits>& _other)
            {
                reset(_other.get(), scoped_policy::RETAIN);
            }

            void reset(element_type object = traits::invalid_value(), scoped_policy::ownership_policy policy = scoped_policy::ASSUME)
            {
                if (object && policy == scoped_policy::RETAIN)
                    object = traits::retain(object);
                if (object_)
                    traits::release(object_);
                object_ = object;
            }

            bool operator==(const element_type& that) const { return object_ == that; }

            bool operator!=(const element_type& that) const { return object_ != that; }

            operator element_type() const { return object_; }

            element_type get() const { return object_; }

            void swap(scoped_type_ref& that)
            {
                element_type temp = that.object_;
                that.object_ = object_;
                object_ = temp;
            }

            // scoped_type_ref<>::release() is like std::unique_ptr<>::release.  It is NOT
            // a wrapper for Release().  To force a scoped_type_ref<> object to call
            // Release(), use scoped_type_ref<>::reset().
            element_type release()
            {
                element_type temp = object_;
                object_ = traits::invalid_value();
                return temp;
            }

        private:
            element_type object_;
        };

    } // namespace internal
} // namespace core

