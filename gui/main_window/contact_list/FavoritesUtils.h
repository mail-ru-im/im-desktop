#pragma once

#include "types/message.h"
#include "previewer/toast.h"

namespace Favorites
{
    QString firstMessageText();
    QPixmap avatar(int32_t _size);
    QColor nameColor();

    bool isFavorites(const QString& _aimId);

    const QString& aimId();
    QString name();

    const std::vector<QString>& matchingWords();

    void addToFavorites(const Data::QuotesVec& _quotes);

    bool pinnedOnStart();
    void setPinnedOnStart(bool _pinned);

    void showSavedToast(int _count = 1);

    class FavoritesToast_p;

    class FavoritesToast : public Ui::ToastBase
    {
    public:
        FavoritesToast(int _messageCount, QWidget* _parent = nullptr);

    protected:
        void drawContent(QPainter& _p) override;

        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        std::unique_ptr<FavoritesToast_p> d;
    };
}
