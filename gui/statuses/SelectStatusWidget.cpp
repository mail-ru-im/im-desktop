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
#include "CustomStatusWidget.h"
#include "StatusDurationWidget.h"
#include "StatusDurationMenu.h"
#include "StatusCommonUi.h"
#include "Status.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "cache/emoji/Emoji.h"
#include "controls/TextUnit.h"
#include "controls/TooltipWidget.h"
#include "controls/CustomButton.h"
#include "controls/GeneralDialog.h"
#include "styles/ThemeParameters.h"
#include "SelectedStatusWidget.h"
#include "core_dispatcher.h"
#include "my_info.h"
#include "fonts.h"

namespace
{
    constexpr auto leaveTimerInterval() noexcept { return std::chrono::milliseconds(50); }
    constexpr auto statusTimeUpdateTimeInterval() noexcept { return std::chrono::milliseconds(1000); }
    constexpr auto heightAnimationDuration() noexcept { return std::chrono::milliseconds(150); }

    int listWidth()
    {
        return Utils::scale_value(360);
    }

    int32_t statusContentSideMargin()
    {
        return Utils::scale_value(16);
    }

    double listHeightWindowHeightRatio()
    {
        return 0.8;
    }

    QSize itemSize()
    {
        return QSize(listWidth(), Utils::scale_value(44));
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
        if constexpr (platform::is_apple())
            return Utils::scale_value(12);

        return Utils::scale_value(10);
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

    QFont selectStatusFont()
    {
        return Fonts::appFontScaled(13, Fonts::FontWeight::SemiBold);
    }

    QColor selectStatusFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QFont searchLabelFont()
    {
        return Fonts::appFontScaled(15);
    }

    QColor searchLabelFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
    }

