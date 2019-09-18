#include "stdafx.h"

#include "AttachFilePopup.h"
#include "InputWidget.h"
#include "InputWidgetUtils.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/graphicsEffects.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"
#include "core_dispatcher.h"

#include "main_window/history_control/MessageStyle.h"
#include "main_window/MainPage.h"
#include "main_window/ContactDialog.h"

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

    QColor iconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
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

    static QPointer<Ui::AttachFilePopup> popupInstance;
}

namespace Ui
{
    AttachFileMenuItem::AttachFileMenuItem(QWidget* _parent, const QString& _icon, const QString& _caption, const QColor& _iconBgColor)
        : SimpleListItem(_parent)
        , iconBgColor_(_iconBgColor)
        , icon_(Utils::renderSvg(_icon, { pixmapWidth(), pixmapWidth() }, iconColor()))
    {
        caption_ = TextRendering::MakeTextUnit(_caption);
        caption_->init(Fonts::appFontScaled(15), captionColor());
        caption_->setOffsets(textOffset(), itemHeight() / 2);
        caption_->evaluateDesiredSize();

        setFixedHeight(itemHeight());
        setMinimumWidth(caption_->horOffset() + caption_->desiredWidth() + Utils::scale_value(24));

        connect(this, &SimpleListItem::hoverChanged, this, &AttachFileMenuItem::setSelected);
    }

    void AttachFileMenuItem::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;

            emit selectChanged(QPrivateSignal());
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

    AttachFilePopup::AttachFilePopup(QWidget* _parent, InputWidget* _input)
        : ClickableWidget(_parent)
        , input_(_input)
        , opacityEffect_(new Utils::OpacityEffect(this))
        , persistent_(false)
    {
        assert(!popupInstance);
        assert(_input);
        setCursor(Qt::ArrowCursor);

        listWidget_ = new SimpleListWidget(Qt::Vertical, this);
        Testing::setAccessibleName(listWidget_, qsl("AS inputwidget listWidget_"));
        connect(listWidget_, &SimpleListWidget::clicked, this, &AttachFilePopup::onItemClicked);

        const auto addItem = [this](const auto& _icon, const auto& _capt, const auto _iconBg, const auto _id)
        {
            auto item = new AttachFileMenuItem(listWidget_, _icon, _capt, Styling::getParameters().getColor(_iconBg));
            const auto idx = listWidget_->addItem(item);
            connect(item, &AttachFileMenuItem::hoverChanged, this, [this, idx](const bool _hovered)
            {
                if (_hovered)
                    listWidget_->setCurrentIndex(idx);
            });

            assert(std::none_of(items_.begin(), items_.end(), [_id](const auto& _p) { return _p.second == _id; }));
            items_.push_back({ idx, _id });
        };

        addItem(qsl(":/input/attach_photo"), QT_TRANSLATE_NOOP("input_widget", "Photo or Video"), Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE, MenuItemId::photoVideo);
        addItem(qsl(":/input/attach_documents"), QT_TRANSLATE_NOOP("input_widget", "File"), Styling::StyleVariable::SECONDARY_RAINBOW_MAIL, MenuItemId::file);
        //addItem(qsl(":/settings/general"), QT_TRANSLATE_NOOP("input_widget", "Camera"), Styling::StyleVariable::SECONDARY_ATTENTION, MenuItemId::camera);
        addItem(qsl(":/message_type_contact_icon"), QT_TRANSLATE_NOOP("input_widget", "Contact"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, MenuItemId::contact);
        //addItem(qsl(":/message_type_contact_icon"), QT_TRANSLATE_NOOP("input_widget", "Location"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, MenuItemId::geo);
        addItem(qsl(":/input/attach_ptt"), QT_TRANSLATE_NOOP("input_widget", "Voice Message"), Styling::StyleVariable::SECONDARY_RAINBOW_ORANGE, MenuItemId::ptt);

        listWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        widget_ = new AttachPopupBackground(this);
        auto layout = Utils::emptyVLayout(widget_);
        layout->addWidget(listWidget_);

        connect(this, &ClickableWidget::pressed, this, &AttachFilePopup::onBackgroundClicked);
        connect(this, &AttachFilePopup::photoVideoClicked, input_, &InputWidget::onAttachPhotoVideo);
        connect(this, &AttachFilePopup::fileClicked, input_, &InputWidget::onAttachFile);
        connect(this, &AttachFilePopup::cameraClicked, input_, &InputWidget::onAttachCamera);
        connect(this, &AttachFilePopup::contactClicked, input_, &InputWidget::onAttachContact);
        connect(this, &AttachFilePopup::pttClicked, input_, &InputWidget::onAttachPtt);

        setGraphicsEffect(opacityEffect_);

        hideTimer_.setSingleShot(true);
        hideTimer_.setInterval(leaveHideDelay().count());
        connect(&hideTimer_, &QTimer::timeout, this, &AttachFilePopup::onHideTimer);

        hide();
        popupInstance = this;
    }

    AttachFilePopup& AttachFilePopup::instance()
    {
        if (!popupInstance)
        {
            auto mainPage = Utils::InterConnector::instance().getMainPage();
            auto inputWidget = mainPage->getContactDialog()->getInputWidget();
            assert(mainPage);
            assert(inputWidget);

            popupInstance = new AttachFilePopup(mainPage, inputWidget);
        }

        return *popupInstance;
    }

    bool AttachFilePopup::isOpen()
    {
        return popupInstance && popupInstance->isVisible();
    }

    void AttachFilePopup::showPopup(const AttachFilePopup::ShowMode _mode)
    {
        auto& i = instance();
        i.setPersistent(_mode == AttachFilePopup::ShowMode::Persistent);
        i.showAnimated();
    }

    void AttachFilePopup::hidePopup()
    {
        if (popupInstance)
            popupInstance->hideAnimated();
    }

    void AttachFilePopup::showAnimated()
    {
        if (animState_ == AnimState::Showing)
            return;

        animState_ = AnimState::Showing;

        updateSizeAndPos();
        buttonRect_ = getPlusButtonRect();
        mouseAreaPoly_ = getMouseAreaPoly();
        listWidget_->clearSelection();

        const auto startValue = animState_ == AnimState::Hiding ? opacityEffect_->opacity() : 0.0;
        opacityEffect_->setOpacity(startValue);

        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            [this]() { animState_ = AnimState::None; },
            startValue,
            1.0,
            showHideDuration().count(),
            anim::sineInOut);

        show();
    }

