#include "stdafx.h"
#include "toolbar.h"

#include "../../controls/CustomButton.h"
#include "../../controls/PictureWidget.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "styles/ThemeParameters.h"
#include "app_config.h"

#include "controls/LottiePlayer.h"

namespace
{
    int gradientSize()
    {
        return Utils::scale_value(16);
    }
}

namespace Ui
{
    using namespace Smiles;

    AttachedView::AttachedView(QWidget* _view, QWidget* _viewParent)
        : View_(_view)
        , ViewParent_(_viewParent)
    {
    }

    QWidget* AttachedView::getView() const
    {
        return View_;
    }

    QWidget* AttachedView::getViewParent() const
    {
        return ViewParent_;
    }

    //////////////////////////////////////////////////////////////////////////
    // AddButton
    //////////////////////////////////////////////////////////////////////////
    AddButton::AddButton(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedSize(Utils::scale_value(52), Utils::scale_value(_parent->geometry().height()));

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, Utils::scale_value(4), 0);
        layout->setAlignment(Qt::AlignRight);

        auto button = new CustomButton(this, qsl(":/controls/add_gray_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        button->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
        button->setFixedSize(Utils::scale_value(QSize(32, 32)));
        connect(button, &CustomButton::clicked, this, &AddButton::clicked);

        layout->addWidget(button);
    }

    void AddButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        QSize gs(gradientSize(), height());

        auto drawGradient = [&p, &gs](const int _left, const QColor& _start, const QColor& _end)
        {
            QRect grRect(QPoint(_left, 0), gs);

            QLinearGradient gradient(grRect.topLeft(), grRect.topRight());
            gradient.setColorAt(0, _start);
            gradient.setColorAt(1, _end);

            p.fillRect(grRect, gradient);
        };

        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

        drawGradient(0, Qt::transparent, bg);

        p.fillRect(gs.width(), 0, width() - gs.width(), height(), bg);
    }

