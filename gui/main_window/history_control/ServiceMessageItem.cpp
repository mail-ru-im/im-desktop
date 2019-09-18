#include "stdafx.h"
#include "ServiceMessageItem.h"
#include "MessageStyle.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../../controls/TextUnit.h"
#include "../../fonts.h"
#include "../mediatype.h"

namespace
{
    constexpr std::chrono::milliseconds updateTimeout = std::chrono::minutes(1);

    int getPlateNewHeight()
    {
        return Utils::scale_value(24);
    }

    const QMargins& getPlateNewMargins()
    {
        static const QMargins m(
            Utils::scale_value(1),
            Utils::scale_value(8),
            Utils::scale_value(1),
            0);

        return m;
    }

    int getDateHeight()
    {
        return Utils::scale_value(24);
    }

    int getDateBubbleVerMargin(const bool _isFloating)
    {
        return Utils::scale_value(_isFloating ? 12 : 8);
    }

    int getDateTextHorMargins()
    {
        return Utils::scale_value(12);
    }

    int getDateShadowWidth()
    {
        return Utils::scale_value(4);
    }

    qreal getDateShadowRadius()
    {
        return Utils::fscale_value(14.0);
    }

    qreal getDateRadius()
    {
        return Utils::fscale_value(10.0);
    }
}

namespace Ui
{
    ServiceMessageItem::ServiceMessageItem(QWidget* _parent, const bool _overlay)
        : MessageItemBase(_parent)
        , type_(_overlay ? ServiceMessageItem::type::typeOverlay : ServiceMessageItem::type::typeDate)
        , isFloating_(_overlay)
    {
        Utils::grabTouchWidget(this);

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

        timer->start(updateTimeout.count());

        setFixedHeight(getDateHeight() + 2 * getDateBubbleVerMargin(isFloating()));
    }

    ServiceMessageItem::~ServiceMessageItem()
    {
    }

    void ServiceMessageItem::setQuoteSelection()
    {
        /// TODO-quote
        assert(0);
    }


    void ServiceMessageItem::drawDate(QPainter& _p)
    {
        if (!messageUnit_)
            return;

        Utils::PainterSaver ps(_p);

        QRect bubbleRect(0, 0, 2 * getDateTextHorMargins() + messageUnit_->desiredWidth(), getDateHeight());
        bubbleRect.moveCenter(rect().center());

        if (bubbleRect != bubbleGeometry_)
        {
            const auto fill_r = bubbleRect.height() / 2;

            QPainterPath path;
            path.addRoundedRect(bubbleRect, fill_r, fill_r);

            bubble_ = path;
            bubbleGeometry_ = bubbleRect;
        }

        _p.setRenderHint(QPainter::Antialiasing);

        Utils::drawBubbleShadow(_p, bubble_);
        _p.fillPath(bubble_, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        messageUnit_->setOffsets((width() - messageUnit_->desiredWidth() + 1) / 2, height() / 2);
        messageUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
    }

    void ServiceMessageItem::drawPlateNew(QPainter& _p)
    {
        Utils::PainterSaver ps(_p);

        _p.setPen(Qt::transparent);
        _p.setRenderHint(QPainter::Antialiasing);

        const QColor bgColor = Styling::getParameters(getContact()).getColor(Styling::StyleVariable::NEWPLATE_BACKGROUND);

        const auto& margins = getPlateNewMargins();
        const QRect bgRect(rect().marginsRemoved(margins));
        _p.fillRect(bgRect, bgColor);

        if (messageUnit_)
        {
            messageUnit_->setOffsets((width() - messageUnit_->desiredWidth() + 1) / 2, (margins.top() + height()) / 2);
            messageUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void ServiceMessageItem::drawOverlay(QPainter& _p)
    {
        assert(false);
    }

    QFont ServiceMessageItem::getFont()
    {
        if (platform::is_apple() && type_ == type::typeDate)
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::Normal);
        else
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::SemiBold);
    }

    QColor ServiceMessageItem::getColor()
    {
        assert(!getContact().isEmpty());

        switch (type_)
        {
        case ServiceMessageItem::type::typeDate:
        case ServiceMessageItem::type::typeOverlay:
            return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
            break;
        case ServiceMessageItem::type::typePlateNew:
            return Styling::getParameters(getContact()).getColor(Styling::StyleVariable::NEWPLATE_TEXT);
            break;
        default:
            break;
        }

        assert(!"invalid type!");
        return QColor();
    }

    void ServiceMessageItem::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        switch (type_)
        {
        case ServiceMessageItem::type::typeDate:
        case ServiceMessageItem::type::typeOverlay:
            drawDate(p);
            break;
        case ServiceMessageItem::type::typePlateNew:
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

    MediaType ServiceMessageItem::getMediaType() const
    {
        return MediaType::noMedia;
    }

    void ServiceMessageItem::setMessage(const QString& _message)
    {
        if (message_ == _message)
            return;

        if (!messageUnit_)
        {
            assert(!_message.isEmpty());

            messageUnit_ = TextRendering::MakeTextUnit(_message);
            messageUnit_->init(getFont(), getColor());
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

            type_ = ServiceMessageItem::type::typeDate;

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
        if (type_ != ServiceMessageItem::type::typeOverlay)
        {
            type_ = ServiceMessageItem::type::typePlateNew;

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
        return (type_ == ServiceMessageItem::type::typePlateNew);
    }

    bool ServiceMessageItem::isFloating() const
    {
        return isFloating_;
    }

    void ServiceMessageItem::updateStyle()
    {
        if (messageUnit_)
        {
            messageUnit_->setColor(getColor());
            update();
        }
    }

    void ServiceMessageItem::updateFonts()
    {
        if (messageUnit_)
        {
            messageUnit_->init(getFont(), getColor());
            messageUnit_->evaluateDesiredSize();
            update();
        }
    }
}
