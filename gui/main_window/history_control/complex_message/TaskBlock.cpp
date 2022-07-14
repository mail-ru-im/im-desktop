#include "stdafx.h"

#include "fonts.h"
#include "styles/ThemeParameters.h"

#include "core_dispatcher.h"
#include "controls/TextUnit.h"
#include "controls/textrendering/TextRendering.h"
#include "controls/TooltipWidget.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/async/AsyncTask.h"
#include "styles/ThemesContainer.h"
#include "gui_settings.h"

#include "../MessageStyle.h"
#include "../MessageStatusWidget.h"
#include "app_config.h"

#include "../../../cache/avatars/AvatarStorage.h"
#include "../../containers/FriendlyContainer.h"
#include "../../containers/TaskContainer.h"
#include "main_window/contact_list/ContactListModel.h"

#include "../../tasks/TaskEditWidget.h"
#include "../../../controls/GeneralDialog.h"
#include "../../MainWindow.h"

#include "../../tasks/TaskStatusPopup.h"

#include "ComplexMessageItem.h"
#include "TaskBlock.h"

namespace
{
    using Ui::ComplexMessage::TaskBlock;

    auto maxBlockWidth() noexcept
    {
        return Utils::scale_value(360);
    }

    auto titleTextFont()
    {
        return Fonts::adjustedAppFont(14, Fonts::FontWeight::Medium);
    }

    auto avatarSize() noexcept
    {
        return Utils::scale_value(20);
    }

    auto avatarRightMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto verticalOffset() noexcept
    {
        return Utils::scale_value(8);
    }

    auto noAssigneeText()
    {
        return QT_TRANSLATE_NOOP("task_block", "Unassigned");
    }

    auto noAssigneeIconSize()
    {
        return Utils::scale_value(16);
    }

    enum class MessageType
    {
        Incoming,
        Outgoing
    };

    auto noAssigneeAvatarBackgroundColor(MessageType _messageType)
    {
        const auto styleVariable = MessageType::Incoming == _messageType ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::PRIMARY_BRIGHT;
        return Styling::getParameters().getColor(styleVariable);
    }

    auto noAssigneeAvatarIconColor(MessageType _messageType)
    {
        const auto styleVariable = MessageType::Incoming == _messageType ? Styling::StyleVariable::BASE_SECONDARY : Styling::StyleVariable::PRIMARY_PASTEL;
        return Styling::getParameters().getColor(styleVariable);
    }

    QPixmap renderNoAssigneeAvatar(MessageType _messageType)
    {
        const auto size = avatarSize();
        QPixmap result(Utils::scale_bitmap(QSize(size, size)));
        Utils::check_pixel_ratio(result);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        painter.setBrush(noAssigneeAvatarBackgroundColor(_messageType));
        painter.setPen(Qt::NoPen);
        const auto radius = size / 2;
        painter.drawRoundedRect(0, 0, size, size, radius, radius);
        const auto iconOffset = Utils::scale_value(2);
        const auto iconSize = noAssigneeIconSize();
        QPixmap icon = Utils::renderSvg(qsl(":/task/no_assignee_icon"), { iconSize, iconSize }, noAssigneeAvatarIconColor(_messageType));
        painter.drawPixmap(iconOffset, iconOffset, icon);
        return result;
    }

    auto generateAvatars()
    {
        return std::array{ renderNoAssigneeAvatar(MessageType::Incoming), renderNoAssigneeAvatar(MessageType::Outgoing) };
    }

    const QPixmap& noAssigneeAvatar(MessageType _messageType)
    {
        static std::array<QPixmap, 2> avatars = generateAvatars();

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            avatars = generateAvatars();

        return avatars.at(MessageType::Incoming == _messageType ? 0 : 1);
    }

    enum class HasAssignee
    {
        Yes,
        No
    };

    auto noAssigneeTextStyle(MessageType _messageType)
    {
        return MessageType::Incoming == _messageType ? Styling::StyleVariable::BASE_PRIMARY : Styling::StyleVariable::PRIMARY_PASTEL;
    }

