#include "stdafx.h"
#include "Store.h"

#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TextWidget.h"

#include "../../controls/CustomButton.h"
#include "../../controls/ClickWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/ResizePixmapTask.h"
#include "../../utils/graphicsEffects.h"
#include "../../utils/features.h"
#include "../../main_window/MainWindow.h"
#include "../../fonts.h"
#include "../../cache/stickers/stickers.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/TopPanel.h"
#include "../MainPage.h"
#include "../../controls/DialogButton.h"

#include "controls/LottiePlayer.h"

using namespace Ui;
using namespace Stickers;

namespace Ui
{
    enum class IsPurchased
    {
        no,
        yes,
    };
}

namespace
{
    int getMarginLeft()
    {
        return Utils::scale_value(20);
    }

    int getMarginRight()
    {
        return Utils::scale_value(20);
    }

    int getHeaderHeight()
    {
        return Utils::scale_value(52);
    }

    int getPackIconSize()
    {
        return Utils::scale_value(92);
    }

    int getIconMarginV()
    {
        return Utils::scale_value(4);
    }

    int getIconMarginH()
    {
        return Utils::scale_value(8);
    }

    int getPackNameHeight()
    {
        return Utils::scale_value(40);
    }

    int getPackNameMargin()
    {
        return Utils::scale_value(4);
    }

    int getPackDescHeight()
    {
        return Utils::scale_value(20);
    }

    int getPacksViewHeight()
    {
        return (getPackIconSize() + getIconMarginV() + getPackNameHeight() + getPackNameMargin() + getPackDescHeight());
    }

    int getHScrollbarHeight()
    {
        return Utils::scale_value(16);
    }

    int getMyStickerMarginBottom()
    {
        return Utils::scale_value(12);
    }

    int getMyStickerHeight()
    {
        return getMyStickerMarginBottom() + Utils::scale_value(52);
    }

    int getMyPackIconSize()
    {
        return Utils::scale_value(52);
    }

    int getMyPackTextLeftMargin()
    {
        return Utils::scale_value(16);
    }

    int getMyPackTextTopMargin()
    {
        return Utils::scale_value(8);
    }

    int getMyPackDescTopMargin()
    {
        return Utils::scale_value(32);
    }

    int getMyPackIconTopMargin()
    {
        return Utils::scale_value(4);
    }

    int getDelButtonHeight()
    {
        return Utils::scale_value(36);
    }

    QSize getAddButtonSize()
    {
        return { 20, 20 };
    }

    int getAddButtonHeight()
    {
        return Utils::scale_value(36);
    }

    int getDragButtonHeight()
    {
        return Utils::scale_value(36);
    }

    int getPackIconPlaceholderRadius()
    {
        return Utils::scale_value(5);
    }

    QColor getHoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    QString getEmptyMyStickersText()
    {
        return QT_TRANSLATE_NOOP("stickers", "You have not added stickers yet");
    }

    QString getNoSearchResultsText()
    {
        return QT_TRANSLATE_NOOP("stickers", "Nothing found");
    }

    int getMyHeaderHeight()
    {
        return Utils::scale_value(44);
    }

    int getSearchPacksOffset()
    {
        return Utils::scale_value(12);
    }

    int getHoverMargin()
    {
        return Utils::scale_value(2);
    }

    QFont getNameFont()
    {
        return Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold);
    }

    QFont getDescFont()
    {
        return Fonts::appFontScaled(12);
    }

    QColor getButtonColor(bool _hovered, bool _selected)
    {
        return Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::BASE_SECONDARY_HOVER
            : (_selected ? Styling::StyleVariable::BASE_SECONDARY_ACTIVE
                : Styling::StyleVariable::BASE_SECONDARY));
    }

    QColor getAddButtonColor(bool _hovered, bool _selected)
    {
        return Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::PRIMARY_HOVER
            : (_selected ? Styling::StyleVariable::PRIMARY_ACTIVE
                : Styling::StyleVariable::PRIMARY));
    }

    QPixmap getDelButton(bool _hovered, bool _selected)
    {
        return Utils::renderSvgScaled(qsl(":/delete_icon"), QSize(20, 20), getButtonColor(_hovered, _selected));
    }

    QPixmap getDragButton(bool _hovered, bool _selected)
    {
        return Utils::renderSvgScaled(qsl(":/smiles_menu/move_sticker_icon"), QSize(20, 20), getButtonColor(_hovered, _selected));
    }

    QPixmap getAddButton(bool _hovered, bool _selected)
    {
        return Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), getAddButtonColor(_hovered, _selected));
    }

    void drawButton(QPainter& _p, const QPixmap& _image, const QRect& _buttonRect)
    {
        QRect imageRect(QPoint(0, 0), Utils::unscale_bitmap(_image.size()));
        imageRect.moveCenter(_buttonRect.center());
        _p.drawPixmap(imageRect, _image);
    }

    constexpr int back_spacing = 16;
    constexpr int start_search_delay_ms = 700;

    int badgeTopOffset()
    {
        return Utils::scale_value(8);
    }

    int badgeRightOffset()
    {
        return Utils::scale_value(4);
    }

    void loadSetIcon(Ui::Stickers::PackInfoObject* _infoObject, int _size)
    {
        if (!_infoObject || _size <= 0)
            return;

        const auto stickersSet = Stickers::getStoreSet(_infoObject->id_);
        if (!stickersSet)
            return;

        _infoObject->iconRequested_ = true;

        const auto& icon = stickersSet->getBigIcon();
        if (icon.isLottie())
        {
            _infoObject->setIconData(icon);
        }
        else if (icon.isPixmap())
        {
            const auto& pm = icon.getPixmap();
            if (pm.isNull())
                return;

            const auto size = Utils::scale_bitmap(QSize(_size, _size));
            auto task = new Utils::ResizePixmapTask(pm, size);
            QObject::connect(task, &Utils::ResizePixmapTask::resizedSignal, _infoObject, [_infoObject](QPixmap _result)
            {
                StickerData data;
                data.setPixmap(std::move(_result));
                _infoObject->setIconData(std::move(data));
            });
            QThreadPool::globalInstance()->start(task);
        }
    }

    void addPacks(PacksViewBase* _view, const setsIdsArray& _sets, IsPurchased _purshased, bool _replaceLines = false)
    {
        bool purchased = (_purshased == IsPurchased::yes);
        const auto lottieAllowed = Features::isAnimatedStickersInPickerAllowed();

        for (const auto _setId : _sets)
        {
            if (const auto s = Stickers::getStoreSet(_setId); s && s->isPurchased() == purchased)
            {
                if (lottieAllowed || !s->containsLottieSticker())
                {
                    _view->addPack(std::make_unique<PackInfoObject>(
                        nullptr,
                        _setId,
                        _replaceLines ? Utils::replaceLine(s->getName()) : s->getName(),

                        s->getStoreId(),
                        s->isPurchased()));
                }
            }
        }
    }

    bool isSame(std::vector<PackItemBase*> _items, const setsIdsArray& _sets, IsPurchased _purshased)
    {
        bool purchased = (_purshased == IsPurchased::yes);
        const auto lottieAllowed = Features::isAnimatedStickersInPickerAllowed();

        size_t i = 0;
        for (const auto _setId : _sets)
        {
            if (const auto s = Stickers::getStoreSet(_setId); s && s->isPurchased() == purchased)
            {
                if (lottieAllowed || !s->containsLottieSticker())
                    if (i >= _items.size() || _items[i++]->getId() != _setId)
                        return false;
            }
        }
        return i == _items.size();
    }

    void initView(PacksViewBase* _view, const setsIdsArray& _sets, IsPurchased _purshased, bool _replaceLines = false)
    {
        if (_view->isEqual(_sets, _purshased))
            return;

        _view->clear();
        addPacks(_view, _sets, _purshased, _replaceLines);
        _view->onPacksAdded();
    }

    QPixmap noResultsPixmap()
    {
        return Utils::renderSvgScaled(qsl(":/placeholders/empty_search"), QSize(84, 84), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    }
}

