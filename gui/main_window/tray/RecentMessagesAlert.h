#pragma once

#include "../../types/message.h"

class QPainter;


namespace Ui
{
    class RecentItemDelegate;
    class CustomButton;

    enum class AlertType
    {
        alertTypeMessage = 0,
        alertTypeEmail = 1,
        alertTypeMentionMe = 2
    };

    class TextEmojiWidget;

    class ViewAllWidget : public QWidget
    {
        Q_OBJECT

        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void paintEvent(QPaintEvent*) override;

    Q_SIGNALS:

        void clicked();

    public:

        ViewAllWidget(QWidget* _parent);
        virtual ~ViewAllWidget();

    private:

        bool selected_;
    };

    class RecentMessagesAlert : public QWidget
    {
        Q_OBJECT
        Q_SIGNALS:

        void messageClicked(const QString& _aimId, const QString& _mailId, const qint64 _mentionId, const AlertType _alertType);
        void changed();

    public:
        RecentMessagesAlert(const AlertType _alertType);
        ~RecentMessagesAlert();

        void addAlert(const Data::DlgState& state);
        void markShowed();
        bool updateMailStatusAlert(const Data::DlgState& state);

    protected:
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    private:
        void init();
        bool isMailAlert() const;
        bool isMessageAlert() const;
        bool isMentionAlert() const;

    private Q_SLOTS:
        void closeAlert();
        void statsCloseAlert();
        void viewAll();
        void startAnimation();
        void messageAlertClicked(const QString&, const QString&, qint64);
        void messageAlertClosed(const QString&, const QString&, qint64);

    private:

        QVBoxLayout* layout_;
        unsigned alertsCount_;
        CustomButton* closeButton_;
        ViewAllWidget* viewAllWidget_;
        QTimer* timer_;
        QPropertyAnimation* animation_;
        int height_;

        unsigned maxAlertCount_;
        bool cursorIn_;
        bool viewAllWidgetVisible_;

        AlertType alertType_;
    };
}

Q_DECLARE_METATYPE(Ui::AlertType);
