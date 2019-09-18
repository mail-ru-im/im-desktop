#pragma once

#include "chat.h"
#include "message.h"

namespace Logic
{
    class contact_profile;
    using profile_ptr = std::shared_ptr<contact_profile>;
}

namespace Ui
{
    using highlightsV = std::vector<QString>;
}

namespace Data
{
    enum class SearchResultType
    {
        service,
        contact,
        chat,
        message,
        suggest
    };

    struct AbstractSearchResult
    {
        virtual ~AbstractSearchResult() = default;
        virtual SearchResultType getType() const = 0;
        virtual QString getAimId() const = 0;
        virtual QString getSenderAimId() const { return getAimId(); }
        virtual QString getFriendlyName() const = 0;
        virtual QString getNick() const { return QString(); }
        virtual QString getAccessibleName() const { return getAimId(); }
        virtual qint64 getMessageId() const { return -1; }

        bool isService() const { return getType() == SearchResultType::service; }
        bool isContact() const { return getType() == SearchResultType::contact; }
        bool isChat() const { return getType() == SearchResultType::chat; }
        bool isMessage() const { return getType() == SearchResultType::message; }
        bool isSuggest() const { return getType() == SearchResultType::suggest; }

        bool dialogSearchResult_ = false;
        bool isLocalResult_ = true;
        int searchPriority_ = 0;
        Ui::highlightsV highlights_;
    };
    using AbstractSearchResultSptr = std::shared_ptr<AbstractSearchResult>;
    using SearchResultsV = QVector<AbstractSearchResultSptr>;

    struct SearchResultContact : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::contact; }
        QString getAimId() const override;
        QString getFriendlyName() const override;
        QString getNick() const override;

        Logic::profile_ptr profile_;
    };

    struct SearchResultChat : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::chat; }
        QString getAimId() const override;
        QString getFriendlyName() const override;

        Data::ChatInfo chatInfo_;
    };
    using ChatSearchResultSptr = std::shared_ptr<SearchResultChat>;

    struct SearchResultContactChatLocal : public AbstractSearchResult
    {
        SearchResultType getType() const override { return isChat_ ? SearchResultType::chat : SearchResultType::contact; }
        QString getAimId() const override { return aimId_; }
        QString getFriendlyName() const override;
        QString getNick() const override;

        QString aimId_;
        bool isChat_ = false;
    };

    struct SearchResultMessage : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::message; }
        QString getAimId() const override;
        QString getSenderAimId() const override;
        QString getFriendlyName() const override;
        qint64 getMessageId() const override;

        MessageBuddySptr message_;

        QString dialogAimId_;
        QString dialogCursor_;
        int dialogEntryCount_ = -1;
    };
    using MessageSearchResultSptr = std::shared_ptr<SearchResultMessage>;

    struct SearchResultService : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::service; }
        QString getAimId() const override { return serviceAimId_; }
        QString getFriendlyName() const override { return serviceText_; }
        QString getAccessibleName() const override { return accessibleName_; }

        QString serviceAimId_;
        QString serviceText_;
        QString accessibleName_;
    };
    using ServiceSearchResultSptr = std::shared_ptr<SearchResultService>;

    struct SearchResultSuggest : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::suggest; }
        QString getAimId() const override { return suggestAimId_; }
        QString getFriendlyName() const override { return suggestText_; }

        QString suggestAimId_;
        QString suggestText_;
    };
    using SuggestSearchResultSptr = std::shared_ptr<SearchResultSuggest>;

    struct SearchResultChatMember : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::contact; }
        QString getAimId() const override;
        QString getFriendlyName() const override;
        QString getNick() const override;
        QString getRole() const;
        int32_t getLastseen() const;
        bool isCreator() const;

        Data::ChatMemberInfo info_;
    };
    using ChatMemberSearchResultSptr = std::shared_ptr<SearchResultChatMember>;

    struct SearchResultCommonChat : public AbstractSearchResult
    {
        SearchResultType getType() const override { return SearchResultType::chat; }
        QString getAimId() const override;
        QString getFriendlyName() const override;

        Data::CommonChatInfo info_;
    };
    using CommonChatSearchResultSptr = std::shared_ptr<SearchResultCommonChat>;
}

Q_DECLARE_METATYPE(Data::AbstractSearchResultSptr);
Q_DECLARE_METATYPE(Data::ChatSearchResultSptr);
Q_DECLARE_METATYPE(Data::MessageSearchResultSptr);
Q_DECLARE_METATYPE(Data::SearchResultsV);
