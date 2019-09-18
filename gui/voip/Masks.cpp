#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"

#include "Masks.h"

voip_masks::Mask::Mask(const QString& _name, QObject* _parent)
    : QObject(_parent)
    , name_(_name)
    , progressSeq_(0)
{
}

const QString& voip_masks::Mask::id() const
{
    return name_;
}

void voip_masks::Mask::setPreview(const QString _localPath)
{
    preview_.load(_localPath);
    assert(!_localPath.isEmpty());
    emit previewLoaded();
}

const QPixmap& voip_masks::Mask::preview() const
{
    return preview_;
}

void voip_masks::Mask::setJsonPath(const QString _localPath)
{
    jsonPath_ = _localPath;
    emit loaded();
    progressSeq_ = 0;
}

const QString& voip_masks::Mask::jsonPath() const
{
    return jsonPath_;
}

void voip_masks::Mask::setProgressSeq(quint64 _seq)
{
    progressSeq_ = _seq;
}

void voip_masks::Mask::onLoadingProgress(quint64 _seq, unsigned _percent)
{
    if (progressSeq_ != _seq)
        return;

    emit loadingProgress(_percent);
}
