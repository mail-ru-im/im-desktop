#include "stdafx.h"
#include "ThreadRepliesItem.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../../controls/ClickWidget.h"
#include "../../fonts.h"
#include "../../controls/TextWidget.h"

namespace
{
    int bubbleHeight() noexcept { return Utils::scale_value(24); }

    int getThreadRepliesTextToClickableTextGap() noexcept { return Utils::scale_value(4); }

    QString getThreadPlateClickableText(int _numNew, int _numOld)
    {
        if (_numNew > 0)
        {
            return Utils::GetTranslator()
                ->getNumberString(
                    _numNew,
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more new", "1"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more new", "2"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more new", "5"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more new", "21"))
                .arg(_numNew);
        }
        else if (_numOld > 0)
        {
            return Utils::GetTranslator()
                ->getNumberString(_numOld,
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more", "1"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more", "2"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more", "5"),
                    QT_TRANSLATE_NOOP3("threads_feed", "Show %1 more", "21"))
                .arg(_numOld);
        }

        return {};
    }

    QFont getFont()
    {
        return Fonts::adjustedAppFont(12, platform::is_apple() ? Fonts::FontWeight::Normal : Fonts::FontWeight::SemiBold, Fonts::FontAdjust::NoAdjust);
    }

    QString repliesText() { return QT_TRANSLATE_NOOP("threads_feed", "Replies"); }

    QMargins contentMargins() noexcept
    {
        constexpr int bottomMargin = platform::is_windows() ? 1 : 0;
        return Utils::scale_value(QMargins { 16, 0, 16, bottomMargin });
    }

    QMargins rootMargins() noexcept { return Utils::scale_value(QMargins { 0, 8, 0, 8 }); }
} // namespace

namespace Ui
{
    ThreadRepliesItem::ThreadRepliesItem(QWidget* _parent, const QString& _chatAimId)
        : HistoryControlPageItem(_parent)
        , textWidget_ { new TextWidget(this, repliesText(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS) }
        , content_ { new QWidget(this) }
    {
        Utils::grabTouchWidget(this);
        setMultiselectEnabled(false);
        setContact(_chatAimId);
        textWidget_->init({ getFont(), textColorKey() });
        textWidget_->setVerticalPosition(TextRendering::VerPosition::MIDDLE);

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addStretch();
        rootLayout->addWidget(content_);
        rootLayout->addStretch();
        rootLayout->setContentsMargins(rootMargins());

        content_->setFixedHeight(bubbleHeight());
        auto contentLayout = Utils::emptyHLayout(content_);
        contentLayout->addWidget(textWidget_);
        contentLayout->setSpacing(getThreadRepliesTextToClickableTextGap());
        contentLayout->setContentsMargins(contentMargins());
    }

    void ThreadRepliesItem::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(backgroundColor());
        p.setPen(Qt::NoPen);
        const auto radius = content_->height() / 2;
        p.drawRoundedRect(content_->geometry(), radius, radius);

        HistoryControlPageItem::paintEvent(_event);
    }

    QString ThreadRepliesItem::formatRecentsText() const { return repliesText(); }

    MediaType ThreadRepliesItem::getMediaType(MediaRequestMode _mode) const { return MediaType::noMedia; }

    void ThreadRepliesItem::setThreadReplies(int _numNew, int _numOld)
    {
        const auto needShowClickableText = _numNew + _numOld > 0;
        auto message = repliesText();
        if (needShowClickableText)
            message.append(qsl("."));
        textWidget_->setText(message);

        if (!clickableText_ && needShowClickableText)
        {
            clickableText_ = new ClickableTextWidget(this, getFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY }, TextRendering::HorAligment::LEFT);
            connect(clickableText_, &ClickableTextWidget::clicked, this, &ThreadRepliesItem::onMoreClick);
            clickableText_->setFixedHeight(textWidget_->height());
            content_->layout()->addWidget(clickableText_);
        }

        if (clickableText_)
        {
            clickableText_->setVisible(needShowClickableText);
            if (needShowClickableText)
                clickableText_->setText(getThreadPlateClickableText(_numNew, _numOld));
        }
    }

    void ThreadRepliesItem::updateFonts() { textWidget_->init({ getFont(), textColorKey() }); }

    void ThreadRepliesItem::setMessageKey(const Logic::MessageKey& _key)
    {
        Data::MessageBuddy message;
        message.Id_ = _key.getId();
        message.Prev_ = _key.getPrev();
        message.InternalId_ = _key.getInternalId();
        message.SetTime(0);
        message.SetDate(_key.getDate());
        message.SetType(core::message_type::undefined);
        setBuddy(message);
    }

    Styling::ThemeColorKey ThreadRepliesItem::textColorKey() const { return Styling::ThemeColorKey{ Styling::StyleVariable::NEWPLATE_TEXT, getParentAimId() }; }

    QColor ThreadRepliesItem::backgroundColor() const { return Styling::getParameters(getParentAimId()).getColor(Styling::StyleVariable::NEWPLATE_BACKGROUND); }

    void ThreadRepliesItem::onMoreClick() { Q_EMIT clicked(QPrivateSignal {}); }

} // namespace Ui
