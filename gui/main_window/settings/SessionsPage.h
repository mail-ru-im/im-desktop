#pragma once

#include "controls/SimpleListWidget.h"
#include "types/session_info.h"

namespace Ui
{
    class TextWidget;

    class SessionWidget : public SimpleListItem
    {
        Q_OBJECT

    Q_SIGNALS:
        void closeSession(const QString& _hash, const QString& _name, QPrivateSignal) const;

    public:
        SessionWidget(const Data::SessionInfo& _info, QWidget* _parent = nullptr);
        void init(const Data::SessionInfo& _info);

        QString getName() const;
        QString getText() const;
        const QString& getHash() const noexcept { return info_.getHash(); }

        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent* event) override;
        void onResize();

    private:
        TextWidget* caption_ = nullptr;
        TextWidget* text_ = nullptr;
        Data::SessionInfo info_;
    };

    class SessionsPage : public QWidget
    {
        Q_OBJECT

    public:
        SessionsPage(QWidget* _parent);

    private:
        void updateSessions(const std::vector<Data::SessionInfo>& _sessions);
        void onSessionReset(const QString&, bool _success);

        void onResetAllClicked();
        void resetSession(int _idx);
        void resetSessionImpl(const QString& _hash);

        void copyCurSessionInfo();
        void copySessionInfo(int _idx);
        void copySessionInfoImpl(SessionWidget* _widget);

    private:
        SessionWidget* currentSession_;
        SimpleListWidget* sessionsList_;
        QWidget* activeSessionsWidget_;
    };
}