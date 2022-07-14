#pragma once

namespace Utils
{
    class PersonTooltip;
}

namespace Ui
{
    class RotatingSpinner;

    class ContactAvatarWidget : public QWidget
    {
        Q_OBJECT
        friend class AccessibleAvatarBadge;

    protected:
        const int size_;

        QString aimid_;
        QString displayName_;

        void paintEvent(QPaintEvent* _e) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void dragEnterEvent(QDragEnterEvent*) override;
        void dropEvent(QDropEvent*) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

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
            ChangeStatus,
            StatusPicker
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
        void badgeClicked();
        void openSettingsClicked();
        void cancelSelectFileForAvatar();
        void avatarSetToCore();

    private Q_SLOTS:
        void avatarChanged(const QString&);

        void cropAvatar(bool _drop = false);
        void openAvatarPreview();

        void avatarEnter();
        void avatarLeave();
        void setAvatar(qint64 _seq, int _error);
        void onAddStatusTooltipTimer();
        void openSettings() const;

    public Q_SLOTS:

        void selectFileForAvatar();

    public:

        ContactAvatarWidget(QWidget* _parent,
                            const QString& _aimid,
                            const QString& _displayName,
                            const int _size,
                            const bool _autoUpdate,
                            const bool _officialOnly = false,
                            const bool _isVisibleBadge = true);

        ~ContactAvatarWidget();

        void UpdateParams(const QString& _aimid, const QString& _displayName);
        const QString& getAimId() const noexcept { return aimid_; }

        void SetImageCropHolder(QWidget *_holder);
        void SetImageCropSize(const QSize &_size);

        void SetMode(ContactAvatarWidget::Mode _mode, bool _forNavBar = false);
        ContactAvatarWidget::Mode getMode() const noexcept { return mode_; }

        void SetVisibleShadow(bool _isVisibleShadow);
        void SetVisibleSpinner(bool _isVisibleSpinner);
        void SetOutline(bool _isVisibleOutline);
        void SetSmallBadge(bool _small);
        void SetOffset(int _offset);
        void setAvatarProxyFlags(int32_t _flags);

        void applyAvatar(const QPixmap &alter = QPixmap());
        const QPixmap &croppedImage() const;

        void setIgnoreClicks(bool _ignore);
        void setStatusTooltipEnabled(bool _enabled);
        void setProfileTooltipEnabled(bool _enabled);

    private:
        void postSetAvatarToCore(const QPixmap& _avatar);
        void ResetInfoForSetAvatar();
        void updateSpinnerPos();
        void selectFile(const QString& _url = QString(), bool _drop = false);
        bool badgeRectUnderCursor() const;
        void openStatusPicker() const;
        QRect tooltipRect() const;

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
        bool badgeHovered_;
        bool pressed_;
        bool smallBadge_;
        bool displayNameChanged_;
        int offset_;
        int avatarProxyFlags_ = 0;
        QRect badgeRect_;
        Utils::PersonTooltip* personTooltip_;
        bool ignoreClicks_ = false;
        bool statusTooltipEnabled_ = false;
        bool profileTooltipEnabled_ = false;
        QTimer* addStatusTooltipTimer_ = nullptr;
    };

    class AccessibleAvatarBadge : public QAccessibleObject
    {
    public:
        AccessibleAvatarBadge(ContactAvatarWidget* _avatar) : QAccessibleObject(_avatar), avatarWidget_(_avatar) {}
        bool isValid() const override { return true; }
        QObject* object() const override { return nullptr; }
        QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(avatarWidget_); }
        int childCount() const override { return 0; }
        QAccessibleInterface* child(int index) const override { return nullptr; }
        int indexOfChild(const QAccessibleInterface* child) const override { return -1; }
        QRect rect() const override;
        QString text(QAccessible::Text type) const override;
        QAccessible::Role role() const override { return QAccessible::Role::Button; }
        QAccessible::State state() const override { return {}; }

    private:
        ContactAvatarWidget* avatarWidget_;
    };

    class AccessibleAvatarWidget : public QAccessibleWidget
    {
    public:
        AccessibleAvatarWidget(ContactAvatarWidget* _avatarWidget);

        int childCount() const override { return 1; }
        QAccessibleInterface* child(int index) const override;
        int indexOfChild(const QAccessibleInterface* child) const override;
        QString	text(QAccessible::Text type) const override;

    private:
        std::unique_ptr<AccessibleAvatarBadge> badgeInterface_;
    };
}