    QColor searchLabelFontColorHovered()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER);
    }

    QColor searchLabelFontColorPressed()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE);
    }

    int twoLineTextTopMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(6);

        return Utils::scale_value(4);
    }

    int twoLineTextBottomMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(2);

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

    QImage customStatusIcon()
    {
        return Utils::renderSvg(qsl(":/smile"), Utils::scale_value(QSize(34, 34)), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)).toImage();
    }

    QSize placeholderSize()
    {
        return Utils::scale_value(QSize(84, 84));
    }

    QColor placeholderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    QFont headerLabelFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(22, Fonts::FontWeight::Medium);

        return Fonts::appFontScaled(22);
    }

    int avatarTopMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(6);

        return Utils::scale_value(12);
    }

    int labelsWidgetMaximumHeight()
    {
        return Utils::scale_value(40);
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
    SelectStatusWidget_p(SelectStatusWidget* _q) : q(_q) {}

    void startAnimation(QPropertyAnimation::Direction _direction)
    {
        auto isForward = _direction == QPropertyAnimation::Forward;

        const auto topWidgetStartHeight = isForward ? topWidget_->height() : 0;
        const auto topWidgetEndHeight = isForward ? 0 : topWidget_->sizeHint().height();

        auto heightEasing = QEasingCurve::InOutSine;

        topWidget_->setMaximumHeight(topWidgetStartHeight);
        auto topWidgetHeightAnimation = new QPropertyAnimation(topWidget_, QByteArrayLiteral("maximumHeight"));
        topWidgetHeightAnimation->setStartValue(topWidgetStartHeight);
        topWidgetHeightAnimation->setEndValue(topWidgetEndHeight);
        topWidgetHeightAnimation->setEasingCurve(heightEasing);
        topWidgetHeightAnimation->setDuration(heightAnimationDuration().count());
        topWidgetHeightAnimation->start(QAbstractAnimation::DeleteWhenStopped);

        auto opacityEasing = QEasingCurve::InQuint;

        auto topWidgetOpacityAnimation = new QPropertyAnimation(topWidgetOpacity_, QByteArrayLiteral("opacity"));
        topWidgetOpacityAnimation->setStartValue(0);
        topWidgetOpacityAnimation->setEndValue(1);
        topWidgetOpacityAnimation->setEasingCurve(opacityEasing);
        topWidgetOpacityAnimation->setDirection(isForward ? QPropertyAnimation::Backward : QPropertyAnimation::Forward);

        topWidgetOpacityAnimation->setDuration(heightAnimationDuration().count());
        topWidgetOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);

        searchContent_->setVisible(true);

        const auto searchContentStartHeight = isForward ? 0 : searchContent_->maximumHeight();
        const auto searchContentEndHeight = isForward ? topWidget_->sizeHint().height() : 0;

        searchContent_->setMaximumHeight(searchContentStartHeight);
        auto searchContentHeightAnimation = new QPropertyAnimation(searchContent_, QByteArrayLiteral("maximumHeight"));
        searchContentHeightAnimation->setStartValue(searchContentStartHeight);
        searchContentHeightAnimation->setEndValue(searchContentEndHeight);
        searchContentHeightAnimation->setEasingCurve(heightEasing);
        searchContentHeightAnimation->setDuration(heightAnimationDuration().count());
        searchContentHeightAnimation->start(QAbstractAnimation::DeleteWhenStopped);

        QObject::connect(searchContentHeightAnimation, &QPropertyAnimation::finished, q, [this, _direction]()
        {
            if (_direction == QPropertyAnimation::Backward)
                searchContent_->setVisible(false);
        });

        auto searchContentOpacityAnimation = new QPropertyAnimation(searchContentOpacity_, QByteArrayLiteral("opacity"));
        searchContentOpacityAnimation->setStartValue(0);
        searchContentOpacityAnimation->setEndValue(1);
        searchContentOpacityAnimation->setEasingCurve(opacityEasing);
        searchContentOpacityAnimation->setDirection(isForward ? QPropertyAnimation::Forward : QPropertyAnimation::Backward);
        searchContentOpacityAnimation->setDuration(heightAnimationDuration().count());
        searchContentOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);

        const auto labelsWidgetStartHeight = isForward ? labelsWidget_->height() : 0;
        const auto labelsWidgetEndHeight = isForward ? 0 : labelsWidgetMaximumHeight();

        auto labelsWidgetHeightAnimation = new QPropertyAnimation(labelsWidget_, QByteArrayLiteral("maximumHeight"));
        labelsWidgetHeightAnimation->setStartValue(labelsWidgetStartHeight);
        labelsWidgetHeightAnimation->setEndValue(labelsWidgetEndHeight);
        labelsWidgetHeightAnimation->setEasingCurve(heightEasing);
        labelsWidgetHeightAnimation->setDuration(heightAnimationDuration().count());
        labelsWidgetHeightAnimation->start(QAbstractAnimation::DeleteWhenStopped);

        auto labelsWidgetOpacityAnimation = new QPropertyAnimation(labelsWidgetOpacity_, QByteArrayLiteral("opacity"));
        labelsWidgetOpacityAnimation->setStartValue(0);
        labelsWidgetOpacityAnimation->setEndValue(1);
        labelsWidgetOpacityAnimation->setEasingCurve(opacityEasing);
        labelsWidgetOpacityAnimation->setDirection(isForward ? QPropertyAnimation::Backward : QPropertyAnimation::Forward);
        labelsWidgetOpacityAnimation->setDuration(heightAnimationDuration().count());
        labelsWidgetOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void animateSearchMode()
    {
        startAnimation(QPropertyAnimation::Forward);
    }
    void animateCancelSearchMode()
    {
        startAnimation(QPropertyAnimation::Backward);
    }

    void showDurationMenuAt(QPoint _pos)
    {
        auto menu = createMenu(status_);

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto menuHeight = menu->sizeHint().height();
        if (mainWindow && _pos.y() + menuHeight > mainWindow->geometry().bottom())
            _pos.ry() -= menuHeight;
        _pos.rx() -= menu->width();
        menu->move(_pos);
        menu->show();
    }

    QMenu* createMenu(const Status& _status)
    {
        auto menu = new StatusDurationMenu(q);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        Testing::setAccessibleName(menu, qsl("AS SelectStatusWidget popupMenu"));

        QObject::connect(menu, &StatusDurationMenu::durationClicked, q, [_status](std::chrono::seconds _duration)
        {
            GetDispatcher()->setStatus(_status.toString(), _duration.count(), _status.isCustom() ? _status.getDescription() : QString());
            if (!_status.isCustom())
                LocalStatuses::instance()->setDuration(_status.toString(), _duration);
        });

        QObject::connect(menu, &StatusDurationMenu::customTimeClicked, q, [this, _status]()
        {
            q->showCustomDurationDialog(_status.toString(), _status.isCustom() ? _status.getDescription() : QString());
        });

        return menu;
    }

    QWidget* statusSelectionContent_ = nullptr;
    CustomStatusWidget* customStatusWidget_ = nullptr;
    StatusDurationWidget* statusDurationWidget_ = nullptr;
    StatusesList* list_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    SearchWidget* searchWidget_ = nullptr;
    CustomButton* searchLabel_ = nullptr;
    CustomButton* cancelSearchLabel_ = nullptr;
    CustomButton* selectStatusLabel_ = nullptr;
    QTimer* statusTimeTimer_ = nullptr;
    QWidget* placeholder_ = nullptr;
    QWidget* labelsWidget_ = nullptr;
    QWidget* searchContent_ = nullptr;
    QWidget* topWidget_ = nullptr;
    SelectedStatusWidget* statusWidget_ = nullptr;
    QGraphicsOpacityEffect* searchContentOpacity_ = nullptr;
    QGraphicsOpacityEffect* labelsWidgetOpacity_ = nullptr;
    QGraphicsOpacityEffect* topWidgetOpacity_ = nullptr;
    Status status_;
    SelectStatusWidget* q;
};

