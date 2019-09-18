#include "Placeholders.h"

#include "stdafx.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/DialogButton.h"

#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../styles/ThemeParameters.h"

using namespace Ui;

RecentsPlaceholder::RecentsPlaceholder(QWidget *_parent)
    : QWidget(_parent)
    , isPicrureOnly_(false)
{

    auto mainLayout = Utils::emptyVLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    noRecentsWidget_ = new QWidget(this);
    auto noRecentsLayout = new QVBoxLayout(noRecentsWidget_);
    noRecentsWidget_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    noRecentsLayout->setAlignment(Qt::AlignCenter);
    noRecentsLayout->setContentsMargins(0, 0, 0, 0);
    noRecentsLayout->setSpacing(Utils::scale_value(20));

    if (build::is_biz() || build::is_dit())
    {
        auto noRecentsPlaceholder = new QLabel(this);
        auto pixmap = QPixmap(Utils::parse_image_name(qsl(":/placeholders/il_search_100")));
        Utils::check_pixel_ratio(pixmap);
        noRecentsPlaceholder->setPixmap(pixmap);
        Testing::setAccessibleName(noRecentsPlaceholder, qsl("AS noRecentsPlaceholder"));
        noRecentsLayout->addWidget(noRecentsPlaceholder, 0, Qt::AlignCenter);
    }
    else
    {
        auto noRecentsPlaceholder = new QWidget(noRecentsWidget_);
        noRecentsPlaceholder->setObjectName(qsl("noRecents"));
        noRecentsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        noRecentsPlaceholder->setFixedHeight(Utils::scale_value(160));

        noRecentsLayout->addWidget(noRecentsPlaceholder);
        Testing::setAccessibleName(noRecentsPlaceholder, qsl("AS noRecentsPlaceholder"));
    }

    noRecentsLabel_ = new LabelEx(noRecentsWidget_);
    noRecentsLabel_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    noRecentsLabel_->setFont(Fonts::appFontScaled(15));
    noRecentsLabel_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    noRecentsLabel_->setAlignment(Qt::AlignCenter);
    if (build::is_dit() || build::is_biz())
        originalLabel_ = QT_TRANSLATE_NOOP("placeholders", "Find a colleague by email and write to him");
    else
        originalLabel_ = QT_TRANSLATE_NOOP("placeholders", "You have no opened chats yet");
    noRecentsLabel_->setText(originalLabel_);
    noRecentsLayout->addWidget(noRecentsLabel_);
    Testing::setAccessibleName(noRecentsLabel_, qsl("AS noRecentsLabel"));

    if (build::is_dit() || build::is_biz())
    {
        auto button = new DialogButton(noRecentsWidget_, QT_TRANSLATE_NOOP("placeholders", "Find"), DialogButtonRole::CONFIRM);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->adjustSize();
        auto hLayout = Utils::emptyHLayout();
        hLayout->setAlignment(Qt::AlignCenter);
        hLayout->addWidget(button);
        noRecentsLayout->addLayout(hLayout);
        connect(button, &Ui::DialogButton::clicked, this, [](){
            emit Utils::InterConnector::instance().setSearchFocus();
            emit Utils::InterConnector::instance().showSearchDropdownFull();
        });
    }

    Testing::setAccessibleName(noRecentsWidget_, qsl("AS noRecentsWidget"));
    mainLayout->addWidget(noRecentsWidget_);
}

void RecentsPlaceholder::setPictureOnlyView(bool _isPictureOnly)
{
    isPicrureOnly_ = _isPictureOnly;
    noRecentsWidget_->setVisible(!isPicrureOnly_);
}

void RecentsPlaceholder::resizeEvent(QResizeEvent *_event)
{
    noRecentsWidget_->setVisible(!isPicrureOnly_);
    if (isPicrureOnly_)
        return;

    const auto fm = QFontMetrics(noRecentsLabel_->font());
    const auto labelWidth = fm.width(originalLabel_);
    const auto widgetWidth = _event->size().width();
    static const auto dots = qsl("...");
    const auto dotsWidth = fm.width(dots);

    if (widgetWidth < labelWidth)
    {
        QString newLabel = originalLabel_;
        while (fm.width(newLabel) + dotsWidth >= widgetWidth && !newLabel.isEmpty())
            newLabel.chop(1);

        noRecentsLabel_->setText(newLabel % dots);
    }
    else
    {
        noRecentsLabel_->setText(originalLabel_);
    }
}


