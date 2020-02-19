#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../url_config.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../main_window/smiles_menu/StickerPackInfo.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "stickers.h"

UI_STICKERS_NS_BEGIN

std::unique_ptr<Cache> g_cache;
Cache& getCache();

constexpr const std::string_view emoji_type("emoji");

const int32_t getSetIconEmptySize()
{
    return Utils::scale_value(128);
}

Sticker::Sticker() = default;

Sticker::Sticker(const int32_t _id)
    : id_(_id)
{
    assert(id_ > 0);
}

Sticker::Sticker(const QString& _fsId)
    : fsId_(_fsId)
{
}

void Sticker::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");

    fsId_ = QString::fromUtf8(_coll.get_value_as_string("fs_id", ""));

    setId_ = _coll.get_value_as_int("set_id", -1);

    if (_coll.is_value_exist("emoji"))
    {
        const auto emojis = _coll.get_value_as_array("emoji");
        const auto size = emojis->size();
        emojis_.reserve(size);
        for (core::iarray::size_type i = 0; i < size; ++i)
            emojis_.push_back(QString::fromUtf8(emojis->get_at(i)->get_as_string()));
    }

    isGif_ = ComplexMessage::extractContentTypeFromFileSharingId(fsId_).is_gif();
}

int32_t Sticker::getId() const
{
    return id_;
}

int32_t Sticker::getSetId() const
{
    return setId_;
}

const QString& Sticker::getFsId() const
{
    return fsId_;
}

void Sticker::setFailed(const bool _failed)
{
    failed_ = _failed;
}

bool Sticker::isFailed() const
{
    return failed_;
}

QImage Sticker::getImage(const core::sticker_size _size, const bool _scaleIfNeed, bool& _scaled) const
{
    _scaled = false;

    if (const auto found = images_.find(_size); found != images_.end())
    {
        const auto& image = std::get<0>(found->second);

        if (!image.isNull() || !_scaleIfNeed)
            return image;
    }

    if (!images_.empty() && _scaleIfNeed)
    {
        for (const auto& x : boost::adaptors::reverse(boost::adaptors::values(images_)))
        {
            if (const auto& image = std::get<0>(x); !image.isNull())
            {
                _scaled = true;
                return image;
            }
        }

        return std::get<0>(images_.rbegin()->second);
    }

    return QImage();
}

void Sticker::setImage(const core::sticker_size _size, QImage _image)
{
    assert(!_image.isNull());
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    auto &imageData = images_[_size];

    if (std::get<0>(imageData).isNull())
        imageData = std::make_tuple(std::move(_image), false);
}

bool Sticker::isImageRequested(const core::sticker_size _size) const
{
    if (const auto found = images_.find(_size); found != images_.end())
        return std::get<1>(found->second);
    return false;
}

void Sticker::setImageRequested(const core::sticker_size _size, const bool _val)
{
    auto &imageData = images_[_size];

    imageData = std::make_tuple(
        std::get<0>(imageData),
        _val
        );

    assert(!_val || std::get<0>(imageData).isNull());
}

const std::vector<QString>& Sticker::getEmojis()
{
    return emojis_;
}

void Sticker::clearCache()
{
    for (auto &pair : images_)
        pair.second = std::make_tuple(QImage(), false);
}

const std::map<core::sticker_size, Sticker::image_data>& Sticker::getImages() const
{
    return images_;
}

bool Sticker::isGif() const
{
    return isGif_;
}

Set::Set(int32_t _maxSize)
    : id_(-1)
    , purchased_(true)
    , user_(true)
    , maxSize_(_maxSize)
{

}


QImage Set::getStickerImage(const int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        iter = stickersTree_.insert(std::make_pair(_stickerId, std::make_shared<Sticker>(_stickerId))).first;
    }

    bool scaled = false;
    auto image = iter->second->getImage(_size, _scaleIfNeed, scaled);

    if ((scaled || image.isNull()) && !iter->second->isImageRequested(_size) && !iter->second->isFailed())
    {
        const auto setId = getId();
        assert(setId > 0);

        const auto stickerId = iter->second->getId();
        assert(stickerId);

        Ui::GetDispatcher()->getSticker(setId, stickerId, _size);

        iter->second->setImageRequested(_size, true);
    }

    return image;
}