//////////////////////////////////////////////////////////////////////////
// class PackItemBase
//////////////////////////////////////////////////////////////////////////
PackItemBase::PackItemBase(QWidget* _parent, std::unique_ptr<PackInfoObject> _info)
    : ClickableWidget(_parent)
    , info_(std::move(_info))
{
    connect(info_.get(), &PackInfoObject::iconChanged, this, &PackItemBase::onIconChanged);
}

void PackItemBase::onVisibilityChanged(bool _visible)
{
    if (lottie_)
        lottie_->onVisibilityChanged(_visible);
}

void PackItemBase::onIconChanged()
{
    if (info_->iconData_.isPixmap())
        update();
    else if (info_->iconData_.isLottie())
        setLottie(info_->iconData_.getLottiePath());
}

void PackItemBase::setLottie(const QString& _path)
{
    if (_path.isEmpty())
        return;

    if (lottie_ && lottie_->getPath() == _path)
        return;

    delete lottie_;
    lottie_ = new LottiePlayer(this, _path, getIconRect());
}

//////////////////////////////////////////////////////////////////////////
// class TopPackWidget
//////////////////////////////////////////////////////////////////////////

TopPackItem::TopPackItem(QWidget* _parent, std::unique_ptr<PackInfoObject> _info)
    : PackItemBase(_parent, std::move(_info))
{
    setFixedSize(getPackIconSize(), getPacksViewHeight());

    if (!info_->name_.isEmpty())
    {
        name_ = TextRendering::MakeTextUnit(info_->name_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        TextRendering::TextUnit::InitializeParameters params{ getNameFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.align_ = TextRendering::HorAligment::CENTER;
        params.maxLinesCount_ = 2;
        params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
        name_->init(params);
        name_->getHeight(getPackIconSize());
        name_->setOffsets(0, getPacksViewHeight() - getPackNameHeight() - getPackNameMargin() - getPackDescHeight());
    }

    connect(this, &ClickableWidget::clicked, this, [id = info_->id_]()
    {
        Stickers::showStickersPack(id, Stickers::StatContext::Discover);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_pack_tap);
    });
}

void TopPackItem::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.setPen(Qt::PenStyle::NoPen);

    if (!info_->iconRequested_)
        loadSetIcon(info_.get(), getPackIconSize());

    if (isHovered() || !info_->iconData_.isValid())
    {
        Utils::PainterSaver ps(p);
        p.setBrush(getHoverColor());

        const qreal radius = qreal(Utils::scale_value(8));
        p.drawRoundedRect(getIconRect(), radius, radius);
    }

    if (info_->iconData_.isPixmap())
    {
        if (const auto& pm = info_->iconData_.getPixmap(); !pm.isNull())
            p.drawPixmap(getIconRect(), pm);
    }

    if (name_)
        name_->draw(p);
}

QRect TopPackItem::getIconRect() const
{
    return QRect(0, 0, getPackIconSize(), getPackIconSize());
}

//////////////////////////////////////////////////////////////////////////
// class TopPacksView
//////////////////////////////////////////////////////////////////////////
TopPacksView::TopPacksView(QWidget* _parent)
    : QWidget(_parent)
    , scrollArea_(CreateScrollAreaAndSetTrScrollBarH(this))
    , animScroll_(nullptr)
{
    auto innerArea = new QWidget(scrollArea_);
    layout_ = Utils::emptyHLayout(innerArea);
    layout_->setContentsMargins(getMarginLeft(), 0, getMarginRight(), getHScrollbarHeight());
    layout_->setSpacing(getIconMarginH() * 2);

    Utils::grabTouchWidget(scrollArea_->viewport(), true);
    Utils::grabTouchWidget(scrollArea_);
    Utils::grabTouchWidget(innerArea);

    scrollArea_->setStyleSheet(qsl("background: transparent; border: none;"));
    scrollArea_->setWidget(innerArea);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFixedHeight(getPacksViewHeight() + getHScrollbarHeight());
    setFixedHeight(getPacksViewHeight() + getHScrollbarHeight());

    connectNativeScrollbar(scrollArea_->horizontalScrollBar());

    auto rootLayout = Utils::emptyHLayout(this);
    rootLayout->addWidget(scrollArea_);
}

void TopPacksView::onSetIcon(const int32_t _setId)
{
    const auto items = findChildren<TopPackItem*>();
    const auto it = std::find_if(items.begin(), items.end(), [_setId](auto i) { return i->getId() == _setId; });
    if (it != items.end())
        loadSetIcon((*it)->getInfo(), getPackIconSize());
}

void TopPacksView::addPack(std::unique_ptr<PackInfoObject> _pack)
{
    auto item = new TopPackItem(this, std::move(_pack));
    items_.emplace_back(item);
    Testing::setAccessibleName(item, u"AS TopPackItem" % QString::number(item->getId()));
    layout_->addWidget(item);
}

void TopPacksView::updateSize()
{
    const auto spacing = layout_->spacing();
    int left = 0, right = 0;
    layout_->getContentsMargins(&left, nullptr, &right, nullptr);
    const auto count = findChildren<TopPackItem*>().size();
    const auto w = left + count * getPackIconSize() + (count - 1) * spacing + right;
    scrollArea_->widget()->setFixedWidth(w);

    updateItemsVisibility();
}

void TopPacksView::onPacksAdded()
{
    updateSize();
}

void TopPacksView::wheelEvent(QWheelEvent* _e)
{
    const int numDegrees = _e->delta() / 8;
    const int numSteps = numDegrees / 15;

    if (!numSteps || !numDegrees)
        return;

    if (numSteps > 0)
        scrollStep(direction::left);
    else
        scrollStep(direction::right);

    _e->accept();

    scrollArea_->fadeIn();
}

void TopPacksView::scrollStep(direction _direction)
{
    QRect viewRect = scrollArea_->viewport()->geometry();
    auto scrollbar = scrollArea_->horizontalScrollBar();

    const int maxVal = scrollbar->maximum();
    const int minVal = scrollbar->minimum();
    const int curVal = scrollbar->value();

    const int step = viewRect.width() / 2;

    int to = 0;

    if (_direction == TopPacksView::direction::right)
        to = std::min(curVal + step, maxVal);
    else
        to = std::max(curVal - step, minVal);

    if (!animScroll_)
    {
        animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
        animScroll_->setEasingCurve(QEasingCurve::InQuad);
        animScroll_->setDuration(std::chrono::milliseconds(300).count());
    }

    animScroll_->stop();
    animScroll_->setStartValue(curVal);
    animScroll_->setEndValue(to);
    animScroll_->start();
}