//////////////////////////////////////////////////////////////////////////
// SelectStatusWidget
//////////////////////////////////////////////////////////////////////////

SelectStatusWidget::SelectStatusWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<SelectStatusWidget_p>(this))
{
    auto layout = Utils::emptyVLayout(this);

    d->statusSelectionContent_ = new QWidget(this);
    auto statusSelectionContentHContent = Utils::emptyHLayout(d->statusSelectionContent_);
    const auto maxHeight = Ui::ItemLength(false, listHeightWindowHeightRatio(), 0, Utils::InterConnector::instance().getMainWindow());
    auto statusSelectionContentVLayout = Utils::emptyVLayout();
    statusSelectionContentHContent->addLayout(statusSelectionContentVLayout);
    statusSelectionContentHContent->addSpacerItem(new QSpacerItem(0, maxHeight));

    auto label = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "My status"));
    label->init(headerLabelFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    label->setMaxWidthAndResize(listWidth());

    d->status_ = Logic::GetStatusContainer()->getStatus(MyInfo()->aimId());

    d->topWidget_ = new QWidget(this);
    auto topWidgetLayout = Utils::emptyVLayout(d->topWidget_);

    d->topWidgetOpacity_ = new QGraphicsOpacityEffect(d->topWidget_);
    d->topWidgetOpacity_->setOpacity(1.);
    d->topWidget_->setGraphicsEffect(d->topWidgetOpacity_);

    d->statusWidget_ = new SelectedStatusWidget(this);
    Testing::setAccessibleName(d->statusWidget_, qsl("AS SelectStatusWidget SelectedStatusWidget"));

    topWidgetLayout->addSpacing(Utils::scale_value(14));
    topWidgetLayout->addWidget(label);
    topWidgetLayout->addSpacing(avatarTopMargin());
    topWidgetLayout->addWidget(d->statusWidget_);

    statusSelectionContentVLayout->addWidget(d->topWidget_);

    d->searchContent_ = new QWidget(this);
    auto searchContentWidgetVLayout = Utils::emptyVLayout(d->searchContent_);
    auto searchContentWidgetHLayout = Utils::emptyHLayout();

    d->searchContentOpacity_ = new QGraphicsOpacityEffect(d->searchContent_);
    d->searchContentOpacity_->setOpacity(1.);
    d->searchContent_->setGraphicsEffect(d->searchContentOpacity_);

    auto searchLabel = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "Status search"));
    searchLabel->init(headerLabelFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    searchLabel->setMaxWidthAndResize(listWidth());

    searchContentWidgetVLayout->addSpacing(Utils::scale_value(16));
    searchContentWidgetVLayout->addWidget(searchLabel);
    searchContentWidgetVLayout->addSpacing(Utils::scale_value(20));
    searchContentWidgetVLayout->addLayout(searchContentWidgetHLayout);

    d->cancelSearchLabel_ = new CustomButton(d->searchContent_);
    d->cancelSearchLabel_->setFont(searchLabelFont());
    d->cancelSearchLabel_->setNormalTextColor(searchLabelFontColor());
    d->cancelSearchLabel_->setHoveredTextColor(searchLabelFontColorHovered());
    d->cancelSearchLabel_->setPressedTextColor(searchLabelFontColorPressed());
    d->cancelSearchLabel_->setText(QT_TRANSLATE_NOOP("status_popup", "Cancel"));
    d->searchWidget_ = new SearchWidget(this, Utils::scale_value(4));
    d->searchWidget_->setPlaceholderText(QString());
    Testing::setAccessibleName(d->searchWidget_, qsl("AS SelectStatusWidget searchWidget"));

    searchContentWidgetHLayout->addWidget(d->searchWidget_);
    searchContentWidgetHLayout->addWidget(d->cancelSearchLabel_);
    searchContentWidgetHLayout->addSpacing(statusContentSideMargin());
    d->searchContent_->setVisible(false);

    statusSelectionContentVLayout->addWidget(d->searchContent_);

    connect(d->searchWidget_, &SearchWidget::search, this, [this](const QString& _pattern)
    {
        d->list_->filter(_pattern);
        const auto noResults = d->list_->itemsCount() == 0;
        d->scrollArea_->setVisible(!noResults);
        d->placeholder_->setVisible(noResults);
    });
    connect(d->searchWidget_, &SearchWidget::escapePressed, this, &SelectStatusWidget::onCancelSeachLabelClicked);

    d->labelsWidget_ = new QWidget(this);
    d->labelsWidget_->setMaximumHeight(labelsWidgetMaximumHeight());
    d->labelsWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    auto labelsWidgetVLayout = Utils::emptyVLayout(d->labelsWidget_);
    auto labelsWidgetHLayout = Utils::emptyHLayout();

    labelsWidgetVLayout->addLayout(labelsWidgetHLayout);

    d->labelsWidgetOpacity_ = new QGraphicsOpacityEffect(d->labelsWidget_);
    d->labelsWidgetOpacity_->setOpacity(1.);
    d->labelsWidget_->setGraphicsEffect(d->labelsWidgetOpacity_);

    d->selectStatusLabel_ = new CustomButton(d->labelsWidget_);
    d->selectStatusLabel_->setFont(selectStatusFont());
    d->selectStatusLabel_->setNormalTextColor(selectStatusFontColor());
    d->selectStatusLabel_->setText(QT_TRANSLATE_NOOP("status_popup", "SELECT STATUS"));
    d->selectStatusLabel_->setFixedWidth(QFontMetrics(selectStatusFont()).width(d->selectStatusLabel_->text()));
    d->selectStatusLabel_->setEnabled(false);

    d->searchLabel_ = new CustomButton(d->labelsWidget_);
    d->searchLabel_->setFont(searchLabelFont());
    d->searchLabel_->setNormalTextColor(searchLabelFontColor());
    d->searchLabel_->setHoveredTextColor(searchLabelFontColorHovered());
    d->searchLabel_->setPressedTextColor(searchLabelFontColorPressed());
    d->searchLabel_->setText(QT_TRANSLATE_NOOP("status_popup", "Search"));
    d->searchLabel_->setFixedWidth(QFontMetrics(searchLabelFont()).width(d->searchLabel_->text()));
    Testing::setAccessibleName(d->searchLabel_, qsl("AS SelectStatusWidget searchButton"));

    connect(d->searchLabel_, &CustomButton::clicked, this, &SelectStatusWidget::onSeachLabelClicked);
    connect(d->cancelSearchLabel_, &CustomButton::clicked, this, &SelectStatusWidget::onCancelSeachLabelClicked);
    labelsWidgetHLayout->addSpacing(statusContentSideMargin());
    labelsWidgetHLayout->addWidget(d->selectStatusLabel_, Qt::AlignVCenter);
    labelsWidgetHLayout->addStretch();
    labelsWidgetHLayout->addWidget(d->searchLabel_, Qt::AlignVCenter);
    labelsWidgetHLayout->addSpacing(statusContentSideMargin());

    statusSelectionContentVLayout->addSpacing(Utils::scale_value(8));
    statusSelectionContentVLayout->addWidget(d->labelsWidget_);
    statusSelectionContentVLayout->addSpacing(Utils::scale_value(4));

    auto listContentWidget = new QWidget(this);
    auto listContentLayout = Utils::emptyVLayout(listContentWidget);

    d->scrollArea_ = Ui::CreateScrollAreaAndSetTrScrollBarV(this);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));

    d->list_ = new StatusesList(d->scrollArea_);
    d->scrollArea_->setWidget(d->list_);

    d->statusTimeTimer_ = new QTimer(this);
    d->statusTimeTimer_->setInterval(statusTimeUpdateTimeInterval());
    connect(d->statusTimeTimer_, &QTimer::timeout, this, [this]() { d->statusWidget_->setStatus(d->status_); });
    connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, &SelectStatusWidget::onStatusChanged);

    d->list_->updateUserStatus(d->status_);
    d->statusWidget_->setStatus(d->status_);
    d->statusTimeTimer_->start();

    d->placeholder_ = new QWidget(this);
    Testing::setAccessibleName(d->placeholder_, qsl("AS SelectStatusWidget placeholder"));
    auto placeholderLayout = Utils::emptyVLayout(d->placeholder_);
    auto noResultsIcon = new QLabel(d->placeholder_);
    noResultsIcon->setPixmap(Utils::renderSvg(qsl(":/placeholders/empty_search"), placeholderSize(), placeholderColor()));
    auto noResultsText = new TextWidget(this, QT_TRANSLATE_NOOP("placeholders", "Nothing found"));
    noResultsText->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    noResultsText->setMaxWidthAndResize(listWidth());
    placeholderLayout->addStretch();
    placeholderLayout->addWidget(noResultsIcon, 0, Qt::AlignCenter);
    placeholderLayout->addSpacing(Utils::scale_value(24));
    placeholderLayout->addWidget(noResultsText);
    placeholderLayout->addStretch();
    d->placeholder_->setVisible(false);

    listContentLayout->addWidget(d->placeholder_);
    listContentLayout->addWidget(d->scrollArea_);
    statusSelectionContentVLayout->addWidget(listContentWidget);

    auto closeButton = createButton(this, QT_TRANSLATE_NOOP("status_popup", "Close"), DialogButtonRole::CANCEL);
    Testing::setAccessibleName(closeButton, qsl("AS SelectStatusWidget closeButton"));
    connect(closeButton, &DialogButton::clicked, this, [this]()
    {
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({});
        close();
    });

    statusSelectionContentVLayout->addSpacing(Utils::scale_value(20));
    statusSelectionContentVLayout->addWidget(closeButton, 0, Qt::AlignHCenter);
    statusSelectionContentVLayout->addSpacing(Utils::scale_value(16));

    connect(d->statusWidget_, &SelectedStatusWidget::avatarClicked, this, &SelectStatusWidget::onAvatarClicked);
    connect(d->statusWidget_, &SelectedStatusWidget::statusClicked, this, [this]()
    {
        if (!d->status_.isEmpty())
            d->customStatusWidget_->setStatus(d->status_);
        showCustomStatusDialog();
    });
    connect(d->list_, &StatusesList::customStatusClicked, this, [this]()
    {
        d->customStatusWidget_->setStatus({});
        showCustomStatusDialog();
    });

    layout->addWidget(d->statusSelectionContent_);

    d->customStatusWidget_ = new CustomStatusWidget(this);
    d->customStatusWidget_->setVisible(false);
    Testing::setAccessibleName(d->customStatusWidget_, qsl("AS Statuses CustomStatusWidget"));
    d->statusDurationWidget_ = new StatusDurationWidget(this);
    d->statusDurationWidget_->setVisible(false);
    Testing::setAccessibleName(d->statusDurationWidget_, qsl("AS Statuses StatusDurationWidget"));

    connect(d->customStatusWidget_, &CustomStatusWidget::backClicked, this, &SelectStatusWidget::showMainDialog);
    connect(d->statusDurationWidget_, &StatusDurationWidget::backClicked, this, &SelectStatusWidget::showMainDialog);
    connect(d->list_, &StatusesList::customTimeClicked, this, [this](const QString& _status)
    {
        showCustomDurationDialog(_status);
    });
    connect(d->statusWidget_, &SelectedStatusWidget::durationClicked, this, [this]()
    {
        d->showDurationMenuAt(QCursor::pos());
    });

    layout->addWidget(d->customStatusWidget_);
    layout->addWidget(d->statusDurationWidget_);
}