////////////////////////////////////////////////////////////////////////////
ContactsPlaceholder::ContactsPlaceholder(QWidget *_parent)
    : QWidget(_parent)
{
    Testing::setAccessibleName(this, qsl("AS noContactsYet_"));

    auto mainLayout = Utils::emptyVLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto noContactsWidget = new QWidget(this);
    auto noContactsLayout = new QVBoxLayout(noContactsWidget);
    noContactsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    noContactsLayout->setAlignment(Qt::AlignCenter);
    noContactsLayout->setContentsMargins(0, 0, 0, 0);
    noContactsLayout->setSpacing(Utils::scale_value(20));

    auto noContactsPlaceholder = new QLabel(this);
    auto pixmap = QPixmap(Utils::parse_image_name(qsl(":/placeholders/empty_cl_100")));
    Utils::check_pixel_ratio(pixmap);
    noContactsPlaceholder->setPixmap(pixmap);
    Testing::setAccessibleName(noContactsPlaceholder, qsl("AS noContactsPlaceholder"));
    noContactsLayout->addWidget(noContactsPlaceholder, 0, Qt::AlignCenter);

    auto noContactLabel = new LabelEx(noContactsWidget);
    noContactLabel->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    noContactLabel->setFont(Fonts::appFontScaled(15));
    noContactLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    noContactLabel->setAlignment(Qt::AlignCenter);
    noContactLabel->setWordWrap(true);
    noContactLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Looks like you have no contacts yet"));
    Testing::setAccessibleName(this, qsl("AS noContactsYet_"));
    noContactsLayout->addWidget(noContactLabel);

    Testing::setAccessibleName(this, qsl("AS noContactsYet_"));
    mainLayout->addWidget(noContactsWidget);
}



////////////////////////////////////////////////////////////////////////////
DialogPlaceholder::DialogPlaceholder(QWidget *_parent)
    : QWidget(_parent)
{
    Utils::setDefaultBackground(this);

    auto layout = Utils::emptyVLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    {
        auto p = new QWidget(this);
        p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        auto pl = Utils::emptyHLayout(p);
        pl->setAlignment(Qt::AlignCenter);
        {
            const auto size = Utils::scale_value(QSize(80, 80));
            auto logoLabel = new QLabel(p);
            logoLabel->setPixmap(Utils::renderSvg(build::GetProductVariant(qsl(":/logo/logo_icq"), qsl(":/logo/logo_agent"), qsl(":/logo/logo_biz"), qsl(":/logo/logo_dit")), size));
            logoLabel->setFixedSize(size);
            logoLabel->setAlignment(Qt::AlignCenter);
            Testing::setAccessibleName(logoLabel, qsl("AS logoWidget"));
            pl->addWidget(logoLabel);
        }
        Testing::setAccessibleName(p, qsl("AS p"));
        layout->addWidget(p);
    }
    {
        auto p = new QWidget(this);
        p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        auto pl = Utils::emptyHLayout(p);
        pl->setAlignment(Qt::AlignCenter);
        {
            auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Utils::scale_value(44));
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(build::GetProductVariant(QT_TRANSLATE_NOOP("placeholders", "Install ICQ on mobile"), QT_TRANSLATE_NOOP("placeholders", "Install Mail.ru Agent on mobile"),
                                                QT_TRANSLATE_NOOP("placeholders", "Install Myteam on mobile"), QString()));

            Testing::setAccessibleName(w, qsl("AS w"));
            pl->addWidget(w);
            w->setVisible(!build::is_dit());
        }
        Testing::setAccessibleName(p, qsl("AS p"));
        layout->addWidget(p);
    }
    {
        auto p = new QWidget(this);
        p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        auto pl = Utils::emptyHLayout(p);
        pl->setAlignment(Qt::AlignCenter);
        {
            auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Utils::scale_value(30));
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(QT_TRANSLATE_NOOP("placeholders", "to synchronize your contacts"));
            Testing::setAccessibleName(w, qsl("AS w"));
            pl->addWidget(w);
            w->setVisible(!build::is_dit() && !build::is_biz());
        }
        Testing::setAccessibleName(p, qsl("AS p"));
        layout->addWidget(p);
    }
    {
        auto p = new QWidget(this);
        p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        auto pl = new QHBoxLayout(p);
        pl->setContentsMargins(0, Utils::scale_value(28), 0, 0);
        pl->setSpacing(Utils::scale_value(8));
        pl->setAlignment(Qt::AlignCenter);
        {
            auto appStoreButton = new QPushButton(p);
            appStoreButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            appStoreButton->setFlat(true);

            const auto appStoreImage = qsl(":/placeholders/appstore_%1_100").
                arg(Ui::get_gui_settings()->get_value(settings_language, QString()).toUpper());

            const auto appStoreImageStyle = qsl("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                .arg(appStoreImage, appStoreImage);

            Utils::ApplyStyle(appStoreButton, appStoreImageStyle);

            appStoreButton->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
            appStoreButton->setCursor(Qt::PointingHandCursor);

            _parent->connect(appStoreButton, &QPushButton::clicked, []()
            {
                QDesktopServices::openUrl(QUrl(build::GetProductVariant(qsl("https://app.appsflyer.com/id302707408?pid=icq_win"),
                                                                        qsl("https://app.appsflyer.com/id335315530?pid=agent_win"),
                                                                        qsl("https://itunes.apple.com/us/app/id1459096573?mt=8"),
                                                                        qsl(""))));
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_ios);
            });
            Testing::setAccessibleName(appStoreButton, qsl("AS appStoreButton"));
            pl->addWidget(appStoreButton);
            appStoreButton->setVisible(!build::is_dit());

            auto googlePlayWidget = new QPushButton(p);
            googlePlayWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            googlePlayWidget->setFlat(true);

            const auto googlePlayImage = qsl(":/placeholders/gplay_%1_100")
                .arg(Ui::get_gui_settings()->get_value(settings_language, QString()).toUpper());
            const auto googlePlayStyle = qsl("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                .arg(googlePlayImage, googlePlayImage);

            Utils::ApplyStyle(googlePlayWidget, googlePlayStyle);

            googlePlayWidget->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
            googlePlayWidget->setCursor(Qt::PointingHandCursor);
            _parent->connect(googlePlayWidget, &QPushButton::clicked, []()
            {
                QDesktopServices::openUrl(QUrl(build::GetProductVariant(qsl("https://app.appsflyer.com/com.icq.mobile.client?pid=icq_win"),
                                                                        qsl("https://app.appsflyer.com/ru.mail?pid=agent_win"),
                                                                        qsl("https://app.appsflyer.com/ru.mail.biz.avocado?pid=icq_win"),
                                                                        qsl(""))));
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_android);
            });
            Testing::setAccessibleName(googlePlayWidget, qsl("AS googlePlayWidget"));
            pl->addWidget(googlePlayWidget);
            googlePlayWidget->setVisible(!build::is_dit());
        }
        Testing::setAccessibleName(p, qsl("AS p"));
        layout->addWidget(p);
    }
}