void TopPacksView::updateItemsVisibility()
{
    const auto visibleRect = scrollArea_->widget()->visibleRegion().boundingRect();
    const auto items = findChildren<TopPackItem*>();
    for (auto item : items)
        item->onVisibilityChanged(visibleRect.intersects(item->geometry()));
}

void TopPacksView::clear()
{
    while (layout_->count() > 0)
    {
        auto i = layout_->takeAt(0);
        if (auto w = i->widget())
            delete w;
        delete i;
    }
    items_.clear();
    updateSize();
}

bool TopPacksView::isEqual(const setsIdsArray& _sets, IsPurchased _purshased) const
{
    return isSame(items_, _sets, _purshased);
}

void TopPacksView::connectNativeScrollbar(QScrollBar* _sb)
{
    connect(_sb, &QScrollBar::valueChanged, this, &TopPacksView::updateItemsVisibility);
    connect(_sb, &QScrollBar::rangeChanged, this, &TopPacksView::updateItemsVisibility);
}

//////////////////////////////////////////////////////////////////////////
// class PacksWidget
//////////////////////////////////////////////////////////////////////////
TopPacksWidget::TopPacksWidget(QWidget* _parent)
    : QWidget(_parent)
{
    Ui::GetDispatcher()->getStickersStore();

    setFixedHeight(getPacksViewHeight() + getHeaderHeight() + getHScrollbarHeight() + Utils::scale_value(8));

    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setAlignment(Qt::AlignTop);

    auto header = new QWidget;
    header->setFixedHeight(getHeaderHeight());
    auto headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(getMarginLeft(), 0, 0, 0);
    auto title = new TextWidget(this, QT_TRANSLATE_NOOP("stickers", "TRENDING"), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    title->init({ Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
    headerLayout->addWidget(title, Qt::AlignVCenter);
    headerLayout->addStretch();
    rootLayout->addWidget(header);

    packs_ = new TopPacksView(this);
    rootLayout->addWidget(packs_);

    init();
}

void TopPacksWidget::init()
{
    initView(packs_, Stickers::getStoreStickersSets(), IsPurchased::no);
}

void TopPacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}

void TopPacksWidget::connectNativeScrollbar(QScrollBar* _sb)
{
    packs_->connectNativeScrollbar(_sb);
}

//////////////////////////////////////////////////////////////////////////
// PackItem
//////////////////////////////////////////////////////////////////////////
PackItem::PackItem(QWidget* _parent, std::unique_ptr<PackInfoObject> _info)
    : PackItemBase(_parent, std::move(_info))
{
    setFixedHeight(getMyStickerHeight());

    if (!info_->name_.isEmpty())
    {
        name_ = TextRendering::MakeTextUnit(info_->name_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        name_->init({ getNameFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        name_->setOffsets(getMyPackIconSize() + getMyPackTextLeftMargin() + getMarginLeft(), getMyPackTextTopMargin());
        name_->evaluateDesiredSize();
    }

    connect(this, &ClickableWidget::clicked, this, &PackItem::onClicked);
    connect(this, &ClickableWidget::pressed, this, &PackItem::onPressed);
    connect(this, &ClickableWidget::released, this, &PackItem::onReleased);
}

void PackItem::setDragging(bool _isDragging)
{
    if (dragging_ != _isDragging)
    {
        dragging_ = _isDragging;

        if (isDragging())
        {
            im_assert(canDrag());
            if (!graphicsEffect())
            {
                auto effect = new Utils::OpacityEffect(this);
                effect->setOpacity(0.7);
                setGraphicsEffect(effect);
            }
        }
        else
        {
            if (graphicsEffect())
                setGraphicsEffect(nullptr);
        }

        setDragCursor(_isDragging);
        onVisibilityChanged(!_isDragging);
        update();
    }
}

void PackItem::setHoverable(bool _isHoverable)
{
    if (hoverable_ != _isHoverable)
    {
        hoverable_ = _isHoverable;
        update();
    }
}

void PackItem::setMode(PackViewMode _mode)
{
    if (mode_ != _mode)
    {
        mode_ = _mode;
        update();
    }
}

void PackItem::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.setPen(Qt::PenStyle::NoPen);

    const auto hovered = isHoverable() && isHovered();
    const auto selected = isPressed();
    const bool highlight = hovered || selected;

    if (highlight)
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

    if (!info_->iconRequested_)
        loadSetIcon(info_.get(), getPackIconSize());

    if (!info_->iconData_.isValid())
    {
        const double radius = Utils::scale_value(8);
        const auto var = highlight ? Styling::StyleVariable::BASE_GLOBALWHITE : Styling::StyleVariable::BASE_BRIGHT_INVERSE;

        p.setBrush(Styling::getParameters().getColor(var));
        p.drawRoundedRect(getIconRect(), radius, radius);
    }
    else if (info_->iconData_.isPixmap())
    {
        if (const auto& pm = info_->iconData_.getPixmap(); !pm.isNull())
            p.drawPixmap(getIconRect(), pm);
    }

    if (name_)
        name_->draw(p);

    if (info_->purchased_)
    {
        drawButton(p, getDelButton(hovered, selected), getDelButtonRect());

        if (mode_ == PackViewMode::myPacks)
            drawButton(p, getDragButton(hovered, selected), getDragButtonRect());
    }
    else
    {
        drawButton(p, getAddButton(hovered, selected), getAddButtonRect());
    }
}

void PackItem::resizeEvent(QResizeEvent* _e)
{
    if (width() != _e->oldSize().width())
    {
        const int textWidth = width() - getMarginLeft() - getMyPackIconSize() - getDelButtonHeight() - 2 * getMarginRight() - getDragButtonHeight();
        if (name_)
            name_->elide(textWidth);

        update();
    }
}

void PackItem::mouseMoveEvent(QMouseEvent* _e)
{
    PackItemBase::mouseMoveEvent(_e);

    const auto showDrag = canDrag() && getDragButtonRect().contains(_e->pos());
    setDragCursor(isDragging() || showDrag);
    if (isDragging())
        _e->ignore();
}

QRect PackItem::getIconRect() const
{
    return QRect(getMarginLeft(), getMyPackIconTopMargin(), getMyPackIconSize(), getMyPackIconSize());
}

QRect PackItem::getDelButtonRect() const
{
    auto left = width() - getDelButtonHeight() - getMarginRight();
    if (mode_ == PackViewMode::myPacks)
        left -= getDragButtonHeight();

    return QRect(
        left,
        (height() - getDelButtonHeight()) / 2,
        getDelButtonHeight(),
        getDelButtonHeight());
}

QRect PackItem::getAddButtonRect() const
{
    return QRect(
        width() - getAddButtonHeight() - getMarginRight(),
        (height() - getAddButtonHeight()) / 2,
        getAddButtonHeight(),
        getAddButtonHeight());
}

QRect PackItem::getDragButtonRect() const
{
    return QRect(
        width() - getDragButtonHeight() - getMarginRight(),
        (height() - getDragButtonHeight()) / 2,
        getDragButtonHeight(),
        getDragButtonHeight());
}

void PackItem::onClicked()
{
    if (isDragging())
        return;

    const auto pos = mapFromGlobal(QCursor::pos());
    if (info_->purchased_)
    {
        if (getDelButtonRect().contains(pos))
        {
            QTimer::singleShot(0, [packId = getId()]()
            {
                const auto confirm = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    QT_TRANSLATE_NOOP("popup_window", "Yes"),
                    QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to remove this sticker pack?"),
                    QT_TRANSLATE_NOOP("popup_window", "Remove sticker pack"),
                    nullptr);

                if (confirm)
                    GetDispatcher()->removeStickersPack(packId, QString());
            });
            return;
        }
        else if (getDragButtonRect().contains(pos))
        {
            return;
        }
    }
    else
    {
        if (getAddButtonRect().contains(pos))
        {
            if (auto pack = Stickers::getStoreSet(getId()))
                GetDispatcher()->addStickersPack(pack->getId(), pack->getStoreId());

            if (mode_ == PackViewMode::search)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_add_pack_button_tap);

            return;
        }
    }

    const auto context = (mode_ == PackViewMode::myPacks) ? Stickers::StatContext::Discover : Stickers::StatContext::Search;
    Stickers::showStickersPack(getId(), context);

    if (mode_ == PackViewMode::search)
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_pack_tap);
}

void PackItem::onPressed()
{
    if (canDrag() && getDragButtonRect().contains(mapFromGlobal(QCursor::pos())))
    {
        setDragging(true);
        Q_EMIT dragStarted(QPrivateSignal());
    }
}

void PackItem::onReleased()
{
    if (isDragging())
    {
        setDragging(false);
        Q_EMIT dragEnded(QPrivateSignal());
    }
}

bool PackItem::canDrag() const noexcept
{
    return info_->purchased_ && mode_ == PackViewMode::myPacks;
}

void PackItem::setDragCursor(bool _dragging)
{
    if (_dragging)
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::PointingHandCursor);
}


