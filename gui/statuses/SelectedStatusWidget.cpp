#include "stdafx.h"

#include "SelectedStatusWidget.h"
#include "StatusCommonUi.h"
#include "cache/avatars/AvatarStorage.h"
#include "my_info.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "controls/TooltipWidget.h"
#include "controls/CustomButton.h"
#include "styles/ThemeParameters.h"
#include "core_dispatcher.h"
#include "fonts.h"
#include "Status.h"

namespace
{
    constexpr auto animationDuration() noexcept { return std::chrono::milliseconds(150); }

    QSize avatarSize()
    {
        return Utils::scale_value(QSize(68, 68));
    }

    int statusCircleOffset()
    {
        return Utils::scale_value(20);
    }

    int listWidth()
    {
        return Utils::scale_value(360);
    }

    int32_t statusContentSideMargin()
    {
        return Utils::scale_value(20);
    }

    int32_t statusContentWidth()
    {
        return listWidth() - 2 * statusContentSideMargin();
    }

    QColor descriptionFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    constexpr int maxDescriptionLength() noexcept
    {
        return 120;
    }

    void elideDescription(QString& text)
    {
        if (text.size() > maxDescriptionLength())
        {
            const auto ellipsis = Ui::getEllipsis();
            text.truncate(maxDescriptionLength() - ellipsis.size());
            text += ellipsis;
        }
    }

    QFont statusDescriptionFont()
    {
        return Fonts::appFontScaled(17);
    }

    QSize deleteButtonSizeUnscaled()
    {
        return QSize(18, 18);
    }

    QSize deleteButtonSize()
    {
        return Utils::scale_value(deleteButtonSizeUnscaled());
    }

    int borderXMargin()
    {
        return Utils::scale_value(12);
    }

    int borderWidth()
    {
        return Utils::scale_value(1);
    }

    int borderRadius()
    {
        return Utils::scale_value(8);
    }

    int durationIconHMargin()
    {
        return Utils::scale_value(4);
    }

    int durationIconVMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(-1);
        
        return Utils::scale_value(2);
    }

    int durationButtonHeight()
    {
        return Utils::scale_value(20);
    }

    QPoint emojiOffset()
    {
        return Utils::scale_value(QPoint(62, 14));
    }

    QSize emojiSize()
    {
        return Utils::scale_value(QSize(40, 40));
    }

    QPoint editIconOffset()
    {
        return Utils::scale_value(QPoint(48, 48));
    }

    const QColor& circleColor()
    {
        static auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        return color;
    }

    const QColor& borderColor()
    {
        static auto color = []()
        {
            auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
            c.setAlphaF(0.8);
            return c;
        }();
        return color;
    }

    const QColor& durationColor(bool _hovered, bool _pressed)
    {
        if (_pressed)
        {
            static auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE);
            return color;
        }
        else if (_hovered)
        {
            static auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER);
            return color;
        }

        static auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
        return color;
    }

    QSize editIconSize()
    {
        return Utils::scale_value(QSize(16, 16));
    }

    int topSpacerHeight()
    {
        return Utils::scale_value(8);
    }

    int avatarTopSpacerHeight()
    {
        return Utils::scale_value(8);
    }

    const QPixmap& editIcon(bool _hovered, bool _pressed)
    {
        auto icon = [](const QColor& _color){ return Utils::renderSvg(qsl(":/edit_icon"), editIconSize(), _color); };
        if (_pressed)
        {
            static auto pixmap = icon(durationColor(false, true));
            return pixmap;
        }
        else if (_hovered)
        {
            static auto pixmap = icon(durationColor(true, false));
            return pixmap;
        }

        static auto pixmap = icon(durationColor(false, false));
        return pixmap;
    }

    int durationBottomMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(0);

        return Utils::scale_value(2);
    }
}


