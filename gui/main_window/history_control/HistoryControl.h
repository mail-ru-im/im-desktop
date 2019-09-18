#pragma once

#include "../../types/message.h"
#include "../../styles/WallpaperId.h"

namespace core
{
    enum class typing_status;
}

namespace Ui
{
    enum class ConnectionState;
    class ConnectionWidget;
    enum class FrameCountMode;

    class SelectDialogWidget : public QWidget
    {
        Q_OBJECT

    public:
        SelectDialogWidget(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        void connectionStateChanged(const Ui::ConnectionState& _state);
        void onGlobalWallpaperChanged();
        void setTextColor(const QColor& _color);

        ConnectionWidget* connectionWidget_;
        QLabel* selectDialogLabel_;
        QLayout* rootLayout_;
    };



    class history_control;
    class HistoryControlPage;
    class ConnectionWidget;

    class HistoryControl : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void quote(const Data::QuotesVec&) const;
        void forward(const Data::QuotesVec&) const;
        void messageIdsFetched(const QString&, const Data::MessageBuddies&, QPrivateSignal) const;
        void clicked() const;
        void needUpdateWallpaper(const QString& _aimId, QPrivateSignal) const;
        void setTopWidget(const QString, QWidget*, QPrivateSignal) const;

    public Q_SLOTS:
        void contactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId);
        void switchToEmpty();

        void addPageToDialogHistory(const QString& _aimId);
        void clearDialogHistory();
        void switchToPrevDialogPage(const bool _calledFromKeyboard);

    private Q_SLOTS:
        void dlgStates(const QVector<Data::DlgState>& _states);

    public:
        HistoryControl(QWidget* parent);
        ~HistoryControl();

        void cancelSelection();
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        bool hasMessageUnderCursor() const;

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        void scrollHistoryToBottom(const QString& _contact) const;
        void inputTyped();
        const QString& currentAimId() const;
        void setFrameCountMode(FrameCountMode _mode);

    protected:
        void mouseReleaseEvent(QMouseEvent *) override;

    private Q_SLOTS:
        void updatePages();
        void closeDialog(const QString& _aimId);
        void leaveDialog(const QString& _aimId);
        void mainPageChanged();
        void onContactSelected(const QString& _aimId);
        void onBackToChats();

        void updateUnreads();
        void onGlobalWallpaperChanged();
        void onContactWallpaperChanged(const QString& _aimId);
        void onWallpaperAvailable(const Styling::WallpaperId& _id);

    private:
        HistoryControlPage* getCurrentPage() const;
        void updateWallpaper(const QString& _aimId = QString()) const;
        void updateEmptyPageWallpaper() const;

        QMap<QString, HistoryControlPage*> pages_;
        QMap<QString, QTime> times_;
        QString current_;
        QTimer* timer_;
        QStackedWidget* stackPages_;
        QWidget* emptyPage_;
        std::deque<QString> dialogHistory_;
        FrameCountMode frameCountMode_;
        QString unreadText_;
    };
}