void Set::setStickerFailed(const int32_t _stickerId)
{
    assert(_stickerId > 0);

    stickerSptr updateSticker;

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        updateSticker = std::make_shared<Ui::Stickers::Sticker>(_stickerId);
        stickersTree_[_stickerId] = updateSticker;
    }
    else
    {
        updateSticker = iter->second;
    }

    updateSticker->setFailed(true);
}

void Set::setStickerImage(const int32_t _stickerId, const core::sticker_size _size, QImage _image)
{
    assert(_stickerId > 0);

    stickerSptr updateSticker;

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        updateSticker = std::make_shared<Ui::Stickers::Sticker>(_stickerId);
        stickersTree_[_stickerId] = updateSticker;
    }
    else
    {
        updateSticker = iter->second;
    }

    updateSticker->setImage(_size, std::move(_image));
}

void Set::setBigIcon(QImage _image)
{
    bigIcon_ = QPixmap::fromImage(std::move(_image));
}

void Set::setId(int32_t _id)
{
    id_= _id;
}

int32_t Set::getId() const
{
    return id_;
}

bool Set::empty() const
{
    return stickers_.empty();
}

void Set::setName(const QString& _name)
{
    name_ = _name;
}

QString Set::getName() const
{
    return name_;
}

void Set::setStoreId(const QString& _storeId)
{
    storeId_ = _storeId;
}

QString Set::getStoreId() const
{
    return storeId_;
}

void Set::setDescription(const QString& _description)
{
    description_ = _description;
}

QString Set::getDescription() const
{
    return description_;
}

void Set::setSubtitle(const QString &_subtitle)
{
    subtitle_ = _subtitle;
}

QString Set::getSubtitle() const
{
    return subtitle_;
}

void Set::setPurchased(const bool _purchased)
{
    purchased_ = _purchased;
}

bool Set::isPurchased() const
{
    return purchased_;
}

void Set::setUser(const bool _user)
{
    user_ = _user;
}

bool Set::isUser() const
{
    return user_;
}

void Set::loadIcon(const char* _data, int32_t _size)
{
    icon_.loadFromData((const uchar*)_data, _size);

    Utils::check_pixel_ratio(icon_);
}

QPixmap Set::getIcon() const
{
    return icon_;
}

QPixmap Set::getBigIcon() const
{
    if (bigIcon_.isNull())
        Ui::GetDispatcher()->getSetIconBig(getId());

    return bigIcon_;
}

void Set::loadBigIcon(const char* _data, int32_t _size)
{
    bigIcon_.loadFromData((const uchar*)_data, _size);
}

stickerSptr Set::getSticker(int32_t _stickerId) const
{
    if (const auto iter = stickersTree_.find(_stickerId); iter != stickersTree_.end())
        return iter->second;

    return nullptr;
}

int32_t Set::getCount() const
{
    return (int32_t) stickers_.size();
}

int32_t Set::getStickerPos(const QString& _fsId) const
{
    for (size_t i = 0; i < stickers_.size(); ++i)
    {
        if (stickers_[i] == _fsId)
            return int32_t(i);
    }

    return -1;
}