    auto assigneeTextColor(HasAssignee _hasAssignee, MessageType _messageType)
    {
        return Styling::ThemeColorKey{ HasAssignee::Yes == _hasAssignee ? Styling::StyleVariable::TEXT_SOLID : noAssigneeTextStyle(_messageType) };
    }

    auto taskStatusBubbleHeight() noexcept
    {
        return Utils::scale_value(24);
    }

    auto statusTextFont()
    {
        return Fonts::adjustedAppFont(platform::is_apple() ? 12 : 13);
    }

    auto statusTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT };
    }

    auto statusBackgroundColor(core::tasks::status _status, Ui::ComplexMessage::StatusChipState _state)
    {
        using core::tasks::status;
        using Styling::StyleVariable;
        using Ui::ComplexMessage::StatusChipState;

        im_assert(status::unknown != _status);

        static const std::unordered_map<status, std::unordered_map<StatusChipState, StyleVariable>> colorMap =
        {
            {
                status::new_task,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_CREATE },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_CREATE_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_CREATE_ACTIVE }
                }
            },
            {
                status::in_progress,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_IN_PROGRESS },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_IN_PROGRESS_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_IN_PROGRESS_ACTIVE }
                }
            },
            {
                status::ready,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_READY },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_READY_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_READY_ACTIVE }
                }
            },
            {
                status::rejected,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_DENIED },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_DENIED_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_DENIED_ACTIVE }
                }
            },
            {
                status::closed,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_CLOSED },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_CLOSED_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_CLOSED_ACTIVE }
                }
            },
            {
                status::unknown,
                {
                    { StatusChipState::None, StyleVariable::TASK_CHEAPS_CREATE },
                    { StatusChipState::Hovered, StyleVariable::TASK_CHEAPS_CREATE_HOVER },
                    { StatusChipState::Pressed, StyleVariable::TASK_CHEAPS_CREATE_ACTIVE }
                }
            }
        };

        const auto colorVariable = colorMap.at(_status).at(_state);
        return Styling::getParameters().getColor(colorVariable);
    }

    auto statusArrowLeftMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto statusArrowIconSize() noexcept
    {
        return Utils::scale_value(12);
    }

    auto statusBubbleHorizontalPadding() noexcept
    {
        return Utils::scale_value(10);
    }

    auto statusBubbleRadius() noexcept
    {
        return Utils::scale_value(12);
    }

    auto statusBubbleRightMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto statusArrowVerticalMargin()
    {
        return Utils::scale_value(6);
    }

    Utils::StyledPixmap& statusArrowIcon()
    {
        static auto arrow = Utils::StyledPixmap(qsl(":/task/status_arrow"), { statusArrowIconSize(), statusArrowIconSize() }, statusTextColor());
        return arrow;
    }

    auto bubbleBottomPadding() noexcept
    {
        return Utils::scale_value(4);
    }

    auto skeletonHeight() noexcept
    {
        return Utils::scale_value(60);
    }

    auto skeletonTopRectRadius()
    {
        return Utils::scale_value(8);
    }

    QRect skeletonTopRect(int width) noexcept
    {
        return { 0, 0, width, Utils::scale_value(40) };
    }

    auto skeletonBottomRectRadius()
    {
        return Utils::scale_value(6);
    }

    QRect skeletonBottomRect(int width) noexcept
    {
        return { 0, Utils::scale_value(48), std::min(width, Utils::scale_value(256)), Utils::scale_value(12) };
    }

    constexpr std::chrono::milliseconds animationDuration() noexcept
    {
        return std::chrono::seconds(2);
    }

    QString timePrintFormat()
    {
        return qsl("hh:mm");
    }
} // namespace

UI_COMPLEX_MESSAGE_NS_BEGIN