SelectStatusWidget::~SelectStatusWidget() = default;

void SelectStatusWidget::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape)
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());

    QWidget::keyPressEvent(_event);
}

void SelectStatusWidget::onStatusChanged(const QString& _contactId)
{
    if (_contactId != MyInfo()->aimId())
        return;

    d->status_ = Logic::GetStatusContainer()->getStatus(MyInfo()->aimId());
    d->list_->updateUserStatus(d->status_);
    d->statusWidget_->setStatus(d->status_);

    if (d->topWidget_->maximumHeight() > 0)
        d->topWidget_->setMaximumHeight(d->topWidget_->sizeHint().height());
}

void SelectStatusWidget::onSeachLabelClicked()
{
    d->animateSearchMode();
    d->searchWidget_->setFocus();
}

void SelectStatusWidget::onCancelSeachLabelClicked()
{
    d->searchWidget_->clearInput();
    d->animateCancelSearchMode();
    setFocus();
}

void SelectStatusWidget::onAvatarClicked()
{
    if (auto mainPage = MainPage::instance())
    {
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({});
        mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Profile);
    }
}

void SelectStatusWidget::showMainDialog()
{
    d->statusSelectionContent_->setVisible(true);
    d->statusDurationWidget_->setVisible(false);
    d->customStatusWidget_->setVisible(false);
}

