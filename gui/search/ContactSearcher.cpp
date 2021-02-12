#include "stdafx.h"

#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/contact_list/FavoritesUtils.h"

#include "ContactSearcher.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "utils/utils.h"

namespace
{
    bool compareWithPatternsFromOffset(const QString& _str, int _pos, const std::vector<std::vector<QString>>& _patterns)
    {
        const auto end = std::min(_pos + _patterns.size(), static_cast<size_t>(_str.size()));
        auto match = false;
        for (size_t i = _pos; i < end; i++)
        {
            match = false;

            for (auto& pattern : _patterns[i - _pos])
            {
                if (pattern.size() != 1)
                    continue;

                match = (_str.at(i).toLower() == pattern.at(0).toLower());

                if (match)
                    break;
            }

            if (!match)
                return false;
        }

        return match;
    }

    struct CompareResult
    {
        bool match_ = false;
        size_t from_ = 0;
    };

    CompareResult compareWithPatterns(const QString& _str, const std::vector<std::vector<QString>>& _patterns)
    {
        if (static_cast<size_t>(_str.size()) < _patterns.size())
            return CompareResult{false, 0};

        for (auto i = 0u; i <= _str.size() - _patterns.size(); ++i)
        {
            if (compareWithPatternsFromOffset(_str, i, _patterns))
                return CompareResult{true, i};
        }

        return CompareResult{false, 0};
    }
}


namespace Logic
{
    ContactSearcher::ContactSearcher(QObject * _parent)
        : AbstractSearcher(_parent)
        , excludeChats_(SearchDataSource::none)
        , hideReadonly_(false)
        , forceAddFavorites_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedContactsLocal, this, &ContactSearcher::onLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedContactsServer, this, &ContactSearcher::onServerResults);
    }

    void ContactSearcher::setExcludeChats(SearchDataSource _exclude)
    {
        excludeChats_ = _exclude;
    }

    SearchDataSource ContactSearcher::getExcludeChats() const
    {
        return excludeChats_;
    }

    void ContactSearcher::setHideReadonly(const bool _hide)
    {
        hideReadonly_ = _hide;
    }

    bool ContactSearcher::getHideReadonly() const
    {
        return hideReadonly_;
    }

    const QString &ContactSearcher::getPhoneNumber() const
    {
        return phoneNumber_;
    }

    void ContactSearcher::setPhoneNumber(const QString &_phoneNumber)
    {
        phoneNumber_ = _phoneNumber;
    }

    void ContactSearcher::setForceAddFavorites(bool _enable)
    {
        forceAddFavorites_ = _enable;
    }

    void ContactSearcher::doLocalSearchRequest()
    {
        uint32_t searchPatternsCount = 0;
        const auto needle = platform::is_apple() ? searchPattern_.toLower() : searchPattern_;
        const auto searchPatterns = Utils::GetPossibleStrings(needle, searchPatternsCount);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        if (!searchPatterns.empty())
        {
            core::ifptr<core::iarray> symbolsArray(collection->create_array());
            symbolsArray->reserve(searchPatterns.size());

            for (const auto& patternList : searchPatterns)
            {
                core::coll_helper symbol_coll(collection->create_collection(), true);
                core::ifptr<core::iarray> patternForSymbolArray(symbol_coll->create_array());
                patternForSymbolArray->reserve(patternList.size());

                for (const auto& pattern : patternList)
                    patternForSymbolArray->push_back(collection.create_qstring_value(pattern).get());

                symbol_coll.set_value_as_array("symbols_patterns", patternForSymbolArray.get());
                core::ifptr<core::ivalue> symbols_val(symbol_coll->create_value());
                symbols_val->set_as_collection(symbol_coll.get());
                symbolsArray->push_back(symbols_val.get());
            }
            collection.set_value_as_array("symbols_array", symbolsArray.get());
            collection.set_value_as_uint("fixed_patterns_count", searchPatternsCount);
        }

        if (!needle.isEmpty())
            collection.set_value_as_qstring("init_pattern", needle);

        localReqId_ = Ui::GetDispatcher()->post_message_to_core("contacts/search/local", collection.get());
    }

    void ContactSearcher::doServerSearchRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", searchPattern_);
        collection.set_value_as_qstring("phoneNumber", getPhoneNumber());

        serverReqId_ = Ui::GetDispatcher()->post_message_to_core("contacts/search/server", collection.get());
    }

    void ContactSearcher::onLocalResults(const Data::SearchResultsV& _localResults, qint64 _reqId)
    {
        if (_reqId != localReqId_)
            return;

        localResults_.reserve(localResults_.size() + _localResults.size());

        auto hasFavorites = false;

        for (const auto& res : _localResults)
        {
            if (!res->isContact() && !res->isChat())
                continue;

            hasFavorites |= Favorites::isFavorites(res->getAimId());

            if (const auto ci = getContactListModel()->getContactItem(res->getAimId()))
            {
                if (res->isChat())
                {
                    if (excludeChats_ & SearchDataSource::local)
                        continue;

                    if (hideReadonly_ && ci->is_readonly())
                        continue;
                }

                localResults_.push_back(res);
            }
        }

        if (forceAddFavorites_)
        {
            const auto& favoritesMatchingWords = Favorites::matchingWords();
            unsigned int fixedPatternsCount = 0;
            const auto searchPatterns = Utils::GetPossibleStrings(searchPattern_, fixedPatternsCount);

            auto favoritesMatch = !searchPattern_.isEmpty() && !hasFavorites &&
                    std::any_of(favoritesMatchingWords.begin(), favoritesMatchingWords.end(), [&searchPatterns](const auto& word)
            {
                return compareWithPatterns(word, searchPatterns).match_;
            });

            if (!hasFavorites && searchPattern_.isEmpty() || favoritesMatch)
            {
                if (auto aimId = Favorites::aimId(); !aimId.isEmpty())
                {
                    auto favorites = std::make_shared<Data::SearchResultContactChatLocal>();
                    favorites->aimId_ = std::move(aimId);

                    auto nameResult = compareWithPatterns(Favorites::name(), searchPatterns);

                    if (favoritesMatch && nameResult.match_)
                        favorites->highlights_.push_back(Favorites::name().mid(nameResult.from_, searchPattern_.size()));

                    localResults_.push_back(favorites);
                }
            }
        }

        Q_EMIT localResults();
        onRequestReturned();
    }

    void ContactSearcher::onServerResults(const Data::SearchResultsV& _serverResults, qint64 _reqId)
    {
        if (_reqId != serverReqId_)
            return;

        serverTimeoutTimer_->stop();

        serverResults_.reserve(serverResults_.size() + _serverResults.size());

        for (const auto& res : _serverResults)
        {
            if (res->isChat())
            {
                if (auto chat = std::static_pointer_cast<Data::SearchResultChat>(res))
                    getContactListModel()->updateChatInfo(chat->chatInfo_);

                const auto wasFoundLocally = std::any_of(localResults_.cbegin(), localResults_.cend(),
                    [aimid = res->getAimId()](const auto& localResult) {
                        return localResult->getAimId() == aimid;
                    });
                if (wasFoundLocally || (excludeChats_ & SearchDataSource::server))
                    continue;
            }

            serverResults_.push_back(res);
        }

        Q_EMIT serverResults();
        onRequestReturned();
    }
}