TaskBlock::TaskBlock(ComplexMessageItem* _parent, const Data::TaskData& _task, const QString& _text)
    : GenericBlock(_parent, _text, MenuFlags(MenuFlagCopyable), false)
    , skeletonAnimation_(new QVariantAnimation(this))
    , task_{ _task }
{
    skeletonAnimation_->setStartValue(0.0);
    skeletonAnimation_->setEndValue(1.0);
    skeletonAnimation_->setDuration(animationDuration().count());
    skeletonAnimation_->setLoopCount(-1);
    connect(skeletonAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&TaskBlock::update));

    if (task_.hasId())
        syncronizeTaskFromContainer();
}

TaskBlock::~TaskBlock() = default;

IItemBlockLayout* TaskBlock::getBlockLayout() const
{
    return nullptr; // TaskBlock doesn't use a layout
}

Data::FString TaskBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
{
    if (titleTextUnit_)
    {
        const auto textType = _dest == IItemBlock::TextDestination::quote ? TextRendering::TextType::SOURCE : TextRendering::TextType::VISIBLE;
        if (textType == TextRendering::TextType::SOURCE && titleTextUnit_->isAllSelected())
            return getSourceText();

        return titleTextUnit_->getSelectedText(textType);
    }

    return {};
}

bool TaskBlock::updateFriendly(const QString& _aimId, const QString&)
{
    const auto needUpdate = task_.params_.assignee_ == _aimId;
    if (needUpdate)
        notifyBlockContentsChanged();
    return needUpdate;
}

void TaskBlock::updateWith(IItemBlock* _other)
{
    if (auto otherTaskBlock = dynamic_cast<TaskBlock*>(_other))
    {
        const auto otherTask = otherTaskBlock->task_;

        if (otherTask.hasIdAndData())
        {
            im_assert(Logic::GetTaskContainer()->contains(otherTask.id_));
            task_ = otherTask;
        }
        else if (otherTask.hasId())
        {
            syncronizeTaskFromContainer();
        }

        if (!task_.needToLoadData())
            notifyBlockContentsChanged();
    }
}

Data::FString TaskBlock::getSourceText() const
{
    return task_.params_.title_;
}

bool TaskBlock::isSelected() const
{
    return titleTextUnit_ && titleTextUnit_->isSelected();
}

bool TaskBlock::isAllSelected() const
{
    return titleTextUnit_ && titleTextUnit_->isAllSelected();
}

void TaskBlock::selectByPos(const QPoint& _from, const QPoint& _to, bool)
{
    if (titleTextUnit_)
    {
        const auto localFrom = mapFromGlobal(_from);
        const auto localTo = mapFromGlobal(_to);

        titleTextUnit_->select(localFrom, localTo);
    }

    update();
}

void TaskBlock::clearSelection()
{
    if (titleTextUnit_)
        titleTextUnit_->clearSelection();
    update();
}

void TaskBlock::releaseSelection()
{
    if (titleTextUnit_)
        titleTextUnit_->releaseSelection();
}

bool TaskBlock::isDraggable() const
{
    return false;
}

bool TaskBlock::isSharingEnabled() const
{
    return true;
}

QString TaskBlock::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("task_block", "Task");
}

Data::Quote TaskBlock::getQuote() const
{
    Data::Quote quote;
    quote.senderId_ = isOutgoing() ? MyInfo()->aimId() : getSenderAimid();
    quote.chatId_ = getChatAimid();
    if (Logic::getContactListModel()->isChannel(quote.chatId_))
        quote.senderId_ = quote.chatId_;
    quote.chatStamp_ = Logic::getContactListModel()->getChatStamp(quote.chatId_);
    auto messageItem = getParentComplexMessage();
    quote.time_ = messageItem->getTime();
    quote.msgId_ = getId();
    if (isSelected())
    {
        quote.text_ = titleTextUnit_->getSelectedText();
        return quote;
    }

    quote.text_ = getSourceText();
    quote.mediaType_ = getMediaType();
    quote.task_ = task_;

    quote.mediaType_ = Ui::MediaType::mediaTypeTask;

    if (quote.isEmpty())
        return {};

    QString senderFriendly = getSenderFriendly();
    if (senderFriendly.isEmpty())
        senderFriendly = Logic::GetFriendlyContainer()->getFriendly(quote.senderId_);
    if (isOutgoing())
        senderFriendly = MyInfo()->friendly();
    quote.senderFriendly_ = std::move(senderFriendly);

    return quote;
}