void Set::unserialize(core::coll_helper _coll)
{
    setId(_coll.get_value_as_int("id"));
    setName(QString::fromUtf8(_coll.get_value_as_string("name")));
    setStoreId(QString::fromUtf8(_coll.get_value_as_string("store_id")));
    setPurchased(_coll.get_value_as_bool("purchased"));
    setUser(_coll.get_value_as_bool("usersticker"));
    setName(QString::fromUtf8(_coll.get_value_as_string("name")));
    setDescription(QString::fromUtf8(_coll.get_value_as_string("description")));
    setSubtitle(QString::fromUtf8(_coll.get_value_as_string("subtitle")));

    if (_coll.is_value_exist("icon"))
    {
        core::istream* iconStream = _coll.get_value_as_stream("icon");
        if (iconStream)
        {
            if (const int32_t iconSize = iconStream->size(); iconSize > 0)
                loadIcon((const char*)iconStream->read(iconSize), iconSize);
        }
    }

    const auto sticks = _coll.get_value_as_array("stickers");

    const auto size = sticks->size();
    stickers_.reserve(size);

    for (core::iarray::size_type i = 0; i < size; ++i)
    {
        core::coll_helper coll_sticker(sticks->get_at(i)->get_as_collection(), false);

        auto insertedSticker = std::make_shared<Sticker>();
        insertedSticker->unserialize(coll_sticker);

        fsStickersTree_[insertedSticker->getFsId()] = insertedSticker;
        stickers_.push_back(insertedSticker->getFsId());

        containsGifSticker_ |= insertedSticker->isGif();
    }
}

void Set::resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size)
{
    if (const auto iter = stickersTree_.find(_stickerId); iter != stickersTree_.end())
        iter->second->setImageRequested(_size, false);
}

void Set::clearCache()
{
    for (auto & iter : stickersTree_)
        iter.second->clearCache();

    for (auto & iter : fsStickersTree_)
        iter.second->clearCache();
}

const stickersMap& Set::getStickersTree() const
{
    return stickersTree_;
}

const FsStickersMap& Set::getFsStickersTree() const
{
    return fsStickersTree_;
}

bool Set::containsGifSticker() const
{
    return containsGifSticker_;
}

const stickersArray& Set::getStickers() const
{
    return stickers_;
}

void unserialize(core::coll_helper _coll)
{
    getCache().unserialize(_coll);
}

void unserialize_store(core::coll_helper _coll)
{
    getCache().unserialize_store(_coll);
}

bool unserialize_store_search(int64_t _seq, core::coll_helper _coll)
{
    return getCache().unserialize_store_search(_seq, _coll);
}

void unserialize_suggests(core::coll_helper _coll)
{
    getCache().unserialize_suggests(_coll);
}

std::vector<Sticker> getFsByIds(core::coll_helper _coll)
{
    std::vector<Sticker> result;

    if (_coll.is_value_exist("stickers"))
    {
        const auto* stickers = _coll.get_value_as_array("stickers");
        const auto size = stickers->size();
        result.reserve(size_t(size));
        for (core::iarray::size_type i = 0; i < size; ++i)
        {
            core::coll_helper coll_sticker(stickers->get_at(i)->get_as_collection(), false);
            Sticker s;
            s.unserialize(coll_sticker);
            result.push_back(std::move(s));
        }
    }

    return result;
}

void clean_search_cache()
{
    getCache().clean_search_cache();
}

void setStickerData(core::coll_helper _coll)
{
    getCache().setStickerData(_coll);
}

void setSetBigIcon(core::coll_helper _coll)
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

const setsIdsArray &getSearchStickersSets()
{
    return getCache().getSearchSets();
}

void clearCache()
{
    getCache().clearCache();
}

std::shared_ptr<Sticker> getSticker(uint32_t _setId, uint32_t _stickerId)
{
    return getCache().getSticker(_setId, _stickerId);
}

std::shared_ptr<Sticker> getSticker(uint32_t /*_setId*/, const QString& _fsId)
{
    return getSticker(_fsId);
}

std::shared_ptr<Sticker> getSticker(const QString& _fsId)
{
    return getCache().getSticker(_fsId);
}

QString getPreviewBaseUrl()
{
    return getCache().getTemplatePreviewBaseUrl();
}

QString getOriginalBaseUrl()
{
    return getCache().getTemplateOriginalBaseUrl();
}

QString getSendBaseUrl()
{
    return getCache().getTemplateSendBaseUrl();
}

Cache::Cache() = default;

