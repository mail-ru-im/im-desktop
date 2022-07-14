#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../utils/utils.h"

#include "../controls/TextUnit.h"

#include "../controls/TextWidget.h"
#include "../fonts.h"
#include "Drawable.h"
#include "styles/ThemeParameters.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

#include "GalleryFrame.h"

namespace
{
    constexpr QStringView imagePath = u":/gallery/glr_";
    const auto backgroundOpacity = 0.92;
    const auto backgroundColor = QColor(u"#1E1E1E");

    auto getMenuBorderRadius() noexcept
    {
        return Utils::scale_value(8);
    }
}

namespace Previewer
{

using namespace Ui;

enum ControlType : short
{
    ZoomOutButton,
    ZoomInButton,
    PrevButton,
    NextButton,
    CloseButton,
    MenuButton,
    SaveButton,
    CounterLabel,
    AuthorLabel,
    DateLabel
};

auto getTopMargin() { return Utils::scale_value(70); }

int topMargin = getTopMargin();

int max_height = Utils::scale_value(268);

enum Margin : int
{
    _4px = 4,
    _8px = 8,
    _12px = 12,
    _16px = 16,
    _20px = 20,
    _24px = 24,
    _32px = 32
};

CaptionArea::CaptionArea(QWidget* _parent)
    : QScrollArea(_parent)
    , expanded_(false)
    , totalWidth_(0)
    , anim_(new QVariantAnimation(this))
{
    textWidget_ = new Ui::TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    textWidget_->init({ Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Styling::ColorParameter{ Qt::white } });

    textPreview_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Styling::ColorParameter{ Qt::white } };
    params.maxLinesCount_ = 2;
    params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
    textPreview_->init(params);

    more_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("previewer", "more"));
    more_->init({ Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Styling::ColorParameter{ Qt::white } });
    more_->setUnderline(true);
    more_->evaluateDesiredSize();

    setStyleSheet(qsl("background: transparent;"));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setWidget(textWidget_);
    setWidgetResizable(true);
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    Utils::ApplyStyle(verticalScrollBar(), Styling::getParameters().getVerticalScrollBarSimpleQss());

    setExpanded(false);
    setMouseTracking(true);
    setAutoFillBackground(true);

    anim_->setDuration(300);
    anim_->setEasingCurve(QEasingCurve::OutExpo);
}

void CaptionArea::setCaption(const QString& _caption, int _totalWidth)
{
    totalWidth_ = _totalWidth;
    textWidget_->setText(_caption);
    textPreview_->setText(_caption);
    setExpanded(false);
}

void CaptionArea::updateSize()
{
    static auto max_width = Utils::scale_value(684);

    if (expanded_)
    {
        textWidget_->setMaxWidthAndResize(max_width - Utils::scale_value(8));
    }
    else
    {
        textPreview_->setLastLineWidth(max_width - more_->cachedSize().width());
        setFixedSize(max_width, textPreview_->getHeight(max_width));
        auto xOffset = textPreview_->desiredWidth() < max_width ? (width() / 2 - textPreview_->desiredWidth() / 2) : 0;
        textPreview_->setOffsets(xOffset, 0);
        more_->setOffsets(max_width - more_->cachedSize().width(), height() - more_->cachedSize().height());
    }
}

