#include "stdafx.h"

#include "../corelib/enumerations.h"

#include "url_config.h"
#include "gui_settings.h"
#include "core_dispatcher.h"

#include "utils/gui_coll_helper.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/LoadStickerDataFromFileTask.h"

#include "main_window/smiles_menu/StickerPackInfo.h"
#include "main_window/MainWindow.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"

#include "types/message.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "../../utils/features.h"
#include "../../previewer/toast.h"

#include "stickers.h"

namespace
{
    constexpr std::string_view emoji_type("emoji");
    constexpr size_t batchLoadCount() noexcept { return 10; }
    constexpr std::chrono::milliseconds batchLoadTimeout() noexcept { return std::chrono::milliseconds(5); }
}

UI_STICKERS_NS_BEGIN

std::unique_ptr<Cache> g_cache;

stickerSptr createSticker(Data::StickerId _id)
{
    im_assert(!_id.isEmpty());
    if (_id.isEmpty())
        return {};

    if (_id.fsId_)
    {
        im_assert(!_id.fsId_->fileId.isEmpty());
        const auto type = ComplexMessage::extractContentTypeFromFileSharingId(_id.fsId_->fileId);
        im_assert(!type.is_undefined());

        if (type.is_lottie())
            return std::make_shared<LottieSticker>(std::move(_id));

        return std::make_shared<ImageSticker>(std::move(_id), type.is_gif() ? Stickers::Type::gif : Stickers::Type::image);
    }

    return std::make_shared<ImageSticker>(std::move(_id));
}

stickerSptr createSticker(const Utils::FileSharingId& _fsId)
{
    return createSticker(Data::StickerId(_fsId));
}

stickerSptr unserializeSticker(const core::coll_helper& _coll)
{
    Data::StickerId id;
    if (auto fsId = QString::fromUtf8(_coll.get_value_as_string("file_id", "")); !fsId.isEmpty())
    {
        auto sourceId = QString::fromUtf8(_coll.get_value_as_string("source_id", ""));
        id.fsId_ = { fsId, sourceId.isEmpty() ? std::nullopt : std::make_optional(sourceId) };
    }

    if (auto setId = _coll.get_value_as_int("set_id", -1); setId >= 0)
        id.obsoleteId_ = { setId, _coll.get_value_as_int("id", 0) };

    if (id.isEmpty())
        return {};

    auto res = createSticker(std::move(id));
    if (res)
    {
        if (_coll.is_value_exist("emoji"))
            res->setEmojis(Utils::toContainerOfString<std::vector<QString>>(_coll.get_value_as_array("emoji")));
    }

    return res;
}

Sticker::Sticker(Data::StickerId _id, Type _type)
    : id_(std::move(_id))
    , type_(_type)
{
    im_assert(!id_.isEmpty());
}

void Sticker::clearCache()
{
    if (cacheLocked_)
        return;

    clearCacheImpl();
}

void Sticker::lockCache(bool _lock)
{
    cacheLocked_ = _lock;
    if (!cacheLocked_)
        clearCache();
}

ImageSticker::ImageSticker(Data::StickerId _id, Type _type)
    : Sticker(std::move(_id), _type)
{
    im_assert(_type == Type::image || _type == Type::gif);
}

const StickerData& ImageSticker::getData(core::sticker_size _size) const
{
    // reimplementing old logic - if requested not available, try returning largest one

    if (const auto it = images_.find(_size); it != images_.end())
    {
        if (it->second.isValid())
            return it->second;
    }

    if (!images_.empty())
    {
        for (const auto& data : boost::adaptors::reverse(boost::adaptors::values(images_)))
        {
            if (data.isValid())
                return data;
        }
    }

    return StickerData::invalid();
}

void ImageSticker::setData(core::sticker_size _size, StickerData _data)
{
    im_assert(_size > core::sticker_size::min);
    im_assert(_size < core::sticker_size::max);
    im_assert(_data.isPixmap());

    images_[_size] = std::move(_data);
    setRequested(_size, false);
}

