#include "stdafx.h"
#include "core_dispatcher.h"
#include "MicroAlert.h"
#include "../controls/CustomButton.h"
#include "../controls/DialogButton.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/GeneralDialog.h"
#include "../utils/utils.h"
#include "fonts.h"
#include "../styles/ThemeParameters.h"
#include "../media/permissions/MediaCapturePermissions.h"
#include "../media/ptt/AudioRecorder2.h"
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

    auto getSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    auto getCaption(Ui::MicroIssue _issue) noexcept
    {
        if (_issue == Ui::MicroIssue::NotFound)
            return QT_TRANSLATE_NOOP("voip_video_panel", "Microphone not found. Check that it is connected and working.");
        else if (_issue == Ui::MicroIssue::NoPermission)
            return QT_TRANSLATE_NOOP("voip_video_panel", "Allow access to microphone. Click \"Settings\" and allow application access to microphone");

        static QString empty;
        return empty;
    }
}

using namespace Ui;

MicroAlert::MicroAlert(QWidget* _parent)
#ifdef __APPLE__
    : BaseTopVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    : BaseTopVideoPanel(_parent, Qt::Widget)
#else
    : BaseTopVideoPanel(_parent)
#endif
    , microButton_(nullptr)
    , text_(nullptr)
    , settingsButton_(nullptr)
    , closeButton_(nullptr)
{
    if constexpr (!platform::is_linux())
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
    const auto minSize = getButtonSize().width() + 2 * (getMargin() + getSpacing());
    setMinimumSize(minSize, minSize);

    auto layout = Utils::emptyHLayout(this);
    layout->setAlignment(Qt::AlignVCenter);

    const auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

    microButton_ = new CustomButton(this, qsl(":/voip/no_micro"), getButtonSize(), borderColor);

    text_ = new TextEmojiWidget(this, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
    text_->setMultiline(true);
    text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    text_->installEventFilter(this);

    settingsButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("voip_video_panel", "Settings"), DialogButtonRole::CUSTOM);
    settingsButton_->setBorderColor(borderColor, borderColor, borderColor);
    settingsButton_->setTextColor(textColor, textColor, textColor);

    closeButton_ = new CustomButton(this, qsl(":/voip/close_alert"), getButtonSize(), borderColor);

    layout->setSpacing(getSpacing());
    layout->addSpacing(getSpacing());
    layout->addWidget(microButton_);
    layout->addWidget(text_);
    layout->addWidget(settingsButton_);
    layout->addWidget(closeButton_);
    layout->addSpacing(getSpacing());

    setIssue(MicroIssue::NoPermission);
    setState(MicroAlertState::Full);

    connect(microButton_, &CustomButton::clicked, this, [this]()
    {
        setState(MicroAlertState::Full);
    });

    connect(closeButton_, &CustomButton::clicked, this, [this]()
    {
        setState(MicroAlertState::Collapsed);
    });

    connect(settingsButton_, &DialogButton::clicked, this, &MicroAlert::openSettings);
}

void MicroAlert::updatePosition(const QWidget&)
{
    BaseTopVideoPanel::updatePosition(*parentWidget());
    updateSize();
}

void MicroAlert::paintEvent(QPaintEvent *_e)
{
    QWidget::paintEvent(_e);
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.fillRect(rect(), Qt::transparent);
    const auto r = Utils::scale_value(12);
    p.setPen(Qt::NoPen);
    p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION, 0.8));
    const auto newRect = rect().adjusted(getMargin(), getMargin(), -getMargin(), -getMargin());
    p.drawRoundedRect(newRect, r, r);
}

void MicroAlert::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange && isActiveWindow())
        raise();
}

bool MicroAlert::eventFilter(QObject* _o, QEvent* _e)
{
    if (_o == text_ && _e->type() == QEvent::Resize)
        updateSize();
    return QWidget::eventFilter(_o, _e);
}

void MicroAlert::setIssue(MicroIssue _issue)
{
    if (issue_ == _issue)
        return;

    issue_ = _issue;
    text_->setText(getCaption(_issue));
}