void SelectStatusWidget::showCustomStatusDialog()
{
    d->statusSelectionContent_->setVisible(false);
    d->customStatusWidget_->setVisible(true);
}

void SelectStatusWidget::showCustomDurationDialog(const QString& _status, const QString& _description)
{
    d->statusDurationWidget_->setStatus(_status, _description);
    d->statusSelectionContent_->setVisible(false);
    d->statusDurationWidget_->setVisible(true);
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
            item->emoji_ = customStatusIcon();

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
        auto customStatus = createItem(QString(), QT_TRANSLATE_NOOP("status_popup", "Select custom status"), std::chrono::seconds::zero());
        customStatusIndex_ = items_.size();
        items_.push_back(customStatus);

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
        currentIndexes_.resize(items_.size());
        std::iota(currentIndexes_.begin(), currentIndexes_.end(), 0);

        if (!Features::isCustomStatusEnabled())
            hideItem(customStatusIndex_);

        if (hiddenIndex_)
            hideItem(*hiddenIndex_);
    }

    void hideItem(int _index)
    {
        auto it = std::lower_bound(currentIndexes_.begin(), currentIndexes_.end(), _index);
        if (it != currentIndexes_.end() && *it == _index)
            currentIndexes_.erase(it);
    }

    void resetMouseState()
    {
        mouseState_ = MouseState();
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
        auto menu = new StatusDurationMenu(q);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(menu, &StatusDurationMenu::durationClicked, q, [_status](std::chrono::seconds _duration)
        {
            GetDispatcher()->setStatus(_status, _duration.count());
            LocalStatuses::instance()->setDuration(_status, _duration);
        });

        QObject::connect(menu, &StatusDurationMenu::customTimeClicked, q, [this, _status]()
        {
            Q_EMIT q->customTimeClicked(_status, StatusesList::QPrivateSignal());
        });

        return menu;
    }

    Items items_;
    ItemsIndexes currentIndexes_;
    std::unordered_map<QString, int, Utils::QStringHasher> itemsEmojiIndex_;
    MouseState mouseState_;
    QTimer* leaveTimer_ = nullptr;
    std::optional<int> hiddenIndex_;
    StatusesList* q;
    int customStatusIndex_ = 0;
    bool filtered_ = false;
    bool customStatus_ = false;
};

