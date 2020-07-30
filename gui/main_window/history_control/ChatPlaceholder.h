#pragma once

namespace Ui
{
    enum class PlaceholderState
    {
        hidden,
        avatar,

        privateChat,

        needJoin,
        pending,
        rejected,
        canceled,
    };

    class ChatPlaceholderAvatar : public QWidget
    {
        Q_OBJECT

    public:
        ChatPlaceholderAvatar(QWidget* _parent, const QString& _contact);

        const QString& getContact() const noexcept { return contact_; }
        void updatePixmap();

        void setState(PlaceholderState _state);
        PlaceholderState getState() const noexcept { return state_; }

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        QPixmap pixmap_;
        QString contact_;
        PlaceholderState state_ = PlaceholderState::hidden;
    };

    class ChatPlaceholder : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized(QPrivateSignal) const;

    public:
        ChatPlaceholder(QWidget* _parent, const QString& _aimId);

        void updateCaptions();
        void updateStyle();

        void setState(PlaceholderState _state);
        PlaceholderState getState() const;

    private:
        struct Caption
        {
            QString caption_;
            QString accessibleName_;
        };
        void setCaptions(std::vector<Caption> _captions);
        void clearCaptions();

        void updateCaptionsChat();
        void updateCaptionsContact();

    protected:
        void resizeEvent(QResizeEvent*) override;

    private:
        ChatPlaceholderAvatar* avatar_;
        QString contactAbout_;
        bool infoPending_ = false;

        std::vector<Caption> captions_;
    };
}