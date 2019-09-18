#include "stdafx.h"

#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/RecentsModel.h"

#include "ContactSearcher.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "utils/utils.h"

namespace Logic
{
    ContactSearcher::ContactSearcher(QObject * _parent)
        : AbstractSearcher(_parent)
        , excludeChats_(false)
        , hideReadonly_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedContactsLocal, this, &ContactSearcher::onLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedContactsServer, this, &ContactSearcher::onServerResults);
    }

    void ContactSearcher::setExcludeChats(const bool _exclude)
    {
        excludeChats_ = _exclude;
    }

    bool ContactSearcher::getExcludeChats() const
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

        for (const auto& res : _localResults)
        {
            if (!res->isContact() && !res->isChat())
                continue;

            if (const auto ci = getContactListModel()->getContactItem(res->getAimId()))
            {
                if (res->isChat())
                {
                    if (excludeChats_)
                        continue;

                    if (hideReadonly_ && ci->is_readonly())
                        continue;
                }

                localResults_.push_back(res);
            }
        }

        emit localResults();
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
            const auto ci = getContactListModel()->getContactItem(res->getAimId());

            if (res->isChat())
            {
                if (ci || excludeChats_)
                    continue;
            }

            serverResults_.push_back(res);
        }

        emit serverResults();
        onRequestReturned();
    }
}
