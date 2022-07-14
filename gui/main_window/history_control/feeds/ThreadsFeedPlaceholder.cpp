#include "stdafx.h"
#include "ThreadsFeedPlaceholder.h"
#include "controls/TextUnit.h"
#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"
#include "../../contact_list/ServiceContacts.h"
#include "fonts.h"

namespace
{

QFont headerFont()
{
    return Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold);
}
QFont textFont()
{
    return Fonts::appFontScaled(13);
}
QSize iconRectSize()
{
    return Utils::scale_value(QSize(56, 56));
}

const QColor& backgroundColor()
{
    static QColor c = Styling::getParameters(ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed)).getColor(Styling::StyleVariable::CHATEVENT_BACKGROUND);
    return c;
}

constexpr QSize iconSize() noexcept
{
    return QSize(32, 32);
}

QSize iconSizeScaled() noexcept
{
    return Utils::scale_value(iconSize());
}

QPixmap icon()
{
    static auto icon = Utils::StyledPixmap::scaled(qsl(":/thread_icon_filled"), iconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
    return icon.actualPixmap();
}

auto textColor()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::CHATEVENT_TEXT, ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed) };
}

int textMargin() noexcept
{
    return Utils::scale_value(16);
}

int textTopOffset() noexcept
{
    return Utils::scale_value(8);
}

int textRectTopOffset() noexcept
{
    return Utils::scale_value(16);
}

int maxWidth() noexcept
{
    return Utils::scale_value(380);
}

}

namespace Ui
{
    class ThreadsFeedPlaceholder_p
    {
    public:
        Ui::TextRendering::TextUnitPtr header_;
        Ui::TextRendering::TextUnitPtr text_;
        QRect iconRect_;
        QRect textRect_;
    };

    ThreadsFeedPlaceholder::ThreadsFeedPlaceholder(QWidget* _parent)
        : QWidget(_parent),
          d(std::make_unique<ThreadsFeedPlaceholder_p>())
    {
        d->header_ = Ui::TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("threads_feed", "There is no threads"));

        TextRendering::TextUnit::InitializeParameters params{ headerFont(), textColor() };
        params.align_ = TextRendering::HorAligment::CENTER;
        d->header_->init(params);

        d->text_ = Ui::TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("threads_feed", "Threads you subscribed for will be here"));
        params.setFonts(textFont());
        params.color_ = textColor();
        d->text_->init(params);
    }

    ThreadsFeedPlaceholder::~ThreadsFeedPlaceholder() = default;

    void ThreadsFeedPlaceholder::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        QPainterPath path;
        path.addRoundedRect(d->iconRect_, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
        path.addRoundedRect(d->textRect_, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());

        const auto& threadIcon = icon();
        const auto offset = (iconRectSize().width() - iconSizeScaled().width()) / 2;
        p.drawPixmap(d->iconRect_.left() + offset, offset, threadIcon);

        p.fillPath(path, backgroundColor());

        d->header_->draw(p);
        d->text_->draw(p);
    }

    void ThreadsFeedPlaceholder::resizeEvent(QResizeEvent* _event)
    {
        updateGeometry();
    }

    void ThreadsFeedPlaceholder::updateGeometry()
    {
        d->header_->evaluateDesiredSize();
        d->text_->evaluateDesiredSize();
        const auto w = std::min(maxWidth(), std::max(d->header_->desiredWidth(), d->text_->desiredWidth()) + 2 * textMargin());
        const auto headerHeight = d->header_->getHeight(w);
        const auto textHeight = d->text_->getHeight(w);
        d->iconRect_ = QRect({(w - iconRectSize().width()) / 2, 0}, iconRectSize());
        d->textRect_ = QRect(0, d->iconRect_.bottom() + textRectTopOffset(), w, headerHeight + textHeight + 2 * textMargin() + textTopOffset());
        d->header_->setOffsets({0, d->textRect_.top() + textMargin()});
        d->text_->setOffsets({0, d->textRect_.top() + textMargin() + headerHeight + textTopOffset()});
        setFixedHeight(d->textRect_.bottom());
        setFixedWidth(w);
    }


}