bool ImageSticker::hasData(core::sticker_size _size) const
{
    if (const auto it = images_.find(_size); it != images_.end())
        return it->second.isValid();
    return false;
}

void ImageSticker::setRequested(core::sticker_size _size, bool _requested)
{
    auto it = std::find_if(requested_.begin(), requested_.end(), [_size](auto s) { return s == _size; });
    if (_requested)
    {
        if (it == requested_.end())
            requested_.push_back(_size);
    }
    else if (it != requested_.end())
    {
        requested_.erase(it);
    }
}

bool ImageSticker::isRequested(core::sticker_size _size) const
{
    return std::any_of(requested_.begin(), requested_.end(), [_size](auto s) { return s == _size; });
}

void ImageSticker::clearCacheImpl()
{
    images_.clear();
    requested_.clear();
}

LottieSticker::LottieSticker(Data::StickerId _id)
    : Sticker(std::move(_id), Type::lottie)
{
}

void LottieSticker::setData(core::sticker_size, StickerData _data)
{
    im_assert(_data.isLottie());
    data_ = std::move(_data);
    requested_ = false;
}

void LottieSticker::clearCacheImpl()
{
    // do nothing, lottie string is lightweight enough
}

void Set::setStickerFailed(const int32_t _stickerId)
{
    im_assert(_stickerId >= 0);
    if (_stickerId < 0)
        return;

    auto sticker = getOrCreateSticker(_stickerId);
    sticker->setFailed(true);
}

void Set::setStickerData(const int32_t _stickerId, const core::sticker_size _size, StickerData _data)
{
    im_assert(_stickerId >= 0);
    if (_stickerId < 0)
        return;

    auto sticker = getOrCreateSticker(_stickerId);
    sticker->setData(_size, std::move(_data));
}

const StickerData& Set::getStickerData(const int32_t _stickerId, const core::sticker_size _size)
{
    im_assert(_size > core::sticker_size::min);
    im_assert(_size < core::sticker_size::max);

    auto sticker = getOrCreateSticker(_stickerId);
    const auto& data = sticker->getData(_size);
    if (!sticker->hasData(_size) && !sticker->isRequested(_size) && !sticker->isFailed())
    {
        Ui::GetDispatcher()->getSticker(id_, _stickerId, _size);
        sticker->setRequested(_size, true);
    }

    return data;
}

void Set::setIcon(StickerData _data)
{
    icon_ = std::move(_data);
}

const StickerData& Set::getIcon() const
{
    return icon_;
}

void Set::setBigIcon(StickerData _data)
{
    bigIcon_ = std::move(_data);
}

const StickerData& Set::getBigIcon() const
{
    if (!bigIcon_.isValid())
        Ui::GetDispatcher()->getSetIconBig(getId());

    return bigIcon_;
}

stickerSptr Set::getSticker(int32_t _stickerId) const
{
    if (const auto iter = stickersTree_.find(_stickerId); iter != stickersTree_.end())
        return iter->second;

    return nullptr;
}

const stickerSptr& Set::getOrCreateSticker(int32_t _stickerId)
{
    auto& sticker = stickersTree_[_stickerId];
    if (!sticker)
        sticker = std::make_shared<ImageSticker>(Data::StickerId(id_, _stickerId));

    return sticker;
}

int32_t Set::getCount() const
{
    return (int32_t) stickers_.size();
}

int32_t Set::getStickerPos(const Utils::FileSharingId& _fsId) const
{
    return Utils::indexOf(stickers_.begin(), stickers_.end(), _fsId);
}

void Set::unserialize(const core::coll_helper& _coll)
{
    setId(_coll.get_value_as_int("set_id"));
    setName(QString::fromUtf8(_coll.get_value_as_string("name")));
    setStoreId(QString::fromUtf8(_coll.get_value_as_string("store_id")));
    setPurchased(_coll.get_value_as_bool("purchased"));
    setName(QString::fromUtf8(_coll.get_value_as_string("name")));
    setDescription(QString::fromUtf8(_coll.get_value_as_string("description")));
    lottie_ = _coll.get_value_as_bool("lottie");

    getCache().setSetIcon(_coll);

    const auto sticks = _coll.get_value_as_array("stickers");

    const auto size = sticks->size();
    stickers_.reserve(size);

    for (core::iarray::size_type i = 0; i < size; ++i)
    {
        core::coll_helper coll_sticker(sticks->get_at(i)->get_as_collection(), false);

        auto sticker = unserializeSticker(coll_sticker);
        if (!sticker)
            continue;

        if (auto id = sticker->getId().fsId_; id && !id->fileId.isEmpty())
        {
            fsStickersTree_[*id] = sticker;
            stickers_.push_back(*id);
        }
    }
}

