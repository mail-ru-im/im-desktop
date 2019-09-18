#include "stdafx.h"

#include "AvatarPreview.h"
#include "../utils/utils.h"

namespace Ui
{
    // dip
    const auto size1 = 180;
    const auto size2 = 56;
    const auto size3 = 32;
    const auto size4 = 16;

    const auto margin1 = 12;
    const auto margin2 = 28;

    AvatarPreview::AvatarPreview(QPixmap _img, QWidget* _parent)
        : QWidget(_parent)
        , img_(std::move(_img))
    {}

    AvatarPreview::~AvatarPreview()
    {}

    void AvatarPreview::paintEvent(QPaintEvent* _e)
    {
        const auto avatar = Utils::roundImage(img_, QString(), false, false);
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        auto size1_px = Utils::scale_value(size1);
        auto avatar1 = avatar.scaled(QSize(size1_px, size1_px), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(0, 0, size1_px, size1_px, avatar1);

        auto size2_px = Utils::scale_value(size2);
        auto avatar2 = avatar1.scaled(QSize(size2_px, size2_px), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(Utils::scale_value(size1 + margin1), Utils::scale_value(margin2), size2_px, size2_px, avatar2);

        auto size3_px = Utils::scale_value(size3);
        auto avatar3 = avatar2.scaled(QSize(size3_px, size3_px), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(Utils::scale_value(size1 + margin1), Utils::scale_value(margin2 + size2 + margin1), size3_px, size3_px, avatar3);

        auto size4_px = Utils::scale_value(size4);
        auto avatar4 = avatar3.scaled(QSize(size4_px, size4_px), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(Utils::scale_value(size1 + margin1), Utils::scale_value(margin2 + size2 + margin1 + size3 + margin1), size4_px, size4_px, avatar4);

        return QWidget::paintEvent(_e);
    }

    QSize AvatarPreview::sizeHint() const
    {
        return Utils::scale_value(QSize(size1 + size2 + margin1, size1));
    };
}