void Cache::setStickerData(const core::coll_helper& _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id", -1);

    const qint32 stickerId = _coll.get_value_as_int("sticker_id");
    const qint32 error = _coll.get_value_as_int("error");
    const QString fsId = QString::fromUtf8(_coll.get_value_as_string("fs_id"));

    setSptr stickerSet;
    stickerSptr sticker;

    if (fsId.isEmpty())
    {
        if (const auto iterSet = setsTree_.find(setId); iterSet == setsTree_.end())
        {
            stickerSet = std::make_shared<Set>();
            stickerSet->setId(setId);
            setsTree_[setId] = stickerSet;
            stickerSet->setPurchased(false);
        }
        else
        {
            stickerSet = iterSet->second;
        }
    }
    else
    {
        auto& s = fsStickers_[fsId];
        if (!s)
            s = std::make_shared<Ui::Stickers::Sticker>(fsId);
        sticker = s;
    }


    if (error == 0)
    {
        const auto loadData = [&_coll, stickerSet, sticker, stickerId](std::string_view _id, const core::sticker_size _size)
        {
            if (_coll->is_value_exist(_id))
            {
                auto data = _coll.get_value_as_stream(_id);
                const auto dataSize = data->size();

                QImage image;
                if (!image.loadFromData(data->read(dataSize), dataSize))
                    return false;

                if (stickerSet)
                    stickerSet->setStickerImage(stickerId, _size, std::move(image));
                else if (sticker)
                    sticker->setImage(_size, std::move(image));
                else
                    assert(!"sticker error");
            }
            return true;
        };

        const auto res =
        {
            loadData("data/small", core::sticker_size::small),
            loadData("data/medium", core::sticker_size::medium),
            loadData("data/large", core::sticker_size::large),
            loadData("data/xlarge", core::sticker_size::xlarge),
            loadData("data/xxlarge", core::sticker_size::xxlarge),
        };
        if (std::any_of(res.begin(), res.end(), [](const bool _r) { return !_r; }))
        {
            if (stickerSet)
                stickerSet->setStickerFailed(stickerId);
            else if (sticker)
                sticker->setFailed(true);
            else
                assert(!"sticker error");
        }
    }
    else
    {
        if (stickerSet)
            stickerSet->setStickerFailed(stickerId);
        else if (sticker)
            sticker->setFailed(true);
        else
            assert(!"sticker error");
    }

    for (auto size : { core::sticker_size::small, core::sticker_size::medium, core::sticker_size::large, core::sticker_size::xlarge, core::sticker_size::xxlarge })
    {
        if (stickerSet)
            stickerSet->resetFlagRequested(stickerId, size);
        else if (sticker)
            sticker->setImageRequested(size, false);
        else
            assert(!"sticker error");
    }
}

void Cache::setSetBigIcon(const core::coll_helper& _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id");

    setSptr stickerSet;

    auto iterSet = storeTree_.find(setId);
    if (iterSet == storeTree_.end())
    {
        stickerSet = std::make_shared<Set>();
        stickerSet->setId(setId);
        storeTree_[setId] = stickerSet;
    }
    else
    {
        stickerSet = iterSet->second;
    }

    const qint32 error = _coll.get_value_as_int("error");

    if (error == 0)
    {
        if (!_coll->is_value_exist("icon"))
        {
            return;
        }

        const auto data = _coll.get_value_as_stream("icon");

        const auto dataSize = data->size();

        QImage image;

        if (image.loadFromData(data->read(dataSize), dataSize))
        {
            stickerSet->setBigIcon(std::move(image));
        }
        else
        {
            stickerSet->setBigIcon(
                QImage(
                    getSetIconEmptySize(),
                    getSetIconEmptySize(),
                    QImage::Format_ARGB32_Premultiplied));

            GetDispatcher()->cleanSetIconBig(setId);
        }
    }
}