void Set::resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size)
{
    if (auto s = getSticker(_stickerId))
        s->setRequested(_size, false);
}

void Set::clearCache()
{
    for (auto & iter : stickersTree_)
        iter.second->clearCache();

    for (auto & iter : fsStickersTree_)
        iter.second->clearCache();
}

bool Set::containsGifSticker() const
{
    return std::any_of(fsStickersTree_.begin(), fsStickersTree_.end(), [](const auto& p) { return p.second->getType() == Type::gif; });
}

void unserialize(const core::coll_helper& _coll)
{
    getCache().unserialize(_coll);
}

void unserialize_store(const core::coll_helper& _coll)
{
    getCache().unserialize_store(_coll);
}

bool unserialize_store_search(int64_t _seq, const core::coll_helper& _coll)
{
    return getCache().unserialize_store_search(_seq, _coll);
}

void unserialize_suggests(const core::coll_helper& _coll)
{
    getCache().unserialize_suggests(_coll);
}

std::vector<stickerSptr> getFsByIds(const core::coll_helper& _coll)
{
    std::vector<stickerSptr> result;

    if (_coll.is_value_exist("stickers"))
    {
        const auto* stickers = _coll.get_value_as_array("stickers");
        const auto size = stickers->size();
        result.reserve(size_t(size));
        for (core::iarray::size_type i = 0; i < size; ++i)
        {
            core::coll_helper coll_sticker(stickers->get_at(i)->get_as_collection(), false);
            result.emplace_back(unserializeSticker(coll_sticker));
        }
    }

    return result;
}

void clean_search_cache()
{
    getCache().clean_search_cache();
}

void setStickerData(const core::coll_helper& _coll)
{
    getCache().setStickerData(_coll);
}

void setSetIcon(const core::coll_helper& _coll)
{
    getCache().setSetIcon(_coll);
}

void setSetBigIcon(const core::coll_helper& _coll)
{
    getCache().setSetBigIcon(_coll);
}

const setsIdsArray& getStickersSets()
{
    return getCache().getSets();
}

const setsIdsArray& getStoreStickersSets()
{
    return getCache().getStoreSets();
}

const setsIdsArray& getSearchStickersSets()
{
    return getCache().getSearchSets();
}

void clearCache()
{
    getCache().clearCache();
}

void clearSetCache(int _setId)
{
    getCache().clearSetCache(_setId);
}

stickerSptr getSticker(uint32_t _setId, uint32_t _stickerId)
{
    return getCache().getSticker(_setId, _stickerId);
}

stickerSptr getSticker(const Utils::FileSharingId& _fsId)
{
    return getCache().getSticker(_fsId);
}

QString getSendUrl(const Utils::FileSharingId& _fsId)
{
    return u"https://" % Ui::getUrlConfig().getUrlFilesParser() % u'/' % _fsId.fileId % (_fsId.sourceId ? (u"?source=" % *_fsId.sourceId) : QString());
}

Cache::Cache(QObject* _parent)
    : QObject(_parent)
{

}