//////////////////////////////////////////////////////////////////////////
// PacksView
//////////////////////////////////////////////////////////////////////////
PacksView::PacksView(PackViewMode _mode, QWidget* _parent)
    : mode_(_mode)
{
    setMouseTracking(true);
}

void PacksView::addPack(std::unique_ptr<PackInfoObject> _pack)
{
    auto item = new PackItem(this, std::move(_pack));
    item->setMode(mode_);
    connect(item, &PackItem::dragStarted, this, &PacksView::startDrag);
    connect(item, &PackItem::dragEnded, this, &PacksView::stopDrag);
    connect(item, &PackItem::hoverChanged, this, [this]()
    {
        clearSelection();
        updateHover();
    });

    item->show();

    Testing::setAccessibleName(item, u"AS PackItem" % QString::number(item->getId()));
    items_.emplace_back(item);
}

void PacksView::updateSize()
{
    setFixedHeight(items_.size() * getMyStickerHeight());
}

void PacksView::clear()
{
    for (auto item : items_)
        item->deleteLater();
    items_.clear();

    stopDrag();
    clearSelection();
}

int PacksView::moveCursor(CursorAction cursorAction)
{
    int newY = -1;
    const auto prevIdx = selectedIdx_;
    const auto pageSize = visibleRegion().boundingRect().height() / getMyHeaderHeight(); // not quite accurate

    auto hoveredIdx = -1;
    const auto it = std::find_if(items_.begin(), items_.end(), [](auto i) { return i->isHovered(); });
    if (it != items_.end())
        hoveredIdx = std::distance(items_.begin(), it);

    selectedIdx_ = (hoveredIdx != -1) ? hoveredIdx : selectedIdx_; // if last was hovered, then start from it

    switch (cursorAction)
    {
        case MoveUp:
            selectedIdx_ = std::max<int>(0 , --selectedIdx_);
            newY =  getStickerRect(selectedIdx_).top();
            break;

        case MoveDown:
            selectedIdx_ = std::min<int>(items_.size() - 1, ++selectedIdx_);
            newY = getStickerRect(selectedIdx_).bottom();
            break;

        case MovePageUp:
            selectedIdx_ = std::max<int>(0 , selectedIdx_ - pageSize);
            newY = getStickerRect(selectedIdx_).top();
            break;

        case MovePageDown:
            selectedIdx_ = std::min<int>(items_.size() - 1 , selectedIdx_ + pageSize);
            newY = getStickerRect(selectedIdx_).bottom();
            break;
    }

    updateHover();

    return newY;
}

int PacksView::selectedPackId() const
{
    if (selectedIdx_ >= 0 && selectedIdx_ < static_cast<int>(items_.size()))
        return items_[selectedIdx_]->getId();

    return -1;
}

void PacksView::setScrollBar(QWidget *_scrollBar)
{
    scrollBar_ = _scrollBar;
}

void PacksView::connectNativeScrollbar(QScrollBar* _sb)
{
    connect(_sb, &QScrollBar::valueChanged, this, &PacksView::updateItemsVisibility);
    connect(_sb, &QScrollBar::rangeChanged, this, &PacksView::updateItemsVisibility);
}

void PacksView::selectFirst()
{
    selectedIdx_ = 0;
    updateHover();
}

void PacksView::onPacksAdded()
{
    updateSize();
    arrangePacks();
    updateItemsVisibility();
}

bool PacksView::isEqual(const setsIdsArray& _sets, IsPurchased _purshased) const
{
    return isSame(items_, _sets, _purshased);
}

void PacksView::arrangePacks()
{
    int i = 0;
    for (auto item : items_)
    {
        auto r = getStickerRect(i++);
        if (isDragInProgress() && item->getId() == dragPack_)
            item->setGeometry(r.translated(0, dragOffset_));
        else
            item->setGeometry(r);
    }
    update();
}

QRect PacksView::getStickerRect(const int _pos) const
{
    return QRect(0, _pos * getMyStickerHeight(), width(), getMyStickerHeight());
}

void PacksView::mouseReleaseEvent(QMouseEvent*)
{
    stopDrag();
}

int PacksView::getDragPackNum()
{
    if (!isDragInProgress())
        return -1;

    for (int i = 0; i < (int) items_.size(); ++i)
    {
        if (items_[i]->getId() == dragPack_)
            return i;
    }
    return -1;
}

int PacksView::getPackUnderCursor() const
{
    const auto point = mapFromGlobal(QCursor::pos());
    if (point.x() < 0 || point.y() < 0)
        return -1;

    if (scrollBar_ && scrollBar_->isVisible() && point.x() > (scrollBar_->geometry().left() - getHoverMargin()))
        return -1;

    return static_cast<int>(point.y() / getMyStickerHeight());
}

void PacksView::clearSelection()
{
    selectedIdx_ = -1;
}

void PacksView::updateHover()
{
    const int idx = selectedIdx_ != -1 ? selectedIdx_ : getPackUnderCursor();
    for (auto i = 0; i < int(items_.size()); ++i)
    {
        auto item = items_[i];
        const auto isHovered = i == idx || item->getId() == dragPack_;

        QSignalBlocker sb(item);
        item->setHovered(isHovered);
    }
}

void PacksView::updateItemsVisibility()
{
    int i = 0;
    const auto visibleRect = visibleRegion().boundingRect();
    for (auto item : items_)
    {
        const auto r = getStickerRect(i++);
        if (!static_cast<PackItem*>(item)->isDragging())
            item->onVisibilityChanged(visibleRect.intersects(r));
    }
}

