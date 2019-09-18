#include "stdafx.h"

#include "WallpaperPreviewWidget.h"
#include "controls/BackgroundWidget.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemesContainer.h"

#include "main_window/history_control/complex_message/ComplexMessageItemBuilder.h"
#include "main_window/history_control/complex_message/ComplexMessageItem.h"

namespace
{
    template <typename T>
    void foreachMessage(QWidget* _parent, const T& _func)
    {
        const auto widgets = _parent->findChildren<Ui::ComplexMessage::ComplexMessageItem*>();
        for (auto msg : widgets)
            _func(msg);
    }
}

namespace Ui
{
    using namespace ComplexMessage;

    WallpaperPreviewWidget::WallpaperPreviewWidget(QWidget* _parent, const PreviewMessagesVector& _messages)
        : QWidget(_parent)
        , bg_(new BackgroundWidget(this))
    {
        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(bg_);

        auto pageLayout = Utils::emptyVLayout(bg_);

        for (const auto& i: _messages)
        {
            auto msg = ComplexMessageItemBuilder::makeComplexItem(
                this,
                -1,
                QString(),
                QDate::currentDate(),
                -1,
                i.text_,
                Styling::getTryOnAccount(),
                Styling::getTryOnAccount(),
                i.senderFriendly_,
                {},
                {},
                {},
                {},
                i.senderFriendly_.isEmpty(),
                false,
                false,
                QString(),
                QString(),
                Data::SharedContact());
            msg->setEdited(i.isEdited_);
            msg->setTime(QDateTime(QDate::currentDate(), i.time_).toTime_t());
            msg->setHasAvatar(false);
            msg->setMchatSender(i.senderFriendly_);
            msg->setHasSenderName(!msg->isOutgoing());
            msg->setContextMenuEnabled(false);

            msg->onActivityChanged(true);
            msg->onVisibilityChanged(true);

            msg->setAttribute(Qt::WA_TransparentForMouseEvents);

            pageLayout->addWidget(msg.release());
        }

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperImageAvailable, this, &WallpaperPreviewWidget::onWallpaperAvailable);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::chatFontParamsChanged, this, &WallpaperPreviewWidget::onFontParamsChanged);
    }

    void WallpaperPreviewWidget::updateFor(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            aimId_ = _aimId;

        bg_->updateWallpaper(aimId_);

        foreachMessage(this, [](const auto msg) { msg->updateStyle(); });
    }

    void WallpaperPreviewWidget::resizeEvent(QResizeEvent *)
    {
        onResize();
    }

    void WallpaperPreviewWidget::showEvent(QShowEvent *)
    {
        onResize();
    }

    void WallpaperPreviewWidget::onWallpaperAvailable(const Styling::WallpaperId& _id)
    {
        const auto wallpaperId = Styling::getThemesContainer().getContactWallpaperId(aimId_);
        if (wallpaperId == _id)
            updateFor(aimId_);
    }

    void WallpaperPreviewWidget::onFontParamsChanged()
    {
        foreachMessage(this, [](const auto msg) { msg->updateFonts(); });
    }

    void WallpaperPreviewWidget::onResize()
    {
        const auto w = width();
        foreachMessage(this, [w](const auto msg)
        {
            msg->setGeometry(msg->pos().x(), msg->pos().y(), w, msg->height());
            msg->updateSize();
            msg->update();
        });
    }
}