void Cache::setStickerData(const core::coll_helper& _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id", -1);
    const qint32 stickerId = _coll.get_value_as_int("sticker_id");
    const qint32 error = _coll.get_value_as_int("error");
    const QString fileId = QString::fromUtf8(_coll.get_value_as_string("file_id"));
    QString sourceId = QString::fromUtf8(_coll.get_value_as_string("source_id", ""));
    Utils::FileSharingId fsId{ fileId, sourceId.isEmpty() ? std::nullopt : std::make_optional(std::move(sourceId)) };
    setSptr stickerSet;
    stickerSptr sticker;

    if (!fileId.isEmpty())
    {
        auto& s = fsStickers_[fsId];
        if (!s)
            s = createSticker(fsId);
        sticker = s;
    }
    else if (setId >= 0 && stickerId >= 0)
    {
        stickerSet = insertSet(setId);
        stickerSet->setPurchased(false);
    }

    if (!sticker && !stickerSet)
    {
        if (error == 0)
            im_assert(!"sticker error");
        return;
    }

    if (error != 0)
    {
        if (sticker)
            sticker->setFailed(true);
        else
            stickerSet->setStickerFailed(stickerId);

        Q_EMIT stickerUpdated(error, fsId, setId, stickerId, QPrivateSignal());
        return;
    }

    constexpr static core::sticker_size sizes[] =
    {
        core::sticker_size::small,
        core::sticker_size::medium,
        core::sticker_size::large,
        core::sticker_size::xlarge,
        core::sticker_size::xxlarge,
    };

    for (auto size : sizes)
    {
        if (auto id = to_string(size); !id.empty() && _coll->is_value_exist(id))
        {
            if (std::string_view path = _coll.get_value_as_string(id); !path.empty())
            {
                StickerLoadData data;
                data.sticker_ = std::move(sticker);
                data.set_ = std::move(stickerSet);
                data.fsId_ = std::move(fsId);
                data.path_ = QString::fromUtf8(path.data(), path.size());
                data.size_ = size;
                data.id_ = stickerId;
                data.setId_ = setId;

                if (ComplexMessage::isLottieFileSharingId(data.fsId_.fileId))
                {
                    data.data_ = StickerData::makeLottieData(data.path_);
                    onStickersBatchLoaded({ std::move(data) });
                }
                else
                {
                    batchloadData_.emplace_back(std::move(data));
                    if (batchloadData_.size() >= batchLoadCount())
                        loadStickerDataBatch();
                    else
                        startBatchLoadTimer();
                }

                return;
            }
        }
    }
}

void Cache::setSetIcon(const core::coll_helper& _coll)
{
    if (!_coll.is_value_exist("icon_path"))
        return;

    if (auto setId = _coll.get_value_as_int("set_id"); setId >= 0)
    {
        if (std::string_view path = _coll.get_value_as_string("icon_path"); !path.empty())
        {
            StickerLoadData data;
            data.path_ = QString::fromUtf8(path.data(), path.size());
            auto task = new Utils::LoadStickerDataFromFileTask({ std::move(data) });
            connect(task, &Utils::LoadStickerDataFromFileTask::loadedBatch, this, [this, setId](auto _dataV)
            {
                if (_dataV.empty() || !_dataV.front().data_.isValid())
                    return;

                if (auto setPtr = getSet(setId))
                {
                    setPtr->setIcon(std::move(_dataV.front().data_));
                    Q_EMIT setIconUpdated(setId, QPrivateSignal());
                }
            });
            QThreadPool::globalInstance()->start(task);
        }
    }
}

void Cache::setSetBigIcon(const core::coll_helper& _coll)
{
    if (!_coll->is_value_exist("icon_path"))
        return;

    if (auto setId = _coll.get_value_as_int("set_id"); setId >= 0)
    {
        if (std::string_view path = _coll.get_value_as_string("icon_path"); !path.empty())
        {
            StickerLoadData data;
            data.path_ = QString::fromUtf8(path.data(), path.size());
            auto task = new Utils::LoadStickerDataFromFileTask({ std::move(data) });
            connect(task, &Utils::LoadStickerDataFromFileTask::loadedBatch, this, [this, setId](auto _dataV)
            {
                if (_dataV.empty())
                    return;

                if (auto& data = _dataV.front().data_; data.isValid())
                {
                    auto& setPtr = storeTree_[setId];
                    if (!setPtr)
                        setPtr = std::make_shared<Set>(setId);

                    setPtr->setBigIcon(std::move(data));
                    Q_EMIT setBigIconUpdated(setId, QPrivateSignal());
                }
                else
                {
                    GetDispatcher()->cleanSetIconBig(setId);
                }
            });
            QThreadPool::globalInstance()->start(task);
        }
    }
}