bool PacksView::isDragInProgress() const
{
    return (dragPack_ != -1);
}

void PacksView::mouseMoveEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::NoButton)
        updateHover();

    if (!isDragInProgress())
        return;

    const QRect viewportRect = visibleRegion().boundingRect();

    if (_e->pos().y() > viewportRect.bottom())
        Q_EMIT startScrollDown();
    else if (_e->pos().y() < viewportRect.top())
        Q_EMIT startScrollUp();
    else
        Q_EMIT stopScroll();

    const int dragPackPos = getDragPackNum();
    if (dragPackPos == -1)
    {
        im_assert(false);
        return;
    }

    const auto dragPackRect = getStickerRect(dragPackPos);

    const int dragOffsetPrev = dragOffset_;

    dragOffset_ = _e->pos().y() - startDragPos_.y();

    const QRect oldRect(dragPackRect.left(), dragPackRect.top() + dragOffsetPrev, dragPackRect.width(), dragPackRect.height());
    const QRect newRect(dragPackRect.left(), dragPackRect.top() + dragOffset_, dragPackRect.width(), dragPackRect.height());

    for (int i = 0; i < int(items_.size()); ++i)
    {
        if (i == dragPackPos)
            continue;

        const auto packRect = getStickerRect(i);

        const QRect interectRect = newRect.intersected(packRect);

        if (double(interectRect.height())/double(packRect.height()) > 0.8f)
        {
            const int distance = packRect.top() - dragPackRect.top();

            dragOffset_ -= distance;

            startDragPos_.setY(startDragPos_.y() + distance);

            std::swap(items_[i], items_[dragPackPos]);
            orderChanged_ = true;
        }
    }
    arrangePacks();
}

void PacksView::wheelEvent(QWheelEvent *_e)
{
    QWidget::wheelEvent(_e);

    // update hover after view has been scrolled
    QMetaObject::invokeMethod(this, [this]()
    {
        clearSelection();
        updateHover();
    }, Qt::QueuedConnection);
}

void PacksView::resizeEvent(QResizeEvent* _e)
{
    if (width() != _e->oldSize().width())
        arrangePacks();
}

void PacksView::leaveEvent(QEvent* _e)
{
    if (!isDragInProgress())
    {
        for (auto item : items_)
        {
            QSignalBlocker sb(item);
            item->setHovered(false);
        }
    }
}

void PacksView::startDrag()
{
    const auto pos = getPackUnderCursor();
    if (pos >= 0 && pos < static_cast<int>(items_.size()))
    {
        startDragPos_ = mapFromGlobal(QCursor::pos());
        dragOffset_ = 0;
        dragPack_ = items_[pos]->getId();

        for (int i = 0; i < int(items_.size()); ++i)
            static_cast<PackItem*>(items_[i])->setHoverable(i == pos);

        arrangePacks();
    }
}

void PacksView::stopDrag()
{
    if (isDragInProgress() && orderChanged_)
        postStickersOrder();

    dragPack_ = -1;
    dragOffset_ = 0;
    startDragPos_ = QPoint();
    orderChanged_ = false;

    for (auto item : items_)
        static_cast<PackItem*>(item)->setHoverable(true);

    arrangePacks();
    Q_EMIT stopScroll();
}

void PacksView::postStickersOrder() const
{
    im_assert(orderChanged_);

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    core::ifptr<core::iarray> values_array(collection->create_array(), true);

    values_array->reserve(items_.size());

    for (auto item : items_)
    {
        core::ifptr<core::ivalue> ival(collection->create_value(), true);
        ival->set_as_int(item->getId());
        values_array->push_back(ival.get());
    }

    collection.set_value_as_array("values", values_array.get());
    Ui::GetDispatcher()->post_message_to_core("stickers/set/order", collection.get());
}

void PacksView::onSetIcon(const int32_t _setId)
{
    const auto items = findChildren<PackItem*>();
    const auto it = std::find_if(items.begin(), items.end(), [_setId](auto i) { return i->getId() == _setId; });
    if (it != items.end())
        loadSetIcon((*it)->getInfo(), getMyPackIconSize());
}

bool PacksView::empty() const
{
    return items_.empty();
}

MyPacksHeader::MyPacksHeader(QWidget* _parent)
    : QWidget(_parent)
    , marginText_(Utils::scale_value(8))
    , buttonWidth_(Utils::scale_value(20))
{
    setFixedHeight(getMyHeaderHeight());

    createStickerPack_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("stickers", "Add your stickers"));
    createStickerPack_->init({ Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY } });
    createStickerPack_->evaluateDesiredSize();

    myText_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("stickers", "MY"));
    myText_->init({ Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
    myText_->evaluateDesiredSize();

    buttonAdd_ = new ClickableWidget(this);
    connect(buttonAdd_, &ClickableWidget::clicked, this, [this]() { Q_EMIT addClicked(QPrivateSignal()); });
}

void MyPacksHeader::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    static auto addPackButtonImage = Utils::StyledPixmap::scaled(qsl(":/controls/add_icon"), getAddButtonSize(), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });

    myText_->setOffsets(getMarginLeft(), height() / 2);
    myText_->draw(p, TextRendering::VerPosition::MIDDLE);

    const int textX = rect().width() - createStickerPack_->desiredWidth() - getMarginRight();
    createStickerPack_->setOffsets(textX, height() / 2);
    createStickerPack_->draw(p, TextRendering::VerPosition::MIDDLE);

    const int offsetY = (height() - buttonWidth_) / 2;

    QRect addPackRect(textX - buttonWidth_ - marginText_, offsetY, buttonWidth_, buttonWidth_);

    p.drawPixmap(addPackRect, addPackButtonImage.actualPixmap());
}

void MyPacksHeader::resizeEvent(QResizeEvent* _e)
{
    const auto buttonWidthAll = createStickerPack_->desiredWidth() + 2 * getMarginRight() + marginText_ + Utils::scale_value(getAddButtonSize()).width();
    const auto rc = geometry();
    buttonAdd_->setGeometry(rc.width() - buttonWidthAll, 0, buttonWidthAll, rc.height());
}

//////////////////////////////////////////////////////////////////////////
// MyPacksWidget
//////////////////////////////////////////////////////////////////////////
MyPacksWidget::MyPacksWidget(QWidget* _parent)
    : QWidget(_parent)
    , syncedWithServer_(false)
    , scrollAnimationUp_(new QVariantAnimation(this))
    , scrollAnimationDown_(new QVariantAnimation(this))
{
    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setAlignment(Qt::AlignTop);

    auto headerWidget = new MyPacksHeader(this);

    QObject::connect(headerWidget, &MyPacksHeader::addClicked, this, &MyPacksWidget::createPackButtonClicked);

    rootLayout->addWidget(headerWidget);

    packs_ = new PacksView(PackViewMode::myPacks, this);

    QObject::connect(packs_, &PacksView::startScrollUp, this, &MyPacksWidget::onScrollUp);
    QObject::connect(packs_, &PacksView::startScrollDown, this, &MyPacksWidget::onScrollDown);
    QObject::connect(packs_, &PacksView::stopScroll, this, &MyPacksWidget::onStopScroll);

    scrollAnimationUp_->setStartValue(0.0);
    scrollAnimationUp_->setEndValue(1.0);
    scrollAnimationUp_->setEasingCurve(QEasingCurve::Linear);
    scrollAnimationUp_->setDuration(200);
    QObject::connect(scrollAnimationUp_, &QVariantAnimation::valueChanged, this, [this]()
    {
        Q_EMIT scrollStep(Utils::scale_value(-12));
    });

    scrollAnimationDown_->setStartValue(0.0);
    scrollAnimationDown_->setEndValue(1.0);
    scrollAnimationDown_->setEasingCurve(QEasingCurve::Linear);
    scrollAnimationDown_->setDuration(200);
    QObject::connect(scrollAnimationDown_, &QVariantAnimation::valueChanged, this, [this]()
    {
        Q_EMIT scrollStep(Utils::scale_value(12));
    });

    rootLayout->addWidget(packs_);

    init(PacksViewBase::InitFrom::local);
}

