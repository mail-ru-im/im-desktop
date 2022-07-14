#include "stdafx.h"
#include "WindowController.h"
#include "VideoWindowHeader.h"
#include "styles/ThemesContainer.h"
#include "../controls/CustomButton.h"
#include "../controls/TextEmojiWidget.h"
#include "../utils/utils.h"

namespace
{
    constexpr QSize kLogoSize{ 16, 16 };
    constexpr QSize kButtonSize{ 44, 28 };
}

namespace Ui
{
    class VideoWindowHeaderPrivate
    {
    public:
        VideoWindowHeader* q;
        QLabel* logo_;
        TextEmojiWidget* title_;
        CustomButton* hideButton_;
        CustomButton* maximizeButton_;
        CustomButton* closeButton_;

        VideoWindowHeaderPrivate(VideoWindowHeader* _q) : q(_q) {}

        void createLogo()
        {
            logo_ = new QLabel(q);
            logo_->setObjectName(qsl("windowIcon"));
            logo_->setPixmap(Utils::renderSvgScaled(qsl(":/logo/logo_16"), kLogoSize));
            logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            logo_->setFocusPolicy(Qt::NoFocus);
            Testing::setAccessibleName(logo_, qsl("AS VoipWindow logo"));
        }

        void createTitle()
        {
            title_ = new TextEmojiWidget(q, Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            title_->setFocusPolicy(Qt::NoFocus);
            title_->disableFixedPreferred();
            title_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            title_->setEllipsis(true);
            title_->setText(QString());
            Testing::setAccessibleName(title_, qsl("AS VoipWindow title"));
        }

        void createButtons()
        {
            hideButton_ = new CustomButton(q, qsl(":/titlebar/minimize_button"), kButtonSize);
            Styling::Buttons::setButtonDefaultColors(hideButton_);
            hideButton_->setBackgroundNormal(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE });
            hideButton_->setBackgroundHovered(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER });
            hideButton_->setBackgroundPressed(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT });
            hideButton_->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(hideButton_, qsl("AS VoipWindow hideButton"));
            QObject::connect(hideButton_, &QPushButton::clicked, q, &VideoWindowHeader::onMinimized);

            maximizeButton_ = new CustomButton(q, qsl(":/titlebar/expand_button"), kButtonSize);
            Styling::Buttons::setButtonDefaultColors(maximizeButton_);
            maximizeButton_->setBackgroundNormal(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE });
            maximizeButton_->setBackgroundHovered(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER });
            maximizeButton_->setBackgroundPressed(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT });
            maximizeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            maximizeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            maximizeButton_->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(maximizeButton_, qsl("AS VoipWindow maximizeButton"));
            QObject::connect(maximizeButton_, &QPushButton::clicked, q, &VideoWindowHeader::onFullscreen);

            closeButton_ = new CustomButton(q, qsl(":/titlebar/close_button"), kButtonSize);
            closeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            closeButton_->setDefaultColor(Styling::Buttons::defaultColorKey());
            closeButton_->setBackgroundNormal(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE });
            closeButton_->setCursor(Qt::PointingHandCursor);
            closeButton_->setBackgroundHovered(Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION });
            closeButton_->setBackgroundPressed(Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION });
            closeButton_->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });
            closeButton_->setPressedColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });
            QObject::connect(closeButton_, &QPushButton::clicked, q, &VideoWindowHeader::requestHangup);
            Testing::setAccessibleName(closeButton_, qsl("AS VoipWindow closeButton"));
        }

        void createLayout()
        {
            auto leftLayout = Utils::emptyHLayout();
            leftLayout->addWidget(logo_);
            leftLayout->setSizeConstraint(QLayout::SetMaximumSize);
            leftLayout->addStretch(1);

            auto rightLayout = Utils::emptyHLayout();
            rightLayout->addStretch(1);
            rightLayout->addWidget(hideButton_);
            rightLayout->addWidget(maximizeButton_);
            rightLayout->addWidget(closeButton_);

            auto mainLayout = Utils::emptyHLayout(q);
            mainLayout->addLayout(leftLayout, 1);
            mainLayout->addSpacing(Utils::scale_value(8));
            mainLayout->addWidget(title_);
            mainLayout->addLayout(rightLayout, 1);
        }

        void createController(QWidget* _parent)
        {
            WindowController* controller = new WindowController(q);
            controller->setBorderWidth(0);
            controller->setOptions(WindowController::Dragging);
            controller->setWidget(_parent, q);
        }

        void setupWidget()
        {
            q->setAttribute(Qt::WA_NoSystemBackground, false);
            q->setAttribute(Qt::WA_TranslucentBackground, false);

            q->setAutoFillBackground(true);
            Utils::ApplyStyle(q, Styling::getParameters().getTitleQss());

            q->setObjectName(qsl("titleWidget"));
            q->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            Testing::setAccessibleName(q, qsl("AS VoipWindow titleWidget"));
        }

        void initUi()
        {
            createLogo();
            createTitle();
            createButtons();
            createLayout();
        }
    };

    VideoWindowHeader::VideoWindowHeader(QWidget* _parent)
        : QWidget(_parent)
        , d(std::make_unique<VideoWindowHeaderPrivate>(this))
    {
        d->setupWidget();
        d->initUi();
        d->createController(_parent);
        setTitle(_parent->windowTitle());
        updateTheme();
        connect(_parent, &QWidget::windowTitleChanged, this, &VideoWindowHeader::setTitle);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &VideoWindowHeader::updateTheme);
    }

    VideoWindowHeader::~VideoWindowHeader() = default;

    void VideoWindowHeader::setTitle(const QString& _text)
    {
        d->title_->setText(_text);
    }

    void VideoWindowHeader::mouseDoubleClickEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton &&
            _event->modifiers() == Qt::NoModifier)
        {
            Q_EMIT onFullscreen();
            return;
        }
        QWidget::mouseDoubleClickEvent(_event);
    }

    void VideoWindowHeader::updateTheme()
    {
        Utils::updateBgColor(this, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
    }
}