void MicroAlert::setState(MicroAlertState _state)
{
    if (state_ == _state)
        return;

    state_ = _state;
    text_->setVisible(state_ == MicroAlertState::Full);
    settingsButton_->setVisible(state_ == MicroAlertState::Full);
    closeButton_->setVisible(state_ == MicroAlertState::Full);
    updateSize();
}

void MicroAlert::openSettings() const
{
    if (issue_ == MicroIssue::NoPermission)
    {
        media::permissions::openPermissionSettings(media::permissions::DeviceType::Microphone);
    }
    else if (issue_ == MicroIssue::NotFound)
    {
        if (auto mainPage = MainPage::instance())
        {
            if (auto wnd = static_cast<MainWindow*>(mainPage->window()))
            {
                wnd->raise();
                wnd->activate();
            }
            mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        }
    }
}

void MicroAlert::updateSize()
{
    auto newHeight = (text_->height() < getHeight()) ? getHeight() + 2 * getMargin() : text_->height() + 2 * (getMargin() + getSpacing());
    setFixedHeight(state_ == MicroAlertState::Full ? newHeight : getButtonSize().width() + 2 * (getMargin() + getSpacing()));
    setFixedWidth(state_ == MicroAlertState::Full ? parentWidget()->width() : (getButtonSize().width() + 2 * (getMargin() + getSpacing())));
    microButton_->move(microButton_->x(), (height() - getButtonSize().height()) / 2);
    update();
}

void Utils::showPermissionDialog(QWidget* _parent)
{
    if (get_gui_settings()->get_value<bool>(show_microphone_request, show_microphone_request_default()))
    {
        const auto caption = QT_TRANSLATE_NOOP("popup_window", "Allow microphone access, so that interlocutor could hear you");
        const auto text = QT_TRANSLATE_NOOP("voip_video_panel", "Allow access to microphone. Click \"Settings\" and allow application access to microphone");
        const QString btnCancel = QT_TRANSLATE_NOOP("popup_window", "Don't allow");
        const QString btnConfirm = QT_TRANSLATE_NOOP("popup_window", "Settings");

        auto w = new Utils::CheckableInfoWidget(nullptr);
        w->setCheckBoxText(QT_TRANSLATE_NOOP("popup_window", "Don't show again"));
        w->setCheckBoxChecked(false);
        w->setInfoText(text, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        GeneralDialog generalDialog(w, _parent);
        generalDialog.addLabel(caption);
        generalDialog.addButtonsPair(btnCancel, btnConfirm, true);

        if (auto parentAsPanel = qobject_cast<FullVideoWindowPanel*>(_parent))
        {
            QObject::connect(parentAsPanel, &FullVideoWindowPanel::onResize, &generalDialog, &Ui::GeneralDialog::updateSize);
            QObject::connect(parentAsPanel, &FullVideoWindowPanel::aboutToHide, &generalDialog, &Ui::GeneralDialog::reject);
        }

        if (generalDialog.showInCenter())
            media::permissions::openPermissionSettings(media::permissions::DeviceType::Microphone);
        get_gui_settings()->set_value(show_microphone_request, !w->isChecked());
    }
}

ResizeDialogEventFilter::ResizeDialogEventFilter(GeneralDialog* _dialog, QObject* _parent)
    : QObject(_parent)
    , dialog_(_dialog)
{
}

bool ResizeDialogEventFilter::eventFilter(QObject* _obj, QEvent* _e)
{
    QWidget* parent = qobject_cast<QWidget*>(_obj);
    if (parent && (_e->type() == QEvent::Resize ||
                   _e->type() == QEvent::Move ||
                   _e->type() == QEvent::WindowActivate ||
                   _e->type() == QEvent::NonClientAreaMouseButtonPress ||
                   _e->type() == QEvent::ZOrderChange ||
                   _e->type() == QEvent::ShowToParent ||
                   _e->type() == QEvent::WindowStateChange ||
                   _e->type() == QEvent::UpdateRequest))
    {
        if (dialog_)
            dialog_->updateSize();
    }
    return QObject::eventFilter(_obj, _e);
}