////////////////////////////////////////////////////////////////////////////
RotatingSpinner::RotatingSpinner(QWidget * _parent)
    : QWidget(_parent)
    , currentAngle_(0.)
{
    setFixedSize(Utils::scale_value(QSize(32, 32)));
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

RotatingSpinner::~RotatingSpinner()
{
    stopAnimation();
}

void RotatingSpinner::startAnimation(const QColor& _spinnerColor, const QColor& _bgColor)
{
    constexpr std::chrono::milliseconds duration = std::chrono::seconds(2);

    if (_spinnerColor.isValid())
        mainColor_ = _spinnerColor;
    else
        mainColor_ = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    assert(mainColor_.alpha() == 255);

    if (_bgColor.isValid())
        bgColor_ = _bgColor;
    else
        bgColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);

    anim_.finish();
    anim_.start([this]()
    {
        currentAngle_ = anim_.current();
        update();
    }, 0, 360, duration.count(), anim::linear, -1);
}

void RotatingSpinner::stopAnimation()
{
    anim_.finish();
}

void RotatingSpinner::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::NoBrush);

    static const auto penWidth = Utils::scale_value(2);
    static const auto r2 = rect().adjusted(penWidth / 2, penWidth / 2, -(penWidth / 2), -(penWidth / 2));
    if (bgColor_.isValid())
    {
        p.setPen(QPen(bgColor_, penWidth));
        p.drawEllipse(r2);
    }

    constexpr auto QT_ANGLE_MULT = 16;
    constexpr double idleProgressValue = 0.75;
    const auto animAngle = anim_.isRunning() ? anim_.current() : 0.0;
    const auto baseAngle = (animAngle * QT_ANGLE_MULT);
    const auto progressAngle = (int)std::ceil(idleProgressValue * 360 * QT_ANGLE_MULT);

    p.setPen(QPen(mainColor_, penWidth));
    p.drawArc(r2, -baseAngle, -progressAngle);
}
