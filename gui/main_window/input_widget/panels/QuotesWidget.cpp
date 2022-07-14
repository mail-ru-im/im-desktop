#include "stdafx.h"

#include "QuotesWidget.h"

#include "fonts.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/FriendlyContainer.h"
#include "cache/avatars/AvatarStorage.h"
#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "controls/CustomButton.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"

namespace
{
    constexpr auto maxVisibleRows = 3.5;

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

    int quotesHeight(double _count)
    {
        return _count * Ui::getQuoteRowHeight() +
            (int(_count) - 1) * rowOffset() +
            2 * (rowOffset() - verticalMargin());
    }

    int maxQuotesHeight()
    {
        return quotesHeight(maxVisibleRows);
    }

    int avatarSize()
    {
        return Utils::scale_value(16);
    }

    int getAvatarTextOffset()
    {
        return Utils::scale_value(8);
    }

    QString plainTextFromQuote(const Data::Quote& _quote)
    {
        switch (_quote.mediaType_)
        {
            case Ui::MediaType::noMedia:
                return Utils::convertFilesPlaceholders(Utils::convertMentions(_quote.text_.string(), _quote.mentions_), _quote.files_);

            case Ui::MediaType::mediaTypeSticker:
                return QT_TRANSLATE_NOOP("contact_list", "Sticker");

            case Ui::MediaType::mediaTypeVideo:
            case Ui::MediaType::mediaTypeFsVideo:
                return QT_TRANSLATE_NOOP("contact_list", "Video");

            case Ui::MediaType::mediaTypePhoto:
            case Ui::MediaType::mediaTypeFsPhoto:
                return QT_TRANSLATE_NOOP("contact_list", "Photo");

            case Ui::MediaType::mediaTypeGif:
            case Ui::MediaType::mediaTypeFsGif:
                return QT_TRANSLATE_NOOP("contact_list", "GIF");

            case Ui::MediaType::mediaTypePtt:
                return QT_TRANSLATE_NOOP("contact_list", "Voice message");

            case Ui::MediaType::mediaTypeFileSharing:
                return QT_TRANSLATE_NOOP("contact_list", "File");

            case Ui::MediaType::mediaTypeContact:
                return QT_TRANSLATE_NOOP("contact_list", "Contact");

            case Ui::MediaType::mediaTypeGeo:
                return QT_TRANSLATE_NOOP("contact_list", "Location");

            case Ui::MediaType::mediaTypePoll:
                return QT_TRANSLATE_NOOP("contact_list", "Poll");

            case Ui::MediaType::mediaTypeTask:
                return QT_TRANSLATE_NOOP("contact_list", "Task");

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
        : sourceMsgId_(_quote.msgId_)
        , sourceChatId_(_quote.chatId_)
        , currentChatId_(_quote.currentChatId_)
        , currentMsgId_(_quote.currentMsgId_)
    {
        const auto friendly = Logic::GetFriendlyContainer()->getFriendly2(_quote.senderId_);
        if (friendly.default_)
            aimId_ = _quote.quoterId_;
        else
            aimId_ = _quote.senderId_;

        const auto textColor = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };

        textUnit_ = TextRendering::MakeTextUnit(getName() % u": ", {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(14, Fonts::FontWeight::Medium), textColor };
        params.linkColor_ = textColor;
        params.selectionColor_ = textColor;
        params.maxLinesCount_ = 1;
        textUnit_->init(params);

        auto quoteText = TextRendering::MakeTextUnit(plainTextFromQuote(_quote), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        params.setFonts(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));
        quoteText->init(params);
        textUnit_->append(std::move(quoteText));

        const auto horOffset = getInputTextLeftMargin() + avatarSize() + getAvatarTextOffset();
        textUnit_->setOffsets(horOffset, getQuoteRowHeight() / 2);

        updateAvatar();
    }

    void QuoteRow::draw(QPainter& _painter, const QRect& _rect) const
    {
        Utils::PainterSaver ps(_painter);

        _painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

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
        avatar_ = Logic::GetAvatarStorage()->GetRounded(
            aimId_,
            getName(),
            Utils::scale_bitmap(avatarSize()),
            isDef,
            false,
            false
        );
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
        connect(this, &ClickableWidget::pressChanged, this, qOverload<>(&QuotesContainer::update));
        connect(this, &ClickableWidget::clicked, this, &QuotesContainer::onClicked);
    }

    bool QuotesContainer::setQuotes(const Data::QuotesVec& _quotes)
    {
        if (std::equal(rows_.begin(), rows_.end(), _quotes.begin(), _quotes.end(), [](const auto& _r, const auto& _q) { return _r.getSourceMsgId() == _q.msgId_; }))
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
        setFixedHeight(quotesHeight(rows_.size()));
        update();
    }

    int QuotesContainer::rowCount() const
    {
        return rows_.size();
    }

    QRect QuotesContainer::getRowRect(const int _idx) const
    {
        im_assert(_idx >= 0 && _idx < rowCount());

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
            if (const auto msgId = it->getCurrentMsgId(); msgId != -1)
            {
                const auto chatId = it->getCurrentChatId();
                if (Logic::getContactListModel()->isThread(chatId))
                {
                    Q_EMIT Utils::InterConnector::instance().openThread(chatId, msgId);
                    Q_EMIT Utils::InterConnector::instance().scrollThreadToMsg(chatId, msgId);
                }
                else
                {
                    Utils::InterConnector::instance().openDialog(chatId, msgId);
                }
                Q_EMIT rowClicked(std::distance(rows_.begin(), it), QPrivateSignal());
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
            {
                row.updateAvatar();
                update();
            }
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
        Testing::setAccessibleName(scrollArea_, qsl("AS Quote scrollArea"));
        Utils::grabTouchWidget(scrollArea_->viewport(), true);

        container_ = new QuotesContainer(this);
        Utils::grabTouchWidget(container_);
        scrollArea_->setWidget(container_);

        auto buttonHost = new QWidget(this);
        Testing::setAccessibleName(buttonHost, qsl("AS Quote cancelButtonHost"));
        buttonHost->setFixedWidth(getHorMargin() - scrollAddedWidth());

        buttonCancel_ = new CustomButton(buttonHost, qsl(":/controls/close_icon"), getCancelBtnIconSize());
        Testing::setAccessibleName(buttonCancel_, qsl("AS Quote cancelButton"));
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
            Q_EMIT cancelClicked(QPrivateSignal());
        });
        connect(container_, &QuotesContainer::rowClicked, this, &QuotesWidget::scrollToItem);
        connect(scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, container_, &QuotesContainer::updateRowHover);

        // queued to ensure that all layouts and geometry are already updated
        connect(this, &QuotesWidget::needScrollToLastQuote, this, &QuotesWidget::scrollToLastQuote, Qt::QueuedConnection);
    }

    void QuotesWidget::setQuotes(const Data::QuotesVec& _quotes)
    {
        const auto prevCount = container_->rowCount();
        if (!container_->setQuotes(_quotes))
            return;

        const auto newCount = container_->rowCount();
        if (newCount > prevCount && quotesHeight(newCount) < quotesHeight(maxVisibleRows + 1))
            Q_EMIT quoteHeightChanged(QPrivateSignal());

        adjustHeight();
        updateGeometry();

        Q_EMIT needScrollToLastQuote(QPrivateSignal());
        if (newCount != prevCount)
            Q_EMIT quoteCountChanged(QPrivateSignal());
    }

    void QuotesWidget::clearQuotes()
    {
        if (container_->rowCount())
            Q_EMIT quoteHeightChanged(QPrivateSignal());
        container_->clear();
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

        if (const auto newHeight = verticalMargin() * 2 + viewportHeight; newHeight != height())
            setFixedHeight(newHeight);
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
