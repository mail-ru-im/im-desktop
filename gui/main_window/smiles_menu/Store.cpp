#include "stdafx.h"
#include "Store.h"

#include "../../styles/ThemeParameters.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/TextEmojiWidget.h"

#include "../../controls/CustomButton.h"
#include "../../controls/ClickWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/MainPage.h"
#include "../../fonts.h"
#include "../../cache/stickers/stickers.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/TopPanel.h"
#include "../MainPage.h"
#include "../../controls/DialogButton.h"

using namespace Ui;
using namespace Stickers;

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

    QPixmap scaleIcon(const QPixmap& _icon, const int _toSize)
    {
        if (_icon.isNull())
            return QPixmap();

        auto p = _icon.scaled(Utils::scale_bitmap(QSize(_toSize, _toSize)), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        Utils::check_pixel_ratio(p);

        return p;
    }
}

//////////////////////////////////////////////////////////////////////////
// class TopPacksView
//////////////////////////////////////////////////////////////////////////
TopPacksView::TopPacksView(ScrollAreaWithTrScrollBar* _parent)
    : QWidget(_parent)
    , parent_(_parent)
    , hoveredPack_(-1)
    , animScroll_(nullptr)
{
    setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
    setFixedHeight(getPacksViewHeight());
    setCursor(QCursor(Qt::PointingHandCursor));
}

QRect TopPacksView::getStickerRect(const int _pos)
{
    static auto height = getPackIconSize() + getIconMarginV() + getPackNameHeight() + getPackNameMargin() + getPackDescHeight();
    return QRect(getMarginLeft() + _pos*(getPackIconSize() + 2*getIconMarginH()), 0, getPackIconSize(), height);
}

void TopPacksView::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.setPen(Qt::PenStyle::NoPen);

    static auto hoveredBrush = QBrush(getHoverColor());
    static auto nameFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
    static auto nameTextColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);

    static auto descFont(Fonts::appFontScaled(12, Fonts::FontWeight::Normal));
    static auto descTextColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);

    int i = 0;
    for (auto _pack : packs_)
    {
        auto& pack = *_pack;
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.intersects(_e->rect()))
        {
            const QRect imageRect(stickerRect.left(), stickerRect.top(), getPackIconSize(), getPackIconSize());

            if (!pack.iconRequested_)
            {
                if (const auto stickersSet = Stickers::getStoreSet(pack.id_))
                {
                    pack.icon_ = scaleIcon(stickersSet->getBigIcon(), getPackIconSize());
                    pack.iconRequested_ = true;
                }
            }

            const auto hasIcon = !pack.icon_.isNull();
            if (i == hoveredPack_ || !hasIcon)
            {
                Utils::PainterSaver ps(p);
                p.setBrush(hoveredBrush);

                const qreal radius = qreal(Utils::scale_value(8));
                p.drawRoundedRect(imageRect, radius, radius);
            }

            if (hasIcon)
                p.drawPixmap(imageRect, pack.icon_);

            auto& nameUnit = nameUnits_[pack.id_];
            if (!nameUnit)
            {
                nameUnit = TextRendering::MakeTextUnit(pack.name_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                nameUnit->init(nameFont, nameTextColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 2, TextRendering::LineBreakType::PREFER_SPACES);
                nameUnit->getHeight(stickerRect.width());
            }

            nameUnit->setOffsets(stickerRect.x(), stickerRect.height() - getPackNameHeight() - getPackNameMargin() - getPackDescHeight());
            nameUnit->draw(p);

            if (!pack.subtitle_.isEmpty())
            {
                auto& descUnit = descUnits_[pack.id_];
                if (!descUnit)
                {
                    descUnit = TextRendering::MakeTextUnit(pack.subtitle_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                    descUnit->init(descFont, descTextColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
                    descUnit->getHeight(stickerRect.width());
                }

                descUnit->setOffsets(stickerRect.x(), stickerRect.height() - getPackDescHeight() - getPackNameHeight() + nameUnit->cachedSize().height());
                descUnit->draw(p);
            }
        }

        ++i;
    }
}

void TopPacksView::onSetIcon(const int32_t _setId)
{
    const auto it = std::find_if(packs_.begin(), packs_.end(), [_setId](const auto& pack)
        { return pack->id_ == _setId; });
    if (it != packs_.end())
    {
        if (auto pack = Stickers::getStoreSet(_setId))
            (*it)->icon_ = scaleIcon(pack->getBigIcon(), getPackIconSize());

        update(getStickerRect(std::distance(packs_.begin(), it)));
    }
}

void TopPacksView::addPack(PackInfoObject* _pack)
{
    packs_.emplace_back(_pack);
}

void TopPacksView::updateSize()
{
    setFixedWidth(getMarginLeft() + packs_.size() * (getPackIconSize() + getIconMarginH() * 2) + getMarginRight());
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

    parent_->fadeIn();
}

void TopPacksView::scrollStep(direction _direction)
{
    QRect viewRect = parent_->viewport()->geometry();
    auto scrollbar = parent_->horizontalScrollBar();

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

void TopPacksView::mousePressEvent(QMouseEvent* _e)
{
    QWidget::mousePressEvent(_e);

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            Stickers::showStickersPack(packs_[i]->id_, Stickers::StatContext::Discover);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_pack_tap);

            return;
        }
    }
}

