#include "stdafx.h"
#include "ContactDialog.h"

#include "contact_list/ContactListModel.h"
#include "history_control/HistoryControl.h"
#include "history_control/HistoryControlPage.h"
#include "gui_settings.h"

#include "utils/InterConnector.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"

namespace Ui
{
    ContactDialog::ContactDialog(QWidget* _parent)
        : BackgroundWidget(_parent)
        , IEscapeCancellable(this)
        , historyControlWidget_(new HistoryControl(this))
    {
        setFrameCountMode(FrameCountMode::_1);
        setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        auto layout = Utils::emptyVLayout(this);

        Testing::setAccessibleName(historyControlWidget_, qsl("AS Dialog historyControlWidget"));
        layout->addWidget(historyControlWidget_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &ContactDialog::removeTopWidget);

        connect(historyControlWidget_, &HistoryControl::setTopWidget, this, &ContactDialog::insertTopWidget);
        connect(historyControlWidget_, &HistoryControl::needUpdateWallpaper, this, &BackgroundWidget::updateWallpaper);

        escCancel_->addChild(historyControlWidget_);

        updateWallpaper({});
    }

    ContactDialog::~ContactDialog() = default;

    void ContactDialog::onContactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll)
    {
        historyControlWidget_->contactSelected(_aimId, _messageId, _highlights, _ignoreScroll);
    }

    void ContactDialog::initTopWidget()
    {
        if (topWidget_)
            return;

        topWidget_ = new QStackedWidget(this);
        topWidget_->setFixedHeight(Utils::getTopPanelHeight());
        Testing::setAccessibleName(topWidget_, qsl("AS Dialog topWidget"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
            vLayout->insertWidget(0, topWidget_);
    }

    void ContactDialog::cancelSelection()
    {
        im_assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::onSwitchedToEmpty()
    {
        if (topWidget_)
            topWidget_->hide();
    }

    void ContactDialog::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        if (!_widget)
            return;

        initTopWidget();

        if (!topWidgetsCache_.contains(_aimId))
        {
            topWidgetsCache_.insert(_aimId, _widget);
            Testing::setAccessibleName(_widget, qsl("AS Dialog widget"));
            topWidget_->addWidget(_widget);
        }

        im_assert(_widget == topWidgetsCache_[_aimId]);

        topWidget_->setCurrentWidget(topWidgetsCache_[_aimId]);
        topWidget_->show();
    }

    void ContactDialog::removeTopWidget(const QString& _aimId)
    {
        if (!topWidget_)
            return;

        if (auto it = std::as_const(topWidgetsCache_).find(_aimId); it != std::as_const(topWidgetsCache_).end())
        {
            topWidget_->removeWidget(it.value());
            it.value()->deleteLater();
            topWidgetsCache_.remove(_aimId);
        }

        if (!topWidget_->currentWidget())
            topWidget_->hide();
    }

    void ContactDialog::notifyApplicationWindowActive(const bool isActive)
    {
        if (historyControlWidget_)
            historyControlWidget_->notifyApplicationWindowActive(isActive);

        if (!isActive)
            Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    void ContactDialog::notifyUIActive(const bool _isActive)
    {
        if (historyControlWidget_)
            historyControlWidget_->notifyUIActive(_isActive);
    }

    void ContactDialog::setFrameCountMode(FrameCountMode _mode)
    {
        historyControlWidget_->setFrameCountMode(_mode);
    }

    FrameCountMode ContactDialog::getFrameCountMode() const
    {
        return historyControlWidget_->getFrameCountMode();
    }

    const QString& ContactDialog::currentAimId() const
    {
        return historyControlWidget_->currentAimId();
    }
}
