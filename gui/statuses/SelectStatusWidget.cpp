#include "stdafx.h"

#include "controls/TransparentScrollBar.h"
#include "main_window/contact_list/SearchWidget.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/contact_list/Common.h"
#include "main_window/MainWindow.h"
#include "main_window/MainPage.h"
#include "SelectStatusWidget.h"
#include "LocalStatuses.h"
#include "StatusUtils.h"
#include "StatusListItem.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "cache/emoji/Emoji.h"
#include "controls/TextUnit.h"
#include "controls/FlatMenu.h"
#include "controls/ContactAvatarWidget.h"
#include "controls/TooltipWidget.h"
#include "styles/ThemeParameters.h"
#include "core_dispatcher.h"
#include "my_info.h"
#include "fonts.h"

namespace
{
    constexpr auto leaveTimerInterval() noexcept { return std::chrono::milliseconds(50); }
    constexpr auto statusTimeUpdateTimeInterval() noexcept { return std::chrono::milliseconds(1000); }

    int windowWidth()
    {
        return Utils::scale_value(360);
    }

    int32_t leftRightStatusContentMargin()
    {
        return Utils::scale_value(16);
    }

    int32_t statusContentWidth()
    {
        return windowWidth() - 2 * leftRightStatusContentMargin();
    }

    int maxListHeight()
    {
        return Utils::scale_value(614);
    }

    double listHeightWindowHeightRatio()
    {
        return 0.7;
    }

    QSize itemSize()
    {
        return QSize(windowWidth(), Utils::scale_value(44));
    }

    int emojiLeftMargin()
    {
        return Utils::scale_value(12);
    }

    int emojiSize()
    {
        return Utils::scale_value(34);
    }

    int32_t descriptionLeftMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t descriptionRightMargin()
    {
        return Utils::scale_value(16);
    }

    int32_t descriptionTopMargin()
    {
        return Utils::scale_value(12);
    }

    QColor descriptionFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor durationFontColor(bool active = false)
    {
        return Styling::getParameters().getColor(active ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_PRIMARY);
    }

    QFont itemDescriptionFont()
    {
        return Fonts::appFontScaled(16);
    }

    QFont durationFont()
    {
        return Fonts::appFontScaled(15);
    }

    int twoLineTextTopMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(6);
        else
            return Utils::scale_value(4);
    }

    int twoLineTextBottomMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(2);
        else
            return Utils::scale_value(4);
    }

    const QColor& itemHoveredBackground()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        return color;
    }

    QSize buttonSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    const QPixmap& buttonIcon(const bool _hovered, const bool _pressed)
    {
        if (_pressed)
        {
            static auto icon  = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), buttonSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
            return icon;
        }
        else if (_hovered)
        {
            static auto icon  = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), buttonSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
            return icon;
        }
        else
        {
            static auto icon  = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), buttonSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            return icon;
        }
    }

    QSize buttonPressRectSize()
    {
        return Utils::scale_value(QSize(44, 44));
    }

    int buttonRightMargin()
    {
        return Utils::scale_value(8);
    }

    QImage emptyStatusIcon()
    {
        return Utils::renderSvg(qsl(":/statuses/empty_status_icon"), Utils::scale_value(QSize(34, 34)), Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY)).toImage();
    }

    QSize placeholderSize()
    {
        return Utils::scale_value(QSize(84, 84));
    }

    QColor placeholderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
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

    QFont headerLabelFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(22, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(22);
    }

    QFont statusDescriptionFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(17, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(17);
    }
}