void TopPacksView::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    if (hoveredPack_ >= 0)
    {
        const auto prevHoveredPack = hoveredPack_;

        hoveredPack_ = -1;

        const auto prevRect = getStickerRect(prevHoveredPack);

        update(prevRect);
    }
}

void TopPacksView::mouseMoveEvent(QMouseEvent* _e)
{
    QWidget::mouseMoveEvent(_e);

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            if (hoveredPack_ == (int) i)
                return;

            const auto prevHoveredPack = hoveredPack_;

            hoveredPack_ = i;

            if (prevHoveredPack >= 0)
            {
                const auto prevRect = getStickerRect(prevHoveredPack);

                update(prevRect);
            }

            update(stickerRect);

            return;
        }
    }
}

void TopPacksView::clear()
{
    for (auto pack : std::exchange(packs_, {}))
        pack->deleteLater();

    nameUnits_.clear();
    descUnits_.clear();
}


//////////////////////////////////////////////////////////////////////////
// class PacksWidget
//////////////////////////////////////////////////////////////////////////
PacksWidget::PacksWidget(QWidget* _parent)
    : QWidget(_parent)
{
    Ui::GetDispatcher()->getStickersStore();

    setFixedHeight(getPacksViewHeight() + getHeaderHeight() + getHScrollbarHeight() + Utils::scale_value(8));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setAlignment(Qt::AlignTop);
    setLayout(rootLayout);

    QHBoxLayout* headerLayout = new QHBoxLayout(nullptr);
    headerLayout->setContentsMargins(getMarginLeft(), 0, 0, 0);
    auto header = new QLabel(this);
    header->setFixedHeight(getHeaderHeight());
    header->setText(QT_TRANSLATE_NOOP("stickers", "TRENDING"));
    header->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
    header->setStyleSheet(qsl("QLabel{background-color: transparent; color: %1}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));
    headerLayout->addWidget(header);
    rootLayout->addLayout(headerLayout);

    scrollArea_ = CreateScrollAreaAndSetTrScrollBarH(this);
    scrollArea_->setFocusPolicy(Qt::NoFocus);
    scrollArea_->setFixedHeight(getPacksViewHeight() + getHScrollbarHeight());
    Utils::grabTouchWidget(scrollArea_->viewport(), true);

    packs_ = new TopPacksView(scrollArea_);

    scrollArea_->setWidget(packs_);
    scrollArea_->setWidgetResizable(true);

    rootLayout->addWidget(scrollArea_);

    init(false);
}

void PacksWidget::init(const bool _fromServer)
{
    packs_->clear();

    for (const auto _setId : Stickers::getStoreStickersSets())
    {
        const auto stickersSet = Stickers::getStoreSet(_setId);

        if (!stickersSet)
            continue;

        if (!stickersSet->isPurchased())
        {
            packs_->addPack(new PackInfoObject(
                this,
                _setId,
                stickersSet->getName(),
                stickersSet->getSubtitle(),
                stickersSet->getStoreId(),
                scaleIcon(QPixmap(), getPackIconSize()),
                stickersSet->isPurchased(), false));
        }
    }

    packs_->updateSize();
}

void PacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}




//////////////////////////////////////////////////////////////////////////
// PacksView
//////////////////////////////////////////////////////////////////////////
PacksView::PacksView(Mode _mode, QWidget* _parent)
    : mode_(_mode)
    , dragPack_(-1)
    , dragOffset_(0)
    , selectedIdx_(-1)
    , hoveredIdx_(-1)
{
    setCursor(QCursor(Qt::PointingHandCursor));
    setMouseTracking(true);

    setAcceptDrops(true);
}

void PacksView::addPack(PackInfoObject* _pack)
{
    packs_.emplace_back(_pack);
}

void PacksView::updateSize()
{
    int h_need = packs_.size() * getMyStickerHeight();

    setFixedHeight(h_need);
}

void PacksView::clear()
{
    for (auto pack : packs_)
        pack->deleteLater();
    packs_.clear();

    nameUnits_.clear();
    descUnits_.clear();
    clearSelection();
}

int PacksView::moveCursor(CursorAction cursorAction)
{
    int newY = -1;
    const auto prevIdx = selectedIdx_;
    const auto pageSize = visibleRegion().boundingRect().height() / getMyHeaderHeight(); // not quite accurate

    selectedIdx_ = (hoveredIdx_ != -1) ? hoveredIdx_ : selectedIdx_; // if last was hovered, then start from it

    switch (cursorAction)
    {
        case MoveUp:
            selectedIdx_ = std::max<int>(0 , --selectedIdx_);
            newY =  getStickerRect(selectedIdx_).top();
            break;

        case MoveDown:
            selectedIdx_ = std::min<int>(packs_.size() - 1, ++selectedIdx_);
            newY = getStickerRect(selectedIdx_).bottom();
            break;

        case MovePageUp:
            selectedIdx_ = std::max<int>(0 , selectedIdx_ - pageSize);
            newY = getStickerRect(selectedIdx_).top();
            break;

        case MovePageDown:
            selectedIdx_ = std::min<int>(packs_.size() - 1 , selectedIdx_ + pageSize);
            newY = getStickerRect(selectedIdx_).bottom();
            break;
    }

    update(getStickerRect(selectedIdx_));

    if (prevIdx != -1)
        update(getStickerRect(prevIdx));

    if (hoveredIdx_ != -1 && hoveredIdx_ != selectedIdx_)
        update(getStickerRect(hoveredIdx_));

    hoveredIdx_ = -1;

    return newY;
}

int PacksView::selectedPackId() const
{
    if (selectedIdx_ >= 0 && selectedIdx_ < static_cast<int>(packs_.size()))
        return packs_[selectedIdx_]->id_;

    return -1;
}

void PacksView::setScrollBar(QWidget *_scrollBar)
{
    scrollBar_ = _scrollBar;
}

void PacksView::selectFirst()
{
    selectedIdx_ = 0;
}

int Ui::Stickers::PacksView::getPackInfoIndex(PackInfoObject* _packInfo) const
{
    return Utils::indexOf(packs_.cbegin(), packs_.cend(), _packInfo);
}

QRect PacksView::getStickerRect(const int _pos) const
{
    return QRect(0, _pos * getMyStickerHeight(), geometry().width(), getMyStickerHeight());
}

QRect PacksView::getDelButtonRect(const QRect& _stickerRect) const
{
    auto left = _stickerRect.width() - getDelButtonHeight() - getMarginRight();
    if (mode_ == Mode::my_packs)
        left -= getDragButtonHeight();

    return QRect(
        left,
        _stickerRect.top() + (_stickerRect.height() - getDelButtonHeight())/2,
        getDelButtonHeight(),
        getDelButtonHeight());
}

QRect PacksView::getAddButtonRect(const QRect &_stickerRect) const
{
    return QRect(
        _stickerRect.width() - getAddButtonHeight() - getMarginRight(),
        _stickerRect.top() + (_stickerRect.height() - getAddButtonHeight())/2,
        getAddButtonHeight(),
        getAddButtonHeight());
}

QRect PacksView::getDragButtonRect(const QRect& _stickerRect) const
{
    return QRect(
        _stickerRect.width() - getDragButtonHeight() - getMarginRight(),
        _stickerRect.top() + (_stickerRect.height() - getDragButtonHeight()) / 2,
        getDragButtonHeight(),
        getDragButtonHeight());
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
    QPixmap button(Utils::renderSvgScaled(qsl(":/delete_icon"), QSize(20, 20), getButtonColor(_hovered, _selected)));

    return button;
}

QPixmap getDragButton(bool _hovered, bool _selected)
{
    QPixmap button(Utils::renderSvgScaled(qsl(":/smiles_menu/move_sticker_icon"), QSize(20, 20), getButtonColor(_hovered, _selected)));

    return button;
}

QPixmap getAddButton(bool _hovered, bool _selected)
{
    QPixmap button(Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), getAddButtonColor(_hovered, _selected)));

    return button;
}

