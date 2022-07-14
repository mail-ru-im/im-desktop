#pragma once

#include "controls/ClickWidget.h"
#include "types/smartreply_suggest.h"

namespace Ui
{
    class LottiePlayer;

    class SmartReplyItem : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void selected(const Data::SmartreplySuggest& _suggest, QPrivateSignal) const;
        void deleteMe() const;
        void mouseMoved(QPoint _point) const;
        void showPreview(const Data::SmartreplySuggest::Data& _data) const;
        void hidePreview() const;

    public:
        SmartReplyItem(QWidget* _parent, const Data::SmartreplySuggest& _suggest);

        void setFlat(const bool _flat);
        bool isFlat() const noexcept { return isFlat_; }

        void setUnderlayVisible(const bool _visible);
        bool isUnderlayVisible() const noexcept { return needUnderlay_; }

        void setIsDeleted(const bool _deleted) { isDeleted_ = _deleted; }
        bool isDeleted() const noexcept { return isDeleted_; }

        void setIsPreviewVisible(const bool _visible) { isPreviewVisible_ = _visible; }
        bool isPreviewVisible() const noexcept { return isPreviewVisible_; }

        virtual void setHasMargins(const bool _hasLeft, const bool _hasRight) {}

        int64_t getMsgId() const noexcept;
        const Data::SmartreplySuggest& getSuggest() const noexcept { return suggest_; }

        virtual void onVisibilityChanged(bool _visible) {}

    protected:
        Data::SmartreplySuggest suggest_;

    private:
        void onClicked() const;

    private:
        bool isFlat_ = false;
        bool needUnderlay_ = false;
        bool isDeleted_ = false;
        bool isPreviewVisible_ = false;
    };

    class SmartReplySticker : public SmartReplyItem
    {
        Q_OBJECT

    public:
        SmartReplySticker(QWidget* _parent, const Data::SmartreplySuggest& _suggest);
        void onVisibilityChanged(bool _visible) override;

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        QColor getUnderlayColor();
        bool prepareImage();
        const Utils::FileSharingId& getId() const;

        void onStickerLoaded(int _error, const Utils::FileSharingId& _id);

    protected:
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;

    private:
        QPixmap image_;
        QTimer longtapTimer_;
        LottiePlayer* lottie_ = nullptr;
    };

    class SmartReplyText : public SmartReplyItem
    {
        Q_OBJECT

    public:
        SmartReplyText(QWidget* _parent, const Data::SmartreplySuggest& _suggest);

        void setHasMargins(const bool _hasLeft, const bool _hasRight) override;

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        Styling::ThemeColorKey getTextColor() const;
        QColor getBubbleColor() const;

        void updateColors();
        void recalcSize();
        QSize getBubbleSize() const;

    private:
        std::unique_ptr<TextRendering::TextUnit> textUnit_;
        QPainterPath bubble_;

        bool hasMarginLeft_;
        bool hasMarginRight_;
    };

    std::unique_ptr<SmartReplyItem> makeSmartReplyItem(const Data::SmartreplySuggest& _suggest);
}