void Cache::unserialize(const core::coll_helper &_coll)
{
    if (!_coll->is_value_exist("sets"))
        return;

    const core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    sets_.clear();

    const auto size = sets->size();
    sets_.reserve(size);

    for (core::iarray::size_type i = 0; i < size; ++i)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = parseSet(collSet);

        const auto id = insertedSet->getId();
        if (id < 0)
            continue;

        sets_.push_back(id);
        addSet(std::move(insertedSet));
    }
}

void Cache::unserialize_store(const core::coll_helper &_coll)
{
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    storeSets_.clear();

    storeSets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); ++i)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        const auto setId = collSet.get_value_as_int("set_id");

        auto& stickerSet = storeTree_[setId];
        if (stickerSet)
            stickerSet->unserialize(collSet);
        else
            stickerSet = parseSet(collSet);

        storeSets_.push_back(setId);
    }
}

bool Cache::unserialize_store_search(int64_t _seq, const core::coll_helper &_coll)
{
    bool updated = false;

    if (_seq != searchSeqId_)
        return updated;

    updated = true;
    if (!_coll.is_value_exist("sets"))
        return updated;

    auto sets = _coll.get_value_as_array("sets");
    searchSets_.clear();
    searchSets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); ++i)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = parseSet(collSet);

        const auto id = insertedSet->getId();

        searchSets_.push_back(id);

        storeTree_[id] = std::move(insertedSet);
    }

    return updated;
}

void Cache::unserialize_suggests(const core::coll_helper &_coll)
{
    core::iarray* suggests = _coll.get_value_as_array("suggests");
    if (!suggests)
        return;

    suggests_.clear();
    aliases_.clear();

    for (core::iarray::size_type i = 0, suggestsSize = suggests->size(); i < suggestsSize; ++i)
    {
        core::coll_helper collSuggest(suggests->get_at(i)->get_as_collection(), false);

        core::iarray* stickers = collSuggest.get_value_as_array("stickers");
        if (!stickers)
            continue;

        QString emoji = QString::fromUtf8(collSuggest.get_value_as_string("emoji")).toLower();

        Suggest suggest;
        suggest.reserve(stickers->size());

        for (core::iarray::size_type j = 0, stickersSize = stickers->size(); j < stickersSize; ++j)
        {
            core::coll_helper collSticker(stickers->get_at(j)->get_as_collection(), false);

            suggest.emplace_back(
                Utils::FileSharingId{ QString::fromUtf8(collSticker.get_value_as_string("fs_id")), std::nullopt },
                ((collSticker.get_value_as_string("type") == emoji_type) ? SuggestType::suggestEmoji : SuggestType::suggestWord));
        }

        suggests_.emplace(std::move(emoji), std::move(suggest));
    }

    core::iarray* aliases = _coll.get_value_as_array("aliases");
    if (!aliases)
        return;

    for (core::iarray::size_type i = 0, aliasesSize = aliases->size(); i < aliasesSize; ++i)
    {
        core::coll_helper coll_alias(aliases->get_at(i)->get_as_collection(), false);

        core::iarray* emojiArray = coll_alias.get_value_as_array("emojies");
        if (!emojiArray)
            return;

        auto word = QString::fromUtf8(coll_alias.get_value_as_string("word")).toLower();
        aliases_[std::move(word)] = Utils::toContainerOfString<EmojiList>(emojiArray);
    }
}

void Cache::clean_search_cache()
{
    searchSets_.clear();
}

const setsIdsArray& Cache::getSets() const
{
    return sets_;
}

const setsIdsArray& Cache::getStoreSets() const
{
    return storeSets_;
}

const setsIdsArray &Cache::getSearchSets() const
{
    return searchSets_;
}

setSptr Cache::getSet(int32_t _setId) const
{
    if (const auto iter = setsTree_.find(_setId); iter != setsTree_.end())
        return iter->second;
    return {};
}