void drawButton(QPainter& _p, const QPixmap& _image, const QRect& _buttonRect)
{
    QRect imageRect(QPoint(0, 0), Utils::unscale_bitmap(_image.size()));
    imageRect.moveCenter(_buttonRect.center());
    _p.drawPixmap(imageRect, _image);
}

void PacksView::drawStickerPack(QPainter& _p, const QRect& _stickerRect, PackInfo& _pack, bool _hovered, bool _selected)
{
    if (!_pack.iconRequested_)
    {
        if (const auto stickersSet = Stickers::getStoreSet(_pack.id_))
        {
            _pack.icon_ = scaleIcon(stickersSet->getBigIcon(), getMyPackIconSize());
            _pack.iconRequested_ = true;
        }
    }

    const bool highlight = _hovered || _selected;
    if (highlight)
        _p.fillRect(_stickerRect, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

    const QRect iconRect(_stickerRect.left() + getMarginLeft(), _stickerRect.top() + getMyPackIconTopMargin(), getMyPackIconSize(), getMyPackIconSize());
    if (!_pack.icon_.isNull())
    {
        _p.drawPixmap(iconRect, _pack.icon_);
    }
    else
    {
        const auto radius = qreal(Utils::scale_value(8));
        const auto var = highlight ? Styling::StyleVariable::BASE_GLOBALWHITE : Styling::StyleVariable::BASE_BRIGHT_INVERSE;

        Utils::PainterSaver ps(_p);
        _p.setBrush(Styling::getParameters().getColor(var));
        _p.setPen(Qt::NoPen);
        _p.drawRoundedRect(iconRect, radius, radius);
    }

    static auto nameFont = getNameFont();
    static auto descFont = getDescFont();

    const int textWidth = _stickerRect.width() - iconRect.right() - getDelButtonHeight() - 2 * getMarginRight() - getDragButtonHeight();

    auto & nameUnit = nameUnits_[_pack.id_];
    if (!nameUnit)
    {
        nameUnit = TextRendering::MakeTextUnit(_pack.name_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        nameUnit->init(nameFont, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        nameUnit->evaluateDesiredSize();
    }
    nameUnit->elide(textWidth);
    nameUnit->setOffsets(_stickerRect.left() + getMyPackIconSize() + getMyPackTextLeftMargin() + getMarginLeft(), _stickerRect.top() + getMyPackTextTopMargin());
    nameUnit->draw(_p);

    auto & descUnit = descUnits_[_pack.id_];
    if (!descUnit)
    {
        descUnit = TextRendering::MakeTextUnit(_pack.subtitle_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        descUnit->init(descFont, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        descUnit->evaluateDesiredSize();
    }
    descUnit->elide(textWidth);
    descUnit->setOffsets(_stickerRect.left() + getMyPackIconSize() + getMyPackTextLeftMargin() + getMarginLeft(), _stickerRect.top() + getMyPackDescTopMargin());
    descUnit->draw(_p);

    if (_pack.purchased_)
    {
        static QPixmap delButton = getDelButton(_hovered, _selected);
        drawButton(_p, delButton, getDelButtonRect(_stickerRect));

        if (mode_ == Mode::my_packs)
        {
            static QPixmap dragButton = getDragButton(_hovered, _selected);
            drawButton(_p, dragButton, getDragButtonRect(_stickerRect));
        }
    }
    else
    {
        static QPixmap addButton = getAddButton(_hovered, _selected);
        drawButton(_p, addButton, getAddButtonRect(_stickerRect));
    }
}

void PacksView::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.setPen(Qt::NoPen);

    int dragPackPos = -1;

    for (int i = 0; i < (int) packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);
        auto& pack = *packs_[i];

        if (pack.id_ == dragPack_)
        {
            dragPackPos = i;

            p.fillRect(stickerRect, QBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)));

            continue;
        }

        if (stickerRect.intersects(_e->rect()))
        {
            const auto hovered = hoveredIdx_ == i && selectedIdx_ == -1 && dragPack_ == -1;
            const auto selected = selectedIdx_ == i;

            drawStickerPack(p, stickerRect, pack, hovered, selected);
        }
    }

    if (dragPackPos != -1)
    {
        p.setOpacity(0.7);

        const QRect stickerRect = getStickerRect(dragPackPos);

        const QRect drawRect(stickerRect.left(), stickerRect.top() + dragOffset_, stickerRect.width(), stickerRect.height());

        p.fillRect(drawRect, QBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE)));

        drawStickerPack(p, drawRect, *packs_[dragPackPos]);

        p.setOpacity(1.0);
    }
}