void CaptionArea::setExpanded(bool _expanded)
{
    expanded_ = _expanded;
    textWidget_->setVisible(_expanded);

    updateSize();

    auto addSpace = height() + Utils::scale_value(Margin::_32px);
    topMargin = expanded_ ? addSpace : getTopMargin();

    move(totalWidth_ / 2 - width() / 2, expanded_ ? Utils::scale_value(Margin::_16px) : topMargin - height() - Utils::scale_value(_16px));
    if (expanded_)
    {
        if (anim_->state() != QAbstractAnimation::State::Running)
        {
            const auto endHeight = std::min(max_height, textWidget_->height());
            const auto startHeight = height();
            const auto endSpace = endHeight - getTopMargin() + Utils::scale_value(Margin::_32px);

            anim_->stop();
            anim_->setStartValue(0);
            anim_->setEndValue(endSpace);
            anim_->disconnect(this);
            connect(anim_, &QVariantAnimation::valueChanged, this, [this, endSpace, endHeight, startHeight](const QVariant& value)
            {
                Q_EMIT needHeight(value.toInt());
                setFixedHeight(startHeight + (endHeight - startHeight) * (value.toDouble() / endSpace));
            });
            anim_->start();
        }
    }
    else
    {
        Q_EMIT needHeight(0);
    }

    setVerticalScrollBarPolicy((expanded_ && textWidget_->height() > max_height) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    update();
}

bool CaptionArea::isExpanded() const
{
    return expanded_;
}

void CaptionArea::paintEvent(QPaintEvent* _e)
{
    QPainter p(viewport());
    if (!expanded_)
    {
        textPreview_->draw(p);
        if (textPreview_->isElided())
            more_->draw(p);
        return;
    }

    QScrollArea::paintEvent(_e);
}

void CaptionArea::mouseMoveEvent(QMouseEvent* _e)
{
    if (!expanded_ && textPreview_->isElided())
    {
        setCursor(more_->contains(_e->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    QScrollArea::mouseMoveEvent(_e);
}

void CaptionArea::enterEvent(QEvent* _e)
{
    setVerticalScrollBarPolicy((expanded_ && textWidget_->height() > max_height) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    update();

    QScrollArea::enterEvent(_e);
}

void CaptionArea::leaveEvent(QEvent* _e)
{
    setCursor(Qt::ArrowCursor);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    update();

    QScrollArea::leaveEvent(_e);
}

void CaptionArea::mousePressEvent(QMouseEvent* _e)
{
    clicked_ = _e->pos();
    QScrollArea::mousePressEvent(_e);
}

void CaptionArea::mouseReleaseEvent(QMouseEvent* _e)
{
    if (Utils::clicked(clicked_, _e->pos()))
    {
        if (!expanded_ && textPreview_->isElided() && more_->contains(_e->pos()))
            setExpanded(!expanded_);
    }

    QScrollArea::mouseReleaseEvent(_e);
}

class GalleryFrame_p
{
public:

    enum ButtonState
    {
        Default     = 0x1,
        Disabled    = 0x4,
        Hover       = 0x10,
        Pressed     = 0x20
    };

    QString getImagePath(QStringView name, ButtonState state) const
    {
        switch (state)
        {
            case Default:   return imagePath % name % u"_icon";
            case Disabled:  return imagePath % name % u"_icon" % u"_disabled";
            case Hover:     return imagePath % name % u"_icon" % u"_hover";
            case Pressed:   return imagePath % name % u"_icon" % u"_active";
        }
        im_assert(!"invalid kind!");
        return QString();
    }

    auto objectSize(ControlType _type) const
    {
        switch (_type)
        {
            case ZoomInButton:
            case ZoomOutButton:
                return Utils::scale_value(QSize(44, 40));

            case PrevButton:
            case NextButton:
            case CloseButton:
            case MenuButton:
            case SaveButton:
                return Utils::scale_value(QSize(40, 40));

            case CounterLabel:
                return Utils::scale_value(QSize(180, 36));

            case AuthorLabel:
            case DateLabel:
                return Utils::scale_value(QSize(137, 20));

            default:
                return QSize();
        }
    }

    auto solidRectHeight() const { return Utils::scale_value(40); }

    ButtonAccessible* addButton(ControlType _type, QStringView _iconName, QWidget* parent)
    {
        auto button = std::make_unique<ButtonAccessible>(parent);
        const auto size = objectSize(_type);
        button->setDefaultPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Default), size));
        button->setHoveredPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Hover), size));
        button->setPressedPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Pressed), size));
        button->setDisabledPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Disabled), size));

        auto pointer = button.get();
        objects_[_type] = std::move(button);
        return pointer;
    }

    QPoint objectPosition(ControlType _type, int _width, int _height) const
    {
        auto offset = _height - Utils::scale_value(92);
        switch (_type)
        {
            case DateLabel:
                return {objectPosition(AuthorLabel, _width, _height).x(), offset + objectSize(AuthorLabel).height()};
            case AuthorLabel:
                return {objectPosition(ZoomOutButton, _width, _height).x() - Utils::scale_value(Margin::_32px) - objectSize(_type).width(), offset};
            case ZoomOutButton:
                return {objectPosition(ZoomInButton, _width, _height).x() - Utils::scale_value(Margin::_4px) - objectSize(_type).width(), offset};
            case ZoomInButton:
                return {objectPosition(PrevButton, _width, _height).x() - Utils::scale_value(Margin::_32px) - objectSize(_type).width(), offset};
            case PrevButton:
                return {objectPosition(CounterLabel, _width, _height).x() - Utils::scale_value(Margin::_16px) - objectSize(_type).width(), offset};
            case CounterLabel:
                return {(_width - objectSize(_type).width()) / 2, offset};
            case NextButton:
                return {objectPosition(CounterLabel, _width, _height).x() + Utils::scale_value(Margin::_16px) + objectSize(CounterLabel).width(), offset};
            case SaveButton:
                return {objectPosition(NextButton, _width, _height).x() + Utils::scale_value(Margin::_32px) + objectSize(NextButton).width(), offset};
            case MenuButton:
                return {objectPosition(SaveButton, _width, _height).x() + Utils::scale_value(Margin::_12px) + objectSize(SaveButton).width(), offset};
            case CloseButton:
                return {objectPosition(MenuButton, _width, _height).x() + Utils::scale_value(Margin::_32px) + objectSize(MenuButton).width(), offset};
        }
        return QPoint();
    }

    void updateObjectsPositions(int _width, int _height)
    {
        for (auto & [type, object] : objects_)
        {
            object->setRect(QRect(objectPosition(type, _width, _height), objectSize(type)));
        }
    }

    void resetObjects()
    {
        for (auto & [_, object] : objects_)
        {
            object->setHovered(false);
            object->setPressed(false);
        }
    }

    void setCaption(const QString& _caption, int _width)
    {
        caption_->setCaption(_caption, _width);
    }

    void collapseCaption()
    {
        caption_->setExpanded(false);
    }

    bool isCaptionExpanded()
    {
        return caption_->isExpanded();
    }

    std::unordered_map<ControlType, std::unique_ptr<Drawable>> objects_;
    QPointer<CustomMenu> menu_;
    std::unique_ptr<CaptionArea> caption_;
    GalleryFrame::Actions menuActions_;
    bool gotoEnabled_ = true;
};

