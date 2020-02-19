#pragma once

#include "../../../corelib/collection_helper.h"

#define UI_STICKERS_NS_BEGIN namespace Ui { namespace Stickers {
#define UI_STICKERS_NS_END } }

namespace core
{
    enum class sticker_size;
}


UI_STICKERS_NS_BEGIN
class Sticker
{
public:
    typedef std::tuple<QImage, bool> image_data;

private:
    uint32_t id_ = 0;

    int32_t setId_ = -1;

    QString fsId_;

    std::vector<QString> emojis_;

    std::map<core::sticker_size, image_data> images_;

    bool failed_ = false;

    bool isGif_ = false;

public:
    Sticker();
    Sticker(const int32_t _id);
    Sticker(const QString& _fsId);

    int32_t getId() const;

    int32_t getSetId() const;

    const QString& getFsId() const;

    QImage getImage(const core::sticker_size _size, bool _scaleIfNeed, bool& _scaled) const;
    void setImage(const core::sticker_size _size, QImage _image);

    void setFailed(const bool _failed);
    bool isFailed() const;

    bool isImageRequested(const core::sticker_size _size) const;
    void setImageRequested(const core::sticker_size _size, const bool _val);

    const std::vector<QString>& getEmojis();

    void unserialize(core::coll_helper _coll);
    void clearCache();

    const std::map<core::sticker_size, image_data>& getImages() const;

    bool isGif() const;
};

typedef std::shared_ptr<Sticker> stickerSptr;

typedef std::map<int32_t, stickerSptr> stickersMap;

using stickersArray = std::vector<QString>;

using FsStickersMap = std::map<QString, stickerSptr>;

class Set
{
    int32_t id_;
    QString name_;
    QPixmap icon_;
    QPixmap bigIcon_;
    QString storeId_;
    QString description_;
    QString subtitle_;
    bool purchased_;
    bool user_;

    stickersArray stickers_;

    FsStickersMap fsStickersTree_;

    stickersMap stickersTree_;

    int32_t maxSize_;

    bool containsGifSticker_ = false;

public:

    typedef std::shared_ptr<Set> sptr;

    Set(int32_t _maxSize = -1);

    void setId(int32_t _id);
    int32_t getId() const;

    void setName(const QString& _name);
    QString getName() const;

    void setStoreId(const QString& _storeId);
    QString getStoreId() const;

    void setDescription(const QString& _description);
    QString getDescription() const;

    void setSubtitle(const QString& _subtitle);
    QString getSubtitle() const;

    void setPurchased(const bool _purchased);
    bool isPurchased() const;

    void setUser(const bool _user);
    bool isUser() const;

    void loadIcon(const char* _data, int32_t _size);
    QPixmap getIcon() const;

    QPixmap getBigIcon() const;
    void loadBigIcon(const char* _data, int32_t _size);

    int32_t getCount() const;
    int32_t getStickerPos(const QString& _fsId) const;

    const stickersArray& getStickers() const;

    bool empty() const;

    QImage getStickerImage(const int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed);
    void setStickerImage(const int32_t _stickerId, const core::sticker_size _size, QImage _image);
    void setBigIcon(QImage _image);
    void setStickerFailed(const int32_t _stickerId);
    void resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size);

    stickerSptr getSticker(int32_t _stickerId) const;
    void unserialize(core::coll_helper _coll);

    void clearCache();

    const stickersMap& getStickersTree() const;
    const FsStickersMap& getFsStickersTree() const;

    bool containsGifSticker() const;
};

typedef std::shared_ptr<Set> setSptr;

typedef std::vector<int32_t> setsIdsArray;

typedef std::map<int32_t, setSptr> setsMap;


enum class StatContext
{
    Chat,
    Discover,
    Search,
    Browser
};

std::string getStatContextStr(StatContext _context);

enum class SuggestType
{
    suggestEmoji = 0,
    suggestWord = 1
};

struct StickerInfo
{
    const QString fsId_;
    const SuggestType type_;

    StickerInfo(QString _fsId, const SuggestType _type)
        : fsId_(std::move(_fsId))
        , type_(_type)
    {
    }

    bool operator==(const StickerInfo& _other) const
    {
        return _other.fsId_ == fsId_;
    }

};

typedef std::vector<StickerInfo> Suggest;

typedef std::map<QString, Suggest> StickersSuggests;

typedef std::vector<QString> EmojiList;

typedef std::map<QString, EmojiList> SuggestsAliases;

class Cache
{
public:

    Cache();

