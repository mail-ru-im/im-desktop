#include "stdafx.h"

#include "IntroduceYourself.h"

#include "../controls/ContactAvatarWidget.h"
#include "../controls/CommonInput.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/DialogButton.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/features.h"
#include "styles/ThemeParameters.h"
#include "../previewer/toast.h"

namespace
{
    constexpr int fontsize = 15;
}

namespace Ui
{
    IntroduceYourself::IntroduceYourself(const QString& _aimid, const QString& _display_name, QWidget* parent)
        : QWidget(parent)
        , aimid_(_aimid)
    {
        QHBoxLayout *horizontalLayout = Utils::emptyHLayout(this);

        auto mainWidget = new QWidget(this);
        mainWidget->setFixedWidth(Utils::scale_value(314));
        mainWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        mainWidget->setStyleSheet(ql1s("background-color: %1; border: 0;")
        .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        Testing::setAccessibleName(mainWidget, qsl("AS IntroducePage mainWidget"));
        horizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        horizontalLayout->addWidget(mainWidget);
        horizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        horizontalLayout->setAlignment(Qt::AlignHCenter);

        auto globalLayout = Utils::emptyVLayout(mainWidget);

        auto middleWidget = new QWidget(this);
        middleWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        Testing::setAccessibleName(middleWidget, qsl("AS IntroducePage middleWidget"));

        globalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        globalLayout->addWidget(middleWidget);
        globalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

        auto middleLayout = Utils::emptyVLayout(middleWidget);
        middleLayout->setAlignment(Qt::AlignCenter);

        contactAvatar_ = new ContactAvatarWidget(middleWidget, _aimid, QString(), Utils::scale_value(164), true, true);
        if (Features::avatarChangeAllowed())
            contactAvatar_->SetMode(ContactAvatarWidget::Mode::Introduce);

        contactAvatar_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        Testing::setAccessibleName(contactAvatar_, qsl("AS IntroducePage contactAvatarInput"));
        connect(contactAvatar_, &ContactAvatarWidget::afterAvatarChanged, this, &IntroduceYourself::avatarChanged);
        connect(contactAvatar_, &ContactAvatarWidget::avatarSetToCore, this, &IntroduceYourself::avatarSet);

        middleLayout->addWidget(contactAvatar_);
        middleLayout->setAlignment(contactAvatar_, Qt::AlignTop | Qt::AlignHCenter);
        middleLayout->addItem(new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Expanding, QSizePolicy::Fixed));

        auto title = new LabelEx(middleWidget);
        title->setAlignment(Qt::AlignHCenter);
        title->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        title->setFixedWidth(Utils::scale_value(314));
        title->setWordWrap(true);
        title->setTextInteractionFlags(Qt::TextBrowserInteraction);
        title->setOpenExternalLinks(true);
        title->setFont(Fonts::appFontScaled(23));
        title->setColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        title->setText(QT_TRANSLATE_NOOP("placeholders", "Add a name and avatar"));
        middleLayout->addWidget(title);
        middleLayout->setAlignment(title, Qt::AlignTop | Qt::AlignHCenter);
        middleLayout->addItem(new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Expanding, QSizePolicy::Fixed));