bool TaskBlock::onMenuItemTriggered(const QVariantMap& _params)
{
    const auto command = _params[qsl("command")].toString();
    if (command == u"edit_task")
    {
        auto taskWidget = new Ui::TaskEditWidget(task_, getParentComplexMessage()->getMentions());

        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        opt.threadBadge_ = Logic::getContactListModel()->isThread(getChatAimid());
        GeneralDialog gd(taskWidget, Utils::InterConnector::instance().getMainWindow(), opt);
        taskWidget->setFocusOnTaskTitleInput();
        gd.addLabel(taskWidget->getHeader(), Qt::AlignCenter);
        gd.execute();
        return true;
    }
    else if (command == u"dev:copy_task_id")
    {
        QApplication::clipboard()->setText(task_.id_);
        showToast(QT_TRANSLATE_NOOP("task_block", "Task Id copied to clipboard"));
    }

    return GenericBlock::onMenuItemTriggered(_params);
}

IItemBlock::MenuFlags TaskBlock::getMenuFlags(QPoint _p) const
{
    int flags = GenericBlock::getMenuFlags(_p);
    if (!needSkeleton() && canModifyTask() && !isInsideQuote() && !isInsideForward())
    {
        flags |= MenuFlagEditTask;
    }
    return static_cast<MenuFlags>(flags);
}

IItemBlock::ContentType TaskBlock::getContentType() const
{
    return ContentType::Task;
}

MediaType TaskBlock::getMediaType() const
{
    return isSelected() ? MediaType::noMedia : MediaType::mediaTypeTask;
}

void TaskBlock::onVisibleRectChanged(const QRect& _visibleRect)
{
    if (needSkeleton() && _visibleRect.height() > skeletonHeight() / 2)
    {
        if (skeletonAnimation_->state() != QAbstractAnimation::State::Running)
            skeletonAnimation_->start();
    }
    else if (skeletonAnimation_->state() == QAbstractAnimation::State::Running)
    {
        skeletonAnimation_->pause();
    }
}

QRect TaskBlock::setBlockGeometry(const QRect& _rect)
{
    if (!titleTextUnit_)
        return _rect;

    auto timeWidth = 0;

    if (const auto cm = getParentComplexMessage())
    {
        if (const auto timeWidget = cm->getTimeWidget())
            timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();
    }

    const auto availableWidth = std::min(_rect.width(), maxBlockWidth());
    blockWidth_ = availableWidth;
    auto height = needSkeleton() ? updateSkeletonSize(availableWidth) : updateTaskContentSize(availableWidth);

    if (GetAppConfig().IsShowMsgIdsEnabled())
        height += evaluateDebugIdHeight(availableWidth - timeWidth, height);

    contentRect_ = QRect{ _rect.topLeft(), QSize{availableWidth, height} };
    setGeometry(contentRect_);

    return contentRect_;
}

QRect TaskBlock::getBlockGeometry() const
{
    return contentRect_;
}

