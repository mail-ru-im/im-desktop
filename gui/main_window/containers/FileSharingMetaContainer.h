#pragma once

#include "../history_control/complex_message/FileSharingUtils.h"
#include "../../types/filesharing_meta.h"

namespace Data
{
    struct FileSharingDownloadResult;
}

namespace Logic
{
    struct MetaCallbacks
    {
        using ReceiveCallback = std::function<void(const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)>;
        ReceiveCallback successCallback_;
        using ErrorCallback = std::function<void(const Utils::FileSharingId& _fsId, qint32 _errorCode)>;
        ErrorCallback errorCallback_;
    };

    class FileSharingMetaContainer : public QObject
    {
        Q_OBJECT

    public:
        FileSharingMetaContainer(QObject* _parent = nullptr);
        ~FileSharingMetaContainer() = default;

        template <typename T>
        void requestFileSharingMetaInfo(const Utils::FileSharingId& _fsId, T _receiver);

        bool isDownloading(const Utils::FileSharingId& _fsId) const;
    private:
        void requestMeta(const Utils::FileSharingId& _fsId) const;

    private Q_SLOTS:
        void onMetaInfoDownloaded(qint64 _seq, const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta);
        void onFileSharingError(qint64 _seq, const Utils::FileSharingId& _fsId, qint32 _errorCode);
        void onFileSharingDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void onAntivirusCheckResult(const Utils::FileSharingId& _fsId, core::antivirus::check::result _result);

    private:
        using RequestId = int64_t;
        std::unordered_multimap<Utils::FileSharingId, MetaCallbacks, Utils::FileSharingIdHasher> waitList_;
        std::unordered_map<Utils::FileSharingId, Data::FileSharingMeta, Utils::FileSharingIdHasher> cache_;
    };

    FileSharingMetaContainer* GetFileSharingMetaContainer();
    void ResetFileSharingMetaContainer();

    namespace Detail
    {
        template <typename T>
        struct IsSharedPointer : std::false_type {};
        template <typename T>
        struct IsSharedPointer<std::shared_ptr<T>> : std::true_type {};
        template <typename T>
        inline constexpr bool IsSharedPointerV = IsSharedPointer<T>::value;

        template <typename T, std::enable_if_t<std::is_base_of_v<QObject, std::remove_pointer_t<T>>, bool> = true>
        MetaCallbacks createCallbacks(T _receiver)
        {
            MetaCallbacks::ReceiveCallback callback = [guard = QPointer(_receiver)](const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)
            {
                if (guard)
                    guard->processMetaInfo(_fsId, _meta);
            };
            MetaCallbacks::ErrorCallback errorCallback = [guard = QPointer(_receiver)](const Utils::FileSharingId& _fsId, qint32 _errorCode)
            {
                if (guard)
                    guard->processMetaInfoError(_fsId, _errorCode);
            };
            return { callback, errorCallback };
        }

        template <typename T, std::enable_if_t<IsSharedPointerV<T>, bool> = true>
        MetaCallbacks createCallbacks(T _receiver)
        {
            MetaCallbacks::ReceiveCallback callback = [guard = std::weak_ptr(_receiver)](const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)
            {
                if (auto p = guard.lock())
                    p->processMetaInfo(_fsId, _meta);
            };
            MetaCallbacks::ErrorCallback errorCallback = [guard = std::weak_ptr(_receiver)](const Utils::FileSharingId& _fsId, qint32 _errorCode)
            {
                if (auto p = guard.lock())
                    p->processMetaInfoError(_fsId, _errorCode);
            };
            return { callback, errorCallback };
        }
    } // namespace Detail


    template <typename T>
    void FileSharingMetaContainer::requestFileSharingMetaInfo(const Utils::FileSharingId& _fsId, T _receiver)
    {
        static_assert(std::disjunction_v<std::is_base_of<QObject, std::remove_pointer_t<T>>, Detail::IsSharedPointer<T>>);
        if (const auto it = cache_.find(_fsId); it != cache_.end())
            _receiver->processMetaInfo(_fsId, it->second);

        if (waitList_.find(_fsId) == waitList_.end())
            requestMeta(_fsId);
        waitList_.insert({ _fsId, Detail::createCallbacks(_receiver) });
    }
}
