#include "stdafx.h"

#include "IntroduceYourself.h"

#include "../controls/ContactAvatarWidget.h"
#include "../controls/LineEditEx.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/DialogButton.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "styles/ThemeParameters.h"


namespace Ui
{
    IntroduceYourself::IntroduceYourself(const QString& _aimid, const QString& _display_name, QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout *horizontal_layout = Utils::emptyHLayout(this);

        auto main_widget_ = new QWidget(this);
        main_widget_->setStyleSheet(qsl("background-color: %1; border: 0;")
        .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        Testing::setAccessibleName(main_widget_, qsl("AS iy main_widget_"));
        horizontal_layout->addWidget(main_widget_);

        {
            auto global_layout = Utils::emptyVLayout(main_widget_);

            auto upper_widget = new QWidget(this);
            upper_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            Testing::setAccessibleName(upper_widget, qsl("AS iy upper_widget"));
            global_layout->addWidget(upper_widget);

            auto middle_widget = new QWidget(this);
            middle_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            Testing::setAccessibleName(middle_widget, qsl("AS iy middle_widget"));
            global_layout->addWidget(middle_widget);

            auto bottom_widget = new QWidget(this);
            bottom_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            Testing::setAccessibleName(bottom_widget, qsl("AS iy bottom_widget"));
            global_layout->addWidget(bottom_widget);

            auto middle_layout = Utils::emptyVLayout(middle_widget);
            middle_layout->setAlignment(Qt::AlignHCenter);
            {
                auto pl = Utils::emptyHLayout();
                pl->setAlignment(Qt::AlignCenter);
                {
                    auto w = new ContactAvatarWidget(this, _aimid, _display_name, Utils::scale_value(180), true);
                    w->SetMode(ContactAvatarWidget::Mode::Introduce);

                    w->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    Testing::setAccessibleName(w, qsl("AS iy w"));
                    pl->addWidget(w);
                    QObject::connect(w, &ContactAvatarWidget::afterAvatarChanged, this, &IntroduceYourself::avatarChanged);
                }
                middle_layout->addLayout(pl);
            }

            {

                first_name_edit_ = new LineEditEx(middle_widget);
                first_name_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("placeholders", "First name"));
                first_name_edit_->setAlignment(Qt::AlignCenter);
                first_name_edit_->setMinimumWidth(Utils::scale_value(320));
                first_name_edit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                first_name_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
                first_name_edit_->setFont(Fonts::appFontScaled(18));
                Utils::ApplyStyle(first_name_edit_, Styling::getParameters().getLineEditCommonQss());
                Testing::setAccessibleName(first_name_edit_, qsl("AS iy first_name_edit_"));
                middle_layout->addWidget(first_name_edit_);
            }

            {

                last_name_edit_ = new LineEditEx(middle_widget);
                last_name_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("placeholders", "Last name"));
                last_name_edit_->setAlignment(Qt::AlignCenter);
                last_name_edit_->setMinimumWidth(Utils::scale_value(320));
                last_name_edit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                last_name_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
                last_name_edit_->setFont(Fonts::appFontScaled(18));
                Utils::ApplyStyle(last_name_edit_, Styling::getParameters().getLineEditCommonQss());
                Testing::setAccessibleName(last_name_edit_, qsl("AS iy last_name_edit_"));
                middle_layout->addWidget(last_name_edit_);
            }

            {
                auto horizontalSpacer = new QSpacerItem(0, Utils::scale_value(56), QSizePolicy::Preferred, QSizePolicy::Minimum);
                middle_layout->addItem(horizontalSpacer);

                auto pl = Utils::emptyHLayout();
                pl->setAlignment(Qt::AlignCenter);

                next_button_ = new DialogButton(middle_widget, QT_TRANSLATE_NOOP("placeholders", "CONTINUE"), DialogButtonRole::CONFIRM);
                setButtonActive(false);
                next_button_->setCursor(QCursor(Qt::PointingHandCursor));
                next_button_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);

                first_name_edit_->setAlignment(Qt::AlignCenter);
                last_name_edit_->setAlignment(Qt::AlignCenter);
                Testing::setAccessibleName(next_button_, qsl("AS iy next_button_"));
                pl->addWidget(next_button_);
                middle_layout->addLayout(pl);
            }

            {
                error_label_ = new LabelEx(middle_widget);
                error_label_->setAlignment(Qt::AlignCenter);
                error_label_->setFont(Fonts::appFontScaled(15));
                error_label_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
                Testing::setAccessibleName(error_label_, qsl("AS iy error_label_"));
                middle_layout->addWidget(error_label_);
            }

            {
                auto bottom_layout = Utils::emptyVLayout(bottom_widget);
                bottom_layout->setAlignment(Qt::AlignHCenter);

                auto horizontalSpacer = new QSpacerItem(0, Utils::scale_value(32), QSizePolicy::Preferred, QSizePolicy::Minimum);
                bottom_layout->addItem(horizontalSpacer);
            }
        }

          connect(first_name_edit_, &QLineEdit::textChanged, this, &IntroduceYourself::TextChanged);
          connect(next_button_, &QPushButton::clicked, this, &IntroduceYourself::UpdateProfile);
          connect(first_name_edit_, &LineEditEx::enter, this, [this]()
                {
                    last_name_edit_->setFocus();
                });
          connect(last_name_edit_, &LineEditEx::enter, this, [this]()
                {
                    if (!first_name_edit_->text().isEmpty())
                        UpdateProfile();
                    else
                        first_name_edit_->setFocus();
                });
          connect(GetDispatcher(), &Ui::core_dispatcher::updateProfile, this, &IntroduceYourself::RecvResponse);
    }

    void IntroduceYourself::init(QWidget* /*parent*/)
    {

    }

    IntroduceYourself::~IntroduceYourself()
    {

    }

    void IntroduceYourself::TextChanged()
    {
        const auto text = first_name_edit_->text();

        setButtonActive(!text.isEmpty()
            && std::any_of(text.begin(), text.end(), [](auto symb) { return !symb.isSpace(); }));
    }

    void IntroduceYourself::setButtonActive(bool _is_active)
    {
        next_button_->setEnabled(_is_active);
    }

    void IntroduceYourself::UpdateProfile()
    {
        if (!first_name_edit_->text().isEmpty())
        {
            setButtonActive(false);
            first_name_edit_->setEnabled(false);
            last_name_edit_->setEnabled(false);
            Utils::UpdateProfile({ { std::make_pair(std::string("firstName"), first_name_edit_->text()) },
                                   { std::make_pair(std::string("lastName"), last_name_edit_->text()) } });
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_name_set);
        }
    }

    void IntroduceYourself::RecvResponse(int _error)
    {
        if (_error == 0)
        {
             emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        else
        {
            first_name_edit_->setEnabled(true);
            last_name_edit_->setEnabled(true);
            setButtonActive(true);
            error_label_->setText(QT_TRANSLATE_NOOP("placeholders", "Error occurred, try again later"));
        }
    }

    void IntroduceYourself::avatarChanged()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_avatar_changed);
    }
}
