#include "stdafx.h"

#include "MediaControls.h"
#include "../MessageStyle.h"
#include "previewer/Drawable.h"
#include "utils/utils.h"
#include "fonts.h"
#include "main_window/MainPage.h"

namespace
{
    QString durationStr(int32_t _duration)
    {
        QTime t(0, 0);
        t = t.addSecs(_duration);
        QString durationStr;
        if (t.hour())
            durationStr += QString::number(t.hour()) % ql1c(':');
        durationStr += QString::number(t.minute()) % ql1c(':');
        if (t.second() < 10)
            durationStr += ql1c('0');
        durationStr += QString::number(t.second());
        return durationStr;
    }

    QSize faviconSize()
    {
        return QSize(16, 16);
    }
}

namespace Ui
{
    //////////////////////////////////////////////////////////////////////////
    // TextIconBase
    //////////////////////////////////////////////////////////////////////////

    class TextIconBase : public BDrawable
    {
    public:
        TextIconBase()
        {
            label_.setVerticalPosition(TextRendering::VerPosition::MIDDLE);
        }
        ~TextIconBase() = default;
        void draw(QPainter &_p) override;
        void setRect(const QRect& _rect) override;

        Label label_;
        Icon icon_;

    protected:
        virtual int iconLeftMargin() { return Utils::scale_value(6); }
        virtual int labelMargin() { return Utils::scale_value(4); }

        virtual void setLabelRect(const QRect& _rect) = 0;
    };

    void TextIconBase::draw(QPainter &_p)
    {
        if (!visible_)
            return;

        BDrawable::draw(_p);
        label_.draw(_p);
        icon_.draw(_p);
    }

    void TextIconBase::setRect(const QRect &_rect)
    {
        Drawable::setRect(_rect);

        auto iconSize = icon_.pixmapSize();
        auto iconRect = _rect;
        iconRect.setLeft(_rect.left() + iconLeftMargin());
        iconRect.setTop(_rect.top() + (_rect.height() - iconSize.height()) / 2);
        iconRect.setSize(iconSize);
        icon_.setRect(iconRect);

        setLabelRect(_rect);
    }

    //////////////////////////////////////////////////////////////////////////
    // StaticTextIcon
    //////////////////////////////////////////////////////////////////////////

    class StaticTextIcon : public TextIconBase
    {
    private:
        void setLabelRect(const QRect& _rect) override;
    };

    void StaticTextIcon::setLabelRect(const QRect& _rect)
    {
        auto iconRect = icon_.rect();
        auto yOffset = _rect.height() / 2;
        yOffset += 1;                       //!HACK until TextUnit fixed

        auto labelRect = _rect;
        labelRect.setLeft(iconRect.right() + labelMargin());
        label_.setYOffset(yOffset);
        label_.setRect(labelRect);
    }

    //////////////////////////////////////////////////////////////////////////
    // DynamicTextIcon
    //////////////////////////////////////////////////////////////////////////

    class DynamicTextIcon : public TextIconBase
    {
    public:
        void setLabelLeftMargin(int _margin) { labelLeftMargin_ = _margin; }
        void setLabelRightMargin(int _margin) { labelRightMargin_ = _margin; }

    protected:
        void setLabelRect(const QRect& _rect) override;

        int labelLeftMargin_ = 0;
        int labelRightMargin_ = 0;
    };

    void DynamicTextIcon::setLabelRect(const QRect& _rect)
    {
        auto textWidth = label_.desiredWidth();

        auto yOffset = _rect.height() / 2;
//        yOffset += 1;                       //!HACK until TextUnit fixed

        auto labelRect = _rect;
        labelRect.setLeft(_rect.right() + labelLeftMargin_);
        labelRect.setWidth(textWidth);
        label_.setYOffset(yOffset);
        label_.setRect(labelRect);

        auto rect = rect_;
        rect.setWidth(rect_.width() + labelRect.width() + labelLeftMargin_ + labelRightMargin_);

        BDrawable::setRect(rect);
    }

    static auto animationTimeout = static_cast<int>(MessageStyle::getHiddenControlsAnimationTime().count());
    static auto updateInterval = 10;
    static auto hideTimeout = 3200;

    //////////////////////////////////////////////////////////////////////////
    // SiteName
    //////////////////////////////////////////////////////////////////////////
    class SiteName : public DynamicTextIcon
    {
    public:
        void setLabelRect(const QRect& _rect) override;
    };

