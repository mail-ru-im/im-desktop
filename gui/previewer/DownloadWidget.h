#pragma once

namespace Ui
{
    class ActionButtonWidget;
}

namespace Previewer
{
    class DownloadWidget
        : public QFrame
    {
        Q_OBJECT
    public:
        explicit DownloadWidget(QWidget* _parent);

        void startLoading();
        void stopLoading();

    Q_SIGNALS:
        void cancelDownloading();
        void tryDownloadAgain();
        void clicked();

    public Q_SLOTS:
        void onDownloadingError();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;

    private:
        void setupLayout();

    private:
        bool isLayoutSetted_;

        QFrame* holder_;
        QSpacerItem* topSpacer_;
        Ui::ActionButtonWidget* button_;
        QSpacerItem* textSpacer_;
        QLabel* errorMessage_;
    };
}
