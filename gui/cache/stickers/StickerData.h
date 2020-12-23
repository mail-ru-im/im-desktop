#pragma once

namespace Ui
{
    namespace Stickers
    {
        class StickerData
        {
        public:

            bool isPixmap() const noexcept { return std::holds_alternative<QPixmap>(data_); }
            bool isLottie() const noexcept { return std::holds_alternative<QString>(data_); }
            bool isValid() const noexcept { return !std::holds_alternative<std::monostate>(data_); }

            void loadFromFile(const QString& _path);
            void setPixmap(QPixmap _image);

            const QPixmap& getPixmap() const noexcept;
            const QString& getLottiePath() const noexcept;

            static const StickerData& invalid();

            StickerData() = default;
            StickerData(const QString& _path) { loadFromFile(_path); }

            StickerData(const StickerData& other);
            StickerData(StickerData&& other) noexcept;
            StickerData& operator=(StickerData && other) noexcept;

            static StickerData makeLottieData(const QString& _path);

        private:
            static bool canSafelyConstructFrom(const StickerData& other);

            std::variant<std::monostate, QPixmap, QString> data_;

        };
    }
}