void MyPacksWidget::init(PacksViewBase::InitFrom _init)
{
    if (_init == PacksViewBase::InitFrom::server)
        syncedWithServer_ = true;

    if (packs_->isDragInProgress())
        return;

    initView(packs_, Stickers::getStoreStickersSets(), IsPurchased::yes, true);

    QRect rcText(0, packs_->geometry().top(), rect().width(), rect().height() - packs_->geometry().top());

    update(rcText);
}

void MyPacksWidget::setScrollBarToView(QWidget *_scrollBar)
{
    packs_->setScrollBar(_scrollBar);
}

void MyPacksWidget::connectNativeScrollbar(QScrollBar* _scrollBar)
{
    packs_->connectNativeScrollbar(_scrollBar);
}

void MyPacksWidget::onScrollUp()
{
    scrollAnimationDown_->stop();

    if (scrollAnimationUp_->state() == QAbstractAnimation::Running)
        return;

    scrollAnimationUp_->start();
}

void MyPacksWidget::onScrollDown()
{
    scrollAnimationUp_->stop();

    if (scrollAnimationDown_->state() == QAbstractAnimation::Running)
        return;

    scrollAnimationDown_->start();
}

void MyPacksWidget::onStopScroll()
{
    scrollAnimationUp_->stop();
    scrollAnimationDown_->stop();
}


void MyPacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}

void MyPacksWidget::paintEvent(QPaintEvent* _e)
{
    if (syncedWithServer_ && packs_->empty())
    {
        QPainter p(this);

        static auto font(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));

        static const auto pen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        static auto emptyText = getEmptyMyStickersText();

        p.setFont(font);

        p.setPen(pen);

        auto rcText = rect();

        p.drawText(rcText, emptyText, QTextOption(Qt::AlignCenter));
    }

    QWidget::paintEvent(_e);
}

void MyPacksWidget::createPackButtonClicked()
{
    const auto bot = Utils::getStickerBotAimId();

    const auto openBot = [](const QString& _botId)
    {
        Utils::updateAppsPageReopenPage(_botId);
        Utils::InterConnector::instance().openDialog(_botId);
    };

    if (Logic::getContactListModel()->contains(bot))
    {
        openBot(bot);
    }
    else
    {
        Logic::getContactListModel()->addContactToCL(bot, [weak_this = QPointer(this), bot, openBot](bool _res)
        {
            if (!weak_this)
                return;

            openBot(bot);
        });
    }

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_addbot_bottom_tap);
}


//////////////////////////////////////////////////////////////////////////
// class Store
//////////////////////////////////////////////////////////////////////////
Store::Store(QWidget* _parent)
    : QWidget(_parent)
    , searchPacks_(nullptr)
{
    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setAlignment(Qt::AlignTop);

    headerWidget_ = new StickersPageHeader(this);
    connect(headerWidget_, &StickersPageHeader::backClicked, this, &Store::back);
    rootLayout->addWidget(headerWidget_);
    Testing::setAccessibleName(headerWidget_, qsl("AS StickersPage header"));

    connect(headerWidget_, &StickersPageHeader::searchRequested, this, &Store::onSearchRequested);
    connect(headerWidget_, &StickersPageHeader::searchCancelled, this, &Store::onSearchCancelled);

    rootScrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    rootScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rootScrollArea_->setFocusPolicy(Qt::NoFocus);
    rootScrollArea_->setStyleSheet(qsl("background: transparent; border: none;"));
    Utils::grabTouchWidget(rootScrollArea_->viewport(), true);

    connect(QScroller::scroller(rootScrollArea_->viewport()), &QScroller::stateChanged, this, &Store::touchScrollStateChanged, Qt::QueuedConnection);

    auto scrollAreaWidget = new QWidget(rootScrollArea_);
    rootScrollArea_->setWidget(scrollAreaWidget);
    rootScrollArea_->setWidgetResizable(true);

    rootLayout->addWidget(rootScrollArea_);

    QVBoxLayout* widgetLayout = new QVBoxLayout();
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setAlignment(Qt::AlignTop);
    scrollAreaWidget->setLayout(widgetLayout);

    packsView_ = new TopPacksWidget(this);
    packsView_->connectNativeScrollbar(rootScrollArea_->verticalScrollBar());
    widgetLayout->addWidget(packsView_);
    Utils::grabTouchWidget(packsView_);

    myPacks_ = new MyPacksWidget(this);
    myPacks_->setScrollBarToView(rootScrollArea_->getScrollBarV());
    myPacks_->connectNativeScrollbar(rootScrollArea_->verticalScrollBar());
    widgetLayout->addWidget(myPacks_);
    Utils::grabTouchWidget(myPacks_);

    connect(&Stickers::getCache(), &Stickers::Cache::setBigIconUpdated, this, &Store::onSetBigIcon);
    connect(Ui::GetDispatcher(), &core_dispatcher::onStore, this, &Store::onStore);
    connect(Ui::GetDispatcher(), &core_dispatcher::onStoreSearch, this, &Store::onSearchStore);
    connect(Ui::GetDispatcher(), &core_dispatcher::onStickers, this, &Store::onStickers);

    connect(myPacks_, &MyPacksWidget::scrollStep, this, &Store::onScrollStep);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, [this]()
    {
        packsView_->init();
        myPacks_->init(PacksViewBase::InitFrom::local);
        if (searchPacks_)
            searchPacks_->init(PacksViewBase::InitFrom::local);
    });
}

void Store::onScrollStep(const int _step)
{
    auto scrollbar = rootScrollArea_->verticalScrollBar();

    const int maxVal = scrollbar->maximum();
    const int minVal = scrollbar->minimum();

    const int curVal = scrollbar->value();

    int newValue = curVal + _step;

    if (newValue < minVal)
        newValue = minVal;
    if (newValue > maxVal)
        newValue = maxVal;

    scrollbar->setValue(newValue);
}

void Store::initSearchView(PacksViewBase::InitFrom _init)
{
    if (!searchPacks_)
    {
        searchPacks_ = new SearchPacksWidget(this);
        searchPacks_->setScrollBarToView(rootScrollArea_->getScrollBarV());
        rootScrollArea_->widget()->layout()->addWidget(searchPacks_);
        Utils::grabTouchWidget(searchPacks_);
        searchPacks_->hide();
    }
    searchPacks_->init(_init);
}