void PacksView::mousePressEvent(QMouseEvent* _e)
{
    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            auto& onePack = *packs_[i];
            if (onePack.purchased_)
            {
                const QRect delButtonRect = getDelButtonRect(stickerRect);
                const QRect dragButtonRect = getDragButtonRect(stickerRect);

                if (delButtonRect.contains(_e->pos()))
                {
                    const auto packId = onePack.id_;
                    const auto confirm = Utils::GetConfirmationWithTwoButtons(
                        QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                        QT_TRANSLATE_NOOP("popup_window", "Yes"),
                        QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to remove this sticker pack?"),
                        QT_TRANSLATE_NOOP("popup_window", "Remove sticker pack"),
                        nullptr);

                    if (confirm)
                        GetDispatcher()->removeStickersPack(packId, QString());

                    return;
                }
                else if (dragButtonRect.contains(_e->pos()))
                {
                    return startDrag(i, _e->pos());
                }
            }
            else
            {
                const QRect addButtonRect = getAddButtonRect(stickerRect);

                if (addButtonRect.contains(_e->pos()))
                {
                    auto pack = Stickers::getStoreSet(onePack.id_);
                    if (pack)
                        GetDispatcher()->addStickersPack(pack->getId(), pack->getStoreId());

                    if (mode_ == Mode::search)
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_add_pack_button_tap);

                    return;
                }
            }

            auto context = (mode_ == Mode::my_packs) ? Stickers::StatContext::Discover : Stickers::StatContext::Search;
            Stickers::showStickersPack(onePack.id_, context);

            if (mode_ == Mode::search)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_pack_tap);

            return;
        }
    }
}

