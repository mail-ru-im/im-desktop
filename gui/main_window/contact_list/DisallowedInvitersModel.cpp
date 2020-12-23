#include "stdafx.h"

#include "DisallowedInvitersModel.h"
#include "cache/avatars/AvatarStorage.h"
#include "core_dispatcher.h"
#include "main_window/containers/LastseenContainer.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/containers/FriendlyContainer.h"

#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto pageSize = 1000U;
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(300);
}

namespace Logic
{
    DisallowedInvitersModel::DisallowedInvitersModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , timerSearch_(new QTimer(this))
    {
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &DisallowedInvitersModel::onDataUpdated);
        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, &DisallowedInvitersModel::onDataUpdated);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, &DisallowedInvitersModel::onDataUpdated);
        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, &DisallowedInvitersModel::onDataUpdated);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::inviterBlacklistSearchResults, this, &DisallowedInvitersModel::onSearchResults);

        timerSearch_->setSingleShot(true);
        timerSearch_->setInterval(typingTimeout);
        connect(timerSearch_, &QTimer::timeout, this, &DisallowedInvitersModel::doSearch);
    }

    void DisallowedInvitersModel::setSearchPattern(const QString& _pattern)
    {
        const auto prevPattern = std::move(searchPattern_);
        searchPattern_ = _pattern.trimmed();

        if (searchPattern_ != prevPattern || forceSearch_)
        {
            forceSearch_ = false;

            clear();

            if (searchPattern_.isEmpty())
                doSearch();
            else
                timerSearch_->start();
        }
    }

    void DisallowedInvitersModel::onDataUpdated(const QString& _aimId)
    {
        if (_aimId.isEmpty())
            return;

        int i = 0;
        for (const auto& item : std::as_const(results_))
        {
            if (item->getAimId() == _aimId)
            {
                const auto idx = index(i);
                Q_EMIT dataChanged(idx, idx);
                break;
            }
            ++i;
        }
    }

    void DisallowedInvitersModel::onSearchResults(const Data::SearchResultsV& _contacts, const QString& _cursor, bool _resetPage)
    {
        if (_resetPage)
            cursor_.clear();

        if (cursor_.isEmpty())
            results_.clear();

        cursor_ = _cursor;

        results_.reserve(results_.size() + _contacts.size());
        if (searchPattern_.isEmpty())
        {
            results_.append(_contacts);
        }
        else
        {
            for (const auto& c : _contacts)
            {
                results_.push_back(c);
                results_.back()->highlights_ = { searchPattern_ };
            }
        }

        isDoingRequest_ = false;

        hideSpinner();
        if (rowCount() > 0)
            Q_EMIT hideNoSearchResults();
        else if (!searchPattern_.trimmed().isEmpty())
            Q_EMIT showNoSearchResults();
        else
            Q_EMIT showEmptyListPlaceholder(QPrivateSignal());

        emitChanged(0, rowCount());
    }

    void DisallowedInvitersModel::doSearch() const
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", searchPattern_);
        collection.set_value_as_qstring("cursor", cursor_);
        collection.set_value_as_uint("page_size", pageSize);
        Ui::GetDispatcher()->post_message_to_core("group/invitebl/search", collection.get());

        if (results_.isEmpty())
            showSpinner();
        isDoingRequest_ = true;
    }

    void DisallowedInvitersModel::clear()
    {
        timerSearch_->stop();
        cursor_.clear();
        results_.clear();
        isDoingRequest_ = false;

        emitChanged(0, rowCount());
        Q_EMIT hideNoSearchResults();
        hideSpinner();
    }

    bool DisallowedInvitersModel::isNeedAddContact() const
    {
        return !results_.empty() && QStringView(searchPattern_).trimmed().isEmpty();
    }

    QVariant DisallowedInvitersModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (isNeedAddContact())
        {
            if (cur == 0)
            {
                const auto makeIcon = [](const QString& _icon, const Styling::StyleVariable _var)
                {
                    return Utils::renderSvg(_icon, Utils::scale_value(QSize(24, 24)), Styling::getParameters().getColor(_var));
                };

                const auto dropDownBtn = [&makeIcon](QString _aimId, QString _friendly, const QString& _icon)
                {
                    Data::Contact hdr;
                    hdr.AimId_ = std::move(_aimId);
                    hdr.Friendly_ = std::move(_friendly);

                    hdr.iconPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY);
                    hdr.iconHoverPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY_HOVER);
                    hdr.iconPressedPixmap_ = makeIcon(_icon, Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
                    hdr.setType(Data::ContactType::DROPDOWN_BUTTON);
                    return hdr;
                };

                if (Testing::isAccessibleRole(_role))
                    return qsl("add_contact");

                static auto hdr = dropDownBtn(qsl("~addContact~"), QT_TRANSLATE_NOOP("settings", "Add to list"), qsl(":/header/add_user"));
                return QVariant::fromValue(&hdr);
            }

            cur--;
        }

        if (cur >= rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return results_[cur]->getAccessibleName();

        return QVariant::fromValue(results_[cur]);
    }

    int DisallowedInvitersModel::rowCount(const QModelIndex& _parent) const
    {
        return results_.size() + (isNeedAddContact() ? 1 : 0);
    }

    bool DisallowedInvitersModel::contains(const QString& _aimId) const
    {
        return std::any_of(results_.cbegin(), results_.cend(), [&_aimId](const auto& res) { return res->getAimId() == _aimId; });
    }

    void DisallowedInvitersModel::requestMore() const
    {
        if (!isDoingRequest_ && !cursor_.isEmpty())
            doSearch();
    }

    void DisallowedInvitersModel::add(const std::vector<QString>& _contacts)
    {
        if (_contacts.empty())
            return;

        const auto prevSize = results_.size();
        for (const auto& c : _contacts)
        {
            if (!contains(c))
            {
                auto resItem = std::make_shared<Data::SearchResultContactChatLocal>();
                resItem->aimId_ = c;
                resItem->isChat_ = Utils::isChat(resItem->aimId_);
                results_.push_back(std::move(resItem));
            }
        }

        if (results_.size() != prevSize)
        {
            emitChanged(0, rowCount());
            Q_EMIT hideNoSearchResults();
        }
    }

    void DisallowedInvitersModel::remove(const std::vector<QString>& _contacts)
    {
        const auto prevSize = results_.size();
        for (const auto& c : _contacts)
        {
            auto it = std::find_if(results_.begin(), results_.end(), [&c](const auto& res) { return res->getAimId() == c; });
            if (it != results_.end())
                results_.erase(it);
        }

        if (results_.size() != prevSize)
        {
            emitChanged(0, rowCount());
            if (results_.size() == 0)
            {
                if (!searchPattern_.trimmed().isEmpty())
                    Q_EMIT showNoSearchResults();
                else
                    Q_EMIT showEmptyListPlaceholder(QPrivateSignal());
            }
        }
    }

    void DisallowedInvitersModel::showSpinner() const
    {
        Q_EMIT hideNoSearchResults();
        Q_EMIT showSearchSpinner();
    }

    void DisallowedInvitersModel::hideSpinner() const
    {
        Q_EMIT hideSearchSpinner();
    }
}