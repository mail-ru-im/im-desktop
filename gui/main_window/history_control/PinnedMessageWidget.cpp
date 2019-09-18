#include "stdafx.h"

#include "PinnedMessageWidget.h"

#include "complex_message/ComplexMessageItemBuilder.h"
#include "complex_message/ComplexMessageItem.h"
#include "complex_message/IItemBlock.h"

#include "../contact_list/ContactListModel.h"

#include "../../core_dispatcher.h"
#include "../../fonts.h"

#include "../../controls/CustomButton.h"
#include "../../controls/TooltipWidget.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/friendly/FriendlyContainer.h"
#include "../../types/message.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/stat_utils.h"
#include "../../utils/ResizePixmapTask.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    constexpr auto fullHeight = 48;
    constexpr auto collapsedHeight = 24;
    constexpr auto leftMargin = 20;
    constexpr auto rightMargin = 12;

    const auto snippetSize = QSize(44, 32);

    constexpr auto simplePinLeftMargin = 16;

    constexpr auto close_button_offset = 16;
    constexpr auto close_button_size = 20;

    constexpr auto offsetTopName = 5;
    constexpr auto offsetTopText = 24;

    constexpr auto tooltipDelay = std::chrono::milliseconds(400);

    const auto pinPath = qsl(":/pin/pin_icon");

    constexpr auto fullSenderVar = Styling::StyleVariable::PRIMARY;

    void drawBackground(QPainter& _painter, const QRect& _rect, const bool _isHovered)
    {
        const auto bg = Styling::getParameters().getColor(_isHovered ? Styling::StyleVariable::BASE_BRIGHT_INVERSE : Styling::StyleVariable::BASE_GLOBALWHITE);
        const auto border = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        Utils::drawBackgroundWithBorders(_painter, _rect, bg, border, Qt::AlignBottom);
    }

    enum class ArrowState
    {
        normal,
        hovered,
        pressed
    };

    QPixmap getArrow(const ArrowState _state)
    {
        QColor color;
        switch (_state)
        {
        case ArrowState::normal:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
            break;

        case ArrowState::hovered:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            break;

        case ArrowState::pressed:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;

        default:
            break;
        }

        return Utils::renderSvgScaled(qsl(":/controls/top_icon"), QSize(16, 16), color);
    }

    QRect getCollapseButtonRect(const QRect& _widgetRect)
    {
        const QSize btnSize(Utils::scale_value(QSize(snippetSize.height(), snippetSize.height())));
        const QPoint btnPos(_widgetRect.width() - btnSize.width() - Utils::scale_value(rightMargin), (_widgetRect.height() - btnSize.height()) / 2);

        return QRect(btnPos, btnSize);
    }

    constexpr auto strangerTextDefaultColor = Styling::StyleVariable::TEXT_PRIMARY;
    constexpr auto strangerTextHoverColor = Styling::StyleVariable::TEXT_PRIMARY_HOVER;
    constexpr auto strangerTextActiveColor = Styling::StyleVariable::TEXT_PRIMARY_ACTIVE;
}

namespace Ui
{
    CollapsableWidget::CollapsableWidget(QWidget* _parent, const QString& _tooltipText)
        : QWidget(_parent)
        , tooltipText_(_tooltipText)
    {
        setMouseTracking(true);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipDelay.count());