GalleryFrame::GalleryFrame(QWidget *_parent)
    : QWidget(_parent)
    , d(std::make_unique<GalleryFrame_p>())
{
    topMargin = getTopMargin();

    if (auto button = d->addButton(ZoomOutButton, u"zoom_out", this))
        button->setAccessibleName(qsl("AS Preview zoomOut"));

    if (auto button = d->addButton(ZoomInButton, u"zoom_in", this))
        button->setAccessibleName(qsl("AS Preview zoomIn"));

    if (auto button = d->addButton(PrevButton, u"previous", this))
        button->setAccessibleName(qsl("AS Preview previous"));

    if (auto button = d->addButton(NextButton, u"next", this))
        button->setAccessibleName(qsl("AS Preview next"));

    if (auto button = d->addButton(CloseButton, u"close", this))
        button->setAccessibleName(qsl("AS Preview close"));

    if (auto button = d->addButton(MenuButton, u"more", this))
        button->setAccessibleName(qsl("AS Preview more"));

    if (auto button = d->addButton(SaveButton, u"download", this))
        button->setAccessibleName(qsl("AS Preview download"));

    const auto counterFont(Fonts::appFontScaled(20, Fonts::FontWeight::Normal));
    auto counterLabelUnit = TextRendering::MakeTextUnit(QString());
    TextRendering::TextUnit::InitializeParameters params(counterFont, Styling::ColorParameter{ Qt::white });
    params.maxLinesCount_ = 1;
    params.align_ = TextRendering::HorAligment::CENTER;
    counterLabelUnit->init(params);

    auto counterLabel = std::make_unique<Label>();
    counterLabel->setTextUnit(std::move(counterLabelUnit));
    counterLabel->setYOffset(Utils::scale_value(Margin::_8px));
    d->objects_[CounterLabel] = std::move(counterLabel);

    const auto authorFont(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));
    auto authorLabelUnit = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    params.setFonts(authorFont);
    params.align_ = TextRendering::HorAligment::LEFT;
    authorLabelUnit->init(params);

    auto authorLabel = std::make_unique<Label>();
    authorLabel->setTextUnit(std::move(authorLabelUnit));
    authorLabel->setClickable(true);
    authorLabel->setDefaultColor(Styling::ColorParameter{ Qt::white });
    authorLabel->setHoveredColor(Styling::ColorParameter{ QColor(255, 255, 255, static_cast<int>(255 * 0.65)) });
    authorLabel->setPressedColor(Styling::ColorParameter{ QColor(255, 255, 255, static_cast<int>(255 * 0.5)) });
    d->objects_[AuthorLabel] = std::move(authorLabel);

    const auto dateFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));
    auto dateLabelUnit = TextRendering::MakeTextUnit(QString());
    params.setFonts(dateFont);
    dateLabelUnit->init(params);

    auto dateLabel = std::make_unique<Label>();
    dateLabel->setTextUnit(std::move(dateLabelUnit));
    dateLabel->setClickable(true);
    dateLabel->setDefaultColor(Styling::ColorParameter{ Qt::white });
    dateLabel->setHoveredColor(Styling::ColorParameter{ QColor(255, 255, 255, static_cast<int>(255 * 0.65)) });
    dateLabel->setPressedColor(Styling::ColorParameter{ QColor(255, 255, 255, static_cast<int>(255 * 0.5)) });
    d->objects_[DateLabel] = std::move(dateLabel);

    d->caption_ = std::make_unique<CaptionArea>(this);
    connect(d->caption_.get(), &CaptionArea::needHeight, this, &GalleryFrame::needHeight);

    connect(new QShortcut(QKeySequence::Copy, this), &QShortcut::activated, this, &GalleryFrame::copy);

    setMouseTracking(true);
}

GalleryFrame::~GalleryFrame()
{

}

void GalleryFrame::setFixedSize(const QSize& _size)
{
   QWidget::setFixedSize(_size);
   d->updateObjectsPositions(_size.width(), _size.height());

   update();
}

void GalleryFrame::setNextEnabled(bool _enable)
{
    auto & next = d->objects_[NextButton];
    next->setDisabled(!_enable);
    update(next->rect());
}

void GalleryFrame::setPrevEnabled(bool _enable)
{
    auto & prev = d->objects_[PrevButton];
    prev->setDisabled(!_enable);
    update(prev->rect());
}

void GalleryFrame::setZoomInEnabled(bool _enable)
{
    auto & zoomIn = d->objects_[ZoomInButton];
    zoomIn->setDisabled(!_enable);
    update(zoomIn->rect());
}

void GalleryFrame::setZoomOutEnabled(bool _enable)
{
    auto & zoomOut = d->objects_[ZoomOutButton];
    zoomOut->setDisabled(!_enable);
    update(zoomOut->rect());
}