namespace Statuses
{

using namespace Ui;

//////////////////////////////////////////////////////////////////////////
// SelectedStatusWidget_p
//////////////////////////////////////////////////////////////////////////

class SelectedStatusWidget_p
{
public:
    AvatarWithStatus* avatar_;
    TextWidget* description_ = nullptr;
    QGraphicsOpacityEffect* statusContentOpacity_ = nullptr;
    StatusDurationButton* duration_ = nullptr;
    QWidget* statusContent_ = nullptr;
    QWidget* buttonSpacer_ = nullptr;
    QWidget* topSpacer_ = nullptr;
    QWidget* avatarTopSpacer_ = nullptr;
    CustomButton* deleteButton_ = nullptr;
    QPropertyAnimation* statusContentHeightAnimation_ = nullptr;
    Status status_;
};

//////////////////////////////////////////////////////////////////////////
// SelectedStatusWidget
//////////////////////////////////////////////////////////////////////////

SelectedStatusWidget::SelectedStatusWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<SelectedStatusWidget_p>())
{
    auto layout = Utils::emptyVLayout(this);
    layout->setContentsMargins(statusContentSideMargin(), 0, statusContentSideMargin(), 0);

    auto topHLayout = Utils::emptyHLayout();
    d->topSpacer_ = new QWidget(this);
    d->topSpacer_->setFixedHeight(topSpacerHeight());
    d->avatarTopSpacer_ = new QWidget(this);
    d->avatarTopSpacer_->setFixedHeight(avatarTopSpacerHeight());
    d->avatar_ = new AvatarWithStatus(this);
    Testing::setAccessibleName(d->avatar_, qsl("AS SelectedStatusWidget avatarWithStatus"));
    d->deleteButton_ = new CustomButton(this,qsl(":/controls/close_icon"), deleteButtonSizeUnscaled());
    Testing::setAccessibleName(d->deleteButton_, qsl("AS SelectedStatusWidget deleteButton"));
    d->buttonSpacer_ = new QWidget(this);
    d->buttonSpacer_->setFixedSize(deleteButtonSize());

    auto avatarLayout = Utils::emptyVLayout();
    avatarLayout->addWidget(d->avatarTopSpacer_);
    avatarLayout->addWidget(d->avatar_);

    topHLayout->addWidget(d->buttonSpacer_);
    topHLayout->addStretch();
    topHLayout->addLayout(avatarLayout);
    topHLayout->addStretch();
    topHLayout->addWidget(d->deleteButton_, 0, Qt::AlignTop);

    d->statusContent_ = new QWidget(this);
    QVBoxLayout* statusContentLayout = Utils::emptyVLayout(d->statusContent_);

    d->description_ = new TextWidget(this, QString());

    Testing::setAccessibleName(d->description_, qsl("AS SelectStatusWidget statusDescription"));
    d->description_->init(statusDescriptionFont(), descriptionFontColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    d->description_->setMaxWidthAndResize(statusContentWidth());
    d->duration_ = new StatusDurationButton(this);
    Testing::setAccessibleName(d->duration_, qsl("AS SelectStatusWidget statusDuration"));
    d->duration_->setMaxWidth(statusContentWidth());

    connect(d->duration_, &StatusDurationButton::clicked, this, [this](){ Q_EMIT durationClicked(QPrivateSignal()); });
    connect(d->avatar_, &AvatarWithStatus::avatarClicked, this, [this](){ Q_EMIT avatarClicked(QPrivateSignal()); });
    connect(d->avatar_, &AvatarWithStatus::statusClicked, this, [this](){ Q_EMIT statusClicked(QPrivateSignal()); });

    statusContentLayout->addSpacing(Utils::scale_value(8));
    statusContentLayout->addWidget(d->description_);
    statusContentLayout->addSpacing(Utils::scale_value(2));
    statusContentLayout->addWidget(d->duration_, 0, Qt::AlignHCenter);
    statusContentLayout->addSpacing(durationBottomMargin());

    layout->addWidget(d->topSpacer_);
    layout->addLayout(topHLayout);
    layout->addWidget(d->statusContent_);
    layout->addSpacing(Utils::scale_value(4));

    connect(d->deleteButton_, &CustomButton::clicked, this, []()
    {
        GetDispatcher()->setStatus(QString(), 0);
    });
}

SelectedStatusWidget::~SelectedStatusWidget() = default;

void SelectedStatusWidget::setStatus(const Status& _status)
{
    const auto emptyStatus = _status.isEmpty();
    auto description = !emptyStatus ? _status.getDescription() : QString();
    elideDescription(description);

    d->description_->setText(description);
    d->duration_->setText(!emptyStatus ? _status.getTimeString() : QString());
    d->statusContent_->setVisible(!emptyStatus);
    d->topSpacer_->setVisible(!emptyStatus);
    d->avatarTopSpacer_->setVisible(!emptyStatus);

    d->status_ = _status;

    d->deleteButton_->setVisible(!emptyStatus);
    d->buttonSpacer_->setVisible(!emptyStatus);

    d->avatar_->setStatus(_status);

    updateGeometry();

    update();
}

void SelectedStatusWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    if (!d->status_.isEmpty())
    {
        p.setPen(QPen(borderColor(), borderWidth()));
        p.drawRoundedRect(rect().adjusted(borderXMargin(), 0, -borderXMargin(), -borderWidth()), borderRadius(), borderRadius());
    }

    QWidget::paintEvent(_event);
}