    void SiteName::setLabelRect(const QRect& _rect)
    {
        auto iconRect = icon_.rect();
        auto availableForLabel = _rect.width() - iconRect.width() - iconLeftMargin() - labelLeftMargin_ - labelRightMargin_;

        auto labelWidth = std::min(label_.desiredWidth(), availableForLabel);
        auto labelRect = _rect;
        labelRect.setLeft(iconRect.right() + labelLeftMargin_);
        labelRect.setWidth(labelWidth);

        label_.setYOffset(_rect.height() / 2);
        label_.setRect(labelRect);

        auto rect = _rect;
        rect.setWidth(iconLeftMargin() + iconRect.width() + labelRect.width() + labelLeftMargin_ + labelRightMargin_);

        BDrawable::setRect(rect);
    }


    //////////////////////////////////////////////////////////////////////////
    // MediaControls_p
    //////////////////////////////////////////////////////////////////////////

    class MediaControls_p
    {
    public:

        enum AnimationDirection
        {
            None,
            Show,
            Hide
        };

        enum AnimationType
        {
            ShowAnimated,
            HideAnimated,
        };

        int commonMargin() const { return Ui::MessageStyle::getHiddenControlsShift(); }
        int commonHeight() const { return 20; }
        QColor defaultBackground() const { return QColor(0, 0, 0, static_cast<int>(0.5 * 255)); }
        QColor hoveredBackground() const { return QColor(0, 0, 0, static_cast<int>(0.6 * 255)); }
        QColor pressedBackground() const { return QColor(0, 0, 0, static_cast<int>(0.7 * 255)); }

        QSize objectSize(MediaControls::ControlType _type, const QSize& _size) const;
        QPoint objectPosition(MediaControls::ControlType _type, const QSize& _size) const;
        bool shouldBeAnimated(MediaControls::ControlType _type, MediaControls::State _state, bool _visible) const;

        QPoint animationStartPosition(MediaControls::ControlType _type, const QSize& _size) const;
        QPoint animationPosition(MediaControls::ControlType _type, const QSize& _size, int _progress, bool _reversed) const;
        int animationProgress(int _animationDuration, int _elapsedTime) const;

        void createFullscreen();
        void createDuration(int32_t _duration);
        void createEmptyDuration();
        void createGifLabel();
        void createNoSound();
        void createMute();
        void createPlay();
        void createSiteName(const QString& _siteName, const QPixmap& _favicon);
        void createCopyLink();
        void createObjects(const std::set<MediaControls::ControlType>& _controlsSet);
        Drawable* getObject(MediaControls::ControlType _type);

        QRect clickArea(MediaControls::ControlType _type);
        void updateObjectsPositions(const QRect &_rect, int _animationProgress = 100);
        bool visibilityForCurrentMode(MediaControls::ControlType _type);
        bool visibilityForState(MediaControls::ControlType _type, MediaControls::State _state, bool _mouseOver);
        void updateVisibility(bool _mouseOver, MediaControls::State _state);
        void updateVisibilityAnimated(bool _mouseover, MediaControls::State _state);

        QTimer animationTimer_;
        QTimer updateTimer_;
        QTimer hideTimer_;
        bool doubleClick_;
        int32_t duration_ = 0;
        MediaControls::State state_ = MediaControls::Preview;
        uint32_t options_ = MediaControls::ControlsOptions::None;

        std::map<MediaControls::ControlType, std::unique_ptr<Drawable>> objects_;
        std::map<MediaControls::ControlType, AnimationType> animations_;

        struct QueuedAnimationState
        {
            bool mouseOver_;
            MediaControls::State state_;
        };

        std::optional<QueuedAnimationState> queuedAnimationState_;
    };

    QSize MediaControls_p::objectSize(MediaControls::ControlType _type, const QSize& _size) const
    {
        switch (_type)
        {
            case MediaControls::Mute:
            case MediaControls::CopyLink:
            case MediaControls::Fullscreen:
            case MediaControls::EmptyDuration:
                return Utils::scale_value(QSize(28, commonHeight()));

            case MediaControls::NoSound: // 'no sound' item takes rect for icon and expands it based on label size
                return Utils::scale_value(QSize(26, commonHeight()));
            case MediaControls::SiteName:
                return QSize(_size.width() - 2 * commonMargin(), Utils::scale_value(commonHeight()));

            case MediaControls::Duration:
                return Utils::scale_value(QSize(55, commonHeight()));

            case MediaControls::GifLabel:
                return Utils::scale_value(QSize(36, commonHeight()));

            case MediaControls::Play:
                return Utils::scale_value(QSize(48, 48));
        }

        return QSize();
    }

    QPoint MediaControls_p::objectPosition(MediaControls::ControlType _type, const QSize& _size) const
    {
        switch (_type)
        {
            case MediaControls::NoSound:
                return {commonMargin(), _size.height() - objectSize(MediaControls::NoSound, _size).height() - commonMargin()};
            case MediaControls::Mute:
                return {commonMargin(), _size.height() - objectSize(MediaControls::Mute, _size).height() - commonMargin()};
            case MediaControls::SiteName:
                return {commonMargin(), _size.height() - objectSize(MediaControls::SiteName, _size).height() - commonMargin()};
            case MediaControls::Fullscreen:
                return {_size.width() - objectSize(MediaControls::Fullscreen, _size).width() - commonMargin(), commonMargin()};

            case MediaControls::CopyLink:
                if (objects_.find(MediaControls::Mute) != objects_.end() && state_ != MediaControls::Preview)
                    return {objectPosition(MediaControls::Mute, _size).x() + commonMargin() + objectSize(MediaControls::Mute, _size).width(),
                                _size.height() - objectSize(MediaControls::CopyLink, _size).height() - commonMargin()};
                else
                    return objectPosition(MediaControls::Mute, _size);

            case MediaControls::Duration:
            case MediaControls::GifLabel:
            case MediaControls::EmptyDuration:
                return {commonMargin(), commonMargin()};

            case MediaControls::Play:
                return {(_size.width() - objectSize(MediaControls::Play, _size).width()) / 2, (_size.height() - objectSize(MediaControls::Play, _size).height()) / 2};
        }

        return QPoint();
    }

    bool MediaControls_p::shouldBeAnimated(MediaControls::ControlType _type, MediaControls::State _state, bool _visible) const
    {
        switch (_type)
        {
            case MediaControls::SiteName:
                return !_visible;
            case MediaControls::CopyLink:
            case MediaControls::Mute:
                return _state != MediaControls::Paused || !_visible;
            default:
                return true;
        }
    }

    QPoint MediaControls_p::animationStartPosition(MediaControls::ControlType _type, const QSize& _size) const
    {
        switch (_type)
        {
            case MediaControls::Mute:
                return {objectPosition(MediaControls::Mute, _size).x(), objectPosition(MediaControls::Mute, _size).y() + commonMargin()};
            case MediaControls::NoSound:
                return {objectPosition(MediaControls::NoSound, _size).x(), objectPosition(MediaControls::NoSound, _size).y() + commonMargin()};
            case MediaControls::SiteName:
                return {objectPosition(MediaControls::SiteName, _size).x(), objectPosition(MediaControls::SiteName, _size).y() + commonMargin()};
            case MediaControls::Fullscreen:
                return {objectPosition(MediaControls::Fullscreen, _size).x(), objectPosition(MediaControls::Fullscreen, _size).y() - commonMargin()};
            case MediaControls::CopyLink:
                return {objectPosition(MediaControls::CopyLink, _size).x(), objectPosition(MediaControls::CopyLink, _size).y() + commonMargin()};

            case MediaControls::Duration:
            case MediaControls::GifLabel:
            case MediaControls::EmptyDuration:
                return {commonMargin(), 0};

            case MediaControls::Play:
                return objectPosition(MediaControls::Play, _size);
        }

        return QPoint();
    }

    QPoint MediaControls_p::animationPosition(MediaControls::ControlType _type, const QSize& _size, int _progress, bool _reversed) const // progress is from 1 to 100
    {
        if (_progress == 100)
            return objectPosition(_type, _size);

        auto start = (_reversed ? objectPosition(_type, _size) : animationStartPosition(_type, _size));
        auto end = (_reversed ? animationStartPosition(_type, _size) : objectPosition(_type, _size));

        auto dx = QPoint(end - start).x() * _progress / 100;
        auto dy = QPoint(end - start).y() * _progress / 100;

        return start + QPoint(dx, dy);
    }

