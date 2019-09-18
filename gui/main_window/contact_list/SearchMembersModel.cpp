#include "stdafx.h"
#include "SearchMembersModel.h"
#include "ChatMembersModel.h"
#include "ContactItem.h"
#include "AbstractSearchModel.h"
#include "ContactListModel.h"
#include "../friendly/FriendlyContainer.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    SearchMembersModel::SearchMembersModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , searchRequested_(false)
        , chatMembersModel_(nullptr)
    {
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &SearchMembersModel::avatarLoaded);
    }

    int SearchMembersModel::rowCount(const QModelIndex &) const
    {
        return results_.size();
    }

    QVariant SearchMembersModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        const int cur = _index.row();
        if (cur >= rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return results_[cur]->getAccessibleName();

        return QVariant::fromValue(results_[cur]);
    }

    void SearchMembersModel::setFocus()
    {
        results_.clear();
    }

    void SearchMembersModel::setSearchPattern(const QString& _p)
    {
        results_.clear();

        searchPattern_ = _p;

        if (!chatMembersModel_)
            return;

        //!! todo: temporarily for voip
        auto model = qobject_cast<Logic::ChatMembersModel*>(chatMembersModel_);
        const auto& members = model->getMembers();
        if (members.empty())
            return;

        emit hideNoSearchResults();

        const auto addResult = [this](const auto& _item, const std::vector<QString>& _highlights)
        {
            if (std::none_of(results_.cbegin(), results_.cend(), [&_item](const auto& res) { return _item.AimId_ == res->getAimId(); }))
            {
                auto resItem = std::make_shared<Data::SearchResultChatMember>();
                resItem->info_ = _item;
                resItem->highlights_ = _highlights;

                results_.push_back(std::move(resItem));
            }
        };

        const auto match_whole = [](const auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            auto source_iter = source.cbegin();
            auto search_iter = search.cbegin();

            while (source_iter != source.cend() && search_iter != search.cend())
            {
                if (!source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        const auto reverse_and_match_first_chars = [](const auto& source, const auto& search)
        {
            if (int(source.size()) != int(search.size()))
                return false;

            auto source_iter = source.crbegin();
            auto search_iter = search.crbegin();

            while (source_iter != source.crend() && search_iter != search.crend())
            {
                if (search_iter->length() > 1 || !source_iter->startsWith(*search_iter, Qt::CaseInsensitive))
                    return false;

                ++source_iter;
                ++search_iter;
            }

            return true;
        };

        const auto check_words = [&addResult, &match_whole, &reverse_and_match_first_chars](const auto& pattern, const auto& item, const auto& displayName)
        {
            if (const auto words = pattern.splitRef(QChar::Space, QString::SkipEmptyParts); words.size() > 1)
            {
                const auto displayWords = displayName.splitRef(QChar::Space, QString::SkipEmptyParts);
                if (match_whole(displayWords, words) || reverse_and_match_first_chars(displayWords, words))
                {
                    std::vector<QString> highlights;
                    for (const auto& s : words)
                        highlights.push_back(s.toString());

                    addResult(item, highlights);

                    return true;
                }
            }

            return false;
        };

        const auto checkSimple = [&addResult](const auto& _item, const auto& _pattern, const auto& _displayName, const auto& _nickName)
        {
            if (_item.AimId_.contains(_pattern, Qt::CaseInsensitive) || _displayName.contains(_pattern, Qt::CaseInsensitive) || _nickName.contains(_pattern, Qt::CaseInsensitive))
            {
                addResult(_item, { _pattern });
                return true;
            }
            return false;
        };

        const auto checkNickDog = [&addResult](const auto& _item, const auto& _pattern, const auto& _nickName)
        {
            if (_pattern.length() > 1 && _pattern.startsWith(ql1c('@')))
            {
                const auto withoutDog = _pattern.midRef(1);
                if (_nickName.startsWith(withoutDog, Qt::CaseInsensitive) || _item.AimId_.startsWith(withoutDog, Qt::CaseInsensitive))
                {
                    addResult(_item, { _pattern.mid(1) });
                    return true;
                }
            }
            return false;
        };

        if (_p.isEmpty())
        {
            for (const auto& item : members)
                addResult(item, {});
        }
        else
        {
            QString sp = _p.toLower();
            unsigned fixed_patterns_count = 0;
            auto searchPatterns = Utils::GetPossibleStrings(sp, fixed_patterns_count);
            for (const auto& item : members)
            {
                bool founded = false;
                const auto displayName = Logic::GetFriendlyContainer()->getFriendly(item.AimId_).toLower();
                const auto nickName = Logic::GetFriendlyContainer()->getNick(item.AimId_).toLower();
                unsigned i = 0;
                for (; i < fixed_patterns_count; ++i)
                {
                    QString pattern;
                    if (searchPatterns.empty())
                    {
                        pattern = sp;
                    }
                    else
                    {
                        pattern.reserve(searchPatterns.size());
                        for (const auto& iter : searchPatterns)
                        {
                            if (iter.size() > i)
                                pattern += iter.at(i);
                        }
                        pattern = std::move(pattern).trimmed();
                    }

                    if (!pattern.isEmpty())
                    {
                        if (checkSimple(item, pattern, displayName, nickName) || checkNickDog(item, pattern, nickName) || check_words(pattern, item, displayName))
                            founded = true;
                    }
                }

                int min = 0;
                for (const auto& s : searchPatterns)
                {
                    if (min == 0)
                        min = int(s.size());
                    else
                        min = std::min(min, int(s.size()));
                }

                QString pattern;
                pattern.reserve(searchPatterns.size());
                while (!searchPatterns.empty() && int(i) < min && !founded)
                {
                    pattern.resize(0);
                    for (const auto& iter : searchPatterns)
                    {
                        if (iter.size() > i)
                            pattern += iter.at(i);
                    }

                    pattern = std::move(pattern).toLower();

                    if (!pattern.isEmpty())
                    {
                        if (checkSimple(item, pattern, displayName, nickName) || checkNickDog(item, pattern, nickName) || check_words(pattern, item, displayName))
                            break;
                    }

                    ++i;
                }
            }
        }

        emit dataChanged(index(0), index(rowCount()));

        if (!searchPattern_.isEmpty() && results_.empty())
            emit showNoSearchResults();
        else
            emit hideNoSearchResults();
    }

    void SearchMembersModel::setChatMembersModel(CustomAbstractListModel* _chatMembersModel)
    {
        chatMembersModel_ = _chatMembersModel;
    }

    void SearchMembersModel::avatarLoaded(const QString& _aimId)
    {
        int i = 0;
        for (const auto& item : std::as_const(results_))
        {
            if (item->getAimId() == _aimId)
            {
                const auto idx = index(i);
                emit dataChanged(idx, idx);
                break;
            }
            ++i;
        }
    }
}
