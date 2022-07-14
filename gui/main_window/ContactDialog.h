#pragma once

#include "controls/BackgroundWidget.h"
#include "main_window/EscapeCancellable.h"

namespace Ui
{
    using highlightsV = std::vector<QString>;

    class HistoryControl;
    class HistoryControlPage;

    enum class FrameCountMode;
    enum class PageOpenedAs;

    class ContactDialog : public BackgroundWidget, public IEscapeCancellable
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll = false);

    private:
        HistoryControl* historyControlWidget_;
        QStackedWidget* topWidget_ = nullptr;
        QMap<QString, QWidget*> topWidgetsCache_;

        void initTopWidget();

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

    public:
        explicit ContactDialog(QWidget* _parent);
        ~ContactDialog();

        void cancelSelection();
        void onSwitchedToEmpty();

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        void setFrameCountMode(FrameCountMode _mode);
        FrameCountMode getFrameCountMode() const;

        const QString& currentAimId() const;

        void setPageOpenedAs(PageOpenedAs _openedAs);

    private Q_SLOTS:
        void onGlobalThemeChanged();
    };
}