        connect(&tooltipTimer_, &QTimer::timeout, this, [this]()
        {
            if (isVisible() && getCollapseButtonRect(rect()).contains(mapFromGlobal(QCursor::pos())))
                showTooltip();
        });
    }

    void CollapsableWidget::mouseMoveEvent(QMouseEvent * _event)
    {
        update();

        const auto btnRect = getCollapseButtonRect(rect());

        if (btnRect.contains(_event->pos()))
        {
            if (!tooltipTimer_.isActive() && !Tooltip::isVisible())
                tooltipTimer_.start();
        }
        else
        {
            hideTooltip();
        }
    }

    void CollapsableWidget::mousePressEvent(QMouseEvent * _event)
    {
        update();
        hideTooltip();
    }

    void CollapsableWidget::paintEvent(QPaintEvent * _event)
    {
        QPainter p(this);

        drawBackground(p, rect(), isHovered());
        drawCollapseButton(p, getArrowDirection());
    }

    void CollapsableWidget::leaveEvent(QEvent*)
    {
        update();
        hideTooltip();
    }

    void CollapsableWidget::hideEvent(QHideEvent *)
    {
        hideTooltip();
    }

    void CollapsableWidget::drawCollapseButton(QPainter& _painter, const ArrowDirection _dir)
    {
        static const QPixmap arrows[][3] =
        {
            { getArrow(ArrowState::normal), getArrow(ArrowState::hovered), getArrow(ArrowState::pressed) },
            { Utils::mirrorPixmapVer(getArrow(ArrowState::normal)), Utils::mirrorPixmapVer(getArrow(ArrowState::hovered)), Utils::mirrorPixmapVer(getArrow(ArrowState::pressed)) }
        };

        const auto btnRect = getCollapseButtonRect(rect());
        const auto mousePos = mapFromGlobal(QCursor::pos());

        const auto& set = _dir == ArrowDirection::up ? arrows[0] : arrows[1];
        const auto& pm = (isHovered() && btnRect.contains(mousePos)) ? ((QApplication::mouseButtons() & Qt::MouseButton::LeftButton) ? set[2] : set[1]) : set[0];

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto x = width() - Utils::scale_value(rightMargin + snippetSize.height()) + (Utils::scale_value(snippetSize.height()) - pm.width() / ratio) / 2;
        const auto y = (height() - pm.height() / ratio) / 2;
        _painter.drawPixmap(x, y, pm);
    }


    void CollapsableWidget::showTooltip()
    {
        const auto btnRect = getCollapseButtonRect(rect());
        Tooltip::show(tooltipText_, QRect(mapToGlobal(btnRect.topLeft()), btnRect.size()), {-1, -1}, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Center);
    }

    void CollapsableWidget::hideTooltip()
    {
        tooltipTimer_.stop();
        Tooltip::hide();
    }


    using namespace ComplexMessage;

    FullPinnedMessage::FullPinnedMessage(QWidget * _parent)
        : CollapsableWidget(_parent, QT_TRANSLATE_NOOP("pin", "Roll"))
        , requestId_(-1)
        , hovered_(false)
        , snippetHovered_(false)
        , snippetPressed_(false)
        , collapsePressed_(false)
    {
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(Utils::scale_value(fullHeight));

        sender_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        sender_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold),
                      Styling::getParameters().getColor(fullSenderVar),
                      QColor(),
                      QColor(),
                      QColor(),
                      TextRendering::HorAligment::LEFT,
                      1);

        text_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    }

    bool FullPinnedMessage::setMessage(Data::MessageBuddySptr _msg)
    {
        if (!_msg || _msg->Id_ == -1 || _msg->IsServiceMessage() || _msg->IsChatEvent())
            return false;

        if (complexMessage_ && complexMessage_->getId() == _msg->Id_ && !(_msg->GetUpdatePatchVersion() > patchVersion_))
            return false;

        clear();

        patchVersion_ = _msg->GetUpdatePatchVersion();
        complexMessage_ = ComplexMessageItemBuilder::makeComplexItem(
            nullptr,
            _msg->Id_,
            _msg->InternalId_,
            QDate(),
            _msg->Prev_,
            _msg->GetText(),
            _msg->AimId_,
            _msg->AimId_,
            Logic::GetFriendlyContainer()->getFriendly(_msg->GetChatSender()),
            _msg->Quotes_,
            _msg->Mentions_,
            _msg->GetSticker(),
            _msg->GetFileSharing(),
            false,
            false,
            false,
            _msg->GetDescription(),
            _msg->GetUrl(), _msg->sharedContact_);

        previewType_ = complexMessage_->getPinPlaceholder();

        switch (previewType_)
        {
        case PinPlaceholderType::Link:
            makeEmptyPreviewSnippet(qsl(":/pin/placeholder_pin_link"));
            break;
        case PinPlaceholderType::Image:
        case PinPlaceholderType::FilesharingImage:
            makeEmptyPreviewSnippet(qsl(":/pin/placeholder_pin_photo"));
            break;
        case PinPlaceholderType::Video:
        case PinPlaceholderType::FilesharingVideo:
            makeEmptyPreviewSnippet(qsl(":/pin/placeholder_pin_video"));
            break;
        case PinPlaceholderType::Sticker:
            makeEmptyPreviewSnippet(qsl(":/pin/placeholder_pin_sticker"));
            break;
        default:
            break;
        }

        if (previewType_ != PinPlaceholderType::None && previewType_ != PinPlaceholderType::Filesharing)
        {
            connections_ =
            {
                connect(complexMessage_.get(), &ComplexMessageItem::pinPreview, this, &FullPinnedMessage::makePreview),
                connect(complexMessage_.get(), &ComplexMessageItem::needUpdateRecentsText, this, &FullPinnedMessage::updateText)
            };
            complexMessage_->requestPinPreview(previewType_);
        }

        auto updateSender = [this](const QString& friendly)
        {
            sender_->setText(friendly);
            sender_->evaluateDesiredSize();
        };

        connections_.push_back(connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, [updateSender, aimId = _msg->GetChatSender()](const QString& _aimid, const QString& _friendly)
        {
            if (aimId == _aimid)
                updateSender(_friendly);
        }));

        updateSender(complexMessage_->getSenderFriendly());
        updateText();

        return true;
    }

    void FullPinnedMessage::paintEvent(QPaintEvent* _event)
    {
        CollapsableWidget::paintEvent(_event);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        if (isSnippetSimple())
            drawSimpleSnippet(p);
        else
            drawMediaSnippet(p);

        sender_->draw(p);
        text_->draw(p);
    }

    void FullPinnedMessage::resizeEvent(QResizeEvent *)
    {
        updateTextOffsets();
    }

    void FullPinnedMessage::mousePressEvent(QMouseEvent* _e)
    {
        if (hovered_ && _e->button() == Qt::LeftButton && complexMessage_)
        {
            snippetPressed_ = !isSnippetSimple() && getMediaRect().contains(_e->pos());
            collapsePressed_ = getCollapseButtonRect(rect()).contains(_e->pos());
        }

        update();
        CollapsableWidget::mousePressEvent(_e);
    }

    void FullPinnedMessage::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::LeftButton && rect().contains(_e->pos()) && complexMessage_)
        {
            const auto collRect = getCollapseButtonRect(rect());
            if (collapsePressed_)
            {
                if (collRect.contains(_e->pos()))
                    emit collapseClicked();
            }
            else if (_e->pos().x() < collRect.left())
            {
                if (snippetPressed_ && Utils::unscale_bitmap(getMediaRect()).contains(_e->pos()) && previewType_ != PinPlaceholderType::Sticker)
                    openMedia();
                else
                    Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), complexMessage_->getId(), true);
            }
        }

        update();
        CollapsableWidget::mouseReleaseEvent(_e);
    }

    void FullPinnedMessage::enterEvent(QEvent* _e)
    {
        hovered_ = true;

        auto onSnippet = Utils::unscale_bitmap(getMediaRect()).contains(mapFromGlobal(QCursor::pos()));
        if (snippetHovered_ != onSnippet)
            snippetHovered_ = onSnippet;

        update();
        CollapsableWidget::enterEvent(_e);
    }

    void FullPinnedMessage::leaveEvent(QEvent* _e)
    {
        hovered_ = false;
        snippetHovered_ = false;
        update();

        CollapsableWidget::leaveEvent(_e);
    }

    void FullPinnedMessage::mouseMoveEvent(QMouseEvent* _event)
    {
        if (hovered_)
        {
            const auto onSnippet = Utils::unscale_bitmap(getMediaRect()).contains(_event->pos());
            if (snippetHovered_ != onSnippet)
                snippetHovered_ = onSnippet;

            update();
        }

        CollapsableWidget::mouseMoveEvent(_event);
    }

    void FullPinnedMessage::hideEvent(QHideEvent* _e)
    {
        hovered_ = false;
        snippetHovered_ = false;

        CollapsableWidget::hideEvent(_e);
    }

    bool FullPinnedMessage::isHovered() const
    {
        return hovered_ && !snippetHovered_;
    }

    CollapsableWidget::ArrowDirection FullPinnedMessage::getArrowDirection() const
    {
        return CollapsableWidget::ArrowDirection::up;
    }

    void FullPinnedMessage::drawSimpleSnippet(QPainter& _painter)
    {
        static const QPixmap pm(Utils::renderSvgScaled(pinPath, QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto x = Utils::scale_value(simplePinLeftMargin);
        const auto y = (height() - pm.height() / ratio) / 2;
        _painter.drawPixmap(x, y, pm);
    }

    void FullPinnedMessage::drawMediaSnippet(QPainter& _painter)
    {
        if (snippet_.isNull())
            return;

        const QRect mediaRect(Utils::unscale_bitmap(getMediaRect()));
        const qreal radius(Utils::scale_value(4));

        Utils::PainterSaver ps(_painter);
        _painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRoundedRect(mediaRect, radius, radius);
        _painter.setClipPath(path);

        _painter.drawPixmap(mediaRect.topLeft(), snippetHovered_ ? snippetHover_ : snippet_);

        if (isPlayable())
        {
            static const QPixmap play(Utils::renderSvgScaled(qsl(":/pin/pin_video"), snippetSize));
            _painter.drawPixmap(mediaRect.topLeft(), play);
        }
        if (snippetHovered_)
        {
            _painter.setPen(Qt::NoPen);
            _painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_TERTIARY));
            _painter.drawRoundedRect(mediaRect, radius, radius);
        }
    }

    void FullPinnedMessage::makeEmptyPreviewSnippet(const QString& _placeholder)
    {
        snippet_ = Utils::renderSvgLayered(_placeholder,
            {
                { qsl("bg"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT) },
                { qsl("icon"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY) },
            });

        snippetHover_ = Utils::renderSvgLayered(_placeholder,
            {
                { qsl("bg"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER) },
                { qsl("icon"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY) },
            });
    }

    void FullPinnedMessage::makePreview(const QPixmap & _image)
    {
        const auto aspectMode = previewType_ == PinPlaceholderType::Sticker
            ? Qt::KeepAspectRatio
            : Qt::KeepAspectRatioByExpanding;

        auto task = new Utils::ResizePixmapTask(_image, getMediaSnippetSize(), aspectMode);
        connect(task, &Utils::ResizePixmapTask::resizedSignal, this, &FullPinnedMessage::setPreview);
        QThreadPool::globalInstance()->start(task);
    }

    void FullPinnedMessage::updateText()
    {
        if (!complexMessage_)
            return;

        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        text_ = TextRendering::MakeTextUnit(complexMessage_->formatRecentsText(), complexMessage_->getMentions(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        text_->init(Fonts::appFontScaled(14, Fonts::FontWeight::Normal),
                    color,
                    QColor(),
                    QColor(),
                    QColor(),
                    TextRendering::HorAligment::LEFT,
                    1);
        text_->markdown(Fonts::appFontScaled(14, Fonts::FontWeight::Normal), color, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        text_->evaluateDesiredSize();

        updateTextOffsets();
        update();
    }

    void FullPinnedMessage::setPreview(const QPixmap& _image)
    {

        if (_image.isNull())
            return;

        snippet_ = QPixmap(getMediaSnippetSize());
        snippetHover_ = QPixmap(getMediaSnippetSize());

        snippet_.fill(Qt::transparent);
        snippetHover_.fill(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER));

        QRect imgRect(_image.rect());
        imgRect.moveCenter(snippet_.rect().center());
        {
            QPainter p(&snippet_);
            p.drawPixmap(imgRect.topLeft(), _image);
        }
        {
            QPainter p(&snippetHover_);
            p.drawPixmap(imgRect.topLeft(), _image);
        }

        Utils::check_pixel_ratio(snippet_);
        Utils::check_pixel_ratio(snippetHover_);

        updateTextOffsets();
        update();
    }

    bool FullPinnedMessage::isSnippetSimple() const
    {
        return snippet_.isNull();
    }

    bool FullPinnedMessage::isImage() const
    {
        return previewType_ == PinPlaceholderType::Image || previewType_ == PinPlaceholderType::FilesharingImage;
    }

    bool FullPinnedMessage::isPlayable() const
    {
        return previewType_ == PinPlaceholderType::Video || previewType_ == PinPlaceholderType::FilesharingVideo;
    }

    int FullPinnedMessage::getTextOffsetLeft() const
    {
        return isSnippetSimple() ? Utils::scale_value(56) : Utils::scale_value(72);
    }

    QSize FullPinnedMessage::getMediaSnippetSize() const
    {
        return Utils::scale_bitmap_with_value(snippetSize);
    }

    QRect FullPinnedMessage::getMediaRect() const
    {
        const QSize mediaSize(getMediaSnippetSize());
        const auto ratio = Utils::scale_bitmap_ratio();
        const QPoint leftTop(Utils::scale_value(leftMargin), (height() - mediaSize.height() / ratio) / 2);

        return QRect(leftTop, mediaSize);
    }

    void FullPinnedMessage::updateTextOffsets()
    {
        const auto offsetLeft = getTextOffsetLeft();

        sender_->setOffsets(offsetLeft, Utils::scale_value(offsetTopName));
        text_->setOffsets(offsetLeft, Utils::scale_value(offsetTopText));

        const auto maxWidth = getCollapseButtonRect(rect()).left() - offsetLeft;
        sender_->elide(maxWidth);
        text_->elide(maxWidth);
    }

    void FullPinnedMessage::clear()
    {
        snippet_ = QPixmap();
        snippetHover_ = QPixmap();
        requestId_ = -1;
        previewType_ = PinPlaceholderType::None;

        for (const auto& c : connections_)
            disconnect(c);
        connections_.clear();

        complexMessage_.reset();
        patchVersion_ = common::tools::patch_version();
    }

    void FullPinnedMessage::updateStyle()
    {
        sender_->setColor(fullSenderVar);
        updateText();

        update();
    }

    void FullPinnedMessage::openMedia()
    {
        if (isSnippetSimple() || !complexMessage_)
            return;

        if (previewType_ == PinPlaceholderType::Link)
        {
            const auto link = complexMessage_->getFirstLink();
            Utils::openUrl(Utils::normalizeLink(QStringRef(&link)));
        }
        else if (isImage() || isPlayable())
        {
            auto mw = Utils::InterConnector::instance().getMainWindow();
            if (mw)
            {
                const auto link = complexMessage_->getFirstLink();
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(Logic::getContactListModel()->selectedContact()) },{ "from", "chat" },{ "media_type", isImage() ? "photo" : "video" } });
                mw->openGallery(Logic::getContactListModel()->selectedContact(), Utils::normalizeLink(QStringRef(&link)).toString(), complexMessage_->getId());
            }
        }
    }

    //----------------------------------------------------------------------
    CollapsedPinnedMessage::CollapsedPinnedMessage(QWidget* _parent)
        : CollapsableWidget(_parent, QT_TRANSLATE_NOOP("pin", "Unroll"))
    {
        setFixedHeight(Utils::scale_value(collapsedHeight));
        setCursor(Qt::PointingHandCursor);
    }

    void CollapsedPinnedMessage::updateStyle()
    {
        update();
    }

    void CollapsedPinnedMessage::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (underMouse() && _event->button() == Qt::LeftButton)
            emit clicked();

        CollapsableWidget::mouseReleaseEvent(_event);
    }

    void CollapsedPinnedMessage::paintEvent(QPaintEvent* _e)
    {
        CollapsableWidget::paintEvent(_e);

        QPainter p(this);

        p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        static auto font(Fonts::appFontScaled(12, Fonts::FontWeight::Normal));
        p.setFont(font);

        Utils::drawText(p, QPoint(Utils::scale_value(leftMargin), height() / 2), Qt::AlignVCenter, QT_TRANSLATE_NOOP("pin", "Pinned message"));
    }

    void CollapsedPinnedMessage::enterEvent(QEvent* _e)
    {
        update();
        CollapsableWidget::enterEvent(_e);
    }

    bool CollapsedPinnedMessage::isHovered() const
    {
        return underMouse();
    }

    CollapsableWidget::ArrowDirection CollapsedPinnedMessage::getArrowDirection() const
    {
        return CollapsableWidget::ArrowDirection::down;
    }

    //----------------------------------------------------------------------
    StrangerPinnedWidget::StrangerPinnedWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(fullHeight));
        setMouseTracking(true);

        const auto iconSize = QSize(close_button_size, close_button_size);
        close_ = new CustomButton(this, qsl(":/controls/close_icon"), iconSize, Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        close_->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
        close_->setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
        close_->setFixedSize(Utils::scale_value(iconSize));

        connect(close_, &CustomButton::clicked, this, &StrangerPinnedWidget::closeClicked);

        block_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("pin", "Block"));
        block_->init(Fonts::appFontScaled(16, Fonts::FontWeight::Normal), Styling::getParameters().getColor(strangerTextDefaultColor));
        block_->evaluateDesiredSize();
    }

    void StrangerPinnedWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        drawBackground(p, rect(), false);
        block_->draw(p);
        QWidget::paintEvent(_event);
    }

    void StrangerPinnedWidget::mousePressEvent(QMouseEvent* _event)
    {
        pressPoint_ = _event->pos();
        if (block_->contains(_event->pos()))
        {
            block_->setColor(strangerTextActiveColor);
            update();
        }
        QWidget::mousePressEvent(_event);
    }

    void StrangerPinnedWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(pressPoint_, _event->pos()))
        {
            if (block_->contains(_event->pos()))
                emit blockClicked();
        }

        if (block_->contains(_event->pos()))
        {
            block_->setColor(strangerTextDefaultColor);
            update();
        }

        QWidget::mouseReleaseEvent(_event);
    }

    void StrangerPinnedWidget::updateStyle()
    {

    }

    void StrangerPinnedWidget::mouseMoveEvent(QMouseEvent* _event)
    {
        if (block_->contains(_event->pos()))
        {
            setCursor(Qt::PointingHandCursor);
            block_->setColor(strangerTextHoverColor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
            block_->setColor(strangerTextDefaultColor);
        }
        update();
        QWidget::mouseMoveEvent(_event);
    }

    void StrangerPinnedWidget::leaveEvent(QEvent* _event)
    {
        setCursor(Qt::ArrowCursor);
        block_->setColor(strangerTextDefaultColor);
        update();

        QWidget::leaveEvent(_event);
    }

    void StrangerPinnedWidget::resizeEvent(QResizeEvent* _event)
    {
        close_->move(width() - Utils::scale_value(close_button_offset + close_button_size), height() / 2 - Utils::scale_value(close_button_size) / 2);
        auto bSize = block_->cachedSize();
        block_->setOffsets(width() / 2 - bSize.width() / 2, height() / 2 - bSize.height() / 2);
        QWidget::resizeEvent(_event);
    }

    //----------------------------------------------------------------------
    PinnedMessageWidget::PinnedMessageWidget(QWidget* _parent)
        : QWidget(_parent)
        , full_(nullptr)
        , collapsed_(nullptr)
        , stranger_(nullptr)
    {
        setLayout(Utils::emptyVLayout());
    }

    bool PinnedMessageWidget::setMessage(Data::MessageBuddySptr _msg)
    {
        createFull();
        return full_->setMessage(_msg);
    }

    void PinnedMessageWidget::clear()
    {
        if (full_)
            full_->clear();
    }

    bool PinnedMessageWidget::isStrangerVisible() const
    {
        return isVisible() && stranger_ && stranger_->isVisible();
    }

    void PinnedMessageWidget::showExpanded()
    {
        hideWidget(stranger_);
        hideWidget(collapsed_);

        createFull();
        full_->update();
        full_->show();

        show();
    }

    void PinnedMessageWidget::showCollapsed()
    {
        hideWidget(stranger_);
        hideWidget(full_);

        createCollapsed();
        collapsed_->update();
        collapsed_->show();

        show();
    }

    void PinnedMessageWidget::showStranger()
    {
        hideWidget(full_);
        hideWidget(collapsed_);

        createStranger();
        stranger_->show();
        show();
    }

    void PinnedMessageWidget::updateStyle()
    {
        if (stranger_)
            stranger_->updateStyle();

        if (full_)
            full_->updateStyle();

        if (collapsed_)
            collapsed_->updateStyle();
    }

    void PinnedMessageWidget::resizeEvent(QResizeEvent*)
    {
        emit resized();
    }

    void PinnedMessageWidget::createStranger()
    {
        if (!stranger_)
        {
            stranger_ = new StrangerPinnedWidget(this);
            Testing::setAccessibleName(stranger_, qsl("AS pinned auth_"));
            layout()->addWidget(stranger_);

            connect(stranger_, &StrangerPinnedWidget::blockClicked, this, &PinnedMessageWidget::strangerBlockClicked);
            connect(stranger_, &StrangerPinnedWidget::closeClicked, this, &PinnedMessageWidget::strangerCloseClicked);
        }
    }

    void PinnedMessageWidget::createFull()
    {
        if (!full_)
        {
            full_ = new FullPinnedMessage(this);
            Testing::setAccessibleName(full_, qsl("AS pinned full_"));
            layout()->addWidget(full_);

            connect(full_, &FullPinnedMessage::collapseClicked, this, &PinnedMessageWidget::showCollapsed);
        }
    }

    void PinnedMessageWidget::createCollapsed()
    {
        if (!collapsed_)
        {
            collapsed_ = new CollapsedPinnedMessage(this);
            Testing::setAccessibleName(collapsed_, qsl("AS pinned collapsed_"));
            layout()->addWidget(collapsed_);

            connect(collapsed_, &CollapsedPinnedMessage::clicked, this, &PinnedMessageWidget::showExpanded);
        }
    }

    void PinnedMessageWidget::hideWidget(QWidget* _widget)
    {
        if (_widget)
            _widget->hide();
    }
}
