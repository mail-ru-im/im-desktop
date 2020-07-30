#pragma once

#include "async_handler.h"
#include "downloaded_file_info.h"

namespace core
{
    namespace wim
    {
        struct downloadable_file_chunks
        {
            class active_seqs
            {
            public:
                bool has(int64_t _seq) const
                {
                    std::scoped_lock lock(m_);
                    return std::any_of(seqs_.begin(), seqs_.end(), [_seq](auto x) { return x == _seq; });
                }

                bool erase(int64_t _seq)
                {
                    std::scoped_lock lock(m_);
                    const auto it = std::remove_if(seqs_.begin(), seqs_.end(), [_seq](auto x) { return x == _seq; });
                    if (it != seqs_.end())
                    {
                        seqs_.erase(it, seqs_.end());
                        return true;
                    }
                    return false;
                }

                void add(int64_t _seq)
                {
                    std::scoped_lock lock(m_);
                    seqs_.push_back(_seq);
                }

                bool is_empty() const
                {
                    std::scoped_lock lock(m_);
                    return seqs_.empty();
                }


            private:
                mutable std::mutex m_;
                std::vector<int64_t> seqs_;
            };


            downloadable_file_chunks();
            downloadable_file_chunks(priority_t _priority, const std::string& _contact, const std::string& _url, std::wstring_view _file_name, int64_t _total_size);

            priority_t priority_on_start_;
            priority_t priority_;

            std::string url_;

            std::wstring file_name_;
            std::wstring tmp_file_name_;

            int64_t downloaded_;
            int64_t total_size_;

            active_seqs active_seqs_;

            typedef std::vector<async_handler<downloaded_file_info>> handler_list_t;
            handler_list_t handlers_;

            std::vector<hash_t> contacts_;
        };

        using downloadable_file_chunks_ptr = std::shared_ptr<downloadable_file_chunks>;
    }
}