    //////////////////////////////////////////////////////////////////////////
    // TabButton class
    //////////////////////////////////////////////////////////////////////////
    TabButton::TabButton(QWidget* _parent, QPixmap _pixmap)
        : QPushButton(_parent)
        , pixmap_(std::move(_pixmap))
    {
        setFlat(true);
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::PointingHandCursor);
    }

    void TabButton::AttachView(const AttachedView& _view)
    {
        attachedView_ = _view;
    }

    const AttachedView& TabButton::GetAttachedView() const
    {
        return attachedView_;
    }

    void TabButton::setFixed(const bool _isFixed)
    {
        fixed_ = _isFixed;
    }

    bool TabButton::isFixed() const
    {
        return fixed_;
    }

    void TabButton::setPixmap(QPixmap _pixmap)
    {
        pixmap_ = std::move(_pixmap);
        update();
    }

    void TabButton::setLottie(const QString& _path)
    {
        if (_path.isEmpty())
            return;

        if (lottie_ && lottie_->getPath() == _path)
            return;

        delete lottie_;
        lottie_ = new LottiePlayer(this, _path, iconRect());
    }

    QSize TabButton::iconSize()
    {
        return Utils::scale_value(QSize(32, 32));
    }

    void TabButton::paintEvent(QPaintEvent* _e)
    {
        QPushButton::paintEvent(_e);

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (!lottie_ && pixmap_.isNull())
        {
            static const auto radius = Utils::fscale_value(8.);
            static const QMargins margins(Utils::scale_value(4), Utils::scale_value(4), Utils::scale_value(4), Utils::scale_value(4));
            const auto color = (underMouse() || isChecked()) ? Styling::StyleVariable::BASE_GLOBALWHITE : Styling::StyleVariable::BASE_BRIGHT_INVERSE;
            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters().getColor(color));
            p.drawRoundedRect(rect().marginsRemoved(margins), radius, radius);
        }
        else if (!pixmap_.isNull())
        {
            const double ratio = Utils::scale_bitmap_ratio();
            const auto x = (rect().width() - pixmap_.width() / ratio) / 2;
            const auto y = (rect().height() - pixmap_.height() / ratio) / 2;
            p.drawPixmap(x, y, pixmap_);
        }

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = width();
            const auto y = height();
            Utils::drawText(p, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, QString::number(getSetId()));
        }
    }

    void TabButton::onVisibilityChanged(bool _visible)
    {
        if (lottie_)
            lottie_->onVisibilityChanged(_visible);
    }

    QRect TabButton::iconRect() const
    {
        QRect playerRect({}, iconSize());
        playerRect.moveCenter(rect().center());
        return playerRect;
    }

    void TabButton::resizeEvent(QResizeEvent* event)
    {
        if (lottie_)
            lottie_->setRect(iconRect());
        QPushButton::resizeEvent(event);
    }

    //////////////////////////////////////////////////////////////////////////
    // Toolbar class
    //////////////////////////////////////////////////////////////////////////
    Toolbar::Toolbar(QWidget* _parent, buttonsAlign _align)
        : QFrame(_parent)
        , align_(_align)
        , buttonStore_(nullptr)
        , AnimScroll_(nullptr)
    {
        setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        auto rootLayout = Utils::emptyHLayout(this);

        viewArea_ = new ToolbarScrollArea(this);
        viewArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QWidget* scrollAreaWidget = new QWidget(viewArea_);
        scrollAreaWidget->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(scrollAreaWidget);
        viewArea_->setWidgetResizable(true);
        viewArea_->setFocusPolicy(Qt::NoFocus);

        Utils::grabTouchWidget(viewArea_->viewport(), true);
        Utils::grabTouchWidget(scrollAreaWidget);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &Toolbar::touchScrollStateChanged, Qt::QueuedConnection);

        connect(viewArea_->horizontalScrollBar(), &QScrollBar::valueChanged, this, &Toolbar::updateItemsVisibility);
        connect(viewArea_->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &Toolbar::updateItemsVisibility);

        rootLayout->addWidget(viewArea_);

        horLayout_ = Utils::emptyHLayout();

        scrollAreaWidget->setLayout(horLayout_);

        QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horLayout_->addSpacerItem(horizontalSpacer);

        if (_align == buttonsAlign::center)
        {
            QSpacerItem* horizontalSpacerCenter = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            horLayout_->addSpacerItem(horizontalSpacerCenter);
        }
    }

    Toolbar::~Toolbar() = default;

    void Toolbar::addButtonStore()
    {
        buttonStore_ = new AddButton(this);
        buttonStore_->show();

        QObject::connect(buttonStore_, &AddButton::clicked, this, &Toolbar::buttonStoreClick);

        viewArea_->setRightMargin(buttonStore_->width() - gradientSize());
    }

    void Toolbar::selectNext()
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [](const auto _btn) { return _btn->isChecked(); });
        if (it != buttons_.end())
        {
            it = std::next(it);
            if (it != buttons_.end())
            {
                (*it)->setChecked(true);
                scrollToButton(*it);
            }
        }
    }

    void Toolbar::selectPrevious()
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [](const auto _btn) { return _btn->isChecked(); });
        if (it != buttons_.end() && it != buttons_.begin())
        {
            it = std::prev(it);
            (*it)->setChecked(true);
            scrollToButton(*it);
        }
    }

    void Toolbar::selectFirst()
    {
        if (!buttons_.empty())
        {
            buttons_.front()->setChecked(true);
            scrollToButton(buttons_.front());
        }
    }

    void Toolbar::selectLast()
    {
        if (!buttons_.empty())
        {
            buttons_.back()->setChecked(true);
            scrollToButton(buttons_.back());
        }
    }

    bool Toolbar::isFirstSelected() const
    {
        return !buttons_.empty() && buttons_.front()->isChecked();
    }

    bool Toolbar::isLastSelected() const
    {
        return !buttons_.empty() && buttons_.back()->isChecked();
    }

    void Toolbar::buttonStoreClick()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_icon_picker);
        Q_EMIT Utils::InterConnector::instance().showStickersStore();
    }

    void Toolbar::updateItemsVisibility()
    {
        const auto visibleRect = viewArea_->widget()->visibleRegion().boundingRect();
        for (auto btn : buttons_)
            btn->onVisibilityChanged(visibleRect.intersects(btn->geometry()));
    }

    void Toolbar::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (buttonStore_)
        {
            QRect thisRect = geometry();

            buttonStore_->setFixedHeight(thisRect.height() - Utils::scale_value(1));
            buttonStore_->move(thisRect.width() - buttonStore_->width(), 0);
        }

        updateItemsVisibility();
    }

    void Toolbar::wheelEvent(QWheelEvent* _e)
    {
        const int numDegrees = _e->delta() / 8;
        const int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(direction::left);
        else
            scrollStep(direction::right);

        QWidget::wheelEvent(_e);
    }

    void Toolbar::touchScrollStateChanged(QScroller::State state)
    {
        for (auto iter : buttons_)
            iter->setAttribute(Qt::WA_TransparentForMouseEvents, state != QScroller::Inactive);
    }

    void Toolbar::scrollStep(direction _direction)
    {
        QRect viewRect = viewArea_->viewport()->geometry();
        auto scrollbar = viewArea_->horizontalScrollBar();

        const int curVal = scrollbar->value();
        const int step = viewRect.width() / 2;
        int to = 0;

        if (_direction == direction::right)
            to = std::min(curVal + step, scrollbar->maximum());
        else
            to = std::max(curVal - step, scrollbar->minimum());

        constexpr auto duration = std::chrono::milliseconds(300);

        if (!AnimScroll_)
        {
            AnimScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
            AnimScroll_->setDuration(duration.count());
            AnimScroll_->setEasingCurve(QEasingCurve::InQuad);
        }

        AnimScroll_->stop();
        AnimScroll_->setStartValue(curVal);
        AnimScroll_->setEndValue(to);
        AnimScroll_->start();
    }

    void Toolbar::addButton(TabButton* _button)
    {
        _button->setAutoExclusive(true);

        int index = 0;
        if (align_ == buttonsAlign::left)
            index = horLayout_->count() - 1;
        else if (align_ == buttonsAlign::right)
            index = horLayout_->count();
        else
            index = horLayout_->count() - 1;


        Utils::grabTouchWidget(_button);

        horLayout_->insertWidget(index, _button);

        buttons_.push_back(_button);
    }

    void Toolbar::Clear(const bool _delFixed)
    {
        for (auto iter = buttons_.begin(); iter != buttons_.end();)
        {
            auto button = *iter;

            if (!_delFixed && button->isFixed())
            {
                ++iter;
                continue;
            }

            horLayout_->removeWidget(button);
            button->deleteLater();

            iter = buttons_.erase(iter);
        }
    }

    TabButton* Toolbar::addButton(QPixmap _icon)
    {
        TabButton* button = new TabButton(this, std::move(_icon));
        addButton(button);

        return button;
    }

    TabButton* Toolbar::addButton(int32_t _setId)
    {
        auto button = addButton(QPixmap());
        button->setSetId(_setId);
        return button;
    }

    TabButton* Toolbar::selectedButton() const
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [](const auto _btn) { return _btn->isChecked(); });
        if (it != buttons_.end())
            return *it;

        return nullptr;
    }

    TabButton* Toolbar::getButton(int32_t _setId) const
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [_setId](const auto _btn) { return _btn->getSetId() == _setId; });
        if (it != buttons_.end())
            return *it;

        return nullptr;
    }

    void Toolbar::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    const std::list<TabButton*>& Toolbar::GetButtons() const
    {
        return buttons_;
    }

    void Toolbar::scrollToButton(TabButton* _button)
    {
        viewArea_->ensureWidgetVisible(_button);
    }
}
