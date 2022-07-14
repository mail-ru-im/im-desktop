#include "stdafx.h"

#include <QStackedWidget>

#include "../containers/FriendlyContainer.h"
#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "controls/TextWidget.h"
#include "controls/LineEditEx.h"
#include "../contact_list/ContactListWidget.h"
#include "../contact_list/AbstractSearchModel.h"

#include "controls/DialogButton.h"
#include "utils/InterConnector.h"
#include "utils/gui_coll_helper.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "fonts.h"
#include "../input_widget/InputWidgetUtils.h"
#include "TaskEditWidget.h"
#include "omicron/omicron_helper.h"
#include "../common.shared/omicron_keys.h"
#include "../MainWindow.h"
#include "AssigneePopup.h"
#include "AssigneeEdit.h"
#include "../contact_list/TaskAssigneeListDelegate.h"
#include "../contact_list/TaskAssigneeModel.h"

namespace
{
    auto topOffset() noexcept
    {
        return Utils::scale_value(6);
    }

    auto contentPadding() noexcept
    {
        return Utils::scale_value(6);
    }

    auto placeholderHeight() noexcept
    {
        return Utils::scale_value(172);
    }

    constexpr auto maxPopupItemsCount() noexcept
    {
        return 5;
    }

    auto popupHeightForItemsCount(int _itemsCount) noexcept
    {
        if (_itemsCount < 1)
            return placeholderHeight();
        return Logic::TaskAssigneeListDelegate::itemHeight() * std::min(_itemsCount, maxPopupItemsCount()) + contentPadding() * 2;
    }

    auto shadowBlurRadius() noexcept
    {
        return Utils::scale_value(16);
    }

    auto shadowVerticalOffset() noexcept
    {
        return Utils::scale_value(2);
    }
} // namespace

namespace Ui
{

