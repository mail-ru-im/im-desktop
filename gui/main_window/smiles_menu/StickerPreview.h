#pragma once

#include "../history_control/complex_message/FileSharingUtils.h"

namespace Utils
{
    class MediaLoader;
}

namespace Ui
{
    class DialogPlayer;

    namespace Smiles
    {
        class StickerPreview : public QWidget
        {
            Q_OBJECT

        public:

            enum class Context
            {
                Picker,
                Popup
            };

            StickerPreview(
                QWidget* _parent,
                const int32_t _setId,
                const Utils::FileSharingId& _stickerId,
                Context _context);

            ~StickerPreview();

            void showSticker(const Utils::FileSharingId& _stickerId);

            void hide();

        Q_SIGNALS:
            void needClose();

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void mouseReleaseEvent(QMouseEvent* _e) override;
            void resizeEvent(QResizeEvent* _e) override;

        private:
            void init(const Utils::FileSharingId& _stickerId);
            void drawSticker(QPainter& _p);
            void drawEmoji(QPainter& _p);
            void updateEmoji();
            QRect getImageRect() const;
            QRect getAdjustedImageRect() const;
            void loadSticker();
            void scaleSticker();
            void updatePlayerSize();
            void onAnimatedStickerLoaded(const QString& _path);
            void onActivationChanged(bool _active);
            void onStickerLoaded(int _error, const Utils::FileSharingId& _id);

        private:
            Utils::FileSharingId stickerId_;

            QPixmap sticker_;
            std::vector<QImage> emojis_;

            Context context_;
#ifndef STRIP_AV_MEDIA
            std::unique_ptr<DialogPlayer> player_;
#endif
            std::unique_ptr<Utils::MediaLoader> loader_;

            bool hiding_ = false;
        };
    }
}