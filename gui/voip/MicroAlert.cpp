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
#include "../core_dispatcher.h"


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

    auto getVerticalOffset() noexcept
    {
        return Utils::scale_value(40);
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
    : QWidget(_parent)
    , microButton_(nullptr)
    , text_(nullptr)
    , settingsButton_(nullptr)
    , closeButton_(nullptr)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    const auto minSize = getButtonSize().width() + 2 * getMargin();
    setMinimumSize(minSize, minSize);

    const auto textColorKey = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
    const auto borderColorKey = textColorKey;

    microButton_ = new CustomButton(this, qsl(":/voip/no_micro"), getButtonSize(), borderColorKey);
    // workaround for buggy TextEmojiWidget::sizeHint()
    microButton_->setFixedWidth(getButtonSize().width() + 2 * getMargin() - 2 * getSpacing());

    text_ = new TextEmojiWidget(this, Fonts::appFontScaled(15), textColorKey);
    text_->setMultiline(true);
    text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    settingsButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("voip_video_panel", "Settings"), DialogButtonRole::CUSTOM);
    settingsButton_->setBorderColor(borderColorKey, borderColorKey, borderColorKey);
    settingsButton_->setTextColor(textColorKey, textColorKey, textColorKey);
    settingsButton_->setFixedWidth(settingsButton_->sizeHint().width());

    closeButton_ = new CustomButton(this, qsl(":/voip/close_alert"), getButtonSize(), borderColorKey);
    closeButton_->setFixedWidth(getButtonSize().width() + 2 * getMargin() - 2 * getSpacing());

    auto rootLayout = Utils::emptyHLayout(this);
    rootLayout->setContentsMargins(getSpacing(), 0, getSpacing(), 0);
    rootLayout->setSpacing(getSpacing());
    rootLayout->addWidget(microButton_);
    rootLayout->addWidget(text_);
    rootLayout->addWidget(settingsButton_);
    rootLayout->addWidget(closeButton_);

    setIssue(MicroIssue::NoPermission);
    setState(MicroAlertState::Expanded);

    connect(microButton_, &CustomButton::clicked, this, [this]()
    {
        setState(MicroAlertState::Expanded);
    });

    connect(closeButton_, &CustomButton::clicked, this, [this]()
    {
        setState(MicroAlertState::Collapsed);
    });

    connect(settingsButton_, &DialogButton::clicked, this, &MicroAlert::openSettings);
}

void MicroAlert::paintEvent(QPaintEvent *_e)
{
    QWidget::paintEvent(_e);
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const auto r = Utils::scale_value(12);
    p.setPen(Qt::NoPen);
    p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION, 0.8));
    p.drawRoundedRect(rect(), r, r);
}

void MicroAlert::resizeEvent(QResizeEvent* _e)
{
    // workaround for buggy TextEmojiWidget::sizeHint()
    if (state_ == MicroAlertState::Expanded)
        updateTextSize();
    QWidget::resizeEvent(_e);
}

void MicroAlert::updateTextSize()
{
    // workaround for buggy TextEmojiWidget::sizeHint()
    int w = width();
    w -= 4 * getSpacing();
    w -= getButtonSize().width();
    w -= getSpacing();
    w -= getButtonSize().width();
    w -= getSpacing();
    w -= settingsButton_->width();
    w -= getSpacing();
    text_->setMaximumWidth(w);
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
    const bool expanded = state_ == MicroAlertState::Expanded;
    text_->setVisible(expanded);
    settingsButton_->setVisible(expanded);
    closeButton_->setVisible(expanded);
    setMaximumSize(expanded ? QSize(QWIDGETSIZE_MAX, minimumHeight()) : minimumSize());
    setSizePolicy(expanded ? QSizePolicy::Expanding : QSizePolicy::Fixed,
                  expanded ? QSizePolicy::Preferred : QSizePolicy::Fixed);
}

int MicroAlert::alertOffset()
{
    return getVerticalOffset();
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
        w->setInfoText(text, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });

        GeneralDialog generalDialog(w, _parent);
        generalDialog.addLabel(caption);
        generalDialog.addButtonsPair(btnCancel, btnConfirm);

        if (auto parentAsPanel = qobject_cast<FullVideoWindowPanel*>(_parent))
        {
            QObject::connect(parentAsPanel, &FullVideoWindowPanel::onResize, &generalDialog, &Ui::GeneralDialog::updateSize);
            QObject::connect(parentAsPanel, &FullVideoWindowPanel::aboutToHide, &generalDialog, &Ui::GeneralDialog::reject);
        }

        if (generalDialog.execute())
            media::permissions::openPermissionSettings(media::permissions::DeviceType::Microphone);
        get_gui_settings()->set_value(show_microphone_request, !w->isChecked());
    }
}