namespace Statuses
{

using namespace Ui;

//////////////////////////////////////////////////////////////////////////
// SelectStatusWidget
//////////////////////////////////////////////////////////////////////////

class SelectStatusWidget_p
{
public:
    ContactAvatarWidget* avatar_ = nullptr;
    TextWidget* description_ = nullptr;
    TextWidget* duration_ = nullptr;
    StatusesList* list_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    SearchWidget* searchWidget_ = nullptr;
    QTimer* statusTimeTimer_ = nullptr;
    QWidget* placeholder_ = nullptr;
    Status status_;
};

//////////////////////////////////////////////////////////////////////////
// SelectStatusWidget
//////////////////////////////////////////////////////////////////////////

SelectStatusWidget::SelectStatusWidget(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<SelectStatusWidget_p>())
{
    auto layout = Utils::emptyVLayout(this);

    auto label = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "My status"));
    label->init(headerLabelFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    label->setMaxWidthAndResize(windowWidth());

    d->avatar_ = new ContactAvatarWidget(this, MyInfo()->aimId(), QString(), Utils::scale_value(56), true, true);
    d->avatar_->SetMode(ContactAvatarWidget::Mode::StatusPicker);
    d->avatar_->setFixedSize(Utils::scale_value(60), Utils::scale_value(60));

    d->status_ = Logic::GetStatusContainer()->getStatus(MyInfo()->aimId());

    auto statusContent = new QWidget(this);
    auto statusContentVLayout = Utils::emptyVLayout(statusContent);
    auto description = !d->status_.isEmpty() ? d->status_.getDescription() : QString();
    elideDescription(description);

    d->description_ = new TextWidget(this, description);
    Testing::setAccessibleName(d->description_, qsl("AS SelectStatusWidget statusDescription"));
    d->description_->init(statusDescriptionFont(), descriptionFontColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    d->description_->setMaxWidthAndResize(statusContentWidth());
    d->duration_ = new TextWidget(this, !d->status_.isEmpty() ? d->status_.getTimeString() : QString());
    Testing::setAccessibleName(d->duration_, qsl("AS SelectStatusWidget statusDuration"));
    d->duration_->init(Fonts::appFontScaled(13), durationFontColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    d->duration_->setMaxWidthAndResize(statusContentWidth());
    statusContent->setVisible(!d->status_.isEmpty());

    statusContentVLayout->addSpacing(Utils::scale_value(12));
    statusContentVLayout->addWidget(d->description_);
    statusContentVLayout->addSpacing(Utils::scale_value(2));
    statusContentVLayout->addWidget(d->duration_);

    auto statusContentOuterParent = new QWidget(this);
    auto statusContentHLayout = Utils::emptyHLayout(statusContentOuterParent);
    statusContentHLayout->addSpacing(leftRightStatusContentMargin());
    statusContentHLayout->addWidget(statusContent);
    statusContentHLayout->addSpacing(leftRightStatusContentMargin());

    layout->addSpacing(Utils::scale_value(20));
    layout->addWidget(label);
    layout->addSpacing(Utils::scale_value(20));
    layout->addWidget(d->avatar_, 0, Qt::AlignHCenter);
    layout->addWidget(statusContentOuterParent);

    d->searchWidget_ = new SearchWidget(this, Utils::scale_value(4));
    Testing::setAccessibleName(d->searchWidget_, qsl("AS SelectStatusWidget search"));
    layout->addSpacing(Utils::scale_value(12));
    layout->addWidget(d->searchWidget_);
    layout->addSpacing(Utils::scale_value(8));

    connect(d->searchWidget_, &SearchWidget::search, this, [this](const QString& _pattern)
    {
        d->list_->filter(_pattern);
        const auto noResults = d->list_->itemsCount() == 0;
        d->scrollArea_->setVisible(!noResults);
        d->placeholder_->setVisible(noResults);
    });
    connect(d->searchWidget_, &SearchWidget::escapePressed, this, [this]() { setFocus(); });

    auto contentWidget = new QWidget(this);
    auto contentLayout = Utils::emptyVLayout(contentWidget);

    d->scrollArea_ = Ui::CreateScrollAreaAndSetTrScrollBarV(this);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));

    d->list_ = new StatusesList(d->scrollArea_);
    d->scrollArea_->setWidget(d->list_);

    d->statusTimeTimer_ = new QTimer(this);
    d->statusTimeTimer_->setInterval(statusTimeUpdateTimeInterval());
    connect(d->statusTimeTimer_, &QTimer::timeout, this, [this]()
    {
        d->list_->updateUserStatus(d->status_);
        if (!d->status_.isEmpty())
            d->duration_->setText(!d->status_.isEmpty() ? d->status_.getTimeString() : QString());

    });

    connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this, statusContent](const QString& _contactId)
    {
        if (_contactId != MyInfo()->aimId())
            return;

        d->status_ = Logic::GetStatusContainer()->getStatus(MyInfo()->aimId());
        d->list_->updateUserStatus(d->status_);

        auto description = !d->status_.isEmpty() ? d->status_.getDescription() : QString();
        elideDescription(description);
        d->description_->setText(description);

        d->duration_->setText(!d->status_.isEmpty() ? d->status_.getTimeString() : QString());
        statusContent->setVisible(!d->status_.isEmpty());
    });

    d->list_->updateUserStatus(d->status_);
    d->statusTimeTimer_->start();

    d->placeholder_ = new QWidget(this);
    Testing::setAccessibleName(d->placeholder_, qsl("AS SelectStatusWidget placeholder"));
    auto placeholderLayout = Utils::emptyVLayout(d->placeholder_);
    auto noResultsIcon = new QLabel(d->placeholder_);
    noResultsIcon->setPixmap(Utils::renderSvg(qsl(":/placeholders/empty_search"), placeholderSize(), placeholderColor()));
    auto noResultsText = new TextWidget(this, QT_TRANSLATE_NOOP("placeholders", "Nothing found"));
    noResultsText->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    noResultsText->setMaxWidthAndResize(windowWidth());
    placeholderLayout->addStretch();
    placeholderLayout->addWidget(noResultsIcon, 0, Qt::AlignCenter);
    placeholderLayout->addSpacing(Utils::scale_value(24));
    placeholderLayout->addWidget(noResultsText);
    placeholderLayout->addStretch();
    d->placeholder_->setVisible(false);

    contentLayout->addWidget(d->placeholder_);
    contentLayout->addWidget(d->scrollArea_);
    layout->addWidget(contentWidget);

    connect(d->avatar_, &ContactAvatarWidget::badgeClicked, this, []()
    {
        if (auto mainPage = MainPage::instance())
        {
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({});
            mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Profile);
        }
    });

    const auto maxHeight = Ui::ItemLength(false, listHeightWindowHeightRatio(), 0, Utils::InterConnector::instance().getMainWindow());
    setFixedSize(windowWidth(), std::min(maxHeight, maxListHeight()));
}

