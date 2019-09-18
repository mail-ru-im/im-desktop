#pragma once

namespace Utils
{

void onGalleryMediaAction(bool _actionHappened, const std::string& _type, const QString& _aimid);
std::string chatTypeByAimId(const QString& _aimid);
std::string averageCount(int count);

struct GalleryMediaActionCont
{
    GalleryMediaActionCont(const std::string& _mediaType, const QString& _aimid)
        : mediaType_(_mediaType)
        , happened_(false)
        , aimid_(_aimid)
    {}

    ~GalleryMediaActionCont()
    {
        onGalleryMediaAction(happened_, mediaType_, aimid_);
    }

    void happened()
    {
        happened_ = true;
    }

    std::string mediaType_;
    bool happened_;
    QString aimid_;
};

}
