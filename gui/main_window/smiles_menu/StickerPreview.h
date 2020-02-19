#pragma once


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
                const QString& _stickerId,
                Context _context);

            ~StickerPreview();

            void showSticker(const int32_t _setId, const QString& _stickerId);

            void hide();

        Q_SIGNALS:
            void needClose();

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void mouseReleaseEvent(QMouseEvent* _e) override;

        private:
            void init(const int32_t _setId, const QString& _stickerId);
            void drawSticker(QPainter& _p);
            void drawEmoji(QPainter& _p);
            void updateEmoji();
            QRect getImageRect() const;
            QRect getAdjustedImageRect() const;
            void loadSticker();
            void scaleSticker();
            void onGifLoaded(const QString& _path);
            void onActivationChanged(bool _active);

        private:
            int32_t setId_;
            QString stickerId_;

            QPixmap sticker_;
            std::vector<QImage> emojis_;

            Context context_;
            std::unique_ptr<DialogPlayer> player_;
            std::unique_ptr<Utils::MediaLoader> loader_;

            bool hiding_ = false;
        };
    }
}