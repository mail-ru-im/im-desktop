#pragma once

namespace core
{
    class coll_helper;
}

namespace HistoryControl
{

    typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;

    class StickerInfo
    {
    public:
        static StickerInfoSptr Make(const core::coll_helper& _coll);
        static StickerInfoSptr Make(const quint32 _setId, const quint32 _stickerId);

        const quint32 SetId_;

        const quint32 StickerId_;

    private:
        StickerInfo(const quint32 _setId, const quint32 _stickerId);

    };

}