    void unserialize(const core::coll_helper &_coll);
    void unserialize_store(const core::coll_helper &_coll);
    bool unserialize_store_search(int64_t _seq, const core::coll_helper &_coll);
    void unserialize_suggests(const core::coll_helper &_coll);

    void clean_search_cache();

    void setStickerData(const core::coll_helper& _coll);

    void setSetBigIcon(const core::coll_helper& _coll);

    stickerSptr getSticker(uint32_t _setId, uint32_t _stickerId) const;
    stickerSptr getSticker(const QString& _fsId) const;

    const setsIdsArray& getSets() const;
    const setsIdsArray& getStoreSets() const;
    const setsIdsArray& getSearchSets() const;

    setSptr getSet(int32_t _setId) const;
    setSptr getStoreSet(int32_t _setId) const;

    void addSet(std::shared_ptr<Set> _set);
    void addStickerByFsId(const std::vector<QString> &_fsIds, const QString &_keyword, const SuggestType _type);

    setSptr insertSet(int32_t _setId);
    stickerSptr insertSticker(const QString& _fsId);

    void clearCache();

    bool getSuggest(const QString& _keyword, Suggest& _suggest, const std::set<SuggestType>& _types) const;

    void requestSearch(const QString& _term);

    const setsMap& getSetsTree() const;
    const setsMap& getStoreTree() const;

    const QString& getTemplatePreviewBaseUrl() const;
    const QString& getTemplateOriginalBaseUrl() const;

    QString getTemplateSendBaseUrl() const;

private:

    setsMap setsTree_;
    setsIdsArray sets_;

    FsStickersMap fsStickers_;

    setsMap storeTree_;
    setsIdsArray storeSets_;

    setsIdsArray searchSets_;
    int64_t searchSeqId_ = -1;

    StickersSuggests suggests_;
    SuggestsAliases aliases_;

    QString templatePreviewBaseUrl_;
    QString templateOriginalBaseUrl_;
    QString templateSendBaseUrl_;
};

void unserialize(core::coll_helper _coll);
void unserialize_store(core::coll_helper _coll);
bool unserialize_store_search(int64_t _seq, core::coll_helper _coll);
void unserialize_suggests(core::coll_helper _coll);
std::vector<Sticker> getFsByIds(core::coll_helper _coll);

void clean_search_cache();

void setStickerData(core::coll_helper _coll);

void setSetBigIcon(core::coll_helper _coll);

std::shared_ptr<Sticker> getSticker(uint32_t _setId, uint32_t _stickerId);
std::shared_ptr<Sticker> getSticker(uint32_t _setId, const QString& _fsId);
std::shared_ptr<Sticker> getSticker(const QString& _fsId);

QString getPreviewBaseUrl();
QString getOriginalBaseUrl();
QString getSendBaseUrl();

const setsIdsArray& getStickersSets();
const setsIdsArray& getStoreStickersSets();
const setsIdsArray& getSearchStickersSets();
const setsMap& getSetsTree();
const setsMap& getStoreTree();

const stickersArray& getStickers(int32_t _setId);

int32_t getSetStickersCount(int32_t _setId);

int32_t getStickerPosInSet(int32_t _setId, const QString& _fsId);

QImage getStickerImage(int32_t _setId, int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed = false);

QImage getStickerImage(int32_t _setId, const QString& _fsId, const core::sticker_size _size, const bool _scaleIfNeed = false);

QImage getStickerImage(const QString& _fsId, const core::sticker_size _size, const bool _scaleIfNeed = false);

QPixmap getSetIcon(int32_t _setId);

QString getSetName(int32_t _setId);

bool isConfigHasSticker(int32_t _setId, int32_t _stickerId);

void showStickersPack(const int32_t _set_id, StatContext context);
void showStickersPackByStoreId(const QString& _store_id, StatContext context);
void showStickersPackByFileId(const QString& _file_id, StatContext context);

std::shared_ptr<Set> parseSet(core::coll_helper _coll_set);

void addSet(std::shared_ptr<Set> _set);
void addStickers(const std::vector<QString> &_fsIds, const QString &_text, const Stickers::SuggestType _type);

bool isUserSet(const int32_t _setId);
bool isPurchasedSet(const int32_t _setId);

setSptr getSet(const int32_t _setId);
setSptr getStoreSet(const int32_t _setId);

void requestSearch(const QString& _term);

void clearCache();
void resetCache();

bool getSuggest(const QString& _keyword, Suggest& _suggest, const std::set<SuggestType>& _types);
bool getSuggestWithSettings(const QString& _keyword, Suggest& _suggest);

UI_STICKERS_NS_END