setSptr Cache::getStoreSet(int32_t _setId) const
{
    if (const auto iter = storeTree_.find(_setId); iter != storeTree_.end())
        return iter->second;
    return {};
}

setSptr Cache::findSetByFsId(const Utils::FileSharingId& _fsId) const
{
    for (const auto& [_, stickerSet] : setsTree_)
    {
        const auto& stickers = stickerSet->getStickers();
        if (std::find(stickers.begin(), stickers.end(), _fsId) != stickers.end())
            return stickerSet;
    }
    return {};
}

void Cache::addSet(setSptr _set)
{
    if (!_set)
        return;

    for (const auto& [fsId, sticker] : _set->getFsStickersTree())
        fsStickers_[fsId] = sticker;
    const auto id = _set->getId();
    setsTree_[id] = std::move(_set);
}

void Cache::addStickerByFsId(const std::vector<Utils::FileSharingId>& _fsIds, const QString& _keyword, const SuggestType _type)
{
    for (const auto& id : _fsIds)
    {
        if (auto& fsSticker = fsStickers_[id]; !fsSticker)
            fsSticker = createSticker(id);

        auto& infoV = suggests_[_keyword];
        if (std::none_of(infoV.begin(), infoV.end(), [&id](const auto & x) { return x.fsId_ == id; }))
            infoV.emplace_back(id, _type);
    }
}

setSptr Cache::insertSet(int32_t _setId)
{
    auto& stickerSet = setsTree_[_setId];
    if (!stickerSet)
        stickerSet = std::make_shared<Set>(_setId);

    return stickerSet;
}

stickerSptr Cache::insertSticker(const Utils::FileSharingId& _fsId)
{
    auto& s = fsStickers_[_fsId];
    if (!s)
        s = createSticker(_fsId);
    return s;
}

std::shared_ptr<Sticker> Cache::getSticker(uint32_t _setId, uint32_t _stickerId) const
{
    if (auto s = getSet(_setId))
        return s->getSticker(_stickerId);
    return nullptr;
}

stickerSptr Cache::getSticker(const Utils::FileSharingId& _fsId) const
{
    if (const auto it = fsStickers_.find(_fsId); it != fsStickers_.end())
        return it->second;
    return nullptr;
}

void Cache::clearCache()
{
    for (const auto& set : setsTree_)
        set.second->clearCache();

    for (auto& [_, s] : fsStickers_)
        s->clearCache();
}

void Cache::clearSetCache(int _setId)
{
    if (auto set = getSet(_setId))
        set->clearCache();
}

bool Cache::getSuggest(const QString& _keyword, Suggest& _suggest, const std::set<SuggestType>& _types) const
{
    _suggest.clear();
    if (_types.empty())
        return false;

    QVarLengthArray<QString> keywords;
    keywords.push_back(_keyword);

    if (_types.find(SuggestType::suggestWord) != _types.end())
    {
        if (const auto it_alias = aliases_.find(_keyword); it_alias != aliases_.end())
        {
            for (const auto& _emoji : it_alias->second)
                keywords.push_back(_emoji);
        }
    }

    for (const auto& kw : keywords)
    {
        if (auto itSuggest = suggests_.find(kw); itSuggest != suggests_.end())
        {
            for (const auto& sticker : itSuggest->second)
            {
                if (_types.find(sticker.type_) != _types.end())
                {
                    if (std::find(_suggest.begin(), _suggest.end(), sticker) == _suggest.end())
                        _suggest.push_back(sticker);
                }
            }
        }
    }

    return !_suggest.empty();
}

void Cache::requestSearch(const QString &_term)
{
    searchSeqId_ = Ui::GetDispatcher()->searchStickers(_term);
}

void Cache::requestStickersMeta()
{
    if (metaAlreadyRequested_)
        return;

    metaAlreadyRequested_ = true;

    int scale = (int)(Utils::getScaleCoefficient() * 100.0);
    scale = Utils::scale_bitmap(scale);
    std::string_view size = "small";

    switch (scale)
    {
    case 150:
        size = "medium";
        break;
    case 200:
        size = "large";
        break;
    }

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("size", size);
    Ui::GetDispatcher()->post_message_to_core("stickers/meta/get", collection.get());
}