void Cache::unserialize(const core::coll_helper &_coll)
{
    templateOriginalBaseUrl_ = QString::fromUtf8(_coll.get_value_as_string("template_url_original"));
    templatePreviewBaseUrl_ = QString::fromUtf8(_coll.get_value_as_string("template_url_preview"));
    templateSendBaseUrl_ = QString::fromUtf8(_coll.get_value_as_string("template_url_send"));

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

        auto insertedSet = std::make_shared<Set>();

        insertedSet->unserialize(collSet);

        const auto id = insertedSet->getId();

        sets_.push_back(id);

        for (const auto& [fsId, sticker] : insertedSet->getFsStickersTree())
            fsStickers_[fsId] = sticker;

        setsTree_[id] = std::move(insertedSet);
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

        const auto set_id = collSet.get_value_as_int("id");

        auto iter = storeTree_.find(set_id);
        if (iter != storeTree_.end())
        {
            iter->second->unserialize(collSet);
        }
        else
        {
            auto insertedSet = std::make_shared<Set>();

            insertedSet->unserialize(collSet);

            const auto id = insertedSet->getId();

            storeTree_[id] = std::move(insertedSet);
        }

        storeSets_.push_back(set_id);
    }
}

bool Cache::unserialize_store_search(int64_t _seq, const core::coll_helper &_coll)
{
    bool updated = false;

    if (_seq != searchSeqId_)
        return updated;

    updated = true;
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return updated;

    searchSets_.clear();
    searchSets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); ++i)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = std::make_shared<Set>();

        insertedSet->unserialize(collSet);

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
                QString::fromUtf8(collSticker.get_value_as_string("fs_id")),
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

        const QString word = QString::fromUtf8(coll_alias.get_value_as_string("word")).toLower();

        for (core::iarray::size_type j = 0, emojiArraySize = emojiArray->size(); j < emojiArraySize; ++j)
        {
            QString emoji = QString::fromUtf8(emojiArray->get_at(j)->get_as_string());

            aliases_[word].push_back(std::move(emoji));
        }
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
    return nullptr;
}

setSptr Cache::getStoreSet(int32_t _setId) const
{
    if (const auto iter = storeTree_.find(_setId); iter != storeTree_.end())
        return iter->second;
    return nullptr;
}

void Cache::addSet(std::shared_ptr<Set> _set)
{
    for (const auto& [fsId, sticker] : _set->getFsStickersTree())
        fsStickers_[fsId] = sticker;
    const auto id = _set->getId();
    setsTree_[id] = std::move(_set);
}

void Cache::addStickerByFsId(const std::vector<QString>& _fsIds, const QString& _keyword, const SuggestType _type)
{
    for (const auto& id : _fsIds)
    {
        if (auto& fsSticker = fsStickers_[id]; !fsSticker)
            fsSticker = std::make_shared<Ui::Stickers::Sticker>(id);

        auto& infoV = suggests_[_keyword];
        if (std::none_of(infoV.begin(), infoV.end(), [&id](const auto & x) { return x.fsId_ == id; }))
            infoV.emplace_back(id, _type);
    }
}

setSptr Cache::insertSet(int32_t _setId)
{
    auto iter = setsTree_.find(_setId);
    if (iter != setsTree_.end())
    {
        assert(false);
        return iter->second;
    }

    auto insertedSet = std::make_shared<Set>();
    insertedSet->setId(_setId);
    setsTree_[_setId] = insertedSet;

    return insertedSet;
}

stickerSptr Cache::insertSticker(const QString& _fsId)
{
    auto& s = fsStickers_[_fsId];
    if (!s)
        s = std::make_shared<Ui::Stickers::Sticker>(_fsId);
    return s;
}

std::shared_ptr<Sticker> Cache::getSticker(uint32_t _setId, uint32_t _stickerId) const
{
    if (const auto iterSet = setsTree_.find(_setId); iterSet != setsTree_.end())
        return iterSet->second->getSticker(_stickerId);
    return nullptr;
}

stickerSptr Cache::getSticker(const QString& _fsId) const
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

const setsMap &Cache::getSetsTree() const
{
    return setsTree_;
}

const setsMap &Cache::getStoreTree() const
{
    return storeTree_;
}

const QString& Cache::getTemplatePreviewBaseUrl() const
{
    assert(!templatePreviewBaseUrl_.isEmpty());
    return templatePreviewBaseUrl_;
}

