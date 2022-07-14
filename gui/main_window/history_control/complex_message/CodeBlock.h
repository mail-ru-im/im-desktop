#pragma once

#include "GenericBlock.h"

namespace Ui
{
    class ScrollAreaWithTrScrollBar;
    class TextWidget;
    class AnimatedGradientWidget;

    namespace ComplexMessage
    {
        class CodeBlock : public GenericBlock
        {
            Q_OBJECT

        public:
            CodeBlock(ComplexMessageItem* _parent, Data::FStringView _text);

            IItemBlockLayout* getBlockLayout() const override { return nullptr; };
            Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;
            void updateWith(IItemBlock* _other) override;

            bool isSelected() const override;
            bool isAllSelected() const override;
            void selectByPos(const QPoint& _from, const QPoint& _to, bool _topToBottom) override;
            void selectAll() override;
            void clearSelection() override;
            void releaseSelection() override;

            bool isDraggable() const override { return false; };
            bool isSharingEnabled() const override { return false; };

            int desiredWidth(int _width = 0) const override;
            int getMaxWidth() const override;

            QRect setBlockGeometry(const QRect& _rect) override;
            QRect getBlockGeometry() const override;

            bool clicked(const QPoint& _p) override { return false; }
            void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;

            ContentType getContentType() const override { return ContentType::Code; }

            bool isNeedCheckTimeShift() const override { return true; }
            bool needToExpand() const override { return true; }

        protected:
            void drawBlock(QPainter& _painter, const QRect& _rect, const QColor& _quoteColor) override {}

            void initialize() override;

            void updateFonts() override;

            void wheelEvent(QWheelEvent* _event) override;
            void mouseMoveEvent(QMouseEvent* _event) override;
            void mousePressEvent(QMouseEvent* _event) override;
            void mouseReleaseEvent(QMouseEvent* _event) override;
            bool event(QEvent* _event) override;

        private:
            void setTextInternal(Data::FString _text);
            void updateTextFont();

            void initializeScrollArea();
            QWidget* createScrollAreaContainer();
            QWidget* createBackgroundWidget();
            QWidget* createGradientWidget();
            QWidget* createQuoteOverlay();
            void initializeScrollAnimation();
            void setHorizontalScrollAnimationCurve(QEasingCurve::Type _type);

            QPoint textOffset() const;

        private Q_SLOTS:
            void updateGradientIfNeeded();
            void onScrollAnimationValueChanged();
            void onMultiselectChanged();
            void resetScrollAnimation();

        private:
            enum class ScrollDirection
            {
                Left,
                Right,
                None
            };

            struct PressPoint
            {
                QPoint textUnitPoint_;
                QPoint blockPoint_;
            };

        private:
            TextWidget* text_ = nullptr;
            ScrollAreaWithTrScrollBar* scrollArea_ = nullptr;
            AnimatedGradientWidget* leftGradient_ = nullptr;
            AnimatedGradientWidget* rightGradient_ = nullptr;
            QWidget* textContainer_ = nullptr;
            QWidget* backgroundWidget_ = nullptr;
            QVariantAnimation* scrollAnimation_ = nullptr;
            Styling::ThemeChecker themeChecker_;
            std::optional<PressPoint> pressStart_;
            ScrollDirection scrollDirection_ = ScrollDirection::None;
        };
    } // namespace ComplexMessage
} // namespace Ui
