#include "stdafx.h"

#include "CheckForUpdateButton.h"
#include "controls/TooltipWidget.h"
#include "controls/CustomButton.h"
#include "previewer/toast.h"
#include "../ConnectionWidget.h"
#include "../../utils/utils.h"
#include "../../fonts.h"
#include "styles/ThemeParameters.h"
#include "../../core_dispatcher.h"

namespace
{
    auto getIconsSize()
    {
        return QSize(32, 32);
    }

    auto getIconsColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    auto getSpinnerPenWidth()
    {
        return Utils::fscale_value(1.3);
    }

    auto getSpinnerWidth() noexcept
    {
        return Utils::fscale_value(18.0);
    }
}

namespace Ui
{
    CheckForUpdateButton::CheckForUpdateButton(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(getIconsSize()).height());
        auto layout = Utils::emptyHLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins({});

        icon_ = new CustomButton(this);
        icon_->setDefaultImage(qsl(":/controls/reload"), getIconsColor(), getIconsSize());
        icon_->setFixedSize(Utils::scale_value(getIconsSize()));
        icon_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        icon_->setCursor(Qt::ArrowCursor);

        layout->addWidget(icon_);

        animation_ = new ProgressAnimation(this);
        animation_->setProgressWidth(getSpinnerWidth());
        animation_->setFixedSize(getIconsSize());
        animation_->hide();
        layout->addWidget(animation_, Qt::AlignVCenter);

        layout->addSpacing(Utils::scale_value(4));

        const auto str = QT_TRANSLATE_NOOP("about_us", "Check for updates");
        text_ = new TextWidget(this, str);
        text_ ->setFixedHeight(Utils::scale_value(getIconsSize()).height());
        text_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        text_->applyLinks({ { str, str } });
        QObject::connect(text_, &TextWidget::linkActivated, this, &CheckForUpdateButton::onClick);
        layout->addWidget(text_);
        layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Policy::Expanding));

        QObject::connect(GetDispatcher(), &core_dispatcher::updateReady, this, &CheckForUpdateButton::onUpdateReady);
        QObject::connect(GetDispatcher(), &core_dispatcher::upToDate, this, &CheckForUpdateButton::onUpToDate);
    }

    CheckForUpdateButton::~CheckForUpdateButton() = default;

    void CheckForUpdateButton::onClick()
    {
        if (isUpdateReady_)
        {
            showUpdateReadyToast();
        }
        else if (!seq_)
        {
            startAnimation();
            seq_ = GetDispatcher()->post_message_to_core("update/check", nullptr);
        }
    }

    void CheckForUpdateButton::onUpdateReady()
    {
        isUpdateReady_ = true;
        seq_ = std::nullopt;
        stopAnimation();
        showUpdateReadyToast();
    }

    void CheckForUpdateButton::onUpToDate(int64_t _seq, bool _isNetworkError)
    {
        if (seq_ == _seq)
        {
            seq_ = std::nullopt;
            stopAnimation();
            const auto text = _isNetworkError ? QT_TRANSLATE_NOOP("about_us", "Server error") : QT_TRANSLATE_NOOP("about_us", "You have the latest version");
            Utils::showToastOverContactDialog(text);
        }
    }

    void CheckForUpdateButton::startAnimation()
    {
        icon_->hide();
        animation_->setProgressPenColor(getIconsColor());
        animation_->setProgressPenWidth(getSpinnerPenWidth());
        animation_->start();
        animation_->show();
    }

    void CheckForUpdateButton::stopAnimation()
    {
        icon_->show();
        animation_->stop();
        animation_->hide();
    }

    void CheckForUpdateButton::showUpdateReadyToast()
    {
        Utils::showToastOverContactDialog(QT_TRANSLATE_NOOP("about_us", "Update required"));
    }
}
