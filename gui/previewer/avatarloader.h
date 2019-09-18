#pragma once

#include "contentloader.h"

namespace Previewer
{

class AvatarItem;

class AvatarLoader : public ContentLoader
{
    Q_OBJECT
public:
    AvatarLoader(const QString& _aimId);

    ContentItem* currentItem() override;

private:
    std::unique_ptr<AvatarItem> item_;
};

class AvatarItem : public ContentItem
{
    Q_OBJECT
public:
    AvatarItem(const QString& _aimId);

    QString fileName() const override;

    void showMedia(ImageViewerWidget* _viewer) override;

    void save(const QString& _path) override;

    void copyToClipboard() override;

Q_SIGNALS:
    void loaded();

private Q_SLOTS:
    void avatarLoaded(int64_t _seq, const QString& _aimId, QPixmap& _pixmap, int _size, bool _result);

private:
    QString aimId_;
    QPixmap pixmap_;
    int64_t seq_;
};

}