        auto explanation = new LabelEx(middleWidget);
        explanation->setAlignment(Qt::AlignHCenter);
        explanation->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        explanation->setFixedWidth(Utils::scale_value(314));
        explanation->setWordWrap(true);
        explanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
        explanation->setOpenExternalLinks(true);
        explanation->setFont(Fonts::appFontScaled(fontsize));
        explanation->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        explanation->setText(QT_TRANSLATE_NOOP("placeholders", "your contacts will see them"));
        middleLayout->addWidget(explanation);
        middleLayout->setAlignment(explanation, Qt::AlignTop | Qt::AlignHCenter);
        middleLayout->addItem(new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Expanding, QSizePolicy::Fixed));

        firstNameEdit_ = new CommonInputContainer(middleWidget);
        firstNameEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("placeholders", "First name*"));
        firstNameEdit_->setFixedWidth(Utils::scale_value(300));
        firstNameEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        firstNameEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        firstNameEdit_->setFont(Fonts::appFontScaled(fontsize));
        Testing::setAccessibleName(firstNameEdit_, qsl("AS IntroducePage firstNameInput"));
        middleLayout->addWidget(firstNameEdit_);
        middleLayout->addItem(new QSpacerItem(0, Utils::scale_value(6), QSizePolicy::Expanding, QSizePolicy::Fixed));
        middleLayout->setAlignment(firstNameEdit_, Qt::AlignTop | Qt::AlignHCenter);

        lastNameEdit_ = new CommonInputContainer(middleWidget);
        lastNameEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("placeholders", "Last name"));
        lastNameEdit_->setFixedWidth(Utils::scale_value(300));
        lastNameEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        lastNameEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        lastNameEdit_->setFont(Fonts::appFontScaled(fontsize));
        Testing::setAccessibleName(lastNameEdit_, qsl("AS IntroducePage lastNameInput"));
        middleLayout->addWidget(lastNameEdit_);
        middleLayout->setAlignment(lastNameEdit_, Qt::AlignTop | Qt::AlignHCenter);

        auto buttonWidget = new QWidget(middleWidget);
        buttonWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        auto buttonLayout = Utils::emptyHLayout(buttonWidget);
        buttonLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        nextButton_ = new DialogButton(buttonWidget, QT_TRANSLATE_NOOP("placeholders", "Continue"), DialogButtonRole::CONFIRM);
        setButtonActive(false);
        nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
        nextButton_->setFixedHeight(Utils::scale_value(40));
        nextButton_->setMinimumWidth(Utils::scale_value(100));
        nextButton_->setContentsMargins(Utils::scale_value(32), Utils::scale_value(9), Utils::scale_value(32), Utils::scale_value(11));
        nextButton_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);

        Testing::setAccessibleName(nextButton_, qsl("AS IntroducePage nextButton"));

        middleLayout->addItem(new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Fixed));
        buttonLayout->addWidget(nextButton_);
        middleLayout->addWidget(buttonWidget);

        connect(firstNameEdit_, &CommonInputContainer::textChanged, this, &IntroduceYourself::TextChanged);
        connect(nextButton_, &QPushButton::clicked, this, &IntroduceYourself::UpdateProfile);
        connect(firstNameEdit_, &CommonInputContainer::enterPressed, this, [this]()
        {
            lastNameEdit_->setFocus();
        });
        connect(lastNameEdit_, &CommonInputContainer::enterPressed, this, [this]()
        {
            if (!firstNameEdit_->text().trimmed().isEmpty())
                UpdateProfile();
            else
                firstNameEdit_->setFocus();
        });

        connect(GetDispatcher(), &Ui::core_dispatcher::updateProfile, this, &IntroduceYourself::RecvResponse);
    }

    void IntroduceYourself::init(QWidget* /*parent*/)
    {

    }

    IntroduceYourself::~IntroduceYourself()
    {

    }

    void IntroduceYourself::UpdateParams(const QString & _aimid, const QString & _display_name)
    {
        contactAvatar_->UpdateParams(_aimid, _display_name);

        if (aimid_ != _aimid)
        {
            firstNameEdit_->setText(_display_name);
            lastNameEdit_->clear();
        }

        firstNameEdit_->setEnabled(true);
        lastNameEdit_->setEnabled(true);
        setButtonActive(!firstNameEdit_->text().isEmpty());
        firstNameEdit_->setFocus();
    }

    void IntroduceYourself::TextChanged()
    {
        const auto text = firstNameEdit_->text();

        setButtonActive(!text.isEmpty()
            && std::any_of(text.begin(), text.end(), [](auto symb) { return !symb.isSpace(); }));
    }

    void IntroduceYourself::setButtonActive(bool _is_active)
    {
        nextButton_->changeRole(_is_active ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
    }

    void IntroduceYourself::UpdateProfile()
    {
        if (!firstNameEdit_->text().isEmpty())
        {
            setButtonActive(false);
            firstNameEdit_->setEnabled(false);
            lastNameEdit_->setEnabled(false);
            Utils::UpdateProfile({ { std::make_pair(std::string("firstName"), firstNameEdit_->text()) },
                                   { std::make_pair(std::string("lastName"), lastNameEdit_->text()) } });
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_name_set);
            Q_EMIT profileUpdated();
        }
    }

    void IntroduceYourself::RecvResponse(int _error)
    {
        if (_error != 0)
        {
            firstNameEdit_->setEnabled(true);
            lastNameEdit_->setEnabled(true);
            setButtonActive(true);
            firstNameEdit_->setFocus();
            Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("placeholders", "Error occurred, try again later"), Utils::scale_value(10));
        }
    }

    void IntroduceYourself::avatarChanged()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_avatar_changed);
    }
}
