#pragma once

#include "AbstractSearchModel.h"
#include "types/search_result.h"

namespace Logic
{
    class DisallowedInvitersModel : public AbstractSearchModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void showEmptyListPlaceholder(QPrivateSignal);

    public:
        DisallowedInvitersModel(QObject* _parent = nullptr);
        ~DisallowedInvitersModel() = default;

        void setFocus() override {}

        void setSearchPattern(const QString& _pattern) override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        bool contains(const QString& _aimId) const override;

        void requestMore() const;

        void setForceSearch(bool _force = true) { forceSearch_ = _force; }

        void add(const std::vector<QString>& _contacts);
        void remove(const std::vector<QString>& _contacts);

        void showSpinner() const;
        void hideSpinner() const;

    private:
        void onDataUpdated(const QString& _aimId);
        void onSearchResults(const Data::SearchResultsV& _contacts, const QString& _cursor, bool _resetPage);
        void doSearch() const;
        void clear();
        bool isNeedAddContact() const;

    private:
        Data::SearchResultsV results_;
        QString cursor_;
        QTimer* timerSearch_ = nullptr;
        mutable bool isDoingRequest_ = false;
        bool forceSearch_ = true;
    };
}