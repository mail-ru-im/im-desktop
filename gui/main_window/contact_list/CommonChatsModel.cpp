#include "stdafx.h"
#include "CommonChatsModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Logic
{
    CommonChatsModel::CommonChatsModel(QWidget* _parent)
        : CustomAbstractListModel(_parent)
        , seq_(-1)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::commonChatsGetResult, this, &CommonChatsModel::onCommonChatsGet);
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &CommonChatsModel::avatarChanged);
    }


    int CommonChatsModel::rowCount(const QModelIndex&) const
    {
        return chats_.size();
    }

    QVariant CommonChatsModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        int cur = index.row();
        if (cur >= rowCount())
            return QVariant();

        return QVariant::fromValue(chats_[cur]);
    }

    void CommonChatsModel::load(const QString& _sn)
    {
        chats_.clear();
        emit dataChanged(index(0), index(rowCount()));

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("sn", _sn);
        seq_ = Ui::GetDispatcher()->post_message_to_core("common_chats/get", collection.get());
    }

    const std::vector<Data::CommonChatInfo>& CommonChatsModel::getChats() const
    {
        return chats_;
    }

    void CommonChatsModel::onCommonChatsGet(const qint64 _seq, const std::vector<Data::CommonChatInfo>& _chats)
    {
        if (seq_ != _seq)
            return;

        chats_ = _chats;
        emit dataChanged(index(0), index(rowCount()));
    }

    void CommonChatsModel::avatarChanged(const QString& _aimid)
    {
        auto iter = std::find_if(chats_.begin(), chats_.end(), [_aimid](const auto& chat)
        {
            return chat.aimid_ == _aimid;
        });

        if (iter != chats_.end())
        {
            auto i = std::distance(chats_.begin(), iter);
            emit dataChanged(index(i), index(i));
        }
    }

    CommonChatsSearchModel::CommonChatsSearchModel(QWidget* _parent, CommonChatsModel* _model)
        : AbstractSearchModel(_parent)
        , model_(_model)
    {

    }

    void CommonChatsSearchModel::setSearchPattern(const QString& _pattern)
    {
        if (currentPattern_ == _pattern)
            return;

        currentPattern_ = _pattern;
        results_.clear();
        if (!model_)
            return;

        const auto& chats = model_->getChats();
        unsigned fixed_patterns_count = 0;
        auto searchPatterns = Utils::GetPossibleStrings(_pattern, fixed_patterns_count);

        const auto addResult = [this](const auto& _item, const std::vector<QString>& _highlights)
        {
            if (std::none_of(results_.cbegin(), results_.cend(), [&_item](const auto& res) { return _item.aimid_ == res->getAimId(); }))
            {
                auto resItem = std::make_shared<Data::SearchResultCommonChat>();
                resItem->info_ = _item;
                resItem->highlights_ = _highlights;

                results_.push_back(std::move(resItem));
            }
        };

        const auto checkSimple = [&addResult](const auto& _item, const auto& _pattern)
        {
            if (_item.friendly_.contains(_pattern, Qt::CaseInsensitive))
            {
                addResult(_item, { _pattern });
                return true;
            }
            return false;
        };

        if (currentPattern_.isEmpty())
        {
            for (const auto& item : chats)
                addResult(item, {});
        }
        else
        {
            auto found = false;
            for (const auto& chat : chats)
            {
                unsigned i = 0;
                for (; i < fixed_patterns_count; ++i)
                {
                    QString pattern;
                    if (searchPatterns.empty())
                    {
                        pattern = currentPattern_;
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
                        if (checkSimple(chat, pattern))
                        {
                            found = true;
                            break;
                        }
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
                while (!searchPatterns.empty() && int(i) < min && !found)
                {
                    pattern.resize(0);
                    for (const auto& iter : searchPatterns)
                    {
                        if (iter.size() > i)
                            pattern += iter.at(i);
                    }

                    if (!pattern.isEmpty())
                    {
                        if (checkSimple(chat, pattern))
                            break;
                    }

                    ++i;
                }
            }
        }

        if (results_.isEmpty())
            emit showNoSearchResults();
        else
            emit hideNoSearchResults();

        emit dataChanged(index(0), index(rowCount()));
    }

    int CommonChatsSearchModel::rowCount(const QModelIndex& _parent) const
    {
        return results_.size();
    }

    QVariant CommonChatsSearchModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (cur >= rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return results_[cur]->getAccessibleName();

        return QVariant::fromValue(results_[cur]);
    }
}
