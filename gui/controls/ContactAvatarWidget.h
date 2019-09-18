#pragma once

namespace Ui
{
    class RotatingSpinner;

    class ContactAvatarWidget : public QWidget
    {
        Q_OBJECT

    protected:
        const int size_;

        QString aimid_;
        QString displayName_;

        void paintEvent(QPaintEvent* _e) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    private:
        struct InfoForSetAvatar
        {
            QString currentDirectory;
            QImage image;
            QRectF croppingRect;
            QPixmap croppedImage;
            QPixmap roundedCroppedImage;
        };

    public:
        enum class Mode
        {
            Common  = 0,
            Introduce,
            MyProfile,
            CreateChat,
            ChangeAvatar,
        };

        bool isDefault();

    Q_SIGNALS:
        void clickedInternal();
        void avatarDidEdit();
        void mouseEntered();
        void mouseLeft();
        void afterAvatarChanged();
        void summonSelectFileForAvatar();
        void leftClicked();
        void rightClicked();
        void cancelSelectFileForAvatar();

    private Q_SLOTS:
        void avatarChanged(const QString&);

        void cropAvatar();
        void openAvatarPreview();

        void avatarEnter();
        void avatarLeave();
        void setAvatar(qint64 _seq, int _error);

    public Q_SLOTS:

        void selectFileForAvatar();

    public:

        ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _displayName, const int _size, const bool _autoUpdate, const bool _officialOnly = false, const bool _isVisibleBadge = true);
        ~ContactAvatarWidget();

        void UpdateParams(const QString& _aimid, const QString& _displayName);
        void SetImageCropHolder(QWidget *_holder);
        void SetImageCropSize(const QSize &_size);
        void SetMode(ContactAvatarWidget::Mode _mode);
        void SetVisibleShadow(bool _isVisibleShadow);
        void SetVisibleSpinner(bool _isVisibleSpinner);
        void SetOutline(bool _isVisibleOutline);

        void applyAvatar(const QPixmap &alter = QPixmap());
        const QPixmap &croppedImage() const;

    private:
        QString GetState();
        void postSetAvatarToCore(const QPixmap& _avatar);
        void ResetInfoForSetAvatar();
        void updateSpinnerPos();

    private:
        QWidget *imageCropHolder_;
        QSize imageCropSize_;
        Mode mode_;
        bool isVisibleShadow_;
        bool isVisibleOutline_;
        bool isVisibleBadge_;
        bool connected_;
        InfoForSetAvatar infoForSetAvatar_;
        qint64 seq_;
        bool defaultAvatar_;
        bool officialBadgeOnly_;
        RotatingSpinner* spinner_;
        bool hovered_;
        bool pressed_;
    };
}