void GalleryFrame::setSaveEnabled(bool _enable)
{
    auto & save = d->objects_[SaveButton];
    save->setDisabled(!_enable);
    update(save->rect());
}

void GalleryFrame::setMenuEnabled(bool _enable)
{
    auto & menu = d->objects_[MenuButton];
    menu->setDisabled(!_enable);
    update(menu->rect());
}

void GalleryFrame::setGotoEnabled(bool _enable)
{
    d->gotoEnabled_ = _enable;
}

QString GalleryFrame::actionIconPath(Action action)
{
    switch (action)
    {
    case GoTo:
        return qsl(":/context_menu/goto");
    case Copy:
        return qsl(":/context_menu/copy");
    case Forward:
        return qsl(":/context_menu/forward");
    case SaveAs:
        return qsl(":/context_menu/download");
    case SaveToFavorites:
        return qsl(":/context_menu/favorites");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

QString GalleryFrame::actionText(Action action)
{
    switch (action)
    {
    case GoTo:
        return QT_TRANSLATE_NOOP("previewer", "Go to message");
    case Copy:
        return QT_TRANSLATE_NOOP("previewer", "Copy to clipboard");
    case Forward:
        return QT_TRANSLATE_NOOP("previewer", "Forward");
    case SaveAs:
        return QT_TRANSLATE_NOOP("previewer", "Save as");
    case SaveToFavorites:
        return QT_TRANSLATE_NOOP("previewer", "Add to favorites");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

void GalleryFrame::setMenuActions(Actions _actions)
{
    d->menuActions_ = _actions;
}

GalleryFrame::Actions GalleryFrame::menuActions() const
{
    return d->menuActions_;
}

void GalleryFrame::setZoomVisible(bool _visible)
{
    auto & zoomIn = d->objects_[ZoomInButton];
    auto & zoomOut = d->objects_[ZoomOutButton];

    zoomIn->setVisible(_visible);
    zoomOut->setVisible(_visible);

    update(zoomIn->rect());
    update(zoomOut->rect());
}

void GalleryFrame::setNavigationButtonsVisible(bool _visible)
{
    auto & next = d->objects_[NextButton];
    auto & prev = d->objects_[PrevButton];

    next->setVisible(_visible);
    prev->setVisible(_visible);

    update(next->rect());
    update(prev->rect());
}

void GalleryFrame::setCounterText(const QString &_text)
{
    setLabelText(CounterLabel, _text);
}

void GalleryFrame::setAuthorText(const QString &_text)
{
    setLabelText(AuthorLabel, _text);
}

void GalleryFrame::setDateText(const QString &_text)
{
    setLabelText(DateLabel, _text);
}

void GalleryFrame::setCaption(const QString& _caption)
{
    d->setCaption(_caption, width());
    update();
}

void GalleryFrame::setAuthorAndDateVisible(bool _visible)
{
    auto & author = d->objects_[AuthorLabel];
    auto & date = d->objects_[DateLabel];

    author->setVisible(_visible);
    date->setVisible(_visible);

    update(author->rect());
    update(date->rect());
}

void GalleryFrame::collapseCaption()
{
    d->collapseCaption();
    update();
}

bool GalleryFrame::isCaptionExpanded() const
{
    return d->isCaptionExpanded();
}

void GalleryFrame::onNext()
{
    closeMenu();
}

void GalleryFrame::onPrev()
{
    closeMenu();
}

void GalleryFrame::paintEvent(QPaintEvent* _event)
{
    QWidget::paintEvent(_event);
    const auto rect = _event->rect();

    QPainter p(this);

    p.setOpacity(backgroundOpacity);
    p.fillRect(0, 0, width(), height(), backgroundColor);

    for (auto & [_, object] : d->objects_)
    {
        if (object->rect().intersects(rect))
            object->draw(p);
    }
}

void GalleryFrame::mouseMoveEvent(QMouseEvent *_event)
{
    bool updateHover = _event->buttons() == Qt::NoButton;

    bool handCursor = false;
    for (auto & [_, object] : d->objects_)
    {
        const auto overObject = object->rect().contains(_event->pos());
        if (overObject || object->hovered())
        {
            handCursor |= overObject && object->clickable() && (updateHover || object->pressed());

            if (updateHover)
            {
                object->setHovered(overObject);
                update(object->rect());
            }
        }
    }

    setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void GalleryFrame::mousePressEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(_event);

    for (auto & [_, object] : d->objects_)
    {
        if (object->rect().contains(_event->pos()) && object->clickable())
        {
            object->setPressed(true);
            update(object->rect());
        }
    }
}

void GalleryFrame::mouseReleaseEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mouseReleaseEvent(_event);

    for (auto & [type, object] : d->objects_)
    {
        if (!object->clickable())
            continue;

        const auto overObject = object->rect().contains(_event->pos());
        object->setHovered(overObject);

        if (!object->disabled() && object->pressed() && overObject)
            onClick(type);

        object->setPressed(false);

        update(object->rect());
    }
}

bool GalleryFrame::event(QEvent *_event)
{
    return QWidget::event(_event);
}

void GalleryFrame::leaveEvent(QEvent *_event)
{
    d->resetObjects();
    repaint();

    QWidget::leaveEvent(_event);
}

void GalleryFrame::hideEvent(QHideEvent* _event)
{
    if (d->menu_)
        d->menu_->hide();

    return QWidget::hideEvent(_event);
}

void GalleryFrame::onClick(ControlType _type)
{
    switch (_type)
    {
        case AuthorLabel:
            Q_EMIT openContact();
            break;
        case DateLabel:
            Q_EMIT goToMessage();
            break;
        case ZoomOutButton:
            Q_EMIT zoomOut();
            break;
        case ZoomInButton:
            Q_EMIT zoomIn();
            break;
        case PrevButton:
            Q_EMIT prev();
            break;
        case NextButton:
            Q_EMIT next();
            break;
        case CloseButton:
            Q_EMIT close();
            break;
        case SaveButton:
            Q_EMIT save();
            break;
        case MenuButton:
        {
            auto buttonRect = d->objects_[MenuButton]->rect();
            auto pos = QPoint(buttonRect.x() + buttonRect.width() / 2, buttonRect.y());

            showMenuAt(mapToGlobal(pos));

            d->resetObjects();
        }
        break;

        default:
            break;
    }
}

void GalleryFrame::setLabelText(ControlType _type, const QString &_text)
{
    if (auto object = d->objects_[_type].get())
    {
        auto label = dynamic_cast<Label*>(object);
        label->setText(_text);
        update(label->rect());
    }
}

CustomMenu* GalleryFrame::createMenu()
{
    auto menu = new CustomMenu();

    if ((d->menuActions_ & GoTo) && d->gotoEnabled_)
        addAction(GoTo, menu, this, &GalleryFrame::goToMessage);

    if (d->menuActions_ & Copy)
        addAction(Copy, menu, this, &GalleryFrame::copy);

    if (d->menuActions_ & Forward)
        addAction(Forward, menu, this, &GalleryFrame::forward);

    if (d->menuActions_ & SaveAs)
        addAction(SaveAs, menu, this, &GalleryFrame::saveAs);

    if (d->menuActions_ & SaveToFavorites)
        addAction(SaveToFavorites, menu, this, &GalleryFrame::saveToFavorites);

    return menu;
}
template<typename ...Args>
QAction* GalleryFrame::makeAction(Action action, CustomMenu* parent, Args&& ...args)
{
    auto a = new QAction(actionText(action), parent);
    connect(a, &QAction::triggered, std::forward<Args>(args)...);
    return a;
}

template<typename ...Args>
void GalleryFrame::addAction(Action action, CustomMenu* parent, Args && ...args)
{
    const auto iconSize = Utils::scale_value(QSize(20, 20));
    const auto iconColor = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY);
    auto a = makeAction(action, parent, std::forward<Args>(args)...);
    parent->addAction(a, Utils::renderSvg(actionIconPath(action), iconSize, iconColor));
}

void GalleryFrame::showMenuAt(const QPoint& _pos)
{
    if (d->menu_)
        d->menu_->deleteLater();

    d->menu_ = createMenu();
    d->menu_->showAtPos(_pos);
}

void GalleryFrame::closeMenu()
{
    if (d->menu_)
        d->menu_->close();
}

class MenuItem : public BDrawable
{
public:
    virtual ~MenuItem() = default;
    void draw(QPainter& _p) override;
    void draw(QPainter& _p, Icon* _checkMark);

    Label label_;
    Icon icon_;
};

void MenuItem::draw(QPainter& _p)
{
    BDrawable::draw(_p);

    icon_.draw(_p);
    label_.draw(_p);
}

void MenuItem::draw(QPainter& _p, Icon* _checkMark)
{
    MenuItem::draw(_p);
    if (_checkMark)
        _checkMark->draw(_p);
}

class CustomMenu_p
{
public:
    CustomMenu* q;
    QFont font_;
    Icon checkMark_;
    int itemHeight_;
    QSize arrowSize_;
    bool needUpdate_;
    QColor fontColor_;
    QColor hoverColor_;
    QColor pressedColor_;
    QColor backgroundColor_;
    CustomMenu::Features features_;

    using ItemData = std::pair<QAction*, std::unique_ptr<MenuItem>>;

    std::vector<ItemData> items_;

    CustomMenu_p(CustomMenu* _q)
        : q(_q)
    {}

    int updateLabels(int _width)
    {
        const auto xOffsetLeft = Utils::scale_value(12);
        const auto xOffsetRight = Utils::scale_value(12);

        auto fm = QFontMetrics(font_);
        auto maxTextWidth = 0;
        auto maxTotalWidth = _width;
        for (auto & [action, menuItem] : items_)
            maxTextWidth = std::max(maxTextWidth, fm.horizontalAdvance(action->text()));

        const auto height = fm.height();

        for (auto i = 0u; i < items_.size(); ++i)
        {
            auto & label = items_[i].second->label_;
            auto & icon = items_[i].second->icon_;
            label.setXOffset(xOffsetLeft);
            label.setYOffset((itemHeight_ - height) / 2);

            icon.setXOffset(xOffsetLeft);
            icon.setYOffset((itemHeight_ - icon.pixmapSize().height()) / 2); // center icon by height

            icon.setRect(icon.isNullPixmap() ? QRect() : QRect(0, i * itemHeight_, xOffsetLeft + icon.pixmapSize().width(), itemHeight_));
            label.setRect(QRect(icon.rect().width(), i * itemHeight_, maxTextWidth + xOffsetLeft + xOffsetRight, itemHeight_));

            const auto itemWidth = std::max(icon.rect().width() + label.rect().width(), q->minimumWidth());
            items_[i].second->setRect(QRect(0, i * itemHeight_, itemWidth, itemHeight_));
            maxTotalWidth = std::max(maxTotalWidth, itemWidth);
        }

        needUpdate_ = false;

        return maxTotalWidth;
    }

    auto rectForUpdate(int itemIdx)
    {
        if (itemIdx >= static_cast<int>(items_.size()))
        {
            im_assert(!"wrong item index");
            return QRect();
        }

        auto updateRect = items_[itemIdx].second->rect();

        if (itemIdx == static_cast<int>(items_.size()) - 1) // last item updates with arrow
            updateRect.setBottom(updateRect.bottom() + arrowSize_.height());

        return updateRect;
    }

    void resetLabelsState()
    {
        for (auto & [_, label] : items_)
        {
            label->setHovered(false);
            label->setPressed(false);
        }
    }

    QPoint preferredMenuPoint(const QRect& _r, Qt::Alignment _align) const
    {
        const QSize s = q->sizeHint();
        QPoint p = _r.bottomLeft();
        for (int i = 1; i < Qt::AlignBaseline; i <<= 1)
        {
            if (!(i & _align))
                continue;

            switch (i)
            {
            case Qt::AlignTop:
                p.ry() = _r.top() - s.height();
                break;
            case Qt::AlignBottom:
                p.ry() = _r.bottom();
                break;
            case Qt::AlignLeft:
                p.rx() = _r.left() - s.width();
                break;
            case Qt::AlignRight:
                p.rx() = _r.right() - s.width();
                break;
            case Qt::AlignVCenter:
                p.ry() = _r.center().y() - s.height() / 2;
                break;
            case Qt::AlignHCenter:
                p.rx() = _r.center().x() - s.width() / 2;
                break;
            default:
                break;
            }
        }
        return p;
    }

    QPoint screenAdjustment(const QRect& _screenRect, const QRect& _menuRect, const QRect& _targetRect)
    {
        int dx = 0, dy = 0;
        if (_menuRect.left() < _screenRect.left())
            dx = _screenRect.left() - _menuRect.left();
        else if (_menuRect.right() >= _screenRect.right())
            dx = -(_menuRect.right() - _screenRect.right());

        if (_menuRect.top() < _screenRect.top())
            dy = _screenRect.top() - _menuRect.top() + _targetRect.height();
        else if (_menuRect.bottom() >= _screenRect.bottom())
            dy = -(_menuRect.bottom() - _screenRect.bottom() - _targetRect.height());

        return QPoint(dx, dy);
    }

    QRect adjustedToScreen(const QRect& _screenRect, const QRect& _menuRect, const QRect& _targetRect, Qt::Alignment _align)
    {
        if (_screenRect.contains(_menuRect))
            return _menuRect;

        QRect r = _menuRect;

        const QPoint delta = screenAdjustment(_screenRect, _menuRect, _targetRect);
        r.translate(delta);

        if ((features_ & CustomMenu::TargetAware))
        {
            int x = 0, y = 0;
            if (r.contains(_targetRect) || _targetRect.contains(r))
            {
                if (delta.y() > 0)
                    y = _targetRect.bottom();
                else if (delta.y() < 0)
                    y = _targetRect.top();

                if (delta.x() > 0)
                    x = _targetRect.right();
                else if (delta.x() < 0)
                    x = _targetRect.left();
            }
            else if (r.intersects(_targetRect))
            {
                if (r.top() >= _targetRect.top() && r.top() <= _targetRect.bottom())
                {
                    if (delta.y() > 0)
                        y = _targetRect.bottom();
                    else if (delta.y() < 0)
                        y = _targetRect.top();
                }

                if (r.left() >= _targetRect.left() && r.top() <= _targetRect.right())
                {
                    if (delta.x() > 0)
                        x = _targetRect.right();
                    else if (delta.x() < 0)
                        x = _targetRect.left();
                }
            }
            r.moveTo(x == 0 ? r.left() : x, y == 0 ? r.top() : y);
        }

        return r;
    }

    void drawItem(QPainter& _painter, const QAction* _action, const std::unique_ptr<MenuItem>& _item, const QRect& _rect)
    {
        const QRect itemRect = _item->rect();
        if (!itemRect.intersects(_rect))
            return;

        Icon* checkMark = !checkMark_.isNullPixmap() && _action->isCheckable() && _action->isChecked() ? &checkMark_ : nullptr;
        if (checkMark)
        {
            QRect markRect{ QPoint{}, checkMark->pixmapSize() };
            markRect.moveCenter(QPoint(itemRect.right() - Utils::scale_value(12) - markRect.width() / 2, itemRect.center().y()));
            checkMark->setRect(markRect);
        }
        _item->draw(_painter, checkMark);
    }
};

CustomMenu::CustomMenu(QWidget* _parent)
    : QMenu(_parent)
    , d(std::make_unique<CustomMenu_p>(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // set defaults
    d->features_ = NoFeatures;
    d->arrowSize_ = Utils::scale_value(QSize(20, 8));
    d->itemHeight_ = Utils::scale_value(36);
    d->backgroundColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT);
    d->hoverColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER);
    d->fontColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    d->font_ = Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    d->needUpdate_ = false;

    setMouseTracking(true);
}

CustomMenu::~CustomMenu() = default;

void CustomMenu::setFeatures(CustomMenu::Features _features)
{
    d->features_ = _features;
    if ((d->features_ & TargetAware) && !(d->features_ & ScreenAware))
        d->features_ |= ScreenAware; // force screen area feature
}

CustomMenu::Features CustomMenu::features() const
{
    return d->features_;
}

void CustomMenu::setArrowSize(const QSize& _size)
{
    if (d->arrowSize_ == _size)
        return;

    d->arrowSize_ = _size;
    update();
}

QSize CustomMenu::arrowSize() const
{
    return d->arrowSize_;
}

void CustomMenu::setCheckMark(const QPixmap& _icon)
{
    d->checkMark_.setPixmap(_icon);
    update();
}

void CustomMenu::addAction(QAction* _action, const QPixmap& _icon)
{
    auto item = std::make_unique<MenuItem>();
    item->background_ = Styling::ColorContainer{ Styling::ColorParameter{ Qt::transparent } };
    item->hoveredBackground_ = Styling::ColorContainer{ Styling::ColorParameter{ d->hoverColor_ } };
    item->pressedBackground_ = Styling::ColorContainer{ Styling::ColorParameter{ d->pressedColor_ } };

    auto textUnit = TextRendering::MakeTextUnit(_action->text());
    TextRendering::TextUnit::InitializeParameters params{ d->font_, Styling::ColorParameter{ d->fontColor_ } };
    params.maxLinesCount_ = 1;
    textUnit->init(params);
    item->label_.setTextUnit(std::move(textUnit));

    item->icon_.setPixmap(_icon);

    d->items_.push_back(std::make_pair(_action, std::move(item)));

    d->needUpdate_ = true;
    updateHeight();
}

void CustomMenu::clear()
{
    d->items_.clear();
    d->updateLabels(0);
    update();
}

void CustomMenu::showAtPos(const QPoint &_pos)
{
    updateHeight();
    setFixedWidth(d->updateLabels(0));

    const auto arrowPeak = QPoint(width() / 2,  height() + d->arrowSize_.height());
    move(_pos.x() - arrowPeak.x(), _pos.y() - arrowPeak.y());

    d->resetLabelsState();

#ifdef __APPLE__
    MacSupport::showOverAll(this);
#else
    show();
#endif
}

void CustomMenu::popupMenu(const QPoint& _globalPos, QScreen* _screen, const QRect& _targetRect, Qt::Alignment _align)
{
    d->resetLabelsState();

    if (d->features_ & DefaultPopup) // let Qt to decide
    {
        QMenu::exec(_globalPos);
        return;
    }

    QPoint popupPos = _globalPos;
    if (d->features_ & ScreenAware)
    {
        im_assert(_screen);
        if (!_screen)
            _screen = qApp->primaryScreen();

        QRect r = rect();
        r.moveTo(_globalPos);
        // Windows and KDE allow menus to cover the taskbar, while GNOME
        // and macOS don't, but we won't distinguish different Linux distributions.
        if constexpr (platform::is_windows())
            r = d->adjustedToScreen(_screen->geometry(), r, _targetRect, _align);
        else
            r = d->adjustedToScreen(_screen->availableGeometry(), r, _targetRect, _align);

        popupPos = r.topLeft();
    }

    Q_EMIT aboutToShow();
    move(popupPos);
#ifdef __APPLE__
    MacSupport::showOverAll(this);
#else
    show();
#endif
}

void CustomMenu::exec(QWidget* _target, Qt::Alignment _align, const QMargins& _margins)
{
    im_assert(_target);
    if (!_target)
    {
        QMenu::exec();
        return;
    }

    QRect r;
    if (_target->parentWidget() == nullptr) // top-level widget
    {
        r = _target->geometry().marginsAdded(_margins);
    }
    else
    {
        r = _target->rect().marginsAdded(_margins);
        r.moveTo(_target->mapToGlobal(r.topLeft()));
    }
    popupMenu(d->preferredMenuPoint(r, _align), _target->screen(), r, _align);
}

void CustomMenu::exec(QWidget* _target, const QRect& _rect, Qt::Alignment _align)
{
    im_assert(_target);
    if (!_target)
    {
        QMenu::exec();
        return;
    }

    if (!_target->rect().contains(_rect))
        return exec(_target, _align);

    QRect r = _rect;
    r.moveTo(_target->mapToGlobal(r.topLeft()));
    popupMenu(d->preferredMenuPoint(r, _align), _target->screen(), r, _align);
}

QSize CustomMenu::sizeHint() const
{
    QSize sz(d->updateLabels(0), d->items_.size() * d->itemHeight_ + d->arrowSize_.height());
    return sz.expandedTo(minimumSize());
}

void CustomMenu::paintEvent(QPaintEvent *_event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (d->arrowSize_.isNull())
    {
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(d->backgroundColor_);
        p.drawRoundedRect(rect(), getMenuBorderRadius(), getMenuBorderRadius());
        p.restore();
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        for (const auto& [action, label] : d->items_)
            d->drawItem(p, action, label, _event->rect());

        return;
    }

    QPainterPath rectPath;
    rectPath.setFillRule(Qt::WindingFill);

    auto r = rect();
    r.setBottom(r.bottom() - d->arrowSize_.height());

    rectPath.addRoundedRect(r, getMenuBorderRadius(), getMenuBorderRadius());
    p.fillPath(rectPath, QBrush(d->backgroundColor_));                                      //    *********************
                                                                                            //    *                   *
    QPainterPath arrowPath;                                                                 //    *                   *
    arrowPath.setFillRule(Qt::WindingFill);                                                 //    *                   *
                                                                                            //    *                   *
    const auto a = QPoint((r.width() - d->arrowSize_.width()) / 2, r.height());             //    *******A     C*******
    const auto b = QPoint(r.width() / 2,  r.height() + d->arrowSize_.height());             //            *   *
    const auto c = QPoint((r.width() + d->arrowSize_.width()) / 2, r.height());             //              B

    arrowPath.moveTo(a);
    arrowPath.lineTo(b);
    arrowPath.lineTo(c);
    arrowPath.closeSubpath();

    // if last item is highlighted, then highlight arrow too
    QColor arrowColor = d->backgroundColor_;
    if (!d->items_.empty())
    {
        auto & label = d->items_.back().second;
        if (label->pressed() && label->pressedBackground_.isValid())
            arrowColor = label->pressedBackground_.cachedColor();
        else if (label->hovered() && label->hoveredBackground_.isValid())
            arrowColor = label->hoveredBackground_.cachedColor();
    }

    p.fillPath(arrowPath, arrowColor);

    p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    for (const auto& [action, label] : d->items_)
        d->drawItem(p, action, label, _event->rect());
}

void CustomMenu::mouseMoveEvent(QMouseEvent *_event)
{
    if (_event->buttons() != Qt::NoButton)
        return QWidget::mouseMoveEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & label = d->items_[i].second;
        const auto overObject = label->rect().contains(_event->pos());
        if (overObject || label->hovered())
        {
            label->setHovered(overObject);
            update(d->rectForUpdate(i));
        }
    }
}

