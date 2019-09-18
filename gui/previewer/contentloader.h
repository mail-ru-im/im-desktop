#pragma once

#include "../types/message.h"

namespace Previewer
{

class ImageViewerWidget;

class ContentItem;

class ContentLoader : public QObject
{
    Q_OBJECT
public:
    virtual void next() {}
    virtual void prev() {}

    virtual bool hasNext() { return false; }
    virtual bool hasPrev() { return false; }

    virtual ContentItem* currentItem() { return nullptr; }
    virtual int currentIndex() const { return 0; }
    virtual int totalCount() const { return 0; }

    virtual void cancelCurrentItemLoading() {}
    virtual void startCurrentItemLoading() {}

    virtual bool navigationSupported() { return false; }

Q_SIGNALS:
    void mediaLoaded();
    void previewLoaded();
    void itemUpdated();
    void contentUpdated();
    void itemSaved(const QString& _path);
    void itemSaveError();
    void error();
};

class ContentItem : public QObject
{
    Q_OBJECT
public:
    virtual ~ContentItem() = default;

    virtual qint64 msg() { return 0; }
    virtual qint64 seq() { return 0; }

    virtual QString link() const { return QString(); }
    virtual QString path() const { return QString(); }

    virtual QString fileName() const { return QString(); }

    virtual QPixmap preview() const { return QString(); }
    virtual QSize originSize() const { return QSize(); }

    virtual bool isVideo() const { return false; }

    virtual void showMedia(ImageViewerWidget* _viewer) {}
    virtual void showPreview(ImageViewerWidget* _viewer) {}

    virtual void save(const QString& _path) {}

    virtual void copyToClipboard() {}

    virtual time_t time() const { return 0; }

    virtual QString sender() const { return QString(); }

    virtual QString caption() const { return QString(); }

    virtual QString aimId() const { return QString(); }

Q_SIGNALS:
    void saved(const QString& _path);
    void saveError();
};


}