    class AssigneePopupPlaceholder : public QWidget
    {
    public:
        AssigneePopupPlaceholder(QWidget* _parent)
            : QWidget(_parent)
        {
            auto layout = Utils::emptyVLayout(this);
            auto text = new TextWidget(this, QT_TRANSLATE_NOOP("task_popup", "User with this name\nnot found"));
            TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(14), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } };
            params.align_ = TextRendering::HorAligment::CENTER;
            text->init(params);
            layout->addWidget(text, 0, Qt::Alignment::enum_type::AlignCenter);
        }
    };

    class AssigneePopupContent : public QWidget
    {
    public:
        AssigneePopupContent(int _width, QWidget* _parent = nullptr)
            : QWidget(_parent)
            , stack_{ new QStackedLayout(this) }
            , contactListWidget_{ new ContactListWidget(nullptr, Logic::MembersWidgetRegim::TASK_ASSIGNEE, nullptr) }
        {
            auto placeholder = new AssigneePopupPlaceholder(this);
            placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            Testing::setAccessibleName(placeholder, qsl("AS AssigneePopup placeholder"));

            stack_->addWidget(contactListWidget_);
            stack_->addWidget(placeholder);
            stack_->setCurrentIndex(0);
            connect(contactListWidget_->getSearchModel(), &Logic::AbstractSearchModel::results, this, [this]
            {
                stack_->setCurrentWidget(contactListWidget_);
            });
            connect(contactListWidget_->getSearchModel(), &Logic::AbstractSearchModel::showNoSearchResults, this, [this, placeholder]
            {
                stack_->setCurrentWidget(placeholder);
            });

            auto shadow = new QGraphicsDropShadowEffect(this);
            shadow->setBlurRadius(shadowBlurRadius());
            shadow->setOffset(0, shadowVerticalOffset());
            shadow->setColor(QColor(0, 0, 0, 255 * 0.2));
            setGraphicsEffect(shadow);
        }

        ContactListWidget* contactListWidget() const { return contactListWidget_; }

        int itemCount() { return contactListWidget_->getSearchModel()->rowCount(); }

    protected:
        void paintEvent(QPaintEvent* _event) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            const auto radius = contentPadding();
            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            p.drawRoundedRect(rect(), radius, radius);
            QWidget::paintEvent(_event);
        }

    private:
        QStackedLayout* stack_;
        ContactListWidget* contactListWidget_;
    };

    AssigneePopup::AssigneePopup(AssigneeEdit* _assigneeEdit)
        : QWidget(Utils::InterConnector::instance().getMainWindow())
        , assigneeEdit_{ _assigneeEdit }
        , content_{ new AssigneePopupContent(_assigneeEdit->width(), this) }
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &AssigneePopup::updatePosition);

        auto cl = content_->contactListWidget();
        connect(assigneeEdit_, &LineEditEx::textChanged, this, &AssigneePopup::onSearchPatternChanged);
        connect(assigneeEdit_, &LineEditEx::upArrow, cl, &ContactListWidget::searchUpPressed);
        connect(assigneeEdit_, &LineEditEx::downArrow, cl, &ContactListWidget::searchDownPressed);
        connect(assigneeEdit_, &LineEditEx::enter, cl, &ContactListWidget::searchResult);
        connect(assigneeEdit_, &LineEditEx::focusOut, this, &QWidget::hide);
        connect(assigneeEdit_, &AssigneeEdit::selectedContactChanged, this, &AssigneePopup::setSelectedContact);

        if (const auto contact = assigneeEdit_->selectedContact())
            setSelectedContact(*contact);

        connect(cl, &ContactListWidget::changeSelected, this, &AssigneePopup::selectContact);
        connect(cl->getSearchModel(), &Logic::AbstractSearchModel::results, this, &AssigneePopup::onSearchResult);
        connect(cl->getSearchModel(), &Logic::AbstractSearchModel::showNoSearchResults, this, &AssigneePopup::onSearchResult);

        Testing::setAccessibleName(this, qsl("AS AssigneePopup"));
    }

    AssigneePopup::~AssigneePopup() = default;

    void AssigneePopup::mousePressEvent(QMouseEvent* _event)
    {
        hide();
    }

    void AssigneePopup::resizeEvent(QResizeEvent* _event)
    {
        updatePosition();
        QWidget::resizeEvent(_event);
    }

    void AssigneePopup::showEvent(QShowEvent* _event)
    {
        updatePosition();
        QWidget::showEvent(_event);
    }

    void AssigneePopup::updatePosition()
    {
        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto mainWindowSize = mainWindow->size();
        const auto titleHeight = Utils::InterConnector::instance().getMainWindow()->getTitleHeight();
        setGeometry(QRect({ 0, titleHeight }, QSize(mainWindowSize.width(), mainWindowSize.height() - titleHeight)));

        auto pos = mapFromGlobal(assigneeEdit_->mapToGlobal({ 0, 0 }));
        const auto availableHeight = pos.y() - topOffset();
        pos.ry() += assigneeEdit_->height() + topOffset();

        content_->move(pos);
        onSearchResult();

        update();
    }

    void AssigneePopup::onSearchPatternChanged(const QString& _searchPattern)
    {
        if (isHidden())
            show();
        content_->contactListWidget()->searchPatternChanged(_searchPattern);
    }

    void AssigneePopup::selectContact(const QString& _aimId)
    {
        assigneeEdit_->selectContact(_aimId);
        hide();
    }

    void AssigneePopup::onSearchResult()
    {
        content_->resize(assigneeEdit_->width(), popupHeightForItemsCount(content_->itemCount()));

        QPainterPath listPath;
        const auto radius = contentPadding();
        listPath.addRoundedRect(content_->rect(), radius, radius);
        content_->setMask(listPath.toFillPolygon().toPolygon());
        update();
    }

    void AssigneePopup::setSelectedContact(const QString& _aimId)
    {
        onSearchPatternChanged({});
        auto cl = content_->contactListWidget();
        auto model = qobject_cast<Logic::TaskAssigneeModel*>(cl->getSearchModel());
        if (!model)
            return;
        model->setSelectedContact(_aimId);
    }
} // namespace Ui
