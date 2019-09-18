#pragma once

#include "CustomAbstractListModel.h"

namespace Logic
{
    class AbstractSearchModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void results() const;
        void showSearchSpinner() const;
        void hideSearchSpinner() const;
        void showNoSearchResults() const;
        void hideNoSearchResults() const;

    public Q_SLOTS:
        void repeatSearch();

    public:
        AbstractSearchModel(QObject* _parent = nullptr);

        virtual void setFocus() {}

        virtual void setSearchPattern(const QString& _p) = 0;
        QString getSearchPattern() const;

        bool isServerSearchEnabled() const noexcept { return isServerSearchEnabled_; };
        void setServerSearchEnabled(const bool _enabled) noexcept { isServerSearchEnabled_ = _enabled; };

    protected:
        QString searchPattern_;
        bool isServerSearchEnabled_;
    };
}