void PacksView::mouseReleaseEvent(QMouseEvent* _e)
{
    if (dragPack_ != -1)
    {
        dragPack_ = -1;

        postStickersOrder();

        Q_EMIT stopScroll();

        update();

        updateCursor();
    }
}

int PacksView::getDragPackNum()
{
    for (int i = 0; i < (int) packs_.size(); ++i)
    {
        if (packs_[i]->id_ == dragPack_)
            return i;
    }

    return -1;
}

int PacksView::getPackUnderCursor() const
{
    const auto point = this->mapFromGlobal(QCursor::pos());
    static const auto height = getMyStickerHeight();

    if (scrollBar_ && scrollBar_->isVisible() && point.x() > (scrollBar_->geometry().left() - getHoverMargin()))
        return -1;

    return static_cast<int>(point.y() / height);
}

void PacksView::clearSelection()
{
    selectedIdx_ = -1;
}

void PacksView::updateHover()
{
    const auto lastHovered = hoveredIdx_;
    hoveredIdx_ = getPackUnderCursor();

    const auto lastSelected = selectedIdx_;
    selectedIdx_ = -1;

    if (lastHovered != hoveredIdx_)
    {
        update(getStickerRect(lastHovered));
        update(getStickerRect(hoveredIdx_));
    }

    if (lastSelected != -1 && lastSelected != hoveredIdx_)
        update(getStickerRect(lastSelected));
}

void PacksView::updateCursor()
{
    const auto stickerRect = getStickerRect(getPackUnderCursor());
    const auto dragButtonRect = getDragButtonRect(stickerRect);

    if (dragButtonRect.contains(this->mapFromGlobal(QCursor::pos())) && mode_ == Mode::my_packs)
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::PointingHandCursor);
}

bool PacksView::isDragInProgress() const
{
    return (dragPack_ != -1);
}

void PacksView::mouseMoveEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::NoButton)
    {
        updateHover();
    }
    if (dragPack_ != -1)
    {
        const QRect viewportRect = visibleRegion().boundingRect();

        if (_e->pos().y() > viewportRect.bottom())
        {
            Q_EMIT startScrollDown();
        }
        else if (_e->pos().y() < viewportRect.top())
        {
            Q_EMIT startScrollUp();
        }
        else
        {
            Q_EMIT stopScroll();
        }

        const int dragPackPos = getDragPackNum();
        if (dragPackPos == -1)
        {
            assert(false);
            return;
        }

        const auto dragPackRect = getStickerRect(dragPackPos);

        const int dragOffsetPrev = dragOffset_;

        dragOffset_ = _e->pos().y() - startDragPos_.y();

        const QRect oldRect(dragPackRect.left(), dragPackRect.top() + dragOffsetPrev, dragPackRect.width(), dragPackRect.height());
        const QRect newRect(dragPackRect.left(), dragPackRect.top() + dragOffset_, dragPackRect.width(), dragPackRect.height());

        for (int i = 0; i < int(packs_.size()); ++i)
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

                std::swap(packs_[i], packs_[dragPackPos]);

                update(packRect);
                update(dragPackRect);

                return QWidget::mouseMoveEvent(_e);
            }
        }

        update(oldRect);
        update(newRect);
    }
    else
    {
        updateCursor();
    }

    return QWidget::mouseMoveEvent(_e);
}

void PacksView::leaveEvent(QEvent *_e)
{
    update(getStickerRect(hoveredIdx_));
    hoveredIdx_ = -1;

    QWidget::leaveEvent(_e);
}

void PacksView::wheelEvent(QWheelEvent *_e)
{
    QWidget::wheelEvent(_e);

    // update hover after view has been scrolled
    QTimer::singleShot(0, this, &PacksView::updateHover);
}

void PacksView::startDrag(const int _packNum, const QPoint& _mousePos)
{
    startDragPos_ = _mousePos;

    dragOffset_ = 0;

    dragPack_ = packs_[_packNum]->id_;

    setCursor(Qt::SizeAllCursor);
}

void PacksView::postStickersOrder() const
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    core::ifptr<core::iarray> values_array(collection->create_array(), true);

    values_array->reserve(packs_.size());

    for (const auto& pack : packs_)
    {
        core::ifptr<core::ivalue> ival(collection->create_value(), true);
        ival->set_as_int(pack->id_);
        values_array->push_back(ival.get());
    }

    collection.set_value_as_array("values", values_array.get());

    Ui::GetDispatcher()->post_message_to_core("stickers/set/order", collection.get());
}

