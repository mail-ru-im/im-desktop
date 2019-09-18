#pragma once

#include "SmilesMenu.h"
#include "../../animation/animation.h"

namespace Ui
{
    class TooltipWidget;

    namespace Stickers
    {
        class StickersWidget : public Ui::Smiles::StickersTable
        {
            Q_OBJECT

            stickersArray stickersArray_;
            Smiles::StickerPreview* stickerPreview_;

        Q_SIGNALS:
            void stickerHovered(const QRect& _stickerRect);

        private:
            void onStickerPreview(const int32_t _setId, const QString& _stickerId);
            void closeStickerPreview();
            void onStickerHovered(const int32_t _setId, const QString& _stickerId);

        protected:
            bool resize(const QSize& _size, bool _force = false) override;

        public:
            StickersWidget(QWidget* _parent);

            void init(const QString& _text);

            void mouseReleaseEvent(QMouseEvent* _e) override;
            void mousePressEvent(QMouseEvent* _e) override;
            void mouseMoveEvent(QMouseEvent* _e) override;
            void leaveEvent(QEvent* _e) override;

            int32_t getStickerPosInSet(const QString& _stickerId) const override;
            const stickersArray& getStickerIds() const override;

            void setSelected(const std::pair<int32_t, QString>& _sticker) override;
        };

        class StickersSuggest : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(qint32, const QString&);

        private Q_SLOTS:

            void stickerHovered(const QRect& _stickerRect);

        public:
            StickersSuggest(QWidget* _parent);

            void showAnimated(const QString& _text, const QPoint& _p, const QSize& _maxSize, const QRect& _rect = QRect());
            void hideAnimated();
            bool isTooltipVisible() const;

        protected:

            void keyPressEvent(QKeyEvent* _event) override;

        private:

            StickersWidget* stickers_;
            TooltipWidget* tooltip_;
            bool keyboardActive_;
        };
    }
}