//////////////////////////////////////////////////////////////////////////
// StatusesList
//////////////////////////////////////////////////////////////////////////

StatusesList::StatusesList(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<StatusesList_p>(this))
{
    setMouseTracking(true);

    d->createItems();
    setFixedSize(listWidth(), d->calcHeight());

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

            const int size = int(d->items_.size());
            for (auto j = 0; j < size; ++j)
            {
                if (matchedIndexes.count(j) || d->hiddenIndex_ && j == *d->hiddenIndex_)
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
    // update new item
    d->hiddenIndex_.reset();
    if (!_status.isEmpty())
    {
        if (_status.isCustom())
        {
            d->hiddenIndex_ = d->customStatusIndex_;
        }
        else
        {
            auto it = d->itemsEmojiIndex_.find(_status.toString());
            if (it != d->itemsEmojiIndex_.end())
                d->hiddenIndex_ = it->second;
        }
    }

    if (!d->filtered_)
    {
        d->setDefaultIndexes();
        d->updateItemsPositions(d->currentIndexes_.begin(), d->currentIndexes_.end());
        setFixedHeight(d->calcHeight());
    }

    update();
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

        if (itemIndex == d->customStatusIndex_)
        {
            Q_EMIT customStatusClicked(QPrivateSignal());
        }
        else if (itemIndex == d->mouseState_.buttonPressedIndex_ && item->buttonPressRect_.contains(_event->pos()))
        {
            d->showMenuAt(mapToGlobal(_event->pos()), item->statusCode_);
        }
        else if (itemIndex == d->mouseState_.pressedIndex_ && item->rect_.contains(_event->pos()))
        {
            auto seconds = LocalStatuses::instance()->statusDuration(item->statusCode_);
            GetDispatcher()->setStatus(item->statusCode_, seconds.count());
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());

            Statuses::showToastWithDuration(seconds);
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
