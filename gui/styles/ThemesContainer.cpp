#include "stdafx.h"
#include "ThemesContainer.h"
#include "ThemeParameters.h"
#include "ThemesUtils.h"
#include "ThemeCommon.h"

#include "../fonts.h"

#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"
#include "../core_dispatcher.h"
#include "../../gui.shared/product.h"

namespace
{
    const auto DEFAULT_JSON_PATH = build::GetProductVariant(qsl(":/themes/default_meta"), qsl(":/themes/default_meta"), qsl(":/themes/biz_meta"), qsl(":/themes/dit_meta"));
    constexpr std::chrono::milliseconds updateCheckDelay = std::chrono::seconds(10);
    constexpr std::chrono::milliseconds unloadCheckTimeout = std::chrono::hours(1);
    constexpr auto checkMetaUpdates = false;
    constexpr auto userWallpaperOverlay = Styling::StyleVariable::GHOST_ULTRALIGHT;

    auto darkThemeId() { return ql1s("black"); }
}

namespace Styling
{
    ThemesContainer::ThemesContainer(QObject* _parent)
        : QObject(_parent)
        , isLoaded_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::onWallpaper, this, &ThemesContainer::onWallpaper);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::onWallpaperPreview, this, &ThemesContainer::onWallpaperPreview);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &ThemesContainer::onDialogClosed);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &ThemesContainer::onImCreated);
        if (Ui::GetDispatcher()->isImCreated())
            onImCreated();

        auto unloadTimer = new QTimer(this);
        connect(unloadTimer, &QTimer::timeout, this, &ThemesContainer::unloadUnusedWallpaperImages);
        unloadTimer->start(unloadCheckTimeout.count());
    }

    void ThemesContainer::unserialize(core::coll_helper _collection)
    {
        if (isLoaded_)
            return;

        QString metaStr;
        if (_collection.is_value_exist("meta"))
        {
            if (core::istream* meta = _collection.get_value_as_stream("meta"); !meta->empty())
            {
                const auto size = meta->size();
                metaStr = QString::fromUtf8((const char *)meta->read(size), (int)size);
            }
        }

        unserializeMeta(metaStr);
        unserializeUserWallpapers(_collection);

        postImageUrlsToCore();

        unserializeSettings(_collection); // after meta is loaded

        isLoaded_ = true;
    }

    void ThemesContainer::unserializeMeta(const QString& _meta)
    {
        rapidjson::Document doc;
        std::string json = _meta.toStdString();
        doc.Parse(json);

        if (doc.HasParseError() || !doc.HasMember("style"))
        {
            assert(QFile::exists(DEFAULT_JSON_PATH));

            QFile file(DEFAULT_JSON_PATH);
            if (!file.open(QFile::ReadOnly | QFile::Text))
                return;

            json = file.readAll().toStdString();

            doc.Parse(json);
            auto i = doc.GetParseError();
            i;
            if (doc.HasParseError())
            {
                assert(false);
                return;
            }
            file.close();
        }

        const auto styleIter = doc.FindMember("style");
        const auto themeIter = doc.FindMember("themes");
        const auto wallpaperIter = doc.FindMember("wallpapers");
        const auto urlIter = doc.FindMember("base_url");

        if (themeIter != doc.MemberEnd() && themeIter->value.IsArray())
            availableThemes_.reserve(themeIter->value.Size());

        Theme defaultTheme;
        if (styleIter != doc.MemberEnd() && styleIter->value.IsObject())
            defaultTheme.unserialize(doc);

        if (themeIter != doc.MemberEnd() && themeIter->value.IsArray())
        {
            auto themesNode = themeIter->value.GetArray();

            for (auto &themeNodeIt : themesNode)
            {
                Theme tmpTheme = defaultTheme;
                tmpTheme.unserialize(themeNodeIt.GetObject());
                availableThemes_.push_back(std::make_shared<Theme>(tmpTheme));
            }
        }

        if (availableThemes_.empty())
        {
            assert(!"no themes in JSON");
            availableThemes_.push_back(std::make_shared<Theme>(defaultTheme));
        }

        assert(std::any_of(availableThemes_.begin(), availableThemes_.end(), [](const auto& _theme) { return _theme->getId() == qsl("default"); }));

        if (wallpaperIter != doc.MemberEnd() && wallpaperIter->value.IsArray())
        {
            const auto addWallpaper = [](const ThemePtr& _theme, const auto& _node)
            {
                ThemeWallpaper wp(*_theme);
                wp.unserialize(_node);
                wp.setParentThemeId(_theme->getId());

                const auto wpPtr = std::make_shared<ThemeWallpaper>(wp);
                _theme->addWallpaper(wpPtr);
            };

            for (const auto& wallpaperNodeIt : wallpaperIter->value.GetArray())
            {
                const auto wpThemeIter = wallpaperNodeIt.FindMember("themes");
                if (wpThemeIter != wallpaperNodeIt.MemberEnd() && wpThemeIter->value.IsArray())
                {
                    for (const auto& th : wpThemeIter->value.GetArray())
                    {
                        const auto id = StylingUtils::getString(th);
                        if (wpThemeIter->value.Size() == 1 && id == ql1s("*"))
                        {
                            for (const auto& theme : availableThemes_)
                                addWallpaper(theme, wallpaperNodeIt);
                            break;
                        }
                        else if (const auto& theme = getTheme(id))
                        {
                            addWallpaper(theme, wallpaperNodeIt);
                        }
                        else
                        {
                            assert(false);
                        }
                    }
                }
            }
        }

        baseUrl_ = StylingUtils::getString(urlIter->value);
    }

    void ThemesContainer::unserializeUserWallpapers(core::coll_helper _collection)
    {
        core::iarray* valuesArray = _collection.get_value_as_array("user_folders");
        if (!valuesArray)
        {
            assert(false);
            return;
        }

        for (int i = 0; i < valuesArray->size(); ++i)
        {
            const core::ivalue* val = valuesArray->get_at(i);
            const auto id = WallpaperId(QString::fromUtf8(val->get_as_string()));

            for (const auto& theme : availableThemes_)
            {
                auto wp = theme->getWallpaper(id);
                if (!wp)
                {
                    wp = createUserWallpaper(theme, id);
                    theme->addWallpaper(wp);
                }
            }
        }
    }

    void ThemesContainer::unserializeSettings(core::coll_helper _collection)
    {
        core::iarray* valuesArray = _collection.get_value_as_array("values");
        if (!valuesArray)
        {
            assert(false);
            return;
        }

        QString globalWallpaperId;
        QString globalTheme;
        std::map<QString, QString> contactsWP;
        std::vector<QString> tiledWP;

        for (int i = 0; i < valuesArray->size(); ++i)
        {
            const core::ivalue* val = valuesArray->get_at(i);

            Ui::gui_coll_helper collVal(val->get_as_collection(), false);
            const std::string_view name = collVal.get_value_as_string("name");
            core::istream* idata = collVal.get_value_as_stream("value");
            int len = idata->size();

            const char* data = (char *)idata->read(len);
            if (name == get_global_wp_id_setting_field())
            {
                if (const auto id = QString::fromUtf8(data, len); !id.isEmpty())
                    globalWallpaperId = id;
                else
                    assert(!"invalid global wallpaper id");
            }
            else if (name == "id" && globalWallpaperId.isEmpty()) // legacy, for backward compatibility
            {
                if (len == sizeof(int32_t))
                    globalWallpaperId = QString::number(*((int32_t*)data));
                else if (len == sizeof(int64_t))
                    globalWallpaperId = QString::number((int32_t) *((int64_t*)data));
                else
                    assert(!"invalid legacy wallpaper id");
            }
            else if (name == "contacts_themes")
            {
                const auto contactsThemes = QString::fromUtf8(data, len);
                if (contactsThemes.length() > 1)
                {
                    const auto list = contactsThemes.splitRef(ql1c(','));
                    for (const auto& item : list)
                    {
                        const auto contactTheme = item.split(ql1c(':'));
                        if (contactTheme.count() == 2)
                        {
                            const auto aimId = contactTheme[0];
                            const auto wallId = contactTheme[1];
                            contactsWP[aimId.toString()] = wallId.toString();
                        }
                    }
                }
            }
            else if (name == "tiled_wallpapers")
            {
                const auto ids = QString::fromUtf8(data, len);
                if (ids.length() > 1)
                {
                    const auto list = ids.splitRef(ql1c(','));
                    for (const auto& id : list)
                        tiledWP.push_back(id.toString());
                }
            }
            else if (name == "theme")
            {
                globalTheme = QString::fromUtf8(data, len);
            }
        }


        QSignalBlocker sb(this);

        assert(!availableThemes_.empty());

        if (!globalTheme.isEmpty())
            setCurrentTheme(globalTheme, GuiCall::no);

        if (!currentTheme_ && !availableThemes_.empty())
        {
            auto trySetDarkTheme = [this]()
            {
                if (isAppInDarkMode())
                {
                    const auto it = std::find_if(availableThemes_.begin(), availableThemes_.end(), [](const auto& t) { return t->getId() == darkThemeId(); });
                    if (it != availableThemes_.end())
                    {
                        setCurrentTheme((*it)->getId(), GuiCall::no);
                        return true;
                    }
                }
                return false;
            };

            if (!trySetDarkTheme())
                setCurrentTheme(availableThemes_.front()->getId(), GuiCall::no);
        }

        if (!globalWallpaperId.isEmpty())
            setGlobalWallpaper(globalWallpaperId, GuiCall::no);

        if (!currentWallpaper_)
        {
            if (auto wp = getThemeDefaultWallpaper())
                setGlobalWallpaper(wp->getId(), GuiCall::no);
        }

        for (const auto& wp : getAllAvailableWallpapers())
        {
            const auto id = wp->getId();
            if (id.isUser() && std::find(tiledWP.begin(), tiledWP.end(), id.id_) != tiledWP.end())
                wp->setTiled(true);
        }

        for (const auto& [aimId, wpId] : contactsWP)
            setContactWallpaper(aimId, wpId, GuiCall::no);
    }

    void ThemesContainer::postImageUrlsToCore() const
    {
        assert(!baseUrl_.isEmpty());

        Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);

        const auto allWallpapers = getAllAvailableWallpapers();
        std::set<QString> wpIds;

        core::ifptr<core::iarray> wallsArray(coll->create_array());
        wallsArray->reserve(allWallpapers.size());

        for (const auto& wall : allWallpapers)
        {
            const auto wpId = wall->getId().id_;
            if (!wall->getId().isUser() && !wpIds.count(wpId) && wall->hasWallpaper())
            {
                Ui::gui_coll_helper collWall(coll->create_collection(), true);

                collWall.set_value_as_qstring("id", wpId);
                collWall.set_value_as_qstring("image_url",   baseUrl_ % wall->getWallpaperUrl());
                collWall.set_value_as_qstring("preview_url", baseUrl_ % wall->getPreviewUrl());

                core::ifptr<core::ivalue> val(coll->create_value());
                val->set_as_collection(collWall.get());
                wallsArray->push_back(val.get());

                wpIds.insert(wpId);
            }
        }

        coll.set_value_as_array("wallpapers", wallsArray.get());

        Ui::GetDispatcher()->post_message_to_core("themes/wallpaper/urls", coll.get());
    }

    void ThemesContainer::postCurrentThemeIdToCore() const
    {
        assert(currentTheme_);
        if (!currentTheme_)
            return;

        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);
        clColl.set_value_as_qstring("id", currentTheme_->getId());

        Ui::GetDispatcher()->post_message_to_core("themes/default/id", clColl.get());
    }

    void ThemesContainer::postGlobalWallpaperToCore() const
    {
        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);

        if (currentTheme_ && currentWallpaper_ && currentWallpaper_->getId() != currentTheme_->getDefaultWallpaperId())
            clColl.set_value_as_qstring(get_global_wp_id_setting_field(), currentWallpaper_->getId().id_);

        Ui::GetDispatcher()->post_message_to_core("themes/default/wallpaper/id", clColl.get());
    }

    void ThemesContainer::postContactsWallpapersToCore() const
    {
        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(clColl->create_stream());

        const auto result = [this]()
        {
            QString result;

            for (const auto& [aimId, wp] : contactWallpapers_)
                result.append(qsl("%1:%2,").arg(aimId, wp->getId().id_));

            if (!result.isEmpty())
                result.chop(1);

            return result.toUtf8();
        }();

        if (uint size = result.size())
            data_stream->write((const uint8_t*)result.data(), size);

        clColl.set_value_as_string("name", "contacts_themes");
        clColl.set_value_as_stream("value", data_stream.get());

        Ui::GetDispatcher()->post_message_to_core("themes/settings/set", clColl.get());

        postTiledsWallpapersToCore();
    }

    void ThemesContainer::postTiledsWallpapersToCore() const
    {
        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(clColl->create_stream());

        const auto result = [this]()
        {
            QString result;

            for (const auto& [aimId, wp] : contactWallpapers_)
            {
                if (wp->getId().isUser() && wp->isTiled())
                    result.append(wp->getId().id_ % ql1c(','));
            }

            if (!result.isEmpty())
                result.chop(1);

            return result.toUtf8();
        }();

        if (uint size = result.size())
            data_stream->write((const uint8_t*)result.data(), size);

        clColl.set_value_as_string("name", "tiled_wallpapers");
        clColl.set_value_as_stream("value", data_stream.get());

        Ui::GetDispatcher()->post_message_to_core("themes/settings/set", clColl.get());
    }

    void ThemesContainer::onImCreated()
    {
        if (checkMetaUpdates)
        {
            QTimer::singleShot(updateCheckDelay.count(), this, []()
            {
                Ui::GetDispatcher()->post_message_to_core("themes/meta/check", nullptr); // answer "themes/meta/check/result" not checked
            });
        }
    }

    void ThemesContainer::onWallpaper(const WallpaperId& _id, const QPixmap& _image)
    {
        assert(_id.isValid());
        assert(!_image.isNull());

        const auto wallpapers = getAllWallpapersById(_id);
        assert(!wallpapers.empty());

        if (!wallpapers.empty())
        {
            const auto preparedWp = Utils::tintImage(_image, wallpapers.front()->getTint());

            for (const auto& wp : getAllWallpapersById(_id))
                wp->setWallpaperImage(preparedWp);

            emit wallpaperImageAvailable(_id, QPrivateSignal());
        }
    }

    void ThemesContainer::onWallpaperPreview(const WallpaperId& _id, const QPixmap& _image)
    {
        assert(_id.isValid());
        assert(!_image.isNull());

        for (const auto& wp : getAllWallpapersById(_id))
            wp->setPreviewImage(_image);

        emit wallpaperPreviewAvailable(_id, QPrivateSignal());
    }

    void ThemesContainer::onDialogClosed(const QString& _aimId, bool)
    {
        if (tryOnContact_ == _aimId)
            resetTryOnWallpaper();
    }

    WallpaperPtr ThemesContainer::getThemeDefaultWallpaper() const
    {
        assert(currentTheme_);

        if (currentTheme_)
        {
            assert(currentTheme_->getDefaultWallpaperId().isValid());
            auto wp = currentTheme_->getDefaultWallpaper();
            if (!wp)
                wp = currentTheme_->getFirstWallpaper();

            return wp;
        }

        return WallpaperPtr();
    }

    void ThemesContainer::setCurrentTheme(const QString& _themeName, const GuiCall _fromGui)
    {
        if (const auto& theme = getTheme(_themeName))
        {
            const auto changingCurTheme = !!currentTheme_;
            currentTheme_ = theme;

            {
                QSignalBlocker sb(this);

                if (const auto& wp = getThemeDefaultWallpaper(); wp && changingCurTheme)
                    setGlobalWallpaper(wp->getId(), GuiCall::yes);

                for (const auto& [aimId, wp] : contactWallpapers_)
                    setContactWallpaper(aimId, wp->getId(), GuiCall::no);
            }

            if (_fromGui == GuiCall::yes)
                postCurrentThemeIdToCore();

            emit globalThemeChanged(QPrivateSignal());
        }
    }

    void ThemesContainer::setGlobalWallpaper(const WallpaperId& _id, const GuiCall _fromGui)
    {
        if (const auto& t = getCurrentTheme())
        {
            if (const auto& wall = t->getWallpaper(_id))
            {
                if (currentWallpaper_ != wall || _id.isUser())
                {
                    currentWallpaper_ = wall;

                    if (_fromGui == GuiCall::yes)
                        postGlobalWallpaperToCore();

                    emit globalWallpaperChanged(QPrivateSignal());
                }
            }
        }
    }

    void ThemesContainer::setContactWallpaper(const QString& _aimId, const WallpaperId& _wpId, const GuiCall _fromGui)
    {
        if (const auto& t = getCurrentTheme())
        {
            if (const auto& wall = t->getWallpaper(_wpId))
            {
                contactWallpapers_[_aimId] = wall;

                if (_fromGui == GuiCall::yes)
                {
                    emit contactWallpaperChanged(_aimId, QPrivateSignal());
                    postContactsWallpapersToCore();
                }
            }
        }
    }

    void ThemesContainer::resetContactWallpaper(const QString& _aimId, const GuiCall _fromGui)
    {
        if (const auto it = contactWallpapers_.find(_aimId); it != contactWallpapers_.end())
        {
            contactWallpapers_.erase(it);

            if (_fromGui == GuiCall::yes)
            {
                emit contactWallpaperChanged(_aimId, QPrivateSignal());
                postContactsWallpapersToCore();
            }
        }
    }

    void ThemesContainer::resetAllContactWallpapers(const GuiCall _fromGui)
    {
        std::vector<QString> aimIds;
        if (_fromGui == GuiCall::yes)
        {
            aimIds.reserve(contactWallpapers_.size());
            for (const auto& [aimId, _] : contactWallpapers_)
                aimIds.push_back(aimId);
        }

        contactWallpapers_.clear();

        if (_fromGui == GuiCall::yes)
        {
            for (const auto& aimId : aimIds)
                emit contactWallpaperChanged(aimId, QPrivateSignal());

            postContactsWallpapersToCore();
        }
    }

    WallpapersVector ThemesContainer::getAllWallpapersById(const WallpaperId& _id) const
    {
        WallpapersVector res;
        res.reserve(availableThemes_.size());

        for (const auto& theme : availableThemes_)
        {
            if (const auto& wp = theme->getWallpaper(_id))
                res.push_back(wp);
        }

        return res;
    }

    ThemePtr ThemesContainer::getCurrentTheme() const
    {
        assert(currentTheme_);
        return currentTheme_;
    }

    QString ThemesContainer::getCurrentThemeId() const
    {
        if (const auto& t = getCurrentTheme())
            return t->getId();

        return QString();
    }

    ThemePtr ThemesContainer::getTheme(const QString& _themeName) const
    {
        const auto it = std::find_if(availableThemes_.begin(), availableThemes_.end(), [&_themeName](const auto& _theme)
        {
            return _theme->getId() == _themeName;
        });

        if (it != availableThemes_.end())
            return *it;

        return nullptr;
    }

    std::vector<std::pair<QString, QString>> ThemesContainer::getAvailableThemes() const
    {
        std::vector<std::pair<QString, QString>> res;
        res.reserve(availableThemes_.size());

        // For theme naming & its translation
        // !!! Must match names in JSON
        if constexpr (false)
        {
            static auto lightThemeName = QT_TRANSLATE_NOOP("appearance", "Default");
            static auto blueThemeName = QT_TRANSLATE_NOOP("appearance", "Blue");
            static auto darkThemeName = QT_TRANSLATE_NOOP("appearance", "Night");
        }

        for (const auto& t : availableThemes_)
        {
            const auto id = t->getId();
            const auto translatedName = qApp->translate("appearance", t->getName().toUtf8().constData());
            const QString name =  build::is_debug() ? translatedName % ql1s(" [") % id % ql1c(']') : translatedName;
            res.emplace_back(t->getId(), !name.isEmpty() ? name : id);
        }

        assert(!res.empty());
        return res;
    }

    void ThemesContainer::setCurrentTheme(const QString& _themeName)
    {
        setCurrentTheme(_themeName, GuiCall::yes);
    }

    WallpaperPtr ThemesContainer::getContactWallpaper(const QString& _aimId) const
    {
        if (_aimId == tryOnContact_ && tryOnWallpaper_)
            return tryOnWallpaper_;

        const auto it = contactWallpapers_.find(_aimId);
        if (it != contactWallpapers_.cend())
            return it->second;

        return getGlobalWallpaper();
    }

    WallpaperPtr ThemesContainer::getGlobalWallpaper() const
    {
        if (currentWallpaper_)
            return currentWallpaper_;

        return nullptr;
    }

    std::map<QString, int> ThemesContainer::getContactWallpaperCounters() const
    {
        std::map<QString, int> result;

        for (const auto& [_, wall] : contactWallpapers_)
        {
            assert(wall);
            if (wall)
            {
                const auto id = wall->getId();
                if (!id.isUser())
                    result[id.id_]++;
            }
        }

        return result;
    }

    const WallpaperId& ThemesContainer::getGlobalWallpaperId() const
    {
        if (currentWallpaper_)
            return currentWallpaper_->getId();

        return WallpaperId::invalidId();
    }

    const WallpaperId& ThemesContainer::getContactWallpaperId(const QString& _aimId) const
    {
        if (const auto& w = getContactWallpaper(_aimId))
            return w->getId();

        return WallpaperId::invalidId();
    }

    Styling::WallpapersVector ThemesContainer::getAvailableWallpapers(const QString& _themeId) const
    {
        Styling::WallpapersVector res;
        if (const auto& t = getTheme(_themeId))
            res = t->getAvailableWallpapers();

        return res;
    }

    WallpapersVector ThemesContainer::getAllAvailableWallpapers() const
    {
        WallpapersVector res;
        res.reserve(availableThemes_.size());

        for (const auto& theme : availableThemes_)
        {
            for (const auto& wp : theme->getAvailableWallpapers())
                res.push_back(wp);
        }

        return res;
    }

    void ThemesContainer::addUserWallpaper(const WallpaperPtr& _wallpaper)
    {
        assert(_wallpaper);
        if (!_wallpaper)
            return;

        const auto id = _wallpaper->getId();
        const auto image = _wallpaper->getWallpaperImage();

        for (const auto& theme : availableThemes_)
        {
            auto wp = theme->getWallpaper(id);
            if (!wp)
            {
                wp = createUserWallpaper(theme, id);
                theme->addWallpaper(wp);
            }

            wp->setWallpaperImage(image);
            wp->setTiled(_wallpaper->isTiled());
        }

        if (!image.isNull())
        {
            Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);
            clColl.set_value_as_qstring("id", id.id_);

            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "jpg");

            core::ifptr<core::istream> data_stream(clColl->create_stream());
            if (!imageData.isEmpty())
                data_stream->write((const uint8_t*)imageData.constData(), (uint32_t)imageData.size());
            clColl.set_value_as_stream("image", data_stream.get());

            Ui::GetDispatcher()->post_message_to_core("themes/wallpaper/user/set", clColl.get());
        }
    }

    WallpaperPtr ThemesContainer::createUserWallpaper(const ThemePtr& _theme, const WallpaperId& _id)
    {
        assert(_theme);

        const auto defWp = _theme->getDefaultWallpaper();
        auto wp = std::make_shared<Styling::ThemeWallpaper>(*defWp);
        wp->setId(_id);
        wp->setTint(wp->getColor(userWallpaperOverlay));

        return wp;
    }

    void ThemesContainer::setGlobalWallpaper(const WallpaperId& _id)
    {
        setGlobalWallpaper(_id, GuiCall::yes);
    }

    void ThemesContainer::setContactWallpaper(const QString& _aimId, const WallpaperId& _id)
    {
        setContactWallpaper(_aimId, _id, GuiCall::yes);
    }

    void ThemesContainer::resetContactWallpaper(const QString& _aimId)
    {
        resetContactWallpaper(_aimId, GuiCall::yes);
    }

    void ThemesContainer::resetAllContactWallpapers()
    {
        resetAllContactWallpapers(GuiCall::yes);
    }

    void ThemesContainer::requestWallpaper(const WallpaperPtr _wallpaper)
    {
        assert(_wallpaper);
        const auto needRequest =
                _wallpaper &&
                _wallpaper->hasWallpaper() &&
                !_wallpaper->isImageRequested() &&
                _wallpaper->getWallpaperImage().isNull();
        if (!needRequest)
            return;

        _wallpaper->setImageRequested(true);

        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);
        clColl.set_value_as_qstring("id", _wallpaper->getId().id_);

        Ui::GetDispatcher()->post_message_to_core("themes/wallpaper/get", clColl.get());
    }

    void ThemesContainer::requestWallpaperPreview(const WallpaperPtr _wallpaper)
    {
        assert(_wallpaper);
        const auto needRequest =
                _wallpaper &&
                _wallpaper->hasWallpaper() &&
                !_wallpaper->isPreviewRequested() &&
                _wallpaper->getPreviewImage().isNull();
        if (!needRequest)
            return;

        _wallpaper->setPreviewRequested(true);

        Ui::gui_coll_helper clColl(Ui::GetDispatcher()->create_collection(), true);
        clColl.set_value_as_qstring("id", _wallpaper->getId().id_);

        Ui::GetDispatcher()->post_message_to_core("themes/wallpaper/preview/get", clColl.get());
    }

    void ThemesContainer::setTryOnWallpaper(const WallpaperId& _id, const QString& _aimId)
    {
        if (const auto& t = getCurrentTheme())
        {
            tryOnContact_ = _aimId;
            tryOnWallpaper_ = t->getWallpaper(_id);
        }
    }

    void ThemesContainer::setTryOnWallpaper(const WallpaperPtr _wallpaper, const QString & _aimId)
    {
        tryOnContact_ = _aimId;
        tryOnWallpaper_ = _wallpaper;
    }

    void ThemesContainer::resetTryOnWallpaper()
    {
        tryOnContact_.clear();
        tryOnWallpaper_.reset();
    }

    QColor ThemesContainer::getDefaultBackgroundColor()
    {
        if (const auto& t = getCurrentTheme())
        {
            if (const auto& w = t->getDefaultWallpaper())
                return w->getBgColor();
        }
        return QColor();
    }

    void ThemesContainer::unloadUnusedWallpaperImages()
    {
        std::vector<qint64> usedKeys;
        const auto addKey = [&usedKeys](const auto& _wp)
        {
            if (_wp && _wp->hasWallpaper())
            {
                if (const auto& img = _wp->getWallpaperImage(); !img.isNull())
                    usedKeys.push_back(img.cacheKey());
            }
        };

        for (const auto& [_, wp] : contactWallpapers_)
            addKey(wp);
        addKey(tryOnWallpaper_);
        addKey(currentWallpaper_);

        const auto wallpapers = getAllAvailableWallpapers();
        for (const auto& wp : wallpapers)
        {
            if (!wp->isImageRequested() && wp->hasWallpaper())
            {
                if (const auto& img = wp->getWallpaperImage(); !wp->isImageRequested() && !img.isNull())
                    if (std::none_of(usedKeys.begin(), usedKeys.end(), [imgKey = img.cacheKey()](const auto _key){ return _key == imgKey; }))
                        wp->resetWallpaperImage();
            }
        }
    }

    ThemesContainer& getThemesContainer()
    {
        static ThemesContainer cont;
        return cont;
    }

    QString getTryOnAccount()
    {
        return qsl("~tryonaccount~");
    }
}