void Store::setBadgeText(const QString& _text)
{
    headerWidget_->setBadgeText(_text);
}

void Store::setFrameCountMode(FrameCountMode _mode)
{
    headerWidget_->setFrameCountMode(_mode);
}

void Store::hideSearch()
{
    headerWidget_->cancelSearch();
}

void Store::touchScrollStateChanged(QScroller::State _state)
{
    packsView_->blockSignals(_state != QScroller::Inactive);
}

void Store::onStore()
{
    packsView_->init();
    myPacks_->init(PacksViewBase::InitFrom::server);
}

void Store::onSearchStore()
{
    initSearchView(PacksViewBase::InitFrom::server);
}

void Store::onSearchRequested(const QString& term)
{
    Stickers::requestSearch(term);
    myPacks_->hide();
    packsView_->hide();
    initSearchView(PacksViewBase::InitFrom::local);
    searchPacks_->touchSearchRequested();
    searchPacks_->show();

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_request_send);
}

void Store::onSearchCancelled()
{
    if (searchPacks_)
        searchPacks_->hide();
    packsView_->show();
    myPacks_->show();
}

void Store::onStickers()
{
    Ui::GetDispatcher()->getStickersStore();

    const auto searchTerm = headerWidget_->currentTerm();
    if (!searchTerm.isEmpty())
    {
        Stickers::requestSearch(searchTerm);
    }
}


void Store::onSetBigIcon(const qint32 _setId)
{
    packsView_->onSetIcon(_setId);
    myPacks_->onSetIcon(_setId);
    if (searchPacks_)
        searchPacks_->onSetIcon(_setId);
}

void Store::resizeEvent(QResizeEvent * event)
{
    auto rc = geometry();

    packsView_->setFixedWidth(rc.width());
}

void Store::paintEvent(QPaintEvent* _e)
{
    setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
}

void Store::keyPressEvent(QKeyEvent *_e)
{
    if (_e->key() == Qt::Key_Escape)
    {
        hideSearch();
    }
    else if (searchPacks_ && searchPacks_->isVisible())
    {
        auto newY = -1;
        switch (_e->key())
        {
        case Qt::Key_Up:
            newY = searchPacks_->moveCursor(PacksView::MoveUp);
            break;

        case Qt::Key_Down:
            newY = searchPacks_->moveCursor(PacksView::MoveDown);
            break;

        case Qt::Key_PageUp:
            newY = searchPacks_->moveCursor(PacksView::MovePageUp);
            break;

        case Qt::Key_PageDown:
            newY = searchPacks_->moveCursor(PacksView::MovePageDown);
            break;

        case Qt::Key_Return:
            if (searchPacks_->selectedPackId() != -1)
            {
                headerWidget_->setFocus();
                Stickers::showStickersPack(searchPacks_->selectedPackId(), Stickers::StatContext::Search);
                return;
            }
            break;
        }

        if (newY != -1)
            rootScrollArea_->ensureVisible(0, newY + getSearchPacksOffset(), 0, 0);
    }
}

BackToChatsHeader::BackToChatsHeader(QWidget *_parent)
    : QWidget(_parent)
    , textUnit_(TextRendering::MakeTextUnit(QString()))
    , textBadgeUnit_(TextRendering::MakeTextUnit(QString()))
    , backButton_(new CustomButton(this, qsl(":/controls/back_icon")))
    , searchButton_(new CustomButton(this, qsl(":/search_icon")))
    , badgeVisible_(true)
{
    setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
    TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
    params.align_ = TextRendering::HorAligment::CENTER;
    textUnit_->init(params);
    textUnit_->evaluateDesiredSize();
    params.setFonts(Fonts::appFontScaled(11));
    params.color_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
    textBadgeUnit_->init(params);
    textBadgeUnit_->evaluateDesiredSize();

    for (auto button : { backButton_, searchButton_ })
    {
        Styling::Buttons::setButtonDefaultColors(button);
        button->setFixedSize(Utils::scale_value(QSize(20, 20)));
    }

    QObject::connect(backButton_, &CustomButton::clicked, this, [this]() { Q_EMIT backClicked(QPrivateSignal()); });
    QObject::connect(searchButton_, &CustomButton::clicked, this, [this]() { Q_EMIT searchClicked(QPrivateSignal()); });

    auto layout = Utils::emptyHLayout();
    layout->setSpacing(0);

    layout->addSpacerItem(new QSpacerItem(Utils::scale_value(14), 0, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed));
    layout->addWidget(backButton_);
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));
    layout->addWidget(searchButton_);
    layout->addSpacerItem(new QSpacerItem(Utils::scale_value(12), 0, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed));

    setLayout(layout);
}

BackToChatsHeader::~BackToChatsHeader()
{
}

void BackToChatsHeader::setText(const QString& _text)
{
    if (textUnit_)
    {
        textUnit_->setText(_text);
        textUnit_->evaluateDesiredSize();
        update();
    }
}

void BackToChatsHeader::setBadgeText(const QString& _text)
{
    if (textBadgeUnit_)
    {
        textBadgeUnit_->setText(_text);
        textBadgeUnit_->evaluateDesiredSize();
        update();
    }
}

void BackToChatsHeader::setBadgeVisible(bool _value)
{
    if (badgeVisible_ != _value)
    {
        badgeVisible_ = _value;
        update();
    }
}

void BackToChatsHeader::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

    if (textUnit_)
    {
        textUnit_->setOffsets((width() - textUnit_->desiredWidth()) / 2, height() / 2);
        textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    if (badgeVisible_ && textBadgeUnit_ && !textBadgeUnit_->getSourceText().isEmpty())
        Utils::Badge::drawBadge(textBadgeUnit_, p, width() - badgeRightOffset(), badgeTopOffset(), Utils::Badge::Color::Green);
}


