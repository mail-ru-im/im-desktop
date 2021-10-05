#include "stdafx.h"

#include "AttachFilePopup.h"
#include "InputWidgetUtils.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "utils/graphicsEffects.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"
#include "core_dispatcher.h"

#include "main_window/MainPage.h"

namespace
{
    int itemHeight()
    {
        return Utils::scale_value(40);
    }

    int iconWidth()
    {
        return Utils::scale_value(32);
    }

    int pixmapWidth()
    {
        return Utils::scale_value(20);
    }

    int popupLeftOffset()
    {
        return Utils::scale_value(6);
    }

    int popupBottomOffset()
    {
        return Utils::scale_value(50);
    }

    int iconOffset()
    {
        return Ui::getHorSpacer() - popupLeftOffset();
    }

    int textOffset()
    {
        return iconOffset() + iconWidth() + Ui::getHorSpacer();
    }

    QColor captionColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor hoveredBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    int getListVerMargin()
    {
        return Utils::scale_value(2);
    }

    int getShadowSize()
    {
        return Utils::scale_value(8);
    }

    QColor getShadowColor()
    {
        return QColor(162, 137, 137, 0.3 * 255);
    }

    constexpr std::chrono::milliseconds showHideDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds leaveHideDelay() noexcept { return std::chrono::milliseconds(500); }

    static QPointer<Ui::AttachFilePopup> shownInstance;
}

namespace Ui
{
    AttachFileMenuItem::AttachFileMenuItem(QWidget* _parent, const QString& _icon, const QString& _caption, const QColor& _iconBgColor)
        : SimpleListItem(_parent)
        , iconBgColor_(_iconBgColor)
        , icon_(Utils::renderSvg(_icon, { pixmapWidth(), pixmapWidth() }, _iconBgColor))
    {
        caption_ = TextRendering::MakeTextUnit(_caption);
        caption_->init(Fonts::appFontScaled(15), captionColor());
        caption_->setOffsets(textOffset(), itemHeight() / 2);
        caption_->evaluateDesiredSize();

        iconBgColor_.setAlphaF(0.05);

        setFixedHeight(itemHeight());
        setMinimumWidth(caption_->horOffset() + caption_->desiredWidth() + Utils::scale_value(24));

        connect(this, &SimpleListItem::hoverChanged, this, &AttachFileMenuItem::setSelected);
    }

    void AttachFileMenuItem::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;