void PacksView::onSetIcon(const int32_t _setId)
{
    const auto it = std::find_if(packs_.begin(), packs_.end(), [_setId](const auto& pack) { return pack->id_ == _setId; });
    if (it != packs_.end())
    {
        if (auto pack = Stickers::getStoreSet(_setId))
            (*it)->icon_ = scaleIcon(pack->getBigIcon(), getMyPackIconSize());

        update(getStickerRect(std::distance(packs_.begin(), it)));
    }
}


bool PacksView::empty() const
{
    return packs_.empty();
}

MyPacksHeader::MyPacksHeader(QWidget* _parent)
    : QWidget(_parent)
    , marginText_(Utils::scale_value(8))
    , buttonWidth_(Utils::scale_value(20))
{
    setFixedHeight(getMyHeaderHeight());

    createStickerPack_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("stickers", "Add your stickers"));
    createStickerPack_->init(Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
    createStickerPack_->evaluateDesiredSize();

    myText_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("stickers", "MY"));
    myText_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    myText_->evaluateDesiredSize();

    buttonAdd_ = new ClickableWidget(this);
    connect(buttonAdd_, &ClickableWidget::clicked, this, [this]() { Q_EMIT addClicked(QPrivateSignal()); });
}

void MyPacksHeader::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    static QPixmap addPackButtonImage(Utils::renderSvgScaled(qsl(":/controls/add_icon"), getAddButtonSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)));

    myText_->setOffsets(getMarginLeft(), height() / 2);
    myText_->draw(p, TextRendering::VerPosition::MIDDLE);

    const int textX = rect().width() - createStickerPack_->desiredWidth() - getMarginRight();
    createStickerPack_->setOffsets(textX, height() / 2);
    createStickerPack_->draw(p, TextRendering::VerPosition::MIDDLE);

    const int offsetY = (height() - buttonWidth_) / 2;

    QRect addPackRect(textX - buttonWidth_ - marginText_, offsetY, buttonWidth_, buttonWidth_);

    p.drawPixmap(addPackRect, addPackButtonImage);
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

    packs_ = new PacksView(PacksView::Mode::my_packs, this);

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

    init(false);
}

void MyPacksWidget::init(const bool _fromServer)
{
    if (_fromServer)
        syncedWithServer_ = true;

    if (packs_->isDragInProgress())
        return;

    packs_->clear();

    for (const auto _setId : Stickers::getStoreStickersSets())
    {
        const auto stickersSet = Stickers::getStoreSet(_setId);

        if (!stickersSet)
            continue;

        if (stickersSet->isPurchased())
        {
            packs_->addPack(new PackInfoObject(
                packs_,
                _setId,
                Utils::replaceLine(stickersSet->getName()),
                Utils::replaceLine(stickersSet->getSubtitle()),
                stickersSet->getStoreId(),
                scaleIcon(QPixmap(), Utils::scale_value(52)),
                stickersSet->isPurchased(), false));
        }
    }

    packs_->updateSize();

    QRect rcText(0, packs_->geometry().top(), rect().width(), rect().height() - packs_->geometry().top());

    update(rcText);
}

void MyPacksWidget::setScrollBarToView(QWidget *_scrollBar)
{
    packs_->setScrollBar(_scrollBar);
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

    if (Logic::getContactListModel()->contains(bot))
    {
        Logic::getContactListModel()->setCurrent(bot, -1, true);
    }
    else
    {
        Logic::getContactListModel()->addContactToCL(bot, [weak_this = QPointer(this), bot](bool _res)
        {
            if (!weak_this)
                return;

            Logic::getContactListModel()->setCurrent(bot, -1, true);
        });
    }

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_addbot_bottom_tap);
}


