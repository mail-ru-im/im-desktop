#include "stdafx.h"
#include "GridControlPanel.h"
#include "PanelButtons.h"
#include "../utils/utils.h"
#include "fonts.h"
#include "../styles/ThemeParameters.h"
#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "gui_settings.h"

namespace
{
    auto getHeight() noexcept
    {
        return Utils::scale_value(48);
    }

    auto getButtonSize() noexcept
    {
        return QSize(32, 32);
    }

    auto getMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getPanelBackgroundColorLinux() noexcept
    {
        // color rgba(38, 38, 38, 100%) from video_panel_linux.qss
        return QColor(38, 38, 38, 255);
    }
}

using namespace Ui;

GridControlPanel::GridControlPanel(QWidget* _parent)
#ifdef __APPLE__
    : BaseTopVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    : BaseTopVideoPanel(_parent, Qt::Widget)
#else
    : BaseTopVideoPanel(_parent)
#endif
    , rootWidget_(nullptr)
    , rootLayout_(nullptr)
    , changeGridButton_(nullptr)
{
    if constexpr (!platform::is_linux())
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    auto mainLayout = Utils::emptyHLayout(this);
    mainLayout->setAlignment(Qt::AlignVCenter);

    rootWidget_ = new QWidget(this);
    // for changeGridButton_ hover
    rootWidget_->setStyleSheet(qsl("background-color: rgba(0, 0, 0, 0.01);"));

    mainLayout->addStretch(1);
    mainLayout->addWidget(rootWidget_);

    rootLayout_ = Utils::emptyHLayout(rootWidget_);
    rootLayout_->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    rootWidget_->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
    const auto minSize = getButtonSize().width() + 2 * getMargin();
    rootWidget_->setMinimumSize(minSize, minSize);
    changeGridButton_ = new GridButton(rootWidget_);
    rootLayout_->addStretch(1);
    rootLayout_->addWidget(changeGridButton_);

    connect(changeGridButton_, &GridButton::clicked, this, &GridControlPanel::onChangeConferenceMode);
}

void GridControlPanel::changeConferenceMode(voip_manager::VideoLayout _layout)
{
    videoLayout_ = _layout;
    const auto gridState = videoLayout_ == voip_manager::VideoLayout::AllTheSame ? GridButtonState::ShowBig : GridButtonState::ShowAll;
    changeGridButton_->setState(gridState);
    update();
}

void GridControlPanel::paintEvent(QPaintEvent *_e)
{
    QWidget::paintEvent(_e);
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if constexpr (platform::is_linux())
        p.fillRect(rect(), getPanelBackgroundColorLinux());
    else
        p.fillRect(rect(), Qt::transparent);
}

void GridControlPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange && isActiveWindow())
        raise();
}

void GridControlPanel::onChangeConferenceMode()
{
    auto newLayout = videoLayout_ == voip_manager::VideoLayout::AllTheSame
                    ? voip_manager::VideoLayout::OneIsBig
                    : voip_manager::VideoLayout::AllTheSame;
    videoLayout_ = newLayout;
    Q_EMIT updateConferenceMode(videoLayout_);
}