const setsMap &Cache::getSetsTree() const
{
    return setsTree_;
}

const setsMap &Cache::getStoreTree() const
{
    return storeTree_;
}

void Cache::startBatchLoadTimer()
{
    if (!batchLoadTimer_)
    {
        batchLoadTimer_ = new QTimer();
        batchLoadTimer_->setSingleShot(true);
        batchLoadTimer_->setInterval(batchLoadTimeout());
        connect(batchLoadTimer_, &QTimer::timeout, this, &Cache::loadStickerDataBatch);
    }
    batchLoadTimer_->start();
}

void Cache::loadStickerDataBatch()
{
    if (batchloadData_.empty())
        return;

    auto task = new Utils::LoadStickerDataFromFileTask(std::move(batchloadData_));
    batchloadData_.clear();

    connect(task, &Utils::LoadStickerDataFromFileTask::loadedBatch, this, &Cache::onStickersBatchLoaded);
    QThreadPool::globalInstance()->start(task);
}

void Cache::onStickersBatchLoaded(StickerLoadDataV _loadedV)
{
    for (auto& loaded : _loadedV)
    {
        if (loaded.data_.isValid())
        {
            if (loaded.sticker_)
                loaded.sticker_->setData(loaded.size_, std::move(loaded.data_));
            else
                loaded.set_->setStickerData(loaded.id_, loaded.size_, std::move(loaded.data_));
        }
        else
        {
            if (loaded.sticker_)
                loaded.sticker_->setFailed(true);
            else
                loaded.set_->setStickerFailed(loaded.id_);
        }

        Q_EMIT stickerUpdated(
            0,
            loaded.fsId_,
            (loaded.sticker_ && loaded.sticker_->getId().obsoleteId_) ? loaded.sticker_->getId().obsoleteId_->setId_ : loaded.setId_,
            loaded.id_,
            QPrivateSignal());
    }
}

int32_t getSetStickersCount(int32_t _setId)
{
    if (auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getCount();
    else if (searchSet = getCache().getStoreSet(_setId); searchSet)
        return searchSet->getCount();

    im_assert(!"searchSet");
    return 0;
}

int32_t getStickerPosInSet(int32_t _setId, const Utils::FileSharingId& _fsId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getStickerPos(_fsId);
    im_assert(!"searchSet");
    return -1;
}

const stickersArray& getStickers(int32_t _setId)
{
    if (auto searchSet = getCache().getSet(_setId))
        return searchSet->getStickers();

    static const stickersArray empty;
    return empty;
}

const StickerData& getStickerData(const Utils::FileSharingId& _fsId, core::sticker_size _size)
{
    auto sticker = getCache().insertSticker(_fsId);
    const auto& data = sticker->getData(_size);
    if (!sticker->isRequested(_size) && !sticker->isFailed() && !sticker->hasData(_size))
    {
        Ui::GetDispatcher()->getSticker(_fsId, _size);
        sticker->setRequested(_size, true);
    }

    return data;
}

void lockStickerCache(const Utils::FileSharingId& _fsId)
{
    auto s = getCache().getSticker(_fsId);
    if (!s)
        return;

    s->lockCache(true);
}

void unlockStickerCache(const Utils::FileSharingId& _fsId)
{
    auto s = getCache().getSticker(_fsId);
    if (!s)
        return;

    s->lockCache(false);
}

const StickerData& getSetIcon(int32_t _setId)
{
    if (const auto s = getCache().getSet(_setId))
        return s->getIcon();

    im_assert(!"searchSet");
    return StickerData::invalid();
}

const StickerData& getSetBigIcon(int32_t _setId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getBigIcon();

    im_assert(!"searchSet");
    return StickerData::invalid();
}

void cancelDataRequest(std::vector<Utils::FileSharingId> _fsIds, core::sticker_size _size)
{
    std::vector<Utils::FileSharingId> toCancel;
    toCancel.reserve(_fsIds.size());
    for (auto& fsId : _fsIds)
    {
        auto sticker = getSticker(fsId);
        if (sticker && sticker->isRequested(_size) && !sticker->isFailed() && !sticker->hasData(_size))
        {
            toCancel.emplace_back(std::move(fsId));
            sticker->setRequested(_size, false);
        }
    }
    if (!toCancel.empty())
        Ui::GetDispatcher()->getStickerCancel(toCancel, _size);
}

QString getSetName(int32_t _setId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getName();
    im_assert(!"searchSet");
    return QString();
}

void showStickersPack(const int32_t _set_id, StatContext context)
{
    QTimer::singleShot(0, [_set_id, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), _set_id, QString(), {}, context);
        dlg.show();
    });
}