SelectStatusWidget::~SelectStatusWidget() = default;

void SelectStatusWidget::showEvent(QShowEvent* _event)
{
    d->searchWidget_->setFocus();
    QWidget::showEvent(_event);
}

void SelectStatusWidget::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape)
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());

    QWidget::keyPressEvent(_event);
}

//////////////////////////////////////////////////////////////////////////
// StatusesList_p
//////////////////////////////////////////////////////////////////////////

class StatusesList_p
{
public:

    struct MouseState
    {
        std::optional<int> hoveredIndex_;
        std::optional<int> pressedIndex_;
        std::optional<int> buttonHoveredIndex_;
        std::optional<int> buttonPressedIndex_;
    };

    using Items = std::vector<StatusesListItem*>;
    using ItemsIndexes = std::vector<int>;
    using ItemIndexIterator = ItemsIndexes::iterator;

    StatusesList_p(StatusesList* _q) : q(_q) {}

    void updateItemsPositions(ItemIndexIterator _begin, ItemIndexIterator _end)
    {
        auto it = _begin;

        int yOffset = (_begin - currentIndexes_.begin()) * itemSize().height();
        while (it != _end)
        {
            const auto index = *it;
            auto& item = items_[index];
            item->rect_ = QRect({0, yOffset}, itemSize());
            item->emojiRect_ = QRect({emojiLeftMargin(), yOffset + (itemSize().height() - emojiSize()) / 2}, QSize(emojiSize(), emojiSize()));

            if (!item->statusCode_.isEmpty())
            {
                item->buttonPressRect_ = QRect({itemSize().width() - buttonPressRectSize().width(), yOffset}, buttonPressRectSize());
                item->buttonRect_ = QRect({itemSize().width() - buttonSize().width() - buttonRightMargin(), yOffset + (itemSize().height() - buttonSize().height()) / 2}, buttonSize());
            }

            const auto hasDuration = !item->durationUnit_->getText().isEmpty();
            const auto descriptionLeft = item->emojiRect_.right() + descriptionLeftMargin();
            const auto availableWidth = itemSize().width() - descriptionLeft - descriptionRightMargin();
            item->descriptionUnit_->getHeight(availableWidth);
            item->descriptionUnit_->setOffsets(descriptionLeft, yOffset + (hasDuration ? twoLineTextTopMargin() : descriptionTopMargin()));

            if (hasDuration)
            {
                const auto durationHeight = item->durationUnit_->getHeight(availableWidth);
                item->durationUnit_->setOffsets(descriptionLeft, yOffset + itemSize().height() - durationHeight - twoLineTextBottomMargin());
            }

            yOffset += itemSize().height();
            ++it;
        }
    }