//////////////////////////////////////////////////////////////////////////
// AvatarWithStatus_p
//////////////////////////////////////////////////////////////////////////

class AvatarWithStatus_p
{
public:
    AvatarWithStatus_p(QWidget* _q) : q(_q) {}
    void initAnimation()
    {
        animation_ = new QVariantAnimation(q);

        animation_->setStartValue(0.);
        animation_->setEndValue(1.);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration().count());

        const auto widthDiff = avatarSize().width() - statusCircleOffset();

        auto onValueChanged = [this, widthDiff](const QVariant& value)
        {
            statusOpacity_ = value.toDouble();
            q->setFixedWidth(avatarSize().width() + value.toDouble() * widthDiff);
            q->update();
        };
        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, onValueChanged);
        QObject::connect(animation_, &QVariantAnimation::finished, q, [this]()
        {
            if (animation_->direction() == QVariantAnimation::Backward)
                emoji_ = QImage();
        });
    }

    void showStatusAnimated()
    {
        animation_->setDirection(QVariantAnimation::Forward);
        animation_->start();
    }
    void hideStatusAnimated()
    {
        animation_->setDirection(QVariantAnimation::Backward);
        animation_->start();
    }

    bool statusLoaded_ = false;
    QVariantAnimation* animation_ = nullptr;
    double statusOpacity_ = 0;
    QPoint pressPos_;
    Status status_;
    QImage emoji_;
    QWidget* q;
};

//////////////////////////////////////////////////////////////////////////
// AvatarWithStatus
//////////////////////////////////////////////////////////////////////////

AvatarWithStatus::AvatarWithStatus(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<AvatarWithStatus_p>(this))
{
    setFixedSize(avatarSize());
    setCursor(Qt::PointingHandCursor);
    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &AvatarWithStatus::avatarChanged);
    d->initAnimation();
}

AvatarWithStatus::~AvatarWithStatus() = default;

void AvatarWithStatus::setStatus(const Status& _status)
{
    auto newEmpty = _status.isEmpty();
    auto currentEmpty = d->status_.isEmpty();
    const auto animate = newEmpty != currentEmpty && d->statusLoaded_;

    d->statusLoaded_ = true;
    d->status_ = _status;

    if (!newEmpty)
        d->emoji_ = _status.getImage(emojiSize().width());

    if (animate)
    {
        if (!newEmpty)
            d->showStatusAnimated();
        else
            d->hideStatusAnimated();
    }
    else if (d->animation_->state() != QPropertyAnimation::Running)
    {
        const auto newWidth = avatarSize().width() + (newEmpty ? 0 : (avatarSize().width() - statusCircleOffset()));
        setFixedWidth(newWidth);
        d->statusOpacity_ = newEmpty ? 0 : 1;
        update();
    }
}