    void AttachFilePopup::hideAnimated()
    {
        if (!isVisible() || animState_ == AnimState::Hiding)
            return;

        animState_ = AnimState::Hiding;

        const auto startValue = animState_ == AnimState::Showing ? opacityEffect_->opacity() : 1.0;
        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            [this]() { animState_ = AnimState::None; hide(); },
            startValue,
            0.0,
            showHideDuration().count(),
            anim::sineInOut);
    }

    void AttachFilePopup::selectFirstItem()
    {
        listWidget_->setCurrentIndex(0);
    }

    void AttachFilePopup::setPersistent(const bool _persistent)
    {
        persistent_ = _persistent;
        if (persistent_)
            window()->installEventFilter(this);
        else
            window()->removeEventFilter(this);
    }

    void AttachFilePopup::updateSizeAndPos()
    {
        const auto inputRect = input_->rect().translated(parentWidget()->mapFromGlobal(input_->mapToGlobal({ 0, 0 })));
        const auto popupSize = widget_->sizeHint();

        auto x = inputRect.left() - getShadowSize() + popupLeftOffset();
        if (inputRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
            x += (inputRect.width() - MessageStyle::getHistoryWidgetMaxWidth()) / 2;

        const auto y = inputRect.bottom() + 1 - popupBottomOffset() - popupSize.height() + getShadowSize();
        setGeometry(x, y, popupSize.width(), popupSize.height() + popupBottomOffset());
        widget_->move(0, 0);
    }

    bool AttachFilePopup::eventFilter(QObject* _obj, QEvent* _event)
    {
        constexpr QEvent::Type events[] =
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
        emit Utils::InterConnector::instance().attachFilePopupVisiblityChanged(true);
    }

    void AttachFilePopup::hideEvent(QHideEvent*)
    {
        emit Utils::InterConnector::instance().attachFilePopupVisiblityChanged(false);

        const auto mainPage = Utils::InterConnector::instance().getMainPage();
        if (input_ && mainPage && !mainPage->isSemiWindowVisible())
        {
            if (persistent_)
                input_->setFocusOnAttach();
            else
                input_->setFocusOnInput();
        }

        setPersistent(false);
    }

    void AttachFilePopup::onItemClicked(const int _idx)
    {
        hideAnimated();

        const auto it = std::find_if(items_.begin(), items_.end(), [_idx](const auto& _p) { return _p.first == _idx; });
        if (it == items_.end())
            return;

        switch (it->second)
        {
        case MenuItemId::photoVideo:
            emit photoVideoClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_openmedgallery_action, "plus");
            break;

        case MenuItemId::file:
            emit fileClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_openfile_action, "plus");
            break;

        case MenuItemId::camera:
            emit cameraClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_opencamera_action, "plus");
            break;

        case MenuItemId::contact:
            emit contactClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_opencontact_action, "plus");
            break;

        case MenuItemId::ptt:
            emit pttClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_openptt_action, "plus");
            break;

        case MenuItemId::geo:
            emit geoClicked(QPrivateSignal());
            sendStat(core::stats::stats_event_names::chatscr_opengeo_action, "plus");
            break;

        default:
            return;
        }
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

    QRect AttachFilePopup::getPlusButtonRect() const
    {
        auto rect = input_->getAttachFileButtonRect();
        rect.moveTopLeft(mapFromGlobal(rect.topLeft()));
        return rect;
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