//////////////////////////////////////////////////////////////////////////
// class Store
//////////////////////////////////////////////////////////////////////////
Store::Store(QWidget* _parent)
    : QWidget(_parent)
{
    Utils::ApplyStyle(this, Styling::getParameters().getStoreQss());

    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setAlignment(Qt::AlignTop);
    setLayout(rootLayout);

    headerWidget_ = new StickersPageHeader(this);
    connect(headerWidget_, &StickersPageHeader::backClicked, this, &Store::back);
    rootLayout->addWidget(headerWidget_);

    connect(headerWidget_, &StickersPageHeader::searchRequested, this, &Store::onSearchRequested);
    connect(headerWidget_, &StickersPageHeader::searchCancelled, this, &Store::onSearchCancelled);

    rootScrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    rootScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rootScrollArea_->setFocusPolicy(Qt::NoFocus);
    Utils::grabTouchWidget(rootScrollArea_->viewport(), true);

    connect(QScroller::scroller(rootScrollArea_->viewport()), &QScroller::stateChanged, this, &Store::touchScrollStateChanged, Qt::QueuedConnection);

    QWidget* scrollAreaWidget = new QWidget(rootScrollArea_);
    scrollAreaWidget->setObjectName(qsl("RootWidget"));

    rootScrollArea_->setWidget(scrollAreaWidget);
    rootScrollArea_->setWidgetResizable(true);

    rootLayout->addWidget(rootScrollArea_);

    QVBoxLayout* widgetLayout = new QVBoxLayout();
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setAlignment(Qt::AlignTop);
    scrollAreaWidget->setLayout(widgetLayout);

    packsView_ = new PacksWidget(this);
    widgetLayout->addWidget(packsView_);
    Utils::grabTouchWidget(packsView_);

    myPacks_ = new MyPacksWidget(this);
    myPacks_->setScrollBarToView(rootScrollArea_->getScrollBarV());
    widgetLayout->addWidget(myPacks_);
    Utils::grabTouchWidget(myPacks_);

    searchPacks_ = new SearchPacksWidget(this);
    searchPacks_->setScrollBarToView(rootScrollArea_->getScrollBarV());
    widgetLayout->addWidget(searchPacks_);
    Utils::grabTouchWidget(searchPacks_);
    searchPacks_->hide();

    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::setBigIconChanged, this, &Store::onSetBigIcon);
    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onStore, this, &Store::onStore);
    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onStoreSearch, this, &Store::onSearchStore);
    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onStickers, this, &Store::onStickers);

    QObject::connect(myPacks_, &MyPacksWidget::scrollStep, this, &Store::onScrollStep);
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
    packsView_->init(true);
    myPacks_->init(true);
}

void Store::onSearchStore()
{
    searchPacks_->init(true);
}

void Store::onSearchRequested(const QString& term)
{
    Stickers::requestSearch(term);
    myPacks_->hide();
    packsView_->hide();
    searchPacks_->init(false);
    searchPacks_->touchSearchRequested();
    searchPacks_->show();


    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_request_send);
}

void Store::onSearchCancelled()
{
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
    auto newY = -1;
    switch (_e->key())
    {
        case Qt::Key_Up:
            if (searchPacks_->isVisible())
                newY = searchPacks_->moveCursor(PacksView::MoveUp);
            break;

        case Qt::Key_Down:
            if (searchPacks_->isVisible())
                newY = searchPacks_->moveCursor(PacksView::MoveDown);
            break;

        case Qt::Key_PageUp:
            if (searchPacks_->isVisible())
                newY = searchPacks_->moveCursor(PacksView::MovePageUp);
            break;

        case Qt::Key_PageDown:
            if (searchPacks_->isVisible())
                newY = searchPacks_->moveCursor(PacksView::MovePageDown);
            break;

        case Qt::Key_Return:
            if (searchPacks_->isVisible() && searchPacks_->selectedPackId() != -1)
            {
                headerWidget_->setFocus();
                Stickers::showStickersPack(searchPacks_->selectedPackId(), Stickers::StatContext::Search);
                return;
            }
            break;

        case Qt::Key_Escape:
            hideSearch();
            break;
    }

    if (searchPacks_->isVisible())
    {
        if (newY != -1)
            rootScrollArea_->ensureVisible(0, newY + getSearchPacksOffset(), 0, 0);
    }

    QWidget::keyPressEvent(_e);
}