void AvatarWithStatus::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto isDefault = false;
    auto avatar = Logic::GetAvatarStorage()->GetRounded(MyInfo()->aimId(), QString(), Utils::scale_bitmap(avatarSize().width()), isDefault, false, false);
    p.drawPixmap(QRect({0, 0}, avatarSize()), avatar);

    if (!d->emoji_.isNull())
    {
        p.setOpacity(d->statusOpacity_);
        QPainterPath circle;
        auto circleRect = QRect({avatarSize().width() - statusCircleOffset(), 0}, avatarSize());
        circle.addEllipse(circleRect);
        p.fillPath(circle, circleColor());
        p.drawImage(QRect(emojiOffset(), emojiSize()), d->emoji_);
        if (Features::isCustomStatusEnabled())
            drawEditIcon(p, circleRect.topLeft() + editIconOffset());
    }
}

void AvatarWithStatus::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        const auto avatarRect = QRect({ 0, 0 }, avatarSize());
        const auto avatarRectClicked = avatarRect.contains(_event->pos()) && avatarRect.contains(d->pressPos_);
        const auto statusRect = QRect({ width() - avatarSize().width(), 0 }, avatarSize());
        const auto statusRectClicked = statusRect.contains(_event->pos()) && statusRect.contains(d->pressPos_);

        if (d->status_.isEmpty() || !statusRectClicked && avatarRectClicked || !Features::isCustomStatusEnabled())
            Q_EMIT avatarClicked(QPrivateSignal());
        else
            Q_EMIT statusClicked(QPrivateSignal());
    }

    d->pressPos_ = QPoint();

    QWidget::mouseReleaseEvent(_event);
}

void AvatarWithStatus::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        d->pressPos_ = _event->pos();

    QWidget::mousePressEvent(_event);
}

void AvatarWithStatus::avatarChanged(const QString& _aimId)
{
    if (_aimId == MyInfo()->aimId())
        update();
}

class StatusDurationButton_p
{
public:
    TextRendering::TextUnitPtr duration_;
    QRect iconRect_;
    int maxWidth_ = 0;
};

StatusDurationButton::StatusDurationButton(QWidget* _parent)
    : ClickableWidget(_parent)
    , d(std::make_unique<StatusDurationButton_p>())
{
    setFixedHeight(durationButtonHeight());

    d->duration_ = TextRendering::MakeTextUnit(QString());
    d->duration_->init(Fonts::appFontScaled(13), durationColor(false, false));

    connect(this, &StatusDurationButton::hoverChanged, this, [this](bool _hovered)
    {
        d->duration_->setColor(durationColor(_hovered, isPressed()));
        update();
    });

    connect(this, &StatusDurationButton::pressChanged, this, [this](bool _pressed)
    {
        d->duration_->setColor(durationColor(isHovered(), _pressed));
        update();
    });
}

StatusDurationButton::~StatusDurationButton() = default;

void StatusDurationButton::setText(const QString& _text)
{
    d->duration_->setText(_text);
    d->duration_->elide(d->maxWidth_ - editIconSize().width());

    d->iconRect_ = QRect({d->duration_->cachedSize().width() + durationIconHMargin(), durationIconVMargin()}, editIconSize());
    setFixedWidth(d->iconRect_.right());
}

void StatusDurationButton::setMaxWidth(int _width)
{
    d->maxWidth_ = _width;
}

void StatusDurationButton::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    d->duration_->draw(p);

    if (!d->duration_->getText().isEmpty())
        p.drawPixmap(d->iconRect_, editIcon(underMouse(), isPressed()));
}


}
