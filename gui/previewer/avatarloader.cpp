#include "stdafx.h"

#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "utils/utils.h"
#include "main_window/friendly/FriendlyContainer.h"

#include "ImageViewerWidget.h"

#include "avatarloader.h"

using namespace Previewer;

AvatarLoader::AvatarLoader(const QString &_aimId)
{
    item_ = std::make_unique<AvatarItem>(_aimId);
    connect(item_.get(), &AvatarItem::loaded, this, &AvatarLoader::mediaLoaded);
    connect(item_.get(), &AvatarItem::saved, this, &AvatarLoader::itemSaved);
    connect(item_.get(), &AvatarItem::saveError, this, &AvatarLoader::itemSaveError);
}

ContentItem* AvatarLoader::currentItem()
{
    return item_.get();
}

AvatarItem::AvatarItem(const QString &_aimId)
    : aimId_(_aimId)
{
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::avatarLoaded, this, &AvatarItem::avatarLoaded);

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", _aimId);
    collection.set_value_as_int("size", Utils::getMaxImageSize().height());
    collection.set_value_as_bool("force", true);

    seq_ = Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());
}

QString AvatarItem::fileName() const
{
    return Logic::GetFriendlyContainer()->getFriendly(aimId_) % ql1s(".png");
}

void AvatarItem::showMedia(ImageViewerWidget *_viewer)
{
    _viewer->showPixmap(pixmap_, pixmap_.size(), false);
}

void AvatarItem::save(const QString &_path)
{
    if (pixmap_.save(_path))
        emit saved(_path);
    else
        emit saveError();
}

void AvatarItem::copyToClipboard()
{
    QApplication::clipboard()->setPixmap(pixmap_);
}

void AvatarItem::avatarLoaded(int64_t _seq, const QString &_aimId, QPixmap& _pixmap, int _size, bool _result)
{
    if (seq_ != _seq || !_result)
        return;

    pixmap_ = _pixmap;

    emit loaded();
}