    void updateItemDuration(StatusesListItem* _item, const QString& _duration, bool _active)
    {
        const auto hasDuration = !_duration.isEmpty();
        const auto descriptionLeft = _item->emojiRect_.right() + descriptionLeftMargin();
        _item->descriptionUnit_->setOffsets(descriptionLeft, _item->rect_.top() + (hasDuration ? twoLineTextTopMargin() : descriptionTopMargin()));

        _item->durationUnit_->setText(_duration);
        if (hasDuration)
        {
            auto availableWidth = itemSize().width() - descriptionLeft - descriptionRightMargin();
            const auto durationHeight = _item->durationUnit_->getHeight(availableWidth);
            _item->durationUnit_->setOffsets(descriptionLeft, _item->rect_.top() + itemSize().height() - durationHeight - twoLineTextBottomMargin());
            _item->durationUnit_->setColor(durationFontColor(_active));
        }
    }

    void drawItem(QPainter& _p, int _itemIndex)
    {
        Utils::PainterSaver saver(_p);

        auto item = items_[_itemIndex];

        const auto hovered = mouseState_.hoveredIndex_ == _itemIndex;
        const auto buttonHovered = mouseState_.buttonHoveredIndex_ == _itemIndex;
        const auto buttonPressed = mouseState_.buttonPressedIndex_ == _itemIndex;

        if (hovered)
        {
            _p.fillRect(item->rect_, itemHoveredBackground());
            _p.drawPixmap(item->buttonRect_, buttonIcon(buttonHovered, buttonPressed));
        }

        _p.drawImage(item->emojiRect_, item->emoji_);

        item->descriptionUnit_->draw(_p);
        item->durationUnit_->draw(_p);
    }

    void drawInRect(QPainter& _p, const QRect& _rect)
    {
        for (auto it = itemIndexIteratorAt(_rect.top()); it != currentIndexes_.end(); ++it)
        {
            if (items_[*it]->rect_.top() > _rect.bottom())
                break;

            drawItem(_p, *it);
        }
    }

    StatusesListItem* createItem(const QString& _statusCode, const QString& _description, std::chrono::seconds _duration)
    {
        auto item = new StatusesListItem(q);
        item->setObjectName(qsl("StatusesListItem"));
        item->statusCode_ = _statusCode;
        if (!item->statusCode_.isEmpty())
            item->emoji_ = Emoji::GetEmoji(Emoji::EmojiCode::fromQString(item->statusCode_), Utils::scale_bitmap(emojiSize()));
        else
            item->emoji_ = emptyStatusIcon();

        item->description_ = _description;
        item->descriptionUnit_ = Ui::TextRendering::MakeTextUnit(item->description_, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        item->descriptionUnit_->init(itemDescriptionFont(), descriptionFontColor(), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::LEFT, 1);

        auto duration = getTimeString(_duration);
        item->durationUnit_ = Ui::TextRendering::MakeTextUnit(duration, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        item->durationUnit_->init(durationFont(), durationFontColor(), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::LEFT, 1);

        return item;
    }

    void createItems()
    {
        auto noStatus = createItem(QString(), QT_TRANSLATE_NOOP("status_popup", "No status"), std::chrono::seconds::zero());
        noStatusIndex_ = items_.size();
        items_.push_back(noStatus);

        const auto& localStatuses = LocalStatuses::instance()->statuses();
        for (auto& localStatus : localStatuses)
        {
            auto item = createItem(localStatus.status_, localStatus.description_, localStatus.duration_);
            itemsEmojiIndex_[item->statusCode_] = items_.size();
            items_.push_back(item);
        }

        setDefaultIndexes();
        updateItemsPositions(currentIndexes_.begin(), currentIndexes_.end());
    }

    void setDefaultIndexes()
    {
        currentIndexes_.resize(items_.size() - 1);
        std::iota(currentIndexes_.begin(), currentIndexes_.end(), 1);

        if (!userStatus_.isEmpty())
            currentIndexes_.insert(currentIndexes_.begin(), noStatusIndex_);
    }

    void removeNoStatus()
    {
        if (!currentIndexes_.empty() && currentIndexes_.front() == noStatusIndex_)
        {
            currentIndexes_.erase(currentIndexes_.begin());
            updateItemsPositions(currentIndexes_.begin(), currentIndexes_.end());
        }
    }

    void addNoStatus()
    {
        if (!currentIndexes_.empty() && currentIndexes_.front() != noStatusIndex_)
        {
            currentIndexes_.insert(currentIndexes_.begin(), noStatusIndex_);
            updateItemsPositions(currentIndexes_.begin(), currentIndexes_.end());
        }
    }

    int32_t calcHeight()
    {
        return currentIndexes_.size() * itemSize().height();
    }

    ItemIndexIterator itemIndexIteratorAt(int _yPos)
    {
        if (_yPos < 0 || _yPos > calcHeight())
            return currentIndexes_.end();

        auto index = _yPos / itemSize().height();
        return currentIndexes_.begin() + index;
    }

    bool isValidIndexIterator(ItemIndexIterator _it)
    {
        return _it != currentIndexes_.end();
    }

    void showMenuAt(QPoint _pos, const QString& _status)
    {
        auto menu = createMenu(_status);

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto menuHeight = menu->sizeHint().height();
        if (mainWindow && _pos.y() + menuHeight > mainWindow->geometry().bottom())
            _pos.ry() -= menuHeight;
        _pos.rx() -= menu->width();
        menu->move(_pos);
        menu->show();
    }

    QMenu* createMenu(const QString& _status)
    {
        auto menu = new FlatMenu(q, BorderStyle::SHADOW);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(menu, &FlatMenu::triggered, q, [_status](QAction* _action)
        {
            auto actionDuration = _action->data().toLongLong();
            GetDispatcher()->setStatus(_status, actionDuration);
            LocalStatuses::instance()->setDuration(_status, std::chrono::seconds(actionDuration));
        });

        menu->setFixedWidth(Utils::scale_value(201));

        auto* label = new QLabel(QT_TRANSLATE_NOOP("status_popup", "Status timing"));
        label->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
        Utils::ApplyStyle(label,
                          ql1s("background: transparent; height: 52dip; padding-right: 64dip; padding-top: 20dip; padding-left: 16dip; padding-bottom: 20dip; color: %1;")
                                  .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));

        auto separator = new QWidgetAction(nullptr);
        separator->setDefaultWidget(label);
        menu->addAction(separator);

        for (const auto& time : Statuses::getTimeList())
        {
            auto action = menu->addAction(Statuses::getTimeString(time, Statuses::TimeMode::AlwaysOn));
            action->setData(QVariant::fromValue(std::chrono::duration_cast<std::chrono::seconds>(time).count()));
        }

        return menu;
    }

    Items items_;
    ItemsIndexes currentIndexes_;
    std::unordered_map<QString, int, Utils::QStringHasher> itemsEmojiIndex_;
    MouseState mouseState_;
    QTimer* leaveTimer_ = nullptr;
    QString userStatus_;
    int noStatusIndex_ = 0;
    bool filtered_ = false;
    StatusesList* q;
};

//////////////////////////////////////////////////////////////////////////
// StatusesList
//////////////////////////////////////////////////////////////////////////

StatusesList::StatusesList(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<StatusesList_p>(this))
{
    setMouseTracking(true);

    d->createItems();
    setFixedSize(windowWidth(), d->calcHeight());

    d->leaveTimer_ = new QTimer(this);
    d->leaveTimer_->setInterval(leaveTimerInterval());
    connect(d->leaveTimer_, &QTimer::timeout, this, &StatusesList::onLeaveTimer);
}

StatusesList::~StatusesList() = default;

void StatusesList::filter(const QString& _searchPattern)
{
    d->currentIndexes_.clear();
    if (!_searchPattern.isEmpty())
    {
        std::unordered_set<int> matchedIndexes;
        unsigned fixedPatternsCount = 0;
        auto searchPatterns = Utils::GetPossibleStrings(_searchPattern, fixedPatternsCount);
        for (auto i = 0u; i < fixedPatternsCount; ++i)
        {
            QString pattern;
            if (searchPatterns.empty())
            {
                pattern = _searchPattern;
            }
            else
            {
                pattern.reserve(searchPatterns.size());
                for (const auto& characterPatterns : searchPatterns)
                {
                    if (characterPatterns.size() > i)
                        pattern += characterPatterns[i];
                }
                pattern = std::move(pattern).trimmed();
            }

            for (auto j = 0u; j < d->items_.size(); ++j)
            {
                if (matchedIndexes.count(j))
                    continue;

                if (!d->items_[j]->statusCode_.isEmpty() && (d->items_[j]->description_.contains(pattern, Qt::CaseInsensitive) || d->items_[j]->statusCode_.contains(pattern, Qt::CaseInsensitive)))
                {
                    d->currentIndexes_.push_back(j);
                    matchedIndexes.insert(j);
                }
            }
        }
    }
    else
    {
        d->setDefaultIndexes();
    }

    d->filtered_ = !_searchPattern.isEmpty();
    d->updateItemsPositions(d->currentIndexes_.begin(), d->currentIndexes_.end());
    setFixedHeight(d->calcHeight());
    update();
}

void StatusesList::updateUserStatus(const Status& _status)
{
    QRect updateRect;

    // update new item
    if (!_status.isEmpty())
    {
        auto it = d->itemsEmojiIndex_.find(_status.toString());
        if (it != d->itemsEmojiIndex_.end())
        {
            auto item = d->items_[it->second];
            d->updateItemDuration(item, _status.getTimeString(), true);
            updateRect |= item->rect_;
        }

        if (!d->filtered_)
            d->addNoStatus();
    }
    else
    {
        d->removeNoStatus();
    }

    // update previous item
    if (_status.toString() != d->userStatus_ && !d->userStatus_.isEmpty())
    {
        auto it = d->itemsEmojiIndex_.find(d->userStatus_);
        if (it != d->itemsEmojiIndex_.end())
        {
            auto item = d->items_[it->second];
            d->updateItemDuration(item, getTimeString(LocalStatuses::instance()->statusDuration(item->statusCode_)), false);
            updateRect |= item->rect_;
        }
    }

    d->userStatus_ = _status.toString();

    update(updateRect);
}

int StatusesList::itemsCount() const
{
    return d->currentIndexes_.size();
}

void StatusesList::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    d->drawInRect(p, _event->rect());
}

