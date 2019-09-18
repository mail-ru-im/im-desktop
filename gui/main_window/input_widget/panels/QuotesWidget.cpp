#include "stdafx.h"

#include "QuotesWidget.h"

#include "fonts.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/friendly/FriendlyContainer.h"
#include "cache/avatars/AvatarStorage.h"
#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "controls/CustomButton.h"
#include "utils/utils.h"

namespace
{
    int verticalMargin()
    {
        return Utils::scale_value(2);
    }

    int verticalExtend()
    {
        return Utils::scale_value(4);
    }

    int rowOffset()
    {
        return Utils::scale_value(6);
    }

    int maxQuotesHeight()
    {
        const auto maxCount = 3.5;
        return maxCount * Ui::getQuoteRowHeight() + (rowOffset() * (int(maxCount) - 1)) + 2 * (rowOffset() - verticalMargin());
    }

    int avatarSize()
    {
        return Utils::scale_value(16);
    }

    int getAvatarTextOffset()
    {
        return Utils::scale_value(8);
    }

    QString textByMediaType(Ui::MediaType _type)
    {
        switch (_type)
        {
            case Ui::MediaType::mediaTypeSticker:
                return QT_TRANSLATE_NOOP("contact_list", "Sticker");

            case Ui::MediaType::mediaTypeVideo:
                return QT_TRANSLATE_NOOP("contact_list", "Video");

            case Ui::MediaType::mediaTypePhoto:
                return QT_TRANSLATE_NOOP("contact_list", "Photo");

            case Ui::MediaType::mediaTypeGif:
                return QT_TRANSLATE_NOOP("contact_list", "GIF");

            case Ui::MediaType::mediaTypePtt:
                return QT_TRANSLATE_NOOP("contact_list", "Voice message");

            case Ui::MediaType::mediaTypeFileSharing:
                return QT_TRANSLATE_NOOP("contact_list", "File");

            case Ui::MediaType::mediaTypeContact:
                return QT_TRANSLATE_NOOP("contact_list", "Contact");

            default:
                return QString();
        }
    }

    int scrollAddedWidth()
    {
        return Utils::scale_value(12);
    }
}

namespace Ui
{

    QuoteRow::QuoteRow(const Data::Quote& _quote)
        : msgId_(_quote.msgId_)
    {
        auto friendly = Logic::GetFriendlyContainer()->getFriendly2(_quote.senderId_);
        if (friendly.default_)
            aimId_ = _quote.quoterId_;
        else
            aimId_ = _quote.senderId_;

        QString text;
        if (_quote.mediaType_ == Ui::MediaType::noMedia)
            text = Utils::convertMentions(_quote.text_, _quote.mentions_);
        else
            text = textByMediaType(_quote.mediaType_);

        const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);

        textUnit_ = TextRendering::MakeTextUnit(getName() % ql1s(": "), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        textUnit_->init(Fonts::appFontScaled(14, Fonts::FontWeight::Medium), textColor, textColor, textColor, QColor(), TextRendering::HorAligment::LEFT, 1);

        auto quoteText = TextRendering::MakeTextUnit(text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        quoteText->init(Fonts::appFontScaled(14, Fonts::FontWeight::Normal), textColor, textColor, textColor, QColor(), TextRendering::HorAligment::LEFT, 1);
        textUnit_->append(std::move(quoteText));

        const auto horOffset = getInputTextLeftMargin() + avatarSize() + getAvatarTextOffset();
        textUnit_->setOffsets(horOffset, getQuoteRowHeight() / 2);

        updateAvatar();
    }

    void QuoteRow::draw(QPainter& _painter, const QRect& _rect) const
    {
        Utils::PainterSaver ps(_painter);

        const auto avaX = getInputTextLeftMargin();
        const auto avaY = (_rect.height() - avatar_.height() / Utils::scale_bitmap_ratio()) / 2;
        _painter.drawPixmap(avaX, avaY, avatar_);

        textUnit_->draw(_painter, TextRendering::VerPosition::MIDDLE);
    }

    void QuoteRow::elide(const int _width)
    {
        textUnit_->getHeight(_width - textUnit_->horOffset());
    }

    void QuoteRow::updateAvatar()
    {
        bool isDef = false;
        const auto avatarPtr = Logic::GetAvatarStorage()->GetRounded(
            aimId_,
            getName(),
            Utils::scale_bitmap(avatarSize()),
            QString(),
            isDef,
            false,
            false
        );

        if (avatarPtr)
            avatar_ = *avatarPtr;
    }

    QString QuoteRow::getName() const
    {
        return Logic::GetFriendlyContainer()->getFriendly(aimId_);
    }

    QuotesContainer::QuotesContainer(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &QuotesContainer::onAvatarChanged);
        connect(this, &ClickableWidget::hoverChanged, this, &QuotesContainer::onHoverChanged);
        connect(this, &ClickableWidget::pressChanged, this, Utils::QOverload<>::of(&QuotesContainer::update));
        connect(this, &ClickableWidget::clicked, this, &QuotesContainer::onClicked);
    }

