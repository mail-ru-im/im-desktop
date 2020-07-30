#ifndef __DIALOG_HOLES_H_
#define __DIALOG_HOLES_H_

#pragma once

namespace core
{
    namespace wim
    {
        namespace holes
        {
            class request
            {
                int64_t from_;
                int64_t depth_;
                int32_t recursion_;
                std::string contact_;

            public:

                request(
                    const std::string& _contact,
                    int64_t _from,
                    int64_t _depth,
                    int32_t _recursion)
                    :
                    from_(_from),
                    depth_(_depth),
                    recursion_(_recursion),
                    contact_(_contact)
                {
                }

                void set_from(int64_t _from) { from_ = _from; }
                int64_t get_from() const { return from_; }
                void set_depth(int64_t _depth) { depth_ = _depth; }
                int64_t get_depth() const { return depth_; }
                void set_recursion(int32_t _recursion) { recursion_ = _recursion; }
                int32_t get_recursion() const { return recursion_; }
                void set_contact(const std::string& _contact) { contact_ = _contact; }
                const std::string& get_contact() const { return contact_; }
            };

            class failed_requests
            {
                std::map<std::string, holes::request> requests_;

            public:
                void add(const holes::request& _request)
                {
                    auto iter = requests_.find(_request.get_contact());
                    if (iter == requests_.end())
                    {
                        requests_.insert(std::make_pair(_request.get_contact(), _request));
                    }
                    else
                    {
                        iter->second.set_from(-1);
                        iter->second.set_depth(-1);
                        iter->second.set_recursion(-1);
                    }
                }

                std::shared_ptr<holes::request> get()
                {
                    std::shared_ptr<holes::request> req;

                    if (!requests_.empty())
                    {
                        req = std::make_shared<holes::request>(requests_.begin()->second);
                        requests_.erase(requests_.begin());
                    }

                    return req;
                }

            };
        }
    }
}

#endif //__DIALOG_HOLES_H_
