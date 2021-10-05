#include "stdafx.h"

#include "styles/ThemeParameters.h"
#include "controls/SimpleListWidget.h"

#include "utils/InterConnector.h"
#include "core_dispatcher.h"

#include "fonts.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../ContactDialog.h"
#include "TaskStatusPopup.h"

namespace
{
    auto contentPadding() noexcept
    {
        return Utils::scale_value(6);
    }

    auto shadowBlurRadius() noexcept
    {
        return Utils::scale_value(16);
    }

    auto shadowVerticalOffset() noexcept
    {
        return Utils::scale_value(2);
    }

    auto textOffset() noexcept
    {
        return Utils::scale_value(20);
    }

    auto itemHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    QSize statusItemSize() noexcept
    {
        return { Utils::scale_value(120), itemHeight() };
    }

    auto contentVerticalOffset() noexcept
    {
        return Utils::scale_value(4);
    }
}

namespace Ui
{
    class StatusItem : public SimpleListItem
    {
    public:
        StatusItem(core::tasks::status _status, bool _selected, QWidget* _parent = nullptr)
            : SimpleListItem(_parent)
            , status_{_status }
            , selected_{ _selected }
        {
            description_ = TextRendering::MakeTextUnit(Data::TaskData::statusDescription(status_));
            description_->init(Fonts::appFontScaled(15, _selected ? Fonts::FontWeight::Bold : Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            description_->setOffsets(textOffset(), itemHeight() / 2);
            description_->evaluateDesiredSize();

            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            setCursor(Qt::PointingHandCursor);
        }

        bool isSelected() const override { return selected_; }

        core::tasks::status status() const { return status_; }

        QSize sizeHint() const override
        {
            return statusItemSize();
        }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
            description_->draw(p, TextRendering::VerPosition::MIDDLE);
        }

    private:
        core::tasks::status status_;
        TextRendering::TextUnitPtr description_;
        bool selected_;
    };


    class TaskStatusPopupContent : public QWidget
    {
    public:
        TaskStatusPopupContent(const Data::TaskData& _taskToEdit, QWidget* _parent = nullptr)
            : QWidget(_parent)
            , list_{ new SimpleListWidget(Qt::Vertical, this) }
            , task_{ _taskToEdit }
        {
            for (auto status : task_.params_.allowedStatuses_)
            {
                auto item = new StatusItem(status, _taskToEdit.params_.status_ == status, list_);
                list_->addItem(item);
                Testing::setAccessibleName(item, qsl("AS TaskStatusPopup ") + QString::fromStdString(std::string(core::tasks::status_to_string(status))));
            }

            list_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
            auto layout = Utils::emptyVLayout(this);
            layout->addWidget(list_);

            auto shadow = new QGraphicsDropShadowEffect(this);
            shadow->setBlurRadius(shadowBlurRadius());
            shadow->setOffset(0, shadowVerticalOffset());
            shadow->setColor(QColor(0, 0, 0, 255 * 0.2));
            setGraphicsEffect(shadow);
        }

    protected:
        void mousePressEvent(QMouseEvent* _event) override {}
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

    public:
        SimpleListWidget* list_;
        Data::TaskData task_;
    };


    TaskStatusPopup::TaskStatusPopup(const Data::TaskData& _taskToEdit, const QRect& _statusBubbleRect)
        : QWidget(Utils::InterConnector::instance().getMainWindow())
        , content_{ new TaskStatusPopupContent(_taskToEdit, this) }
    {
        setAttribute(Qt::WA_DeleteOnClose);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &TaskStatusPopup::onWindowResized);

        connect(content_->list_, &SimpleListWidget::clicked, this, [this](int _index)
        {
            close();
            auto item = static_cast<StatusItem*>(content_->list_->itemAt(_index));
            if (item->isSelected())
                return;
            auto clickedStatus = item->status();
            Data::TaskChange taskChange{ content_->task_.id_, clickedStatus };
            GetDispatcher()->editTask(taskChange);
        });

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto mainWindowSize = mainWindow->size();
        const auto titleHeight = Utils::InterConnector::instance().getMainWindow()->getTitleHeight();
        setGeometry(QRect({ 0, titleHeight }, QSize(mainWindowSize.width(), mainWindowSize.height() - titleHeight)));

        const auto statusBubbleY = _statusBubbleRect.y();
        auto h = height();
        const auto placeBottom = (h - statusBubbleY) > (statusBubbleY - titleHeight);
        auto ch = content_->sizeHint().height();
        const auto contentY = placeBottom  ? statusBubbleY + _statusBubbleRect.height() + contentVerticalOffset() : statusBubbleY - ch - contentVerticalOffset();
        const auto contentX = _statusBubbleRect.x();

        content_->move({ contentX, contentY });

        if (auto mp = MainPage::instance())
            mp->getContactDialog()->escCancel_->addChild(this, [this] { close(); });
    }

    TaskStatusPopup::~TaskStatusPopup()
    {
        parentWidget()->setFocus();
        if (auto mp = MainPage::instance())
            mp->getContactDialog()->escCancel_->removeChild(this);
    }

    void TaskStatusPopup::setFocus()
    {
        QWidget::setFocus();
        content_->setFocus();
    }

    void TaskStatusPopup::mousePressEvent(QMouseEvent* _event)
    {
        close();
    }

    void TaskStatusPopup::keyPressEvent(QKeyEvent* _event)
    {
        close();
    }

    void TaskStatusPopup::onWindowResized()
    {
        close();
    }

    void TaskStatusPopup::onSidebarClosed()
    {
        close();
    }

}