void TaskBlock::drawBlock(QPainter& _painter, const QRect&, const QColor&)
{
    if (!needSkeleton())
    {
        titleTextUnit_->draw(_painter);

        const auto aimId = task_.params_.assignee_;
        const auto hasAssignee = !aimId.isEmpty();

        const auto friendly = hasAssignee ? Logic::GetFriendlyContainer()->getFriendly(aimId) : noAssigneeText();

        const auto messageType = getParentComplexMessage()->isOutgoingPosition() ? ::MessageType::Outgoing : MessageType::Incoming;
        auto isDefault = false;
        const auto avatar = hasAssignee ? Logic::GetAvatarStorage()->GetRounded(aimId, friendly, Utils::scale_bitmap(avatarSize()), isDefault, false, false) : noAssigneeAvatar(messageType);

        const auto ratio = Utils::scale_bitmap_ratio();

        const auto x = 0;
        const auto y = verticalOffset() + titleTextUnit_->cachedSize().height();

        _painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        _painter.drawPixmap(QPoint(x, y), avatar);

        assigneeTextUnit_->draw(_painter, TextRendering::VerPosition::MIDDLE);

        if (task_.params_.status_ != core::tasks::status::unknown)
        {
            const auto needDrawArrow = needDrawStatusArrow();
            _painter.setBrush(statusBackgroundColor(task_.params_.status_, statusChipState_));
            const auto radius = statusBubbleRadius();
            const auto statusRect = statusBubbleRect();
            _painter.drawRoundedRect(statusRect, radius, radius);

            if (needDrawArrow)
            {
                const auto arrowX = statusRect.x() + statusTextUnit_->cachedSize().width() + statusBubbleHorizontalPadding() + statusArrowLeftMargin();
                const auto arrowY = statusRect.y() + statusArrowVerticalMargin();
                _painter.drawPixmap(arrowX, arrowY, statusArrowIcon().actualPixmap());
            }
        }

        statusTextUnit_->draw(_painter, TextRendering::VerPosition::MIDDLE);
        timeTextUnit_->draw(_painter, TextRendering::VerPosition::MIDDLE);
    }
    else
    {
        const auto& preloaderBrush = MessageStyle::Snippet::getPreloaderBrush();
        _painter.setBrush(preloaderBrush);

        _painter.setPen(Qt::NoPen);
        _painter.setBrushOrigin(blockWidth_ * skeletonAnimation_->currentValue().toDouble(), 0);
        auto radius = skeletonTopRectRadius();
        _painter.drawRoundedRect(skeletonTopRect(blockWidth_), radius, radius);
        radius = skeletonBottomRectRadius();
        _painter.drawRoundedRect(skeletonBottomRect(blockWidth_), radius, radius);
    }

    if (Q_UNLIKELY(GetAppConfig().IsShowMsgIdsEnabled() && debugIdTextUnit_ && debugCreatorTextUnit_ && debugThreadIdTextUnit_))
    {
        debugIdTextUnit_->draw(_painter);
        debugCreatorTextUnit_->draw(_painter);
        debugThreadIdTextUnit_->draw(_painter);
    }
}

void TaskBlock::initialize()
{
    GenericBlock::initialize();
    initTextUnits();

    if (task_.needToSend())
        task_.params_.status_ = core::tasks::status::new_task;
    else
        syncronizeTaskFromContainer();
    updateContent();

    connect(Logic::GetTaskContainer(), &Logic::TaskContainer::taskChanged, this, &TaskBlock::onTaskUpdated);
    connect(GetDispatcher(), &core_dispatcher::appConfigChanged, this, [this]() { updateContent(); });
}

void TaskBlock::mouseMoveEvent(QMouseEvent* _event)
{
    if (!Utils::InterConnector::instance().isMultiselect(getChatAimid()))
    {
        const auto point = _event->pos();
        const auto globalPos = mapToGlobal(point);
        if (StatusChipState::Pressed != statusChipState_)
            updateStatusChipState((statusBubbleRect().contains(point) && canChangeTaskStatus()) ? StatusChipState::Hovered : StatusChipState::None);

        const auto handForStatus = canChangeTaskStatus() && (StatusChipState::None != statusChipState_);
        const auto handForAssignee = !task_.params_.assignee_.isEmpty() && profileClickAreaRect().contains(point);
        const auto needHandByBubble = handForStatus || handForAssignee || !linkAtPos(globalPos).isEmpty();
        const auto handCursor = !needSkeleton() && needHandByBubble || isInsideQuote() || isInsideForward();
        setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);

        if (assigneeTextUnit_)
        {
            const auto assigneTextHeight = assigneeTextUnit_->cachedSize().height();
            const QRect assigneeTextRect{ assigneeTextUnit_->offsets().x(), assigneeTextUnit_->offsets().y() - assigneTextHeight / 2, assigneeTextUnit_->cachedSize().width(), assigneTextHeight };
            if (assigneeTextRect.contains(point) && assigneeTextUnit_->isElided())
            {
                if (!isTooltipActivated())
                {
                    const auto isFullyVisible = visibleRegion().boundingRect().y() < assigneeTextRect.top();
                    const auto arrowDir = isFullyVisible ? Tooltip::ArrowDirection::Down : Tooltip::ArrowDirection::Up;
                    const auto arrowPos = isFullyVisible ? Tooltip::ArrowPointPos::Top : Tooltip::ArrowPointPos::Bottom;
                    showTooltip(assigneeTextUnit_->getSourceText().string(), QRect(mapToGlobal(assigneeTextRect.topLeft()), assigneeTextRect.size()), arrowDir, arrowPos);
                }
            }
            else
            {
                hideTooltip();
            }
        }
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }

    GenericBlock::mouseMoveEvent(_event);
}

void TaskBlock::mousePressEvent(QMouseEvent* _event)
{
    if (const auto point = _event->pos(); statusBubbleRect().contains(point) && canChangeTaskStatus())
        updateStatusChipState(StatusChipState::Pressed);
    GenericBlock::mousePressEvent(_event);
}

void TaskBlock::mouseReleaseEvent(QMouseEvent* _event)
{
    const auto point = _event->pos();
    updateStatusChipState((statusBubbleRect().contains(point) && canChangeTaskStatus()) ? StatusChipState::Hovered : StatusChipState::None);
    GenericBlock::mouseReleaseEvent(_event);
}

void TaskBlock::leaveEvent(QEvent* _event)
{
    updateStatusChipState(StatusChipState::None);
}

bool TaskBlock::clicked(const QPoint& _p)
{
    hideTooltip();
    const auto mappedPoint = mapFromParent(_p, getBlockGeometry());
    if (const auto statusRect = statusBubbleRect(); canChangeTaskStatus() && statusRect.contains(mappedPoint))
    {
        auto mw = Utils::InterConnector::instance().getMainWindow();
        auto popup = new TaskStatusPopup(task_, statusRect.translated(mapTo(mw, QPoint(0, -mw->getTitleHeight()))));
        popup->show();
        popup->setFocus();
        return true;
    }
    else if (!task_.params_.assignee_.isEmpty() && profileClickAreaRect().contains(mappedPoint))
    {
        Utils::openDialogOrProfile(task_.params_.assignee_);
        return true;
    }
    else if (titleTextUnit_ && titleTextUnit_->isOverLink(mappedPoint))
    {
        titleTextUnit_->clicked(mappedPoint);
        return true;
    }
    return false;
}

void TaskBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
{
    if (titleTextUnit_)
    {
        if (!titleTextUnit_->isAllSelected())
        {
            const auto mappedPoint = mapFromParent(_p, getBlockGeometry());
            titleTextUnit_->doubleClicked(mappedPoint, true, _callback);
            update();
        }
    }
}

Data::LinkInfo TaskBlock::linkAtPos(const QPoint& _globalPos) const
{
    if (titleTextUnit_)
        return titleTextUnit_->getLink(mapFromGlobal(_globalPos));

    return GenericBlock::linkAtPos(_globalPos);
}

int TaskBlock::desiredWidth(int _w) const
{
    return Utils::scale_value(360);
}

void TaskBlock::updateFonts()
{
    reinitTextUnits();
}

