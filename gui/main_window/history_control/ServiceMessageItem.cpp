#include "stdafx.h"
#include "ServiceMessageItem.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../../controls/TextUnit.h"
#include "../../fonts.h"

namespace
{
    constexpr std::chrono::milliseconds updateTimeout = std::chrono::minutes(1);

    int getPlateNewHeight() noexcept
    {
        return Utils::scale_value(20);
    }

    const QMargins& getPlateNewMargins() noexcept
    {
        static const auto m = Utils::scale_value(QMargins(52, 8, 52, 0));
        return m;
    }

    int getPlateNewCornerRadius() noexcept
    {
        return Utils::scale_value(4);
    }

    int getDateHeight() noexcept
    {
        return Utils::scale_value(24);
    }

    int getDateBubbleVerMargin(const bool _isFloating) noexcept
    {
        return Utils::scale_value(_isFloating ? 12 : 8);
    }

    int getDateTextHorMargins() noexcept
    {
        return Utils::scale_value(12);
    }
}

namespace Ui
{
    ServiceMessageItem::ServiceMessageItem(QWidget* _parent, const bool _overlay)
        : HistoryControlPageItem(_parent)
        , type_(_overlay ? Type::overlay : Type::date)
        , isFloating_(_overlay)
    {
        Utils::grabTouchWidget(this);
        setMultiselectEnabled(false);

        setDate(QDate());

        auto timer = new QTimer(this);

        QObject::connect(timer, &QTimer::timeout, this, [this]()
        {
            if (lastUpdated_.isValid() && date_.isValid())
            {
                if (QDate::currentDate() != lastUpdated_)
                {
                    updateDateTranslation();
                    update();
                }
            }
        });

        timer->start(updateTimeout);

        setFixedHeight(getDateHeight() + 2 * getDateBubbleVerMargin(isFloating()));
    }

    ServiceMessageItem::~ServiceMessageItem() = default;

    void ServiceMessageItem::drawDate(QPainter& _p)
    {
        if (!messageUnit_)
            return;

        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);

        QRect bubbleRect(0, 0, 2 * getDateTextHorMargins() + messageUnit_->desiredWidth(), getDateHeight());
        bubbleRect.moveCenter(rect().center());
        if (bubbleRect != bubbleGeometry_)
        {
            const auto radius = bubbleRect.height() / 2;
            bubble_ = QPainterPath();
            bubble_.addRoundedRect(bubbleRect, radius, radius);
            bubbleGeometry_ = bubbleRect;
        }

        Utils::drawBubbleShadow(_p, bubble_);
        _p.fillPath(bubble_, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        auto topOffset = height() / 2;
        if constexpr (platform::is_windows())
            topOffset -= Utils::scale_value(1);
        messageUnit_->setOffsets((width() - messageUnit_->desiredWidth() + 1) / 2, topOffset);
        messageUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
    }

    void ServiceMessageItem::drawPlateNew(QPainter& _p)
    {
        Utils::PainterSaver ps(_p);
        _p.setPen(Qt::transparent);
        _p.setRenderHint(QPainter::Antialiasing);

        const auto bgColor = Styling::getParameters(getParentAimId()).getColor(Styling::StyleVariable::NEWPLATE_BACKGROUND);
        const auto& margins = getPlateNewMargins();
        auto path = QPainterPath();
        path.addRoundedRect(rect().marginsRemoved(margins), getPlateNewCornerRadius(), getPlateNewCornerRadius());
        _p.fillPath(path, bgColor);

        if (messageUnit_)
        {
            messageUnit_->setOffsets((width() - messageUnit_->desiredWidth() + 1) / 2, (margins.top() + height()) / 2);
            messageUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void ServiceMessageItem::drawOverlay(QPainter& _p)
    {
        im_assert(false);
    }

    QFont ServiceMessageItem::getFont()
    {
        if (platform::is_apple() && type_ == Type::date)
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::Normal, Fonts::FontAdjust::NoAdjust);
        else
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::SemiBold, Fonts::FontAdjust::NoAdjust);
    }

    Styling::ThemeColorKey ServiceMessageItem::getColor()
    {
        switch (type_)
        {
        case Type::date:
        case Type::overlay:
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };

        case Type::plateNew:
            im_assert(!getContact().isEmpty());
            return Styling::ThemeColorKey{ Styling::StyleVariable::NEWPLATE_TEXT, getParentAimId() };

        default:
            break;
        }

        im_assert(!"invalid type!");
        return {};
    }

    void ServiceMessageItem::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        switch (type_)
        {
        case Type::date:
        case Type::overlay:
            drawDate(p);
            break;
        case Type::plateNew:
            drawPlateNew(p);
            break;
        default:
            break;
        }

        HistoryControlPageItem::paintEvent(_event);
    }

    QString ServiceMessageItem::formatRecentsText() const
    {
        return message_;
    }

    MediaType ServiceMessageItem::getMediaType(MediaRequestMode) const
    {
        return MediaType::noMedia;
    }

    void ServiceMessageItem::setMessage(const QString& _message)
    {
        if (message_ == _message)
            return;

        if (!messageUnit_)
        {
            im_assert(!_message.isEmpty());

            messageUnit_ = TextRendering::MakeTextUnit(_message);
            messageUnit_->init({ getFont(), getColor() });
        }

        messageUnit_->setText(_message);
        messageUnit_->evaluateDesiredSize();

        message_ = _message;
    }

    void ServiceMessageItem::setDate(const QDate& _date)
    {
        if (date_ != _date)
        {
            date_ = _date;
            type_ = Type::date;
            setFixedHeight(getDateHeight() + 2 * getDateBubbleVerMargin(isFloating()));
            updateDateTranslation();
        }
    }

    void ServiceMessageItem::updateDateTranslation()
    {
        lastUpdated_ = QDate::currentDate();

        const auto sameYear = lastUpdated_.year() == date_.year();
        const auto daysToNow = date_.daysTo(lastUpdated_);

        QString message;

        if (daysToNow == 0)
            message = QT_TRANSLATE_NOOP("chat_page", "Today");
        else if (daysToNow == 1)
            message = QT_TRANSLATE_NOOP("chat_page", "Yesterday");
        else
            message = Utils::GetTranslator()->formatDate(date_, sameYear);

        setMessage(message);
    }

    void ServiceMessageItem::setNew()
    {
        if (type_ != Type::overlay)
        {
            type_ = Type::plateNew;
            setFixedHeight(getPlateNewHeight() + getPlateNewMargins().top() + getPlateNewMargins().bottom());
        }

        setMessage(QT_TRANSLATE_NOOP("chat_page", "New messages"));
    }

    void ServiceMessageItem::setWidth(int _width)
    {
        setFixedWidth(_width);
    }

    bool ServiceMessageItem::isNew() const
    {
        return (type_ == Type::plateNew);
    }

    bool ServiceMessageItem::isDate() const
    {
        return (type_ == Type::date);
    }

    bool ServiceMessageItem::isOverlay() const
    {
        return (type_ == Type::overlay);
    }

    bool ServiceMessageItem::isFloating() const
    {
        return isFloating_;
    }

    void ServiceMessageItem::updateFonts()
    {
        if (messageUnit_)
        {
            messageUnit_->init({ getFont(), getColor() });
            messageUnit_->evaluateDesiredSize();
            update();
        }
    }
}