            Q_EMIT selectChanged(QPrivateSignal());
            update();
        }
    }

    bool AttachFileMenuItem::isSelected() const
    {
        return isSelected_;
    }

    void AttachFileMenuItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (isSelected())
            p.fillRect(rect(), hoveredBackgroundColor());

        const auto iconW = iconWidth();
        const auto iconX = iconOffset();
        const auto iconY = (height() - iconW) / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(iconBgColor_);
        p.drawEllipse(iconX, iconY, iconW, iconW);

        const auto pmX = iconX + (iconW - pixmapWidth()) / 2;
        const auto pmY = iconY + (iconW - pixmapWidth()) / 2;
        p.drawPixmap(pmX, pmY, icon_);

        caption_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    AttachPopupBackground::AttachPopupBackground(QWidget* _parent)
        : QWidget(_parent)
    {
        auto shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(Utils::scale_value(4));
        shadowEffect->setOffset(0, 0);
        shadowEffect->setColor(getShadowColor());

        setGraphicsEffect(shadowEffect);

        setContentsMargins(getShadowSize(), getListVerMargin() + getShadowSize(), getShadowSize(), getListVerMargin() + getShadowSize());
    }

    int AttachPopupBackground::heightForContent(int _height) const
    {
        return _height + contentsMargins().top() + contentsMargins().bottom();
    }

    void AttachPopupBackground::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.setPen(Qt::NoPen);
        p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        const auto radius = Utils::scale_value(4);
        const auto fillRect = rect().adjusted(getShadowSize(), getShadowSize(), -getShadowSize(), -getShadowSize());
        p.drawRoundedRect(fillRect, radius, radius);
    }

    AttachFilePopup::AttachFilePopup(QWidget* _parent)
        : ClickableWidget(_parent)
        , opacityEffect_(new Utils::OpacityEffect(this))
        , opacityAnimation_ (new QVariantAnimation(this))
        , persistent_(false)
        , pttEnabled_(true)
    {
        setCursor(Qt::ArrowCursor);

        listWidget_ = new SimpleListWidget(Qt::Vertical, this);
        Testing::setAccessibleName(listWidget_, qsl("AS Plus listWidget"));
        connect(listWidget_, &SimpleListWidget::clicked, this, &AttachFilePopup::onItemClicked);

        listWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        connect(this, &ClickableWidget::pressed, this, &AttachFilePopup::onBackgroundClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &AttachFilePopup::initItems);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &AttachFilePopup::initItems);

        opacityAnimation_->setDuration(showHideDuration().count());
        opacityAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        connect(opacityAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            opacityEffect_->setOpacity(value.toDouble());
        });
        connect(opacityAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            if (animState_ == AnimState::Hiding)
                hide();
            animState_ = AnimState::None;
        });

        widget_ = new AttachPopupBackground(this);
        auto layout = Utils::emptyVLayout(widget_);
        layout->addWidget(listWidget_);

        setGraphicsEffect(opacityEffect_);

        hideTimer_.setSingleShot(true);
        hideTimer_.setInterval(leaveHideDelay());
        connect(&hideTimer_, &QTimer::timeout, this, &AttachFilePopup::onHideTimer);

        initItems();
        hide();
    }

    void AttachFilePopup::initItems()
    {
        items_.clear();
        listWidget_->clear();

        const auto addItem = [this](const auto& _icon, const auto& _capt, const auto _iconBg, const auto _id, QStringView _testingName)
        {
            auto item = new AttachFileMenuItem(listWidget_, _icon, _capt, Styling::getParameters().getColor(_iconBg));
            Testing::setAccessibleName(item, u"AS AttachPopup " % _testingName);
            const auto idx = listWidget_->addItem(item);
            connect(item, &AttachFileMenuItem::hoverChanged, this, [this, idx](const bool _hovered)
            {
                if (_hovered)
                    listWidget_->setCurrentIndex(idx);
            });

            im_assert(std::none_of(items_.begin(), items_.end(), [_id](const auto& _p) { return _p.second == _id; }));
            items_.push_back({ idx, _id });
        };

        addItem(qsl(":/input/attach_photo"), QT_TRANSLATE_NOOP("input_widget", "Photo or Video"), Styling::StyleVariable::SECONDARY_RAINBOW_PINK, AttachMediaType::photoVideo, u"PhotoVideo");
        addItem(qsl(":/input/attach_documents"), QT_TRANSLATE_NOOP("input_widget", "File"), Styling::StyleVariable::SECONDARY_RAINBOW_MAIL, AttachMediaType::file, u"File");
        //addItem(qsl(":/input/attach_camera"), QT_TRANSLATE_NOOP("input_widget", "Camera"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, MenuItemId::camera);

        if (Features::pollsEnabled())
            addItem(qsl(":/input/attach_poll"), QT_TRANSLATE_NOOP("input_widget", "Poll"), Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE, AttachMediaType::poll, u"Poll");

        addItem(qsl(":/input/attach_contact"), QT_TRANSLATE_NOOP("input_widget", "Contact"), Styling::StyleVariable::SECONDARY_RAINBOW_WARM, AttachMediaType::contact, u"Contact");
        //addItem(qsl(":/message_type_contact_icon"), QT_TRANSLATE_NOOP("input_widget", "Location"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, MenuItemId::geo);

        if (Features::isTaskCreationInChatEnabled())
            addItem(qsl(":/input/attach_task"), QT_TRANSLATE_NOOP("input_widget", "Task"), Styling::StyleVariable::SECONDARY_RAINBOW_YELLOW, AttachMediaType::task, u"Task");

        if (Features::isVcsCallByLinkEnabled())
            addItem(qsl(":/copy_link_icon"), QT_TRANSLATE_NOOP("input_widget", "Call link"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, AttachMediaType::callLink, u"Link");
        if (Features::isVcsWebinarEnabled())
            addItem(qsl(":/input/webinar"), QT_TRANSLATE_NOOP("input_widget", "Webinar"), Styling::StyleVariable::SECONDARY_RAINBOW_EMERALD, AttachMediaType::webinar, u"Webinar");

        if (pttEnabled_)
            addItem(qsl(":/input/attach_ptt"), QT_TRANSLATE_NOOP("input_widget", "Voice Message"), Styling::StyleVariable::SECONDARY_ATTENTION, AttachMediaType::ptt, u"Ptt");

        updateSizeAndPos();
    }

    void AttachFilePopup::showAnimated(const QRect& _plusButtonRect)
    {
        if (animState_ == AnimState::Showing)
            return;

        if (shownInstance && shownInstance != this)
            shownInstance->hideAnimated();

        shownInstance = this;

        buttonRect_ = _plusButtonRect;
        buttonRect_.moveTopLeft(parentWidget()->mapFromGlobal(buttonRect_.topLeft()));

        updateSizeAndPos();
        mouseAreaPoly_ = getMouseAreaPoly();
        listWidget_->clearSelection();

        const auto startValue = animState_ == AnimState::Hiding ? opacityEffect_->opacity() : 0.0;
        opacityEffect_->setOpacity(startValue);
        opacityAnimation_->stop();
        opacityAnimation_->setStartValue(startValue);
        opacityAnimation_->setEndValue(1.0);
        animState_ = AnimState::Showing;
        opacityAnimation_->start();

        show();
    }

    void AttachFilePopup::hideAnimated()
    {
        if (!isVisible() || animState_ == AnimState::Hiding)
            return;

        if (shownInstance == this)
            shownInstance.clear();

        const auto startValue = animState_ == AnimState::Showing ? opacityEffect_->opacity() : 1.0;
        animState_ = AnimState::Hiding;

        opacityAnimation_->stop();
        opacityAnimation_->setStartValue(startValue);
        opacityAnimation_->setEndValue(0.0);
        opacityAnimation_->start();
    }

    void AttachFilePopup::selectFirstItem()
    {
        listWidget_->setCurrentIndex(0);
    }

    void AttachFilePopup::setPersistent(const bool _persistent)
    {
        persistent_ = _persistent;
        if (persistent_)
            parentWidget()->installEventFilter(this);
        else
            parentWidget()->removeEventFilter(this);
    }

    void AttachFilePopup::updateSizeAndPos()
    {
        const auto w = widget_->sizeHint().width();
        const auto h = widget_->heightForContent(items_.size() * itemHeight());

        widget_->setFixedHeight(h);
        widget_->move(0, 0);
        resize(w, h + popupBottomOffset());

        if (buttonRect_.isValid())
        {
            const auto x = buttonRect_.left() - getShadowSize() - popupLeftOffset();
            const auto y = buttonRect_.top() - h;
            move(x, y);
        }
    }

    bool AttachFilePopup::eventFilter(QObject* _obj, QEvent* _event)
    {
        static constexpr QEvent::Type events[] =
        {
            QEvent::MouseMove,
            QEvent::HoverMove,
            QEvent::MouseButtonPress,
        };
        if (persistent_ && std::any_of(std::begin(events), std::end(events), [t = _event->type()](const auto x) { return x == t; }))
        {
            const auto mouseEvent = (QMouseEvent*)_event;
            if (!geometry().contains(mouseEvent->pos()))
                hideWithDelay();
        }

        return false;
    }

    void AttachFilePopup::setPttEnabled(bool _enabled)
    {
        if (pttEnabled_ != _enabled)
        {
            pttEnabled_ = _enabled;
            initItems();
        }
    }

    void AttachFilePopup::mouseMoveEvent(QMouseEvent* _e)
    {
        if (!persistent_)
        {
            if (isMouseInArea(_e->pos()))
                hideTimer_.stop();
            else
                hideWithDelay();
        }

        setCursor(buttonRect_.contains(_e->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void AttachFilePopup::leaveEvent(QEvent*)
    {
        if (!persistent_)
            hideWithDelay();
    }

    void AttachFilePopup::keyPressEvent(QKeyEvent* _event)
    {
        _event->ignore();

        const auto curIdx = listWidget_->getCurrentIndex();
        if (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)
        {
            const auto pressedUp = _event->key() == Qt::Key_Up;
            auto newIdx = -1;
            if (!listWidget_->isValidIndex(curIdx))
            {
                if (pressedUp)
                    newIdx = listWidget_->count() - 1;
                else
                    newIdx = 0;
            }
            else
            {
                newIdx = curIdx + (pressedUp ? -1 : 1);
            }

            if (newIdx >= listWidget_->count())
                newIdx = 0;
            else if (newIdx < 0)
                newIdx = listWidget_->count() - 1;

            if (listWidget_->isValidIndex(newIdx))
            {
                listWidget_->clearSelection();
                listWidget_->setCurrentIndex(newIdx);
            }

            _event->accept();
        }
        else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
        {
            if (listWidget_->isValidIndex(curIdx))
            {
                onItemClicked(curIdx);
                _event->accept();
            }
        }
        else if (_event->key() == Qt::Key_Tab || _event->key() == Qt::Key_Backtab)
        {
            _event->accept();
        }
    }

    void AttachFilePopup::showEvent(QShowEvent*)
    {
        Q_EMIT visiblityChanged(true, QPrivateSignal());
    }

    void AttachFilePopup::hideEvent(QHideEvent*)
    {
        Q_EMIT visiblityChanged(false, QPrivateSignal());
        setPersistent(false);
    }

    void AttachFilePopup::onItemClicked(const int _idx)
    {
        hideAnimated();

        const auto it = std::find_if(items_.begin(), items_.end(), [_idx](const auto& _p) { return _p.first == _idx; });
        if (it == items_.end())
            return;

        Q_EMIT itemClicked(it->second, QPrivateSignal());
    }

    void AttachFilePopup::onBackgroundClicked()
    {
        const auto pos = mapFromGlobal(QCursor::pos());
        if (!widget_->rect().contains(pos))
            hideAnimated();
    }

    void AttachFilePopup::onHideTimer()
    {
        const auto pos = mapFromGlobal(QCursor::pos());
        if (isVisible() && !isMouseInArea(pos))
            hideAnimated();
    }

    void AttachFilePopup::hideWithDelay()
    {
        if (!hideTimer_.isActive() && isVisible())
            hideTimer_.start();
    }

    bool AttachFilePopup::isMouseInArea(const QPoint& _pos) const
    {
        return mouseAreaPoly_.containsPoint(_pos, Qt::OddEvenFill);
    }

    QPolygon AttachFilePopup::getMouseAreaPoly() const
    {
        const QRect wdgRect(QPoint(), widget_->sizeHint());

        QPolygon poly;
        poly << wdgRect.topLeft()
            << (wdgRect.topRight() + QPoint(1, 0))
            << (wdgRect.bottomRight() + QPoint(1, 1))
            << (buttonRect_.bottomRight() + QPoint(1, 1))
            << (buttonRect_.bottomLeft() + QPoint(0, 1))
            << (wdgRect.bottomLeft() + QPoint(0, 1));

        return poly;
    }
}