    int MediaControls_p::animationProgress(int _animationDuration, int _elapsedTime) const
    {
        return static_cast<double>(_elapsedTime) / _animationDuration * 100;;
    }

    void MediaControls_p::createFullscreen()
    {
        auto fullscreenButton = std::make_unique<BButton>();
        fullscreenButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/full_screen_icon"), QSize(20, 20), Qt::white));
        fullscreenButton->setPixmapCentered(true);
        fullscreenButton->setBorderRadius(Utils::scale_value(10));
        fullscreenButton->background_ = defaultBackground();
        fullscreenButton->hoveredBackground_ = hoveredBackground();
        fullscreenButton->pressedBackground_ = pressedBackground();

        objects_[MediaControls::Fullscreen] = std::move(fullscreenButton);
    }

    void MediaControls_p::createDuration(int32_t _duration)
    {
        auto duration = std::make_unique<StaticTextIcon>();
        auto durationUnit = TextRendering::MakeTextUnit(durationStr(_duration));
        durationUnit->init(Fonts::appFontScaled(11, Fonts::FontWeight::Normal), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        duration->label_.setTextUnit(std::move(durationUnit));
        duration->icon_.setPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/duration_time_icon"), QSize(14, 14), Qt::white));
        duration->background_ = defaultBackground();
        duration->setBorderRadius(Utils::scale_value(10));

        objects_[MediaControls::Duration] = std::move(duration);
    }

    void MediaControls_p::createEmptyDuration()
    {
        auto duration = std::make_unique<BButton>();
        duration->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/duration_time_icon"), QSize(14, 14), Qt::white));
        duration->setPixmapCentered(true);
        duration->setBorderRadius(Utils::scale_value(10));
        duration->background_ = defaultBackground();

        objects_[MediaControls::EmptyDuration] = std::move(duration);
    }

    void MediaControls_p::createGifLabel()
    {
        auto gifLabel = std::make_unique<BLabel>();
        auto gifUnit = TextRendering::MakeTextUnit(qsl("GIF"));
        gifUnit->init(Fonts::appFontScaled(13, Fonts::FontWeight::SemiBold), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
        gifLabel->setTextUnit(std::move(gifUnit));
        gifLabel->background_ = defaultBackground();
        gifLabel->setBorderRadius(Utils::scale_value(10));
        gifLabel->setVerticalPosition(TextRendering::VerPosition::MIDDLE);
        gifLabel->setYOffset(Utils::scale_value(commonHeight()) / 2);

        objects_[MediaControls::GifLabel] = std::move(gifLabel);
    }

    void MediaControls_p::createNoSound()
    {
        auto noSound = std::make_unique<DynamicTextIcon>();
        noSound->setLabelRightMargin(Utils::scale_value(4));
        if (!(options_ & MediaControls::ShortNoSound))
        {
            auto noSoundUnit = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("videoplayer", "No sound"));
            noSoundUnit->init(Fonts::appFontScaled(11, Fonts::FontWeight::Normal), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
            noSound->label_.setTextUnit(std::move(noSoundUnit));
        }
        noSound->icon_.setPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/mute_video_icon"), QSize(20, 20), QColor(255, 255, 255, 255 * 0.34)));
        noSound->background_ = defaultBackground();
        noSound->setBorderRadius(Utils::scale_value(10));

        objects_[MediaControls::NoSound] = std::move(noSound);
    }

    void MediaControls_p::createMute()
    {
        auto muteButton = std::make_unique<BButton>();
        muteButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/mute_video_icon"), QSize(20, 20), Qt::white));
        muteButton->setCheckable(true);
        muteButton->setCheckedPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/sound_on_icon"), QSize(20, 20), Qt::white));
        muteButton->setPixmapCentered(true);
        muteButton->setBorderRadius(Utils::scale_value(10));
        muteButton->background_ = defaultBackground();
        muteButton->hoveredBackground_ = hoveredBackground();
        muteButton->pressedBackground_ = pressedBackground();

        objects_[MediaControls::Mute] = std::move(muteButton);
    }

    void MediaControls_p::createPlay()
    {
        auto playButton = std::make_unique<BButton>();
        playButton->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/video_pause"), QSize(28, 28)));
        playButton->setCheckable(options_ & MediaControls::PlayClickable);
        playButton->setCheckedPixmap(Utils::renderSvgScaled(qsl(":/videoplayer/video_play"), QSize(28, 28)));
        playButton->setPixmapCentered(true);
        playButton->setBorderRadius(Utils::scale_value(24));
        playButton->background_ = defaultBackground();
        playButton->hoveredBackground_ = hoveredBackground();
        playButton->pressedBackground_ = pressedBackground();

        objects_[MediaControls::Play] = std::move(playButton);
    }

    void MediaControls_p::createSiteName(const QString& _siteName, const QPixmap& _favicon)
    {
        auto siteName = std::make_unique<SiteName>();
        siteName->setLabelLeftMargin(Utils::scale_value(2));
        siteName->setLabelRightMargin(Utils::scale_value(6));
        auto siteNameUnit = TextRendering::MakeTextUnit(_siteName, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        siteNameUnit->init(Fonts::appFontScaled(12, Fonts::FontWeight::Medium), Qt::white, Qt::white, QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        siteName->label_.setTextUnit(std::move(siteNameUnit));
        const auto pixmap = _favicon.isNull() ? Utils::renderSvgScaled(qsl(":/copy_link_icon"), faviconSize(), Qt::white) : _favicon;
        siteName->icon_.setPixmap(pixmap);
        siteName->background_ = defaultBackground();
        siteName->setBorderRadius(Utils::scale_value(10));

        objects_[MediaControls::SiteName] = std::move(siteName);
    }

    void MediaControls_p::createCopyLink()
    {
        auto copyLink = std::make_unique<BButton>();
        copyLink->setDefaultPixmap(Utils::renderSvgScaled(qsl(":/copy_icon"), QSize(14, 14), Qt::white));
        copyLink->setPixmapCentered(true);
        copyLink->setBorderRadius(Utils::scale_value(10));
        copyLink->background_ = defaultBackground();
        copyLink->hoveredBackground_ = hoveredBackground();
        copyLink->pressedBackground_ = pressedBackground();

        objects_[MediaControls::CopyLink] = std::move(copyLink);
    }

    void MediaControls_p::createObjects(const std::set<MediaControls::ControlType>& _controlsSet)
    {        
        if (_controlsSet.find(MediaControls::Fullscreen) != _controlsSet.end())
            createFullscreen();

        if (_controlsSet.find(MediaControls::Play) != _controlsSet.end())
            createPlay();

        if (_controlsSet.find(MediaControls::Mute) != _controlsSet.end())
            createMute();

        if (_controlsSet.find(MediaControls::Duration) != _controlsSet.end() && !(options_ & MediaControls::CompactMode))
        {
            if (duration_)
                createDuration(duration_);
            else
                createEmptyDuration();
        }

        if (_controlsSet.find(MediaControls::GifLabel) != _controlsSet.end())
            createGifLabel();

        if (_controlsSet.find(MediaControls::SiteName) != _controlsSet.end())
            createSiteName(QString(), QPixmap());

        if (_controlsSet.find(MediaControls::CopyLink) != _controlsSet.end())
            createCopyLink();
    }

    Drawable* MediaControls_p::getObject(MediaControls::ControlType _type)
    {
        auto objectIt = objects_.find(_type);
        if (objectIt != objects_.end())
            return objectIt->second.get();

        return nullptr;
    }

    QRect MediaControls_p::clickArea(MediaControls::ControlType _type)
    {
        switch (_type)
        {
            case MediaControls::Mute:
            case MediaControls::Fullscreen:
                return objects_[_type]->rect().adjusted(-commonMargin(), -commonMargin(), commonMargin(), commonMargin());
            default:
                return objects_[_type]->rect();
        }
    }

    void MediaControls_p::updateObjectsPositions(const QRect& _rect, int _animationProgress)
    {
        for (auto & [type, object] : objects_)
        {
            QPoint position;

            if (auto it = animations_.find(type); it != animations_.end())
                position = animationPosition(type, _rect.size(), _animationProgress, it->second == AnimationType::HideAnimated);
            else
                position = objectPosition(type, _rect.size());

            object->setRect(QRect(position, objectSize(type, _rect.size())));
        }
    }

    bool MediaControls_p::visibilityForCurrentMode(MediaControls::ControlType _type)
    {
        switch (_type)
        {
            case MediaControls::Play:
                return true;
            default:
                return !(options_ & MediaControls::CompactMode);
        }
    }    

    bool MediaControls_p::visibilityForState(MediaControls::ControlType _type, MediaControls::State _state, bool _mouseOver)
    {
        auto visible = false;
        switch (_state)
        {
            case MediaControls::Preview:
                switch (_type)
                {
                    case MediaControls::Play:
                        visible = (options_ & MediaControls::ShowPlayOnPreview);
                        break;
                    case MediaControls::CopyLink:
                        visible = _mouseOver;
                        break;
                    case MediaControls::SiteName:
                        visible = !_mouseOver;
                        break;
                    case MediaControls::GifLabel:
                    case MediaControls::Duration:
                    case MediaControls::EmptyDuration:
                        visible = true;
                        break;
                    default:
                        break;
                }
                break;

            case MediaControls::Paused:
                switch (_type)
                {
                    case MediaControls::Play:
                    case MediaControls::GifLabel:
                    case MediaControls::Duration:
                    case MediaControls::EmptyDuration:
                        visible = true;
                        break;
                    case MediaControls::Mute:
                    case MediaControls::NoSound:
                    case MediaControls::CopyLink:
                    case MediaControls::Fullscreen:
                        visible = _mouseOver;
                        break;
                    case MediaControls::SiteName:
                        visible = !_mouseOver;
                        break;
                    default:
                        break;
                }
                break;
            case MediaControls::Playing:
                switch (_type)
                {
                    case MediaControls::SiteName:
                        visible = false;
                        break;
                    default:
                        visible = _mouseOver;
                        break;
                }
                break;
        }

        return visible && visibilityForCurrentMode(_type);
    }

    void MediaControls_p::updateVisibility(bool _mouseOver, MediaControls::State _state)
    {
        for (auto & [type, object] : objects_)
            object->setVisible(visibilityForState(type, _state, _mouseOver));
    }

    void MediaControls_p::updateVisibilityAnimated(bool _mouseover, MediaControls::State _state)
    {
        if (animationTimer_.isActive())
        {
            queuedAnimationState_ = QueuedAnimationState{_mouseover, _state};
            return;
        }

        animations_.clear();

        for (auto& [type, object] : objects_)
        {
            const auto visible = object->visible();
            const auto visibleForNewState = visibilityForState(type, _state, _mouseover);
            const auto needsAnimation = shouldBeAnimated(type, state_, visible);

            if (visible && !visibleForNewState)
            {
                if (needsAnimation)
                    animations_[type] = AnimationType::HideAnimated;
                else
                    object->setVisible(false);
            }

            if (!visible && visibleForNewState)
            {
                if (needsAnimation)
                    animations_[type] = AnimationType::ShowAnimated;

                object->setVisible(true);
            }
        }

        if (!animations_.empty())
        {
            animationTimer_.start();
            updateTimer_.start();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // MediaControls
    //////////////////////////////////////////////////////////////////////////

    MediaControls::MediaControls(const std::set<MediaControls::ControlType>& _controls, uint32_t _options, QWidget* _parent)
        : QWidget(_parent)
        , d(new MediaControls_p())
    {
        d->options_ = _options;
        d->createObjects(_controls);
        d->updateObjectsPositions(rect());

        d->animationTimer_.setSingleShot(true);
        d->animationTimer_.setInterval(animationTimeout);
        connect(&d->animationTimer_, &QTimer::timeout, this, &MediaControls::onAnimationTimeout);

        d->updateTimer_.setInterval(updateInterval);
        connect(&d->updateTimer_, &QTimer::timeout, this, [this](){ update(); });

        d->hideTimer_.setInterval(hideTimeout);
        d->hideTimer_.setSingleShot(true);

        connect(&d->hideTimer_, &QTimer::timeout, this, [this]()
        {
            if (d->state_ == Playing)
                d->updateVisibilityAnimated(false, d->state_);
        });

        d->updateVisibility(false, d->state_);
        setMouseTracking(true);
    }

    MediaControls::~MediaControls()
    {
    }

    void MediaControls::setState(MediaControls::State _state)
    {
        d->state_ = _state;

        if (_state == Playing)
            d->hideTimer_.start();

        if (auto play = d->getObject(Play))
            play->setChecked(_state != Playing);

        auto mouseOver = rect().contains(mapFromGlobal(QCursor::pos()));

        d->updateObjectsPositions(rect());
        d->updateVisibilityAnimated(mouseOver, _state);

        update();
    }

    void MediaControls::setRect(const QRect& _rect)
    {
        setGeometry(_rect);
        d->updateObjectsPositions(_rect);
    }

    void MediaControls::setMute(bool _enable)
    {
        if (auto mute = d->getObject(Mute))
            mute->setChecked(!_enable);
    }

    void MediaControls::setDuration(int32_t _duration)
    {
        auto emptyDurationIt = d->objects_.find(EmptyDuration);
        if (emptyDurationIt != d->objects_.end())
            d->objects_.erase(emptyDurationIt);

        auto durationIt = d->objects_.find(Duration);
        if (durationIt == d->objects_.end())
        {
            d->createDuration(_duration);
            d->updateObjectsPositions(rect());
            d->duration_ = _duration;
        }

        update();
    }

    void MediaControls::setPosition(int32_t _position)
    {
        if (auto durationObject = d->getObject(Duration))
        {
            auto duration = dynamic_cast<TextIconBase*>(durationObject);
            duration->label_.setText(durationStr(d->duration_ - _position));
            update(duration->rect());
        }
    }

    void MediaControls::setGotAudio(bool _gotAudio)
    {
        if (_gotAudio)
        {
            auto noSoundIt = d->objects_.find(NoSound);
            if (noSoundIt != d->objects_.end())
                d->objects_.erase(noSoundIt);

            auto muteIt = d->objects_.find(Mute);
            if (muteIt == d->objects_.end())
                d->createMute();
        }
        else
        {
            auto muteIt = d->objects_.find(Mute);
            if (muteIt != d->objects_.end())
                d->objects_.erase(muteIt);

            auto noSoundIt = d->objects_.find(NoSound);
            if (noSoundIt == d->objects_.end())
                d->createNoSound();
        }

        d->updateObjectsPositions(rect());
        d->updateVisibility(true, d->state_);
        update();
    }

    void MediaControls::setSiteName(const QString& _siteName)
    {
        auto siteNameIt = d->objects_.find(SiteName);
        if (siteNameIt != d->objects_.end())
        {
            auto siteName = dynamic_cast<TextIconBase*>(siteNameIt->second.get());
            siteName->label_.setText(_siteName);
            update(siteName->rect());
        }
        else
        {
            d->createSiteName(_siteName, QPixmap());
        }

        if (auto copyLinkIt = d->objects_.find(CopyLink); copyLinkIt == d->objects_.end()) // create copy link if sitename control is present
            d->createCopyLink();

        d->updateObjectsPositions(rect());
        d->updateVisibility(false, d->state_);
        update();
    }

    void MediaControls::setFavicon(const QPixmap& _favicon)
    {
        const auto scaledPixmap = _favicon.scaled(Utils::scale_bitmap_with_value(faviconSize()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto siteNameIt = d->objects_.find(SiteName);
        if (siteNameIt != d->objects_.end())
        {
            auto siteName = dynamic_cast<TextIconBase*>(siteNameIt->second.get());
            siteName->icon_.setPixmap(scaledPixmap);
            update(siteName->rect());
        }
        else
        {
            d->createSiteName(QString(), scaledPixmap);
        }

        if (auto copyLinkIt = d->objects_.find(CopyLink); copyLinkIt == d->objects_.end()) // create copy link if sitename control is present
            d->createCopyLink();

        d->updateObjectsPositions(rect());
        d->updateVisibility(false, d->state_);
        update();
    }

    int MediaControls::bottomLeftControlsWidth() const
    {
        if (const auto copyLink = d->getObject(CopyLink))
            return copyLink->rect().right();

        if (const auto noSound = d->getObject(NoSound))
            return noSound->rect().right();

        if (const auto mute = d->getObject(Mute))
            return mute->rect().right();

        return 0;
    }

    void MediaControls::paintEvent(QPaintEvent* _event)
    {
        Q_UNUSED(_event)

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        auto progress = 100;

        if (d->animationTimer_.isActive())
        {
            progress = d->animationProgress(animationTimeout, animationTimeout - d->animationTimer_.remainingTime());
            d->updateObjectsPositions(rect(), progress);
        }

        for (auto & [type, object] : d->objects_)
        {
            auto objectProgress = progress;
            auto drawWithOpacity = false;

            if (auto it = d->animations_.find(type); it != d->animations_.end())
            {
                drawWithOpacity = true;
                if ( it->second == MediaControls_p::AnimationType::HideAnimated)
                    objectProgress = 100 - progress;
            }

            p.setOpacity(drawWithOpacity ? objectProgress / 100. : 1);

            object->draw(p);
        }
    }

    void MediaControls::mouseMoveEvent(QMouseEvent* _event)
    {
        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_event->pos());
            auto needUpdate = object->hovered() != overObject;
            object->setHovered(overObject);

            if (needUpdate)
                update(object->rect());
        }

        QWidget::mouseMoveEvent(_event);

        d->updateVisibilityAnimated(true, d->state_);
        d->hideTimer_.start();
    }

    void MediaControls::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        onMousePressed(_event->pos());

        MainPage::instance()->setFocusOnInput();
    }

    void MediaControls::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        auto overAnyObject = false;

        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_event->pos()) && object->clickable();
            auto pressed = object->pressed();
            object->setPressed(false);

            if ((pressed) && overObject && object->checkable())
                object->setChecked(!object->checked());

            update(object->rect());

            if (pressed && overObject)
                 onClick(type);

            overAnyObject |= overObject;
        }

        if (!overAnyObject)
        {
            if (!d->doubleClick_)
                onDefaultClick();
            else
                onDoubleClick();
        }

        d->doubleClick_ = false;
    }

    void MediaControls::mouseDoubleClickEvent(QMouseEvent* _event)
    {
        if (_event->button() != Qt::LeftButton)
            return _event->ignore();

        d->doubleClick_ = true;

        onMousePressed(_event->pos());
    }

    void MediaControls::enterEvent(QEvent* _event)
    {
        Q_UNUSED(_event)
        d->updateVisibilityAnimated(true, d->state_);
    }

    void MediaControls::leaveEvent(QEvent* _event)
    {
        Q_UNUSED(_event)
        d->updateVisibilityAnimated(false, d->state_);
    }

    void MediaControls::onAnimationTimeout()
    {
        for (auto& [type, animationType] : d->animations_)
        {
            if (animationType == MediaControls_p::AnimationType::HideAnimated)
                d->objects_[type]->setVisible(false);
        }

        d->updateObjectsPositions(rect());
        d->updateTimer_.stop();

        if (d->queuedAnimationState_)
        {
            d->updateVisibilityAnimated(d->queuedAnimationState_->mouseOver_, d->queuedAnimationState_->state_);
            d->queuedAnimationState_.reset();
        }
        else
        {
            d->animations_.clear();
            update();
        }
    }

    void MediaControls::onClick(ControlType _type)
    {
        switch (_type)
        {
            case Fullscreen:
            {
                Q_EMIT fullscreen();
                break;
            }
            case Mute:
            {
                auto & muteButton = d->objects_[Mute];
                if (muteButton)
                    Q_EMIT mute(!muteButton->checked());
                break;
            }
            case Play:
            {
                auto & playButton = d->objects_[Play];
                if (playButton)
                {
                    if (d->options_ & ControlsOptions::PlayClickable)
                    {
                        auto enablePlay = playButton->checked();
                        Q_EMIT play(enablePlay);
                    }
                    else
                    {
                        onDefaultClick();
                    }
                }

                break;
            }
            case CopyLink:
            {
                Q_EMIT copyLink();
                break;
            }
            default:
                break;
        }
        d->hideTimer_.start();
    }

    void MediaControls::onDefaultClick()
    {
        auto playButton = d->getObject(Play);
        if (playButton && d->state_ != Preview && d->options_ & ControlsOptions::PlayClickable)
        {
            auto checked = playButton->checked();
            playButton->setChecked(!checked);
            d->state_ = !checked ? Paused : Playing;
            Q_EMIT play(!checked);
        }
        else
        {
            Q_EMIT clicked();
        }
    }

    void MediaControls::onDoubleClick()
    {
        if (d->state_ != Preview)
            Q_EMIT fullscreen();
    }

    void MediaControls::onMousePressed(const QPoint& _pos)
    {
        for (auto & [type, object] : d->objects_)
        {
            auto overObject = d->clickArea(type).contains(_pos);
            auto needUpdate = object->pressed() != overObject;
            object->setPressed(overObject);

            if (needUpdate)
                update(object->rect());
        }
    }

}