void showStickersPackByStoreId(const QString& _store_id, StatContext context)
{
    QTimer::singleShot(0, [_store_id, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), -1, _store_id, {}, context);
        dlg.show();
    });
}

void showStickersPackByFileId(const Utils::FileSharingId& _filesharingId, StatContext context)
{
    if (_filesharingId.sourceId && !Features::isSharedFederationStickerpacksSupported())
    {
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("stickers", "Stickerpack is not available"));
        return;
    }

    QTimer::singleShot(0, [_filesharingId, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), -1, QString(), _filesharingId, context);
        dlg.show();
    });
}

void showStickersPackByStickerId(const Data::StickerId& _sticker_id, StatContext context)
{
    if (_sticker_id.fsId_)
        showStickersPackByFileId(*_sticker_id.fsId_, context);
    else if (_sticker_id.obsoleteId_)
        showStickersPack(_sticker_id.obsoleteId_->setId_, context);
}

setSptr parseSet(const core::coll_helper& _coll_set)
{
    auto newset = std::make_shared<Set>();
    newset->unserialize(_coll_set);
    return newset;
}

void addSet(setSptr _set)
{
    getCache().addSet(std::move(_set));
}

setSptr getSet(const int32_t _setId)
{
    return getCache().getSet(_setId);
}

setSptr getStoreSet(const int32_t _setId)
{
    return getCache().getStoreSet(_setId);
}

setSptr findSetByFsId(const Utils::FileSharingId& _fsId)
{
    return getCache().findSetByFsId(_fsId);
}

bool getSuggest(const QString& _keyword, Suggest& _suggest, std::set<SuggestType>& _types)
{
    return getCache().getSuggest(_keyword, _suggest, _types);
}

bool getSuggestWithSettings(const QString& _keyword, Suggest& _suggest)
{
    std::set<SuggestType> types;

    if (get_gui_settings()->get_value<bool>(settings_show_suggests_emoji, true))
        types.insert(SuggestType::suggestEmoji);

    if (get_gui_settings()->get_value<bool>(settings_show_suggests_words, true))
        types.insert(SuggestType::suggestWord);

    return getCache().getSuggest(_keyword, _suggest, types);
}

std::string_view recentsStickerSettingsPath()
{
    static const std::string path = get_account_setting_name(settings_recents_fs_stickers);
    return path;
}

void resetCache()
{
    if (g_cache)
        g_cache.reset();
}

Cache& getCache()
{
    if (!g_cache)
        g_cache = std::make_unique<Ui::Stickers::Cache>(nullptr);

    return (*g_cache);
}

void requestSearch(const QString &_term)
{
    getCache().requestSearch(_term);
}

std::string getStatContextStr(StatContext _context)
{
    if (_context == StatContext::Chat)
        return "Chat";
    else if (_context == StatContext::Discover)
        return "Discover";
    else if (_context == StatContext::Search)
        return "Search";
    else if (_context == StatContext::Browser)
        return "Browser";

    return std::string();
}

const setsMap &getSetsTree()
{
    return getCache().getSetsTree();
}

const setsMap &getStoreTree()
{
    return getCache().getStoreTree();
}

void addStickers(const std::vector<Utils::FileSharingId>& _fsIds, const QString& _text, const Stickers::SuggestType _type)
{
    getCache().addStickerByFsId(_fsIds, _text, _type);
}

UI_STICKERS_NS_END