void StatusesList::mouseMoveEvent(QMouseEvent* _event)
{
    const auto pos = _event->pos();
    QRect updateRect;
    auto indexIt = d->itemIndexIteratorAt(pos.y());
    if (d->isValidIndexIterator(indexIt))
    {
        setCursor(Qt::PointingHandCursor);
        const auto itemIndex = *indexIt;
        if (d->mouseState_.hoveredIndex_ && d->mouseState_.hoveredIndex_ != itemIndex)
            updateRect |= d->items_[*d->mouseState_.hoveredIndex_]->rect_;

        auto item = d->items_[itemIndex];
        if (item->buttonPressRect_.contains(pos))
            d->mouseState_.buttonHoveredIndex_= itemIndex;
        else
            d->mouseState_.buttonHoveredIndex_.reset();

        updateRect |= item->rect_;

        d->mouseState_.hoveredIndex_ = itemIndex;
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    update(updateRect);
}

void StatusesList::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        const auto indexIt = d->itemIndexIteratorAt(_event->pos().y());
        if (d->isValidIndexIterator(indexIt))
        {
            auto itemIndex = *indexIt;
            d->mouseState_.pressedIndex_ = itemIndex;
            auto item = d->items_[itemIndex];
            if (item->buttonPressRect_.contains(_event->pos()))
                d->mouseState_.buttonPressedIndex_ = itemIndex;
            else
                d->mouseState_.buttonPressedIndex_.reset();

            update(item->rect_);
        }
    }

    QWidget::mousePressEvent(_event);
}