    bool QuotesContainer::setQuotes(const Data::QuotesVec& _quotes)
    {
        if (std::equal(rows_.begin(), rows_.end(), _quotes.begin(), _quotes.end(), [](const auto& _r, const auto& _q) { return _r.getMsgId() == _q.msgId_; }))
            return false;

        setUpdatesEnabled(false);

        rows_.clear();
        for (const auto& q : _quotes)
            addQuote(q);

        adjustHeight();

        setUpdatesEnabled(true);

        return true;
    }

    void QuotesContainer::adjustHeight()
    {
        const auto h =
            rows_.size() * getQuoteRowHeight() +
            (rows_.size() - 1) * rowOffset() +
            2 * (rowOffset() - verticalMargin());

        setFixedHeight(h);
        update();
    }

    int QuotesContainer::rowCount() const
    {
        return rows_.size();
    }

    QRect QuotesContainer::getRowRect(const int _idx) const
    {
        assert(_idx >= 0 && _idx < rowCount());

        if (_idx >= 0 && _idx < rowCount())
        {
            const auto topOffset = rowOffset() - verticalMargin();
            const auto rowOffsetTotal = _idx > 0 ? (rowOffset() * _idx - 1) : 0;
            const auto y = topOffset + getQuoteRowHeight() * _idx + rowOffsetTotal;
            return QRect(0, y, rowWidth(), getQuoteRowHeight());
        }
        return QRect();
    }

    void QuotesContainer::updateRowHover()
    {
        hoverRowUnderMouse(mapFromGlobal(QCursor::pos()));
    }

    void QuotesContainer::clear()
    {
        rows_.clear();
    }

    void QuotesContainer::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto visibleRect = visibleRegion().boundingRect();

        const QRect rowRect(0, 0, rowWidth(), getQuoteRowHeight());
        const QRect hoverRect = rowRect.adjusted(0, -verticalExtend(), 0, verticalExtend());

        int y = rowOffset() - verticalMargin();
        p.translate(0, rowOffset() - verticalMargin());

