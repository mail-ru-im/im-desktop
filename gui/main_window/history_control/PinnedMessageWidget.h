#pragma once

#include "../../types/message.h"
#include "../../controls/TextUnit.h"
#include "complex_message/ComplexMessageItem.h"

namespace Ui
{
    namespace ComplexMessage
    {
        class ComplexMessageItem;
        enum class PinPlaceholderType;
    }

    class CollapsableWidget : public QWidget
    {
        Q_OBJECT

    public:
        CollapsableWidget(QWidget* _parent, const QString& _tooltipText);

    protected:
        QTimer tooltipTimer_;
        QString tooltipText_;

        virtual bool isHovered() const = 0;
        enum class ArrowDirection
        {
            up,
            down
        };
        virtual ArrowDirection getArrowDirection() const = 0;

        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void leaveEvent(QEvent*) override;
        void hideEvent(QHideEvent*) override;

        void showTooltip();
        void hideTooltip();
        void drawCollapseButton(QPainter& _painter, const ArrowDirection _dir);
    };


    class FullPinnedMessage : public CollapsableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void collapseClicked();

    private Q_SLOTS:
        void makePreview(const QPixmap& _image);
        void updateText();

    public:
        FullPinnedMessage(QWidget* _parent);

        bool setMessage(Data::MessageBuddySptr _msg);
        void clear();

        void updateStyle();

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void hideEvent(QHideEvent*) override;

        bool isHovered() const override;
        ArrowDirection getArrowDirection() const override;

    private:
        void drawSimpleSnippet(QPainter& _painter);
        void drawMediaSnippet(QPainter& _painter);

        void makeEmptyPreviewSnippet(const QString& _placeholder);

        void setPreview(const QPixmap& _image);

        void openMedia();

        bool isSnippetSimple() const;
        bool isImage() const;
        bool isPlayable() const;

        int getTextOffsetLeft() const;

        QSize getMediaSnippetSize() const;
        QRect getMediaRect() const;

        void updateTextOffsets();

    private:
        std::unique_ptr<Ui::ComplexMessage::ComplexMessageItem> complexMessage_;

        Ui::ComplexMessage::PinPlaceholderType previewType_;

        QPixmap snippet_;
        QPixmap snippetHover_;
        int64_t requestId_;
        std::vector<QMetaObject::Connection> connections_;

        TextRendering::TextUnitPtr sender_;
        TextRendering::TextUnitPtr text_;

        bool hovered_;
        bool snippetHovered_;

        bool snippetPressed_;
        bool collapsePressed_;

        common::tools::patch_version patchVersion_;
    };

    //----------------------------------------------------------------------
    class CollapsedPinnedMessage : public CollapsableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();

    public:
        CollapsedPinnedMessage(QWidget* _parent);

        void updateStyle();

    protected:
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void enterEvent(QEvent*) override;

        bool isHovered() const override;
        ArrowDirection getArrowDirection() const override;
    };

    //----------------------------------------------------------------------
    class StrangerPinnedWidget: public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void closeClicked();
        void blockClicked();

    public:
        StrangerPinnedWidget(QWidget* _parent);

        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        TextRendering::TextUnitPtr block_;
        CustomButton* close_;
        QPoint pressPoint_;
    };

    //----------------------------------------------------------------------
    class PinnedMessageWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized();

        void strangerBlockClicked();
        void strangerCloseClicked();

    public Q_SLOTS:
        void showExpanded();
        void showCollapsed();
        void showStranger();
        void updateStyle();

    public:
        PinnedMessageWidget(QWidget* _parent);

        bool setMessage(Data::MessageBuddySptr _msg);
        void clear();

        bool isStrangerVisible() const;

    protected:
        void resizeEvent(QResizeEvent*) override;

    private:
        void createStranger();
        void createFull();
        void createCollapsed();
        void hideWidget(QWidget* _widget);

    private:
        FullPinnedMessage* full_;
        CollapsedPinnedMessage* collapsed_;
        StrangerPinnedWidget* stranger_;
    };
}