void StatusesList::mouseReleaseEvent(QMouseEvent* _event)
{
    auto indexIt = d->itemIndexIteratorAt(_event->pos().y());
    QRect updateRect;

    if (d->isValidIndexIterator(indexIt))
    {
        const auto itemIndex = *indexIt;
        auto item = d->items_[itemIndex];

        if (itemIndex == d->mouseState_.buttonPressedIndex_ && item->buttonPressRect_.contains(_event->pos()))
        {
            d->showMenuAt(mapToGlobal(_event->pos()), item->statusCode_);
        }
        else if (itemIndex == d->mouseState_.pressedIndex_ && item->rect_.contains(_event->pos()))
        {
            GetDispatcher()->setStatus(item->statusCode_, LocalStatuses::instance()->statusDuration(item->statusCode_).count());
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        }

        updateRect = item->rect_;
    }

    d->mouseState_.pressedIndex_.reset();
    d->mouseState_.buttonPressedIndex_.reset();

    update(updateRect);

    QWidget::mouseReleaseEvent(_event);
}

void StatusesList::leaveEvent(QEvent* _event)
{
    d->leaveTimer_->start();
    QWidget::leaveEvent(_event);
}

void StatusesList::enterEvent(QEvent* _event)
{
    d->leaveTimer_->stop();
    QWidget::enterEvent(_event);
}

void StatusesList::onLeaveTimer()
{
    const auto pos = QCursor::pos();
    auto parent = parentWidget();
    const auto insideParent = parent && parent->rect().contains(parent->mapFromGlobal(pos));
    const auto insideRect = rect().contains(mapFromGlobal(pos));
    if (!insideParent || !insideRect)
    {
        if (d->mouseState_.hoveredIndex_)
        {
            auto hoveredIndex = *d->mouseState_.hoveredIndex_;
            auto item = d->items_[hoveredIndex];
            d->mouseState_.hoveredIndex_.reset();
            update(item->rect_);
        }

        d->leaveTimer_->stop();
    }
}

QAccessibleInterface* AccessibleStatusesList::child(int index) const
{
    if (index < 0 || index >= static_cast<int>(list_->d->currentIndexes_.size()))
        return nullptr;

    return QAccessible::queryAccessibleInterface(list_->d->items_[list_->d->currentIndexes_[index]]);
}

int AccessibleStatusesList::indexOfChild(const QAccessibleInterface* child) const
{
    auto it = std::find_if(list_->d->currentIndexes_.begin(), list_->d->currentIndexes_.end(), [this, child](int index)
    {
        return list_->d->items_[index] == child->object();
    });

    if (it == list_->d->currentIndexes_.end())
        return -1;

    return it - list_->d->currentIndexes_.begin();
}

QString AccessibleStatusesList::text(QAccessible::Text type) const
{
    return type == QAccessible::Text::Name ? qsl("AS Statuses AccessibleStatusesList") : QString();
}

}
