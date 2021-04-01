#pragma once

#include "StickerData.h"
#include "types/StickerId.h"
#include "utils/utils.h"

#define UI_STICKERS_NS_BEGIN namespace Ui { namespace Stickers {
#define UI_STICKERS_NS_END } }

namespace core
{
    enum class sticker_size;
    class coll_helper;
}

UI_STICKERS_NS_BEGIN

enum class Type
{
    image,
    gif,
    lottie,
};

class Sticker
{
public:
    Sticker() = default;
    Sticker(Data::StickerId _id, Type _type);
    virtual ~Sticker() = default;

    const Data::StickerId& getId() const noexcept { return id_; }
    Type getType() const noexcept { return type_; }

    virtual const StickerData& getData(core::sticker_size _size) const = 0;
    virtual void setData(core::sticker_size _size, StickerData _data) = 0;
    virtual bool hasData(core::sticker_size _size) const = 0;

    void setEmojis(std::vector<QString> _emojis) { emojis_ = std::move(_emojis); }
    const std::vector<QString>& getEmojis() const noexcept { return emojis_; }

    bool isFailed() const noexcept { return failed_; }
    void setFailed(bool _failed) { failed_ = _failed; }

    void clearCache();
    void lockCache(bool _lock);

    bool isGif() const noexcept { return type_ == Type::gif; }
    bool isLottie() const noexcept { return type_ == Type::lottie; }
    bool isAnimated() const noexcept { return isGif() || isLottie(); }

    virtual void setRequested(core::sticker_size _size, bool _requested) {}
    virtual bool isRequested(core::sticker_size _size) const { return false; }

protected:
    virtual void clearCacheImpl() {}

private:
    Data::StickerId id_;
    std::vector<QString> emojis_;

    bool failed_ = false;
    bool cacheLocked_ = false;
    Type type_ = Type::image;
};

class ImageSticker : public Sticker
{
public:
    ImageSticker(Data::StickerId _id, Type _type = Type::image);

    const StickerData& getData(core::sticker_size _size) const override;
    void setData(core::sticker_size _size, StickerData _data) override;
    bool hasData(core::sticker_size _size) const override;

    void setRequested(core::sticker_size _size, bool _requested) override;
    bool isRequested(core::sticker_size _size) const override;

    using StickerDataMap = std::map<core::sticker_size, StickerData>;
    const StickerDataMap& getImages() const noexcept { return images_; }

protected:
    void clearCacheImpl() override;

private:
    StickerDataMap images_;
    std::vector<core::sticker_size> requested_;
};

class LottieSticker : public Sticker
{
public:
    LottieSticker(Data::StickerId _id);

    const StickerData& getData(core::sticker_size) const override { return data_; }
    void setData(core::sticker_size, StickerData _data) override;
    bool hasData(core::sticker_size) const override { return data_.isValid(); }

    void setRequested(core::sticker_size, bool _requested) override { requested_ = _requested; }
    bool isRequested(core::sticker_size) const override { return requested_; }

protected:
    void clearCacheImpl() override;

private:
    StickerData data_;
    bool requested_ = false;
};

using stickerSptr = std::shared_ptr<Sticker>;
using stickersMap = std::unordered_map<int32_t, stickerSptr>;
using stickersArray = std::vector<QString>;
using FsStickersMap = std::unordered_map<QString, stickerSptr, Utils::QStringHasher>;

class Set
{
    int32_t id_ = -1;
    QString name_;
    StickerData icon_;
    StickerData bigIcon_;
    QString storeId_;
    QString description_;
    bool purchased_ = true;
    bool lottie_ = false;

    stickersArray stickers_;

    FsStickersMap fsStickersTree_;

    stickersMap stickersTree_;

public:
    Set(int _id = -1) : id_(_id) {}

    void setId(int32_t _id) { id_ = _id; }
    int32_t getId() const noexcept { return id_; }

    void setName(const QString& _name) { name_ = _name; }
    const QString& getName() const noexcept { return name_; }

    void setStoreId(const QString& _storeId) { storeId_ = _storeId; }
    const QString& getStoreId() const noexcept { return storeId_; }

    void setDescription(const QString& _description) { description_ = _description; }
    const QString& getDescription() const noexcept { return description_; }

    void setPurchased(const bool _purchased) { purchased_ = _purchased; }
    bool isPurchased() const noexcept { return purchased_; }

    void setIcon(StickerData _data);
    const StickerData& getIcon() const;

    void setBigIcon(StickerData _data);
    const StickerData& getBigIcon() const;

    int32_t getCount() const;
    int32_t getStickerPos(const QString& _fsId) const;

    bool isEmpty() const { return stickers_.empty(); }

    const StickerData& getStickerData(const int32_t _stickerId, const core::sticker_size _size);
    void setStickerData(const int32_t _stickerId, const core::sticker_size _size, StickerData _data);

    void setStickerFailed(const int32_t _stickerId);
    void resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size);

    stickerSptr getSticker(int32_t _stickerId) const; // returns nullptr if not found
    const stickerSptr& getOrCreateSticker(int32_t _stickerId); // creates if not found
    void unserialize(const core::coll_helper& _coll);

    void clearCache();

    const stickersArray& getStickers() const noexcept { return  stickers_; }
    const stickersMap& getStickersTree() const noexcept { return stickersTree_; }
    const FsStickersMap& getFsStickersTree() const noexcept { return fsStickersTree_; }

    bool containsGifSticker() const;
    bool containsLottieSticker() const noexcept { return lottie_; }
};