void TaskBlock::initTextUnits()
{
    titleTextUnit_ = Ui::TextRendering::MakeTextUnit(QString(), getParentComplexMessage()->getMentions(), Ui::TextRendering::LinksVisible::SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
    TextRendering::TextUnit::InitializeParameters params{ titleTextFont(), MessageStyle::getTextColorKey() };
    params.monospaceFont_ = MessageStyle::getTextMonospaceFont();
    params.linkColor_ = MessageStyle::getLinkColorKey();
    params.selectionColor_ = MessageStyle::getTextSelectionColorKey(getChatAimid());
    titleTextUnit_->init(params);

    assigneeTextUnit_ = Ui::TextRendering::MakeTextUnit(QString(), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    assigneeTextUnit_->init({ Fonts::adjustedAppFont(14), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });

    statusTextUnit_ = Ui::TextRendering::MakeTextUnit(QString(), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    statusTextUnit_->init({ statusTextFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });

    timeTextUnit_ = Ui::TextRendering::MakeTextUnit(QString(), {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    timeTextUnit_->init({ Fonts::adjustedAppFont(14), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
}

void TaskBlock::reinitTextUnits()
{
    initTextUnits();
    updateContent();
}

void TaskBlock::updateStatusChipState(StatusChipState _statusChipState)
{
    if (_statusChipState != statusChipState_)
    {
        statusChipState_ = _statusChipState;
        update();
    }
}

void TaskBlock::updateContent()
{
    titleTextUnit_->setText(task_.params_.title_);
    const auto stopwatchEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x23F1));
    const auto current = QDateTime::currentDateTime();
    const auto date = task_.params_.date_.date();

    const auto aimId = task_.params_.assignee_;
    const auto hasAssignee = !aimId.isEmpty();
    const auto messageType = getParentComplexMessage()->isOutgoingPosition() ? ::MessageType::Outgoing : MessageType::Incoming;
    const auto friendly = hasAssignee ? Logic::GetFriendlyContainer()->getFriendly(aimId) : noAssigneeText();
    assigneeTextUnit_->setText(friendly, assigneeTextColor(hasAssignee ? HasAssignee::Yes : HasAssignee::No, messageType));
    if (task_.params_.status_ != core::tasks::status::unknown)
        statusTextUnit_->setText(Data::TaskData::statusDescription(task_.params_.status_), statusTextColor());

    if (task_.params_.date_.isValid())
        timeTextUnit_->setText(stopwatchEmoji % u" " % Utils::GetTranslator()->formatDate(date, date.year() == current.date().year()) % u" " % task_.params_.date_.time().toString(timePrintFormat()));
    else
        timeTextUnit_->setText(stopwatchEmoji % u" " % QT_TRANSLATE_NOOP("task_block", "Not set"));

    if (GetAppConfig().IsShowMsgIdsEnabled())
    {
        TextRendering::TextUnit::InitializeParameters params{ Fonts::adjustedAppFont(14, Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        debugIdTextUnit_ = TextRendering::MakeTextUnit(ql1s("Task Id: %1").arg(task_.id_));
        debugIdTextUnit_->init(params);
        debugCreatorTextUnit_ = TextRendering::MakeTextUnit(ql1s("Creator: %1").arg(task_.params_.creator_.isEmpty() ? QString() : Logic::GetFriendlyContainer()->getFriendly(task_.params_.creator_)));
        debugCreatorTextUnit_->init(params);
        debugThreadIdTextUnit_ = TextRendering::MakeTextUnit(ql1s("Thread Id: %1").arg(task_.params_.threadId_));
        debugThreadIdTextUnit_->init(params);
    }
    else
    {
        debugIdTextUnit_.reset();
        debugCreatorTextUnit_.reset();
        debugThreadIdTextUnit_.reset();
    }

    notifyBlockContentsChanged();
}

int TaskBlock::updateTaskContentSize(int _width)
{
    auto height = titleTextUnit_->getHeight(_width) + verticalOffset();

    statusTextUnit_->evaluateDesiredSize();
    timeTextUnit_->evaluateDesiredSize();

    assigneeTextUnit_->elide(assigneeAvailableWidth());
    assigneeTextUnit_->evaluateDesiredSize();

    const auto avatarWidth = avatarSize();
    assigneeTextUnit_->setOffsets(avatarWidth + avatarRightMargin(), height + avatarWidth / 2);

    const auto statusTextOffset = taskStatusBubbleHeight() / 2;

    height += avatarSize() + verticalOffset();
    statusTextUnit_->setOffsets(statusBubbleHorizontalPadding(), height + statusTextOffset);
    timeTextUnit_->setOffsets(timeTextLeftOffset(), height + statusTextOffset);
    height += taskStatusBubbleHeight() + bubbleBottomPadding();

    return height;
}

int TaskBlock::updateSkeletonSize(int width)
{
    return skeletonHeight();
}

int TaskBlock::evaluateDebugIdHeight(int _availableWidth, int _verticalOffset)
{
    auto debugIdHeight = 0;
    if (debugIdTextUnit_ && debugCreatorTextUnit_ && debugThreadIdTextUnit_)
    {
        debugIdTextUnit_->setOffsets(0, _verticalOffset);
        debugIdHeight += debugIdTextUnit_->getHeight(_availableWidth);
        debugCreatorTextUnit_->setOffsets(0, _verticalOffset + debugIdHeight);
        debugIdHeight += debugCreatorTextUnit_->getHeight(_availableWidth);
        debugThreadIdTextUnit_->setOffsets(0, _verticalOffset + debugIdHeight);
        debugIdHeight += debugThreadIdTextUnit_->getHeight(_availableWidth);
    }

    return debugIdHeight;
}

int TaskBlock::timeTextLeftOffset() const
{
    const auto statusRect = statusBubbleRect();
    return statusRect.x() + statusRect.width() + statusBubbleRightMargin();
}

int TaskBlock::taskStatusBubbleWidth() const
{
    const auto arrowWidth = needDrawStatusArrow() ? statusArrowLeftMargin() + statusArrowIconSize() : 0;
    return statusBubbleHorizontalPadding() * 2 + statusTextUnit_->cachedSize().width() + arrowWidth;
}

QRect TaskBlock::statusBubbleRect() const
{
    const auto y = verticalOffset() + titleTextUnit_->cachedSize().height();
    const auto bubbleY = y + avatarSize() + verticalOffset();
    return { 0, bubbleY, taskStatusBubbleWidth(), taskStatusBubbleHeight() };
}

QRect TaskBlock::profileClickAreaRect() const
{
    const auto y = titleTextUnit_->cachedSize().height();
    const auto width = assigneeTextUnit_->horOffset() + assigneeTextUnit_->cachedSize().width();
    const auto height = avatarSize() + 2 * verticalOffset();
    return { 0, y, width, height };
}

int TaskBlock::assigneeAvailableWidth() const
{
    return blockWidth_ - avatarSize() - avatarRightMargin();
}

bool TaskBlock::needSkeleton() const
{
    return task_.needToLoadData() || task_.isEmpty();
}

bool TaskBlock::canModifyTask() const
{
    return task_.hasId() && (MyInfo()->aimId() == task_.params_.creator_);
}

bool TaskBlock::canChangeTaskStatus() const
{
    return task_.didReceiveFetch() && task_.hasAllowedStatuses();
}

bool TaskBlock::needDrawStatusArrow() const
{
    return task_.hasAllowedStatuses() || !task_.didReceiveFetch();
}

void TaskBlock::syncronizeTaskFromContainer()
{
    task_ = Logic::GetTaskContainer()->getTask(task_.id_);

    const auto messageItem = getParentComplexMessage();
    if (auto& messageTask = messageItem->buddy().task_)
    {
        messageTask = task_;
        const auto& threadId = task_.params_.threadId_;
        const auto& chat = messageItem->getContact();
        // move task thread update to message
        if (!threadId.isEmpty() && !Logic::getContactListModel()->isThread(chat))
        {
            messageItem->applyThreadUpdate(task_.threadUpdate_);
            GetDispatcher()->addChatsThread(messageItem->getChatAimid(), messageItem->getId(), task_.threadUpdate_->threadId_);
        }
    }
}

void TaskBlock::onTaskUpdated(const QString& _taskId)
{
    if (_taskId != task_.id_)
        return;
    syncronizeTaskFromContainer();
    updateContent();
}

UI_COMPLEX_MESSAGE_NS_END