        for (const auto& row : rows_)
        {
            if(visibleRect.intersects(hoverRect.translated(0, y)))
            {
                if (row.isUnderMouse())
                {
                    p.setBrush(isPressed() ? getPopupPressedColor() : getPopupHoveredColor());
                    p.setPen(Qt::NoPen);

                    const auto radius = Utils::scale_value(6);
                    p.drawRoundedRect(hoverRect, radius, radius);
                }

                row.draw(p, rowRect);
            }

            p.translate(0, rowRect.height() + rowOffset());
            y += rowRect.height() + rowOffset();
        }
    }

    void QuotesContainer::resizeEvent(QResizeEvent* _e)
    {
        if (width() != _e->oldSize().width())
        {
            for (auto& row : rows_)
                row.elide(rowWidth());
            update();
        }
    }

    void QuotesContainer::mouseMoveEvent(QMouseEvent* _e)
    {
        ClickableWidget::mouseMoveEvent(_e);
        hoverRowUnderMouse(_e->pos());
    }

    void QuotesContainer::wheelEvent(QWheelEvent* _e)
    {
        ClickableWidget::wheelEvent(_e);
        hoverRowUnderMouse(_e->pos());
    }

    void QuotesContainer::onClicked()
    {
        const auto it = std::find_if(rows_.begin(), rows_.end(), [](const auto& _row) { return _row.isUnderMouse(); });
        if (it != rows_.end())
        {
            if (const auto msgId = it->getMsgId(); msgId != -1)
            {
                Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), msgId, true);
                emit rowClicked(std::distance(rows_.begin(), it), QPrivateSignal());
            }
        }
    }

    void QuotesContainer::onHoverChanged(const bool _isHovered)
    {
        if (!_isHovered)
        {
            for (auto& row : rows_)
                row.setUnderMouse(false);
            update();
        }
    }

    void QuotesContainer::onAvatarChanged(const QString& _aimid)
    {
        for (auto& row : rows_)
        {
            if (row.getAimId() == _aimid)
                row.updateAvatar();
        }
    }

    void QuotesContainer::addQuote(const Data::Quote & _quote)
    {
        rows_.emplace_back(_quote);
        rows_.back().elide(rowWidth());
    }

    int QuotesContainer::rowWidth() const
    {
        return width() - scrollAddedWidth();
    }

    void QuotesContainer::hoverRowUnderMouse(const QPoint& _pos)
    {
        int i = 0;
        for (auto& row : rows_)
            row.setUnderMouse(getRowRect(i++).contains(_pos));

        update();
    }






    QuotesWidget::QuotesWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setStyleSheet(qsl("background: transparent; border: none"));
        Testing::setAccessibleName(scrollArea_, qsl("AS inputwidget scrollArea_"));
        Utils::grabTouchWidget(scrollArea_->viewport(), true);

        container_ = new QuotesContainer(this);
        Utils::grabTouchWidget(container_);
        scrollArea_->setWidget(container_);

        auto buttonHost = new QWidget(this);
        Testing::setAccessibleName(buttonHost, qsl("AS inputwidget buttonHost"));
        buttonHost->setFixedWidth(getHorMargin() - scrollAddedWidth());

        buttonCancel_ = new CustomButton(buttonHost, qsl(":/controls/close_icon"), getCancelBtnIconSize());
        Testing::setAccessibleName(buttonCancel_, qsl("AS inputwidget buttonCancel_"));
        updateButtonColors(buttonCancel_, InputStyleMode::Default);
        buttonCancel_->setFixedSize(getCancelButtonSize());

        auto buttonHostLayout = Utils::emptyHLayout(buttonHost);
        buttonHostLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        buttonHostLayout->addSpacing((getHorMargin() - getCancelButtonSize().width()) / 2 - scrollAddedWidth());
        buttonHostLayout->addWidget(buttonCancel_);

        auto vLayout = Utils::emptyVLayout();
        vLayout->addSpacing(verticalMargin());
        vLayout->addWidget(scrollArea_);
        vLayout->addSpacing(verticalMargin());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addSpacing(getHorMargin());
        rootLayout->addLayout(vLayout);
        rootLayout->addWidget(buttonHost);

        connect(buttonCancel_, &CustomButton::clicked, this, [this]()
        {
            emit cancelClicked(QPrivateSignal());
        });
        connect(container_, &QuotesContainer::rowClicked, this, &QuotesWidget::scrollToItem);
        connect(scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, container_, &QuotesContainer::updateRowHover);

        // queued to ensure that all layouts and geometry are already updated
        connect(this, &QuotesWidget::needScrollToLastQuote, this, &QuotesWidget::scrollToLastQuote, Qt::QueuedConnection);
    }

    void QuotesWidget::setQuotes(const Data::QuotesVec& _quotes)
    {
        const auto prevCount = container_->rowCount();
        if (container_->setQuotes(_quotes))
        {
            adjustHeight();
            updateGeometry();

            emit needScrollToLastQuote(QPrivateSignal());
        }

        if (prevCount != container_->rowCount())
            emit quoteCountChanged(QPrivateSignal());
    }

    void QuotesWidget::clearQuotes()
    {
        container_->clear();
    }

    bool QuotesWidget::isMaxHeight() const
    {
        return scrollArea_->height() == maxQuotesHeight();
    }

    void QuotesWidget::updateStyleImpl(const InputStyleMode _mode)
    {
        updateButtonColors(buttonCancel_, _mode);
    }

    void QuotesWidget::adjustHeight()
    {
        const auto curHeight = container_->height();
        const auto maxHeight = maxQuotesHeight();
        const auto viewportHeight = std::clamp(curHeight, 0, maxHeight);

        setFixedHeight(verticalMargin() * 2 + viewportHeight);
    }

    void QuotesWidget::scrollToItem(const int _idx)
    {
        if (auto r = container_->getRowRect(_idx); r.isValid())
            scrollArea_->ensureVisible(0, r.bottom() + 1, 0, r.height() + verticalExtend());
    }

    void QuotesWidget::scrollToLastQuote()
    {
        if (const auto count = container_->rowCount(); count > 0)
            scrollToItem(count - 1);
    }
}