const QString& Cache::getTemplateOriginalBaseUrl() const
{
    assert(!templateOriginalBaseUrl_.isEmpty());
    return templateOriginalBaseUrl_;
}

QString Cache::getTemplateSendBaseUrl() const
{
    assert(!templateSendBaseUrl_.isEmpty());
    if (templateSendBaseUrl_.isEmpty())
        return ql1s("https://") % Ui::getUrlConfig().getUrlFilesParser() % ql1c('/');

    return templateSendBaseUrl_;
}

int32_t getSetStickersCount(int32_t _setId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getCount();
    assert(!"searchSet");
    return 0;
}

int32_t getStickerPosInSet(int32_t _setId, const QString& _fsId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getStickerPos(_fsId);
    assert(!"searchSet");
    return -1;
}

const stickersArray& getStickers(int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return getCache().insertSet(_setId)->getStickers();
    }

    return searchSet->getStickers();
}

QImage getStickerImage(int32_t _setId, int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    auto searchSet = getCache().getSet(_setId);
    if (!searchSet)
        searchSet = getCache().insertSet(_setId);

    return searchSet->getStickerImage(_stickerId, _size, _scaleIfNeed);
}

QImage getStickerImage(int32_t /*_setId*/, const QString& _fsId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    return getStickerImage(_fsId, _size, _scaleIfNeed);
}

QImage getStickerImage(const QString& _fsId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    auto s = getCache().getSticker(_fsId);
    if (!s)
        s = getCache().insertSticker(_fsId);

    bool scaled = false;
    auto image = s->getImage(_size, _scaleIfNeed, scaled);

    if ((scaled || image.isNull()) && !s->isImageRequested(_size) && !s->isFailed())
    {
        Ui::GetDispatcher()->getSticker(_fsId, _size);

        s->setImageRequested(_size, true);
    }

    return image;
}

QPixmap getSetIcon(int32_t _setId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getIcon();
    assert(!"searchSet");
    return QPixmap();
}

QString getSetName(int32_t _setId)
{
    if (const auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->getName();
    assert(!"searchSet");
    return QString();
}

bool isConfigHasSticker(int32_t _setId, int32_t _stickerId)
{
    bool found = false;

    const auto& sets = getStickersSets();

    for (auto _id : sets)
    {
        if (_id == _setId)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    return !!getSticker(_setId, _stickerId);
}

void showStickersPack(const int32_t _set_id, StatContext context)
{
    QTimer::singleShot(0, [_set_id, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), _set_id, QString(), QString(), context);

        dlg.show();
    });
}

void showStickersPackByStoreId(const QString& _store_id, StatContext context)
{
    QTimer::singleShot(0, [_store_id, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), -1, _store_id, QString(), context);

        dlg.show();
    });
}

void showStickersPackByFileId(const QString& _file_id, StatContext context)
{
    QTimer::singleShot(0, [_file_id, context]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), -1, QString(), _file_id, context);

        dlg.show();
    });
}

std::shared_ptr<Set> parseSet(core::coll_helper _coll_set)
{
    auto newset = std::make_shared<Set>();

    newset->unserialize(_coll_set);

    return newset;
}

void addSet(std::shared_ptr<Set> _set)
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

bool isUserSet(const int32_t _setId)
{
    if (auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->isUser();
    return true;
}

bool isPurchasedSet(const int32_t _setId)
{
    if (auto searchSet = getCache().getSet(_setId); searchSet)
        return searchSet->isPurchased();

    return true;
}

void resetCache()
{
    if (g_cache)
        g_cache.reset();
}

Cache& getCache()
{
    if (!g_cache)
        g_cache = std::make_unique<Ui::Stickers::Cache>();

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
    else
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

void addStickers(const std::vector<QString>& _fsIds, const QString& _text, const Stickers::SuggestType _type)
{
    getCache().addStickerByFsId(_fsIds, _text, _type);
}

UI_STICKERS_NS_END