void CustomMenu::mousePressEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & label = d->items_[i].second;
        if (label->rect().contains(_event->pos()))
        {
            label->setPressed(true);
            update(d->rectForUpdate(i));
        }
    }

    QWidget::mousePressEvent(_event);
}

void CustomMenu::mouseReleaseEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mouseReleaseEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & [action, label] = d->items_[i];
        const auto overObject = label->rect().contains(_event->pos());
        label->setHovered(overObject);

        if (label->pressed() && overObject)
        {
            close();
            action->trigger();
        }

        label->setPressed(false);

        update(d->rectForUpdate(i));
    }
}

void CustomMenu::updateHeight()
{
    setFixedHeight(d->items_.size() * d->itemHeight_ + d->arrowSize_.height());
}

AccessibleGalleryFrame::AccessibleGalleryFrame(GalleryFrame* widget)
    : QAccessibleWidget(widget)
{
    static constexpr const std::array<ControlType, 7> controlTypes{
        ControlType::ZoomOutButton,
        ControlType::ZoomInButton,
        ControlType::PrevButton,
        ControlType::NextButton,
        ControlType::CloseButton,
        ControlType::MenuButton,
        ControlType::SaveButton,
    };

    auto galleryFrame = qobject_cast<GalleryFrame*>(object());
    for (auto control : controlTypes)
    {
        auto drawable = galleryFrame->d->objects_.at(control).get();
        auto controlObject = dynamic_cast<QObject*>(drawable);
        im_assert(controlObject);
        if (controlObject)
        {
            auto accessibleControlObject = QAccessible::queryAccessibleInterface(controlObject);
            im_assert(accessibleControlObject);
            children_.emplace_back(accessibleControlObject);
        }
    }
}

int AccessibleGalleryFrame::childCount() const
{
    return children_.size();
}

QAccessibleInterface* AccessibleGalleryFrame::child(int index) const
{
    if (index > -1 && index < static_cast<int>(children_.size()))
        return children_.at(index);
    return nullptr;
}

int AccessibleGalleryFrame::indexOfChild(const QAccessibleInterface* child) const
{
    return Utils::indexOf(children_.cbegin(), children_.cend(), child);
}

}