//////////////////////////////////////////////////////////////////////////
// class StickersPageHeader
//////////////////////////////////////////////////////////////////////////
StickersPageHeader::StickersPageHeader(QWidget *_parent)
    : QFrame(_parent)
{
    auto layout = Utils::emptyHLayout(this);

    setFixedHeight(Utils::getTopPanelHeight());

    backToChatsHeader_ = new BackToChatsHeader(this);
    backToChatsHeader_->setText(QT_TRANSLATE_NOOP("stickers", "Stickers"));

    searchHeader_ = new QWidget(this);

    auto searchLayout = Utils::emptyHLayout(searchHeader_);
    searchLayout->addSpacing(Utils::scale_value(20) - 2); // qlineedit has a built in 2px left padding

    searchInput_ = new LineEditEx(this);
    searchInput_->setFrame(false);
    searchInput_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
    searchInput_->setCustomPlaceholderColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
    searchInput_->setCustomPlaceholder(QT_TRANSLATE_NOOP("stickers", "Search"));
    searchInput_->setAttribute(Qt::WA_MacShowFocusRect, false);
    searchInput_->installEventFilter(this);
    searchInput_->setStyleSheet(qsl("background-color: transparent"));
    searchLayout->addWidget(searchInput_);
    searchLayout->addSpacing(Utils::scale_value(back_spacing));

    searchButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("stickers", "Search"), DialogButtonRole::CONFIRM);
    searchButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    searchButton_->setEnabled(false);
    searchButton_->adjustSize();
    searchLayout->addWidget(searchButton_);
    searchLayout->addSpacing(Utils::scale_value(back_spacing));

    auto cancelButton = new DialogButton(this, QT_TRANSLATE_NOOP("stickers", "Cancel"), DialogButtonRole::CANCEL);
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cancelButton->adjustSize();
    searchLayout->addWidget(cancelButton);
    searchLayout->addSpacing(Utils::scale_value(back_spacing));

    layout->addWidget(backToChatsHeader_);
    layout->addWidget(searchHeader_);

    searchHeader_->hide();

    connect(backToChatsHeader_, &BackToChatsHeader::backClicked, this, &StickersPageHeader::backClicked);
    connect(backToChatsHeader_, &BackToChatsHeader::searchClicked, this, &StickersPageHeader::onSearchActivated);
    connect(cancelButton, &CustomButton::clicked, this, &StickersPageHeader::onSearchCancelled);
    connect(searchButton_, &QPushButton::clicked, this, &StickersPageHeader::onSearch);
    connect(searchInput_, &LineEditEx::returnPressed, this, &StickersPageHeader::onSearch);
    connect(searchInput_, &LineEditEx::escapePressed, this, &StickersPageHeader::onSearchCancelled);
    connect(searchInput_, &LineEditEx::textChanged, this, &StickersPageHeader::onSearchTextChanged);

    auto findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, &StickersPageHeader::onSearchActivated);

    searchTimer_.setSingleShot(true);
    searchTimer_.setInterval(start_search_delay_ms);
    connect(&searchTimer_, &QTimer::timeout, this, [this](){Q_EMIT searchRequested(lastTerm_);});
}

void StickersPageHeader::cancelSearch()
{
    onSearchCancelled();
}

void StickersPageHeader::setBadgeText(const QString& _text)
{
    backToChatsHeader_->setBadgeText(_text);
}

void StickersPageHeader::setFrameCountMode(FrameCountMode _mode)
{
    backToChatsHeader_->setBadgeVisible(_mode == FrameCountMode::_1);
}

void StickersPageHeader::onSearch()
{
    const auto text = searchInput_->text();
    if (text != lastTerm_ && !text.isEmpty())
    {
        searchTimer_.stop();
        lastTerm_ = text;
        Q_EMIT searchRequested(text);
    }
}

void StickersPageHeader::onSearchCancelled()
{
    lastTerm_.clear();
    searchHeader_->hide();
    backToChatsHeader_->show();
    Q_EMIT searchCancelled();
}

void StickersPageHeader::onSearchActivated()
{
    if (searchHeader_->isVisible())
        return;

    backToChatsHeader_->hide();
    searchHeader_->show();
    searchInput_->setFocus();

    QSignalBlocker sb(searchInput_);
    searchInput_->clear();

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_input_tap);
}

void StickersPageHeader::onSearchTextChanged(const QString &_text)
{
    searchTimer_.stop();
    lastTerm_ = _text;
    searchButton_->setEnabled(!_text.isEmpty());

    if (!_text.isEmpty())
        searchTimer_.start();
    else
        Q_EMIT searchCancelled();
}

bool StickersPageHeader::eventFilter(QObject *_o, QEvent *_e)
{
    if (_o == searchInput_ && _e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_e);
        if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
        {
            _e->ignore(); // to pass event further to parent
            return true;
        }
    }
    return QWidget::eventFilter(_o, _e);
}

//////////////////////////////////////////////////////////////////////////
// class SearchPacksWidget
//////////////////////////////////////////////////////////////////////////
SearchPacksWidget::SearchPacksWidget(QWidget *_parent)
    : QWidget(_parent)
    , packs_{ new PacksView(PackViewMode::search, this) }
    , noResultsWidget_{ new QWidget(this) }
    , noResultsPlaceholder_{ new QLabel(noResultsWidget_) }
    , noResultsLabel_{ new QLabel(noResultsWidget_) }
    , syncedWithServer_(false)
{
    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignTop);

    rootLayout->addSpacing(getSearchPacksOffset());

    noResultsWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    auto noResultsLayout = Utils::emptyVLayout(noResultsWidget_);

    updateStyle();

    noResultsLabel_->setText(getNoSearchResultsText());
    noResultsLabel_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
    noResultsLabel_->setAlignment(Qt::AlignHCenter);
    rootLayout->setAlignment(noResultsWidget_, Qt::AlignCenter);

    noResultsLayout->addStretch();
    noResultsLayout->addWidget(noResultsPlaceholder_);
    noResultsLayout->addSpacing(Utils::scale_value(30));
    noResultsLayout->addWidget(noResultsLabel_);
    noResultsLayout->addStretch();
    noResultsLayout->setAlignment(noResultsPlaceholder_, Qt::AlignHCenter);
    noResultsLayout->setAlignment(noResultsLabel_, Qt::AlignHCenter);

    noResultsWidget_->hide();

    rootLayout->addWidget(packs_);
    rootLayout->addWidget(noResultsWidget_);

    setLayout(rootLayout);

    init(PacksViewBase::InitFrom::local);

    connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &SearchPacksWidget::updateStyle);
}

void SearchPacksWidget::init(PacksViewBase::InitFrom _init)
{
    if (_init == PacksViewBase::InitFrom::server)
        syncedWithServer_ = true;

    packs_->clear();
    addPacks(packs_, Stickers::getSearchStickersSets(), IsPurchased::yes, true);
    addPacks(packs_, Stickers::getSearchStickersSets(), IsPurchased::no, true);

    // since download tasks are pushed in front of the queue, go through sets in reverse order and initiate icon downloading
    const auto stickers = Stickers::getSearchStickersSets();
    for (auto it = stickers.rbegin(); it != stickers.rend(); ++it)
    {
        if (const auto set = Stickers::getStoreSet(*it))
            set->getBigIcon();
    }
    packs_->onPacksAdded();
    packs_->selectFirst();

    if (_init == PacksViewBase::InitFrom::server)
    {
        noResultsWidget_->setVisible(packs_->empty());
        packs_->setVisible(!packs_->empty());

        if (!packs_->empty() && searchRequested_)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_results_receive);

        searchRequested_ = false;
    }
    else
    {
        noResultsWidget_->hide();
    }
}

int SearchPacksWidget::moveCursor(PacksView::CursorAction cursorAction)
{
    return packs_->moveCursor(cursorAction);
}

int SearchPacksWidget::selectedPackId()
{
    return packs_->selectedPackId();
}

void SearchPacksWidget::setScrollBarToView(QWidget *_scrollBar)
{
    packs_->setScrollBar(_scrollBar);
}

void SearchPacksWidget::connectNativeScrollbar(QScrollBar* _scrollBar)
{
    packs_->connectNativeScrollbar(_scrollBar);
}

void SearchPacksWidget::touchSearchRequested()
{
    searchRequested_ = true;
}

void Ui::Stickers::SearchPacksWidget::updateStyle()
{
    noResultsPlaceholder_->setPixmap(noResultsPixmap());
    QPalette noResultsTextPalette;
    noResultsTextPalette.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    noResultsLabel_->setPalette(noResultsTextPalette);
}

void SearchPacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}