namespace {
const int back_spacing = 16;
const int start_search_delay_ms = 700;

int badgeTopOffset()
{
    return Utils::scale_value(8);
}

int badgeLeftOffset()
{
    return Utils::scale_value(28);
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
    textUnit_->init(Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    textUnit_->evaluateDesiredSize();
    textBadgeUnit_->init(Fonts::appFontScaled(11), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
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
        Utils::Badge::drawBadge(textBadgeUnit_, p, badgeLeftOffset(), badgeTopOffset(), Utils::Badge::Color::Green);
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
    Utils::setDefaultBackground(searchHeader_);

    auto searchLayout = Utils::emptyHLayout(searchHeader_);
    searchLayout->addSpacing(Utils::scale_value(20) - 2); // qlineedit has a built in 2px left padding

    searchInput_ = new LineEditEx(this);
    searchInput_->setFrame(false);
    searchInput_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
    searchInput_->setCustomPlaceholderColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    Utils::ApplyStyle(searchInput_, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)));
    searchInput_->setCustomPlaceholder(QT_TRANSLATE_NOOP("stickers", "Search"));
    searchInput_->setAttribute(Qt::WA_MacShowFocusRect, false);
    searchInput_->installEventFilter(this);
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

QString StickersPageHeader::currentTerm() const
{
    return lastTerm_;
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
    , syncedWithServer_(false)
{
    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignTop);

    rootLayout->addSpacing(getSearchPacksOffset());
    packs_ = new PacksView(PacksView::Mode::search, this);

    noResultsWidget_ = new QWidget(this);
    noResultsWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    auto noResultsLayout = Utils::emptyVLayout(noResultsWidget_);

    auto noResultsPlaceholder = new QLabel(noResultsWidget_);
    noResultsPlaceholder->setPixmap(Utils::renderSvgScaled(qsl(":/placeholders/empty_search"), QSize(84, 84), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));

    auto noResultsLabel = new QLabel(noResultsWidget_);
    noResultsLabel->setText(getNoSearchResultsText());

    QPalette noResultsTextPalette;
    noResultsTextPalette.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    noResultsLabel->setPalette(noResultsTextPalette);
    noResultsLabel->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
    noResultsLabel->setAlignment(Qt::AlignHCenter);
    rootLayout->setAlignment(noResultsWidget_, Qt::AlignCenter);

    noResultsLayout->addStretch();
    noResultsLayout->addWidget(noResultsPlaceholder);
    noResultsLayout->addSpacing(Utils::scale_value(30));
    noResultsLayout->addWidget(noResultsLabel);
    noResultsLayout->addStretch();
    noResultsLayout->setAlignment(noResultsPlaceholder, Qt::AlignHCenter);
    noResultsLayout->setAlignment(noResultsLabel, Qt::AlignHCenter);

    noResultsWidget_->hide();

    rootLayout->addWidget(packs_);
    rootLayout->addWidget(noResultsWidget_);

    setLayout(rootLayout);

    init(false);
}

void SearchPacksWidget::init(const bool _fromServer)
{
    if (_fromServer)
        syncedWithServer_ = true;

    packs_->clear();

    enum class IsPurchased {no, yes};

    auto addPacksIf = [this](auto predicate)
    {
        bool purchased = (predicate == IsPurchased::yes);
        for (const auto _setId : Stickers::getSearchStickersSets())
        {
            const auto stickersSet = Stickers::getStoreSet(_setId);

            if (!stickersSet)
                continue;

            if (stickersSet->isPurchased() == purchased)
            {
                packs_->addPack(new PackInfoObject(
                    this,
                    _setId,
                    Utils::replaceLine(stickersSet->getName()),
                    Utils::replaceLine(stickersSet->getSubtitle()),
                    stickersSet->getStoreId(),
                    scaleIcon(stickersSet->getBigIcon(), Utils::scale_value(52)),
                    stickersSet->isPurchased(), true));
           }
        }
    };

    addPacksIf(IsPurchased::yes);
    addPacksIf(IsPurchased::no);

    // since download tasks are pushed in front of the queue, go through sets in reverse order and initiate icon downloading
    const auto stickers = Stickers::getSearchStickersSets();
    for (auto it = stickers.rbegin(); it != stickers.rend(); ++it)
    {
        const auto set = Stickers::getStoreSet(*it);
        if (!set)
            continue;
        set->getBigIcon();
    }

    packs_->selectFirst();
    packs_->updateSize();

    if (_fromServer)
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

void SearchPacksWidget::touchSearchRequested()
{
    searchRequested_ = true;
}

void SearchPacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}

QAccessibleInterface* AccessiblePacksView::child(int index) const
{
    if (index < 0 || index + 1 > static_cast<int>(packsView_->packs_.size()))
        return nullptr;

    auto pack = packsView_->packs_.at(index);
    auto result = QAccessible::queryAccessibleInterface(pack);
    assert(result);
    return result;
}

int AccessiblePacksView::indexOfChild(const QAccessibleInterface* child) const
{
    return Utils::indexOf(packsView_->packs_.cbegin(), packsView_->packs_.cend(), child->object());
}

QString AccessiblePacksView::text(QAccessible::Text type) const
{
    return type == QAccessible::Text::Name ? qsl("AS Stickers AccessiblePacksView") : QString();
}

QRect AccessibleStickerPackButton::rect() const
{
    auto packInfo = qobject_cast<PackInfoObject*>(object());
    assert(packInfo);
    auto packsView = qobject_cast<Ui::Stickers::PacksView*>(packInfo->parent());
    assert(packsView);
    auto result = packsView->getStickerRect(packsView->getPackInfoIndex(packInfo));
    auto globalPos = packsView->mapToGlobal({ 0, 0 });
    result.moveTo(result.topLeft() + globalPos);
    return result;
}

QAccessibleInterface* AccessibleStickerPackButton::parent() const
{
    QAccessibleInterface* result = nullptr;
    if (auto obj = object())
        result = QAccessible::queryAccessibleInterface(obj->parent());
    assert(result);
    return result;
}
