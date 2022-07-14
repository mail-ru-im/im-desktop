#include "stdafx.h"

#include "FileSharingMetaContainer.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"


namespace Logic
{
    FileSharingMetaContainer::FileSharingMetaContainer(QObject* _parent)
        : QObject(_parent)
    {
        setObjectName(qsl("FileSharingMetaContainer"));

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded, this, &FileSharingMetaContainer::onMetaInfoDownloaded);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded, this, &FileSharingMetaContainer::onFileSharingDownloaded);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileMetainfoDownloadError, this, &FileSharingMetaContainer::onFileSharingError);
    }

    void FileSharingMetaContainer::requestMeta(const Utils::FileSharingId& _fsId) const
    {
        Ui::GetDispatcher()->downloadFileSharingMetainfo(_fsId);
    }

    bool FileSharingMetaContainer::isDownloading(const Utils::FileSharingId& _fsId) const
    {
        return waitList_.find(_fsId) != waitList_.end();
    }

    void FileSharingMetaContainer::onMetaInfoDownloaded(qint64 _seq, const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)
    {
        const auto [begin, end] = waitList_.equal_range(_fsId);
        std::vector<MetaCallbacks::ReceiveCallback> callbacks;
        callbacks.reserve(std::distance(begin, end));
        for (auto it = begin; it != end; ++it)
            callbacks.emplace_back(std::move(it->second.successCallback_));
        waitList_.erase(begin, end);
        for (const auto& cb : callbacks)
            cb(_fsId, _meta);
        cache_[_fsId] = _meta;
    }

    void FileSharingMetaContainer::onFileSharingError(qint64 _seq, const Utils::FileSharingId& _fsId, qint32 _errorCode)
    {
        const auto [begin, end] = waitList_.equal_range(_fsId);
        std::vector<MetaCallbacks::ErrorCallback> callbacks;
        callbacks.reserve(std::distance(begin, end));
        for (auto it = begin; it != end; ++it)
            callbacks.emplace_back(std::move(it->second.errorCallback_));
        waitList_.erase(begin, end);
        for (const auto& cb : callbacks)
            cb(_fsId, _errorCode);
    }

    void FileSharingMetaContainer::onFileSharingDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
    {
        const auto fsId = Ui::ComplexMessage::extractIdFromFileSharingUri(_result.rawUri_);
        // force to read new meta from disk
        cache_.erase(fsId);
    }

    void FileSharingMetaContainer::onAntivirusCheckResult(const Utils::FileSharingId& _fsId, core::antivirus::check::result _result)
    {
        auto& meta = cache_[_fsId];
        meta.antivirusCheck_.result_ = _result;
    }

    static std::unique_ptr<FileSharingMetaContainer> g_FileSharingMetaContainer;

    FileSharingMetaContainer* GetFileSharingMetaContainer()
    {
        Utils::ensureMainThread();
        if (!g_FileSharingMetaContainer)
            g_FileSharingMetaContainer = std::make_unique<FileSharingMetaContainer>();

        return g_FileSharingMetaContainer.get();
    }

    void ResetFileSharingMetaContainer()
    {
        Utils::ensureMainThread();
        g_FileSharingMetaContainer.reset();
    }
}