using setSptr = std::shared_ptr<Set>;
using setsIdsArray = std::vector<int32_t>;
using setsMap = std::unordered_map<int32_t, setSptr>;

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

struct StickerLoadData
{
    StickerData data_;
    stickerSptr sticker_;
    setSptr set_;
    QString fsId_;
    QString path_;
    core::sticker_size size_;
    int id_;
    int setId_;
};
using StickerLoadDataV = std::vector<StickerLoadData>;


class Cache : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void setIconUpdated(int _setId, QPrivateSignal) const;
    void setBigIconUpdated(int _setId, QPrivateSignal) const;
    void stickerUpdated(int _error, const QString& _fsId, int _setId, int _stickerId, QPrivateSignal) const;

public:
    Cache(QObject* _parent);

    void unserialize(const core::coll_helper &_coll);
    void unserialize_store(const core::coll_helper &_coll);
    bool unserialize_store_search(int64_t _seq, const core::coll_helper &_coll);
    void unserialize_suggests(const core::coll_helper &_coll);

    void clean_search_cache();

    void setStickerData(const core::coll_helper& _coll);
    void setSetIcon(const core::coll_helper& _coll);
    void setSetBigIcon(const core::coll_helper& _coll);

    stickerSptr getSticker(uint32_t _setId, uint32_t _stickerId) const;
    stickerSptr getSticker(const QString& _fsId) const;

    const setsIdsArray& getSets() const;
    const setsIdsArray& getStoreSets() const;
    const setsIdsArray& getSearchSets() const;

    setSptr getSet(int32_t _setId) const;
    setSptr getStoreSet(int32_t _setId) const;
    setSptr findSetByFsId(const QString& _fsId) const;

    void addSet(setSptr _set);
    void addStickerByFsId(const std::vector<QString> &_fsIds, const QString &_keyword, const SuggestType _type);

    setSptr insertSet(int32_t _setId);
    stickerSptr insertSticker(const QString& _fsId);

    void clearCache();
    void clearSetCache(int _setId);

    bool getSuggest(const QString& _keyword, Suggest& _suggest, const std::set<SuggestType>& _types) const;

    void requestSearch(const QString& _term);

    const setsMap& getSetsTree() const;
    const setsMap& getStoreTree() const;

private:
    void startBatchLoadTimer();
    void loadStickerDataBatch();
    void onStickersBatchLoaded(StickerLoadDataV _loadedV);

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

    StickerLoadDataV batchloadData_;
    QTimer* batchLoadTimer_ = nullptr;
};

void unserialize(const core::coll_helper& _coll);
void unserialize_store(const core::coll_helper& _coll);
bool unserialize_store_search(int64_t _seq, const core::coll_helper& _coll);
void unserialize_suggests(const core::coll_helper& _coll);
std::vector<stickerSptr> getFsByIds(const core::coll_helper& _coll);

void clean_search_cache();

void setStickerData(const core::coll_helper& _coll);

void setSetIcon(const core::coll_helper& _coll);
void setSetBigIcon(const core::coll_helper& _coll);

stickerSptr getSticker(uint32_t _setId, uint32_t _stickerId);
stickerSptr getSticker(const QString& _fsId);

QString getSendUrl(const QString& _fsId);

const setsIdsArray& getStickersSets();
const setsIdsArray& getStoreStickersSets();
const setsIdsArray& getSearchStickersSets();
const setsMap& getSetsTree();
const setsMap& getStoreTree();

const stickersArray& getStickers(int32_t _setId);

int32_t getSetStickersCount(int32_t _setId);

int32_t getStickerPosInSet(int32_t _setId, const QString& _fsId);

const StickerData& getStickerData(const QString& _fsId, core::sticker_size _size);

const StickerData& getSetIcon(int32_t _setId);
const StickerData& getSetBigIcon(int32_t _setId);

void cancelDataRequest(std::vector<QString> _fsIds, core::sticker_size _size);

QString getSetName(int32_t _setId);

void lockStickerCache(const QString& _fsId);

void unlockStickerCache(const QString& _fsId);

void showStickersPack(const int32_t _set_id, StatContext context);
void showStickersPackByStoreId(const QString& _store_id, StatContext context);
void showStickersPackByFileId(const QString& _file_id, StatContext context);
void showStickersPackByStickerId(const Data::StickerId& _sticker_id, StatContext context);

setSptr parseSet(const core::coll_helper& _coll_set);

void addSet(setSptr _set);
void addStickers(const std::vector<QString> &_fsIds, const QString &_text, const Stickers::SuggestType _type);

setSptr getSet(const int32_t _setId);
setSptr getStoreSet(const int32_t _setId);
setSptr findSetByFsId(const QString& _fsId);

void requestSearch(const QString& _term);

void clearCache();
void clearSetCache(int _setId);

void resetCache();
Cache& getCache();

bool getSuggest(const QString& _keyword, Suggest& _suggest, const std::set<SuggestType>& _types);
bool getSuggestWithSettings(const QString& _keyword, Suggest& _suggest);

UI_STICKERS_NS_END
