#include "stdafx.h"

#include "LottiePlayer.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"

#ifndef STRIP_AV_MEDIA
#include "main_window/mplayer/VideoPlayer.h"
#endif

namespace Ui
{
#ifndef STRIP_AV_MEDIA
    class LottiePLayerPrivate
    {
    public:
        LottiePLayerPrivate(LottiePlayer* _lottiePlayer, QWidget* _parent, const QString& _path, QRect _rect, const QPixmap& _preview)
            : player_(std::make_unique<DialogPlayer>(_parent, DialogPlayer::Flags::lottie))
            , visible_(_parent->isVisible())
        {
            player_->setMedia(_path);
            player_->setParent(_parent);
            player_->updateSize(_rect);
            player_->setReplay(true);
            if (!_preview.isNull())
                player_->setPreview(_preview);

            QObject::connect(player_.get(), &DialogPlayer::firstFrameReady, player_.get(), &DialogPlayer::setPreviewFromFirstFrame);
            QObject::connect(player_.get(), &DialogPlayer::firstFrameReady, _lottiePlayer, &LottiePlayer::firstFrameReady);

            startIfVisible();
        }

        void setRect(QRect _rect)
        {
            player_->updateSize(_rect);
        }

        void onVisibilityChanged(bool _visible, QWidget* _parent)
        {
            if (!_parent)
            {
                _visible = false;
            }
            else if (_visible)
            {
                if (_parent->isHidden() || _parent->visibleRegion().isEmpty())
                    _visible = false;

                const auto parentWindow = _parent->window();
                const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
                if (parentWindow == mainWindow)
                    _visible = _visible && mainWindow->isActive();
                else
                    _visible = _visible && parentWindow->isActiveWindow();
            }

            visible_ = _visible;
            startIfVisible();
            player_->updateVisibility(_visible);
        }

        const QPixmap& getPreview() const noexcept
        {
            return player_->getPreview();
        }

        const QString& getPath() const noexcept
        {
            return player_->mediaPath();
        }

    private:
        void startIfVisible()
        {
            if (visible_ && !started_)
            {
                started_ = true;

                player_->start(true);
                player_->show();
            }
        }

    private:
        std::unique_ptr<DialogPlayer> player_;
        bool visible_ = false;
        bool started_ = false;
    };
#else
    class LottiePLayerPrivate
    {
    public:
        LottiePLayerPrivate(LottiePlayer*, QWidget*, const QString&, QRect, const QPixmap&) {}
        void setRect(QRect) {}
        void onVisibilityChanged(bool, QWidget*) {}
        const QPixmap& getPreview() const noexcept
        {
            static const QPixmap pm;
            return pm;
        }

        const QString& getPath() const noexcept
        {
            static const QString s;
            return s;
        }
    };
#endif

    LottiePlayer::LottiePlayer(QWidget* _parent, const QString& _path, QRect _rect, const QPixmap& _preview)
        : QObject(_parent)
        , d(std::make_unique<LottiePLayerPrivate>(this, _parent, _path, _rect, _preview))
    {
        im_assert(parent());
        im_assert(!_path.isEmpty());

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activationChanged, this, &LottiePlayer::onVisibilityChanged, Qt::UniqueConnection);
        _parent->installEventFilter(this);
    }

    LottiePlayer::~LottiePlayer() = default;

    void LottiePlayer::setRect(QRect _rect)
    {
        d->setRect(std::move(_rect));
    }

    void LottiePlayer::onVisibilityChanged(bool _visible)
    {
        d->onVisibilityChanged(_visible, qobject_cast<QWidget*>(parent()));
    }

    const QPixmap& LottiePlayer::getPreview() const noexcept
    {
        return d->getPreview();
    }

    const QString& LottiePlayer::getPath() const noexcept
    {
        return d->getPath();
    }

    bool LottiePlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == parent())
        {
            if (_event->type() == QEvent::Show || _event->type() == QEvent::Paint)
                onVisibilityChanged(true);
            else if (_event->type() == QEvent::Hide)
                onVisibilityChanged(false);
        }
        return QObject::eventFilter(_obj, _event);
    }
}