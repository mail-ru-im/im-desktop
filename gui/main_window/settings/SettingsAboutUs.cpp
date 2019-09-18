#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/GeneralCreator.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollAreaWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->setContentsMargins(0, 0, 0, 0);
    Testing::setAccessibleName(scrollArea, qsl("AS settings about scrollArea"));
    layout->addWidget(scrollArea);

    {
        auto mainWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = new QHBoxLayout(mainWidget);
        mainLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(36), Utils::scale_value(68), 0);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setSpacing(0);
        {
            const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
            auto aboutWidget = new QWidget(mainWidget);
            auto aboutLayout = Utils::emptyVLayout(aboutWidget);
            mainLayout->setSpacing(0);
            Utils::grabTouchWidget(aboutWidget);
            aboutLayout->setAlignment(Qt::AlignTop);
            aboutLayout->setContentsMargins(0, 0, 0, 0);
            {
                auto versionText = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                versionText->setText(Utils::getVersionLabel());
                versionText->setSelectable(true);
                Utils::grabTouchWidget(versionText);
                Testing::setAccessibleName(versionText, qsl("AS settings about us versionText"));
                aboutLayout->addWidget(versionText);
            }
            {
                auto opensslLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(opensslLabel);
                opensslLabel->setMultiline(true);
                opensslLabel->setText(QT_TRANSLATE_NOOP("about_us", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
                Testing::setAccessibleName(opensslLabel, qsl("AS settings about us opensslLabel"));
                aboutLayout->addWidget(opensslLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto opensslButton = new QPushButton(aboutWidget);
                auto opensslLayout = Utils::emptyVLayout(opensslButton);
                opensslButton->setFlat(true);
                opensslButton->setStyleSheet(qsl("background-color: transparent;"));
                opensslButton->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                opensslButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                opensslLayout->setAlignment(Qt::AlignTop);
                {
                    auto opensslLink = new TextEmojiWidget(opensslButton, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
                    Utils::grabTouchWidget(opensslLink);
                    opensslLink->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
                    opensslLink->setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
                    opensslLink->setText(QT_TRANSLATE_NOOP("about_us", "https://openssl.org"));
                    connect(opensslButton, &QPushButton::pressed, [opensslLink]()
                    {
                        QDesktopServices::openUrl(opensslLink->text());
                    });
                    opensslButton->setFixedHeight(opensslLink->height());
                    Testing::setAccessibleName(opensslLink, qsl("AS settings about us opensslLink"));
                    opensslLayout->addWidget(opensslLink);
                }
                Testing::setAccessibleName(opensslButton, qsl("AS settings about us opensslButton"));
                aboutLayout->addWidget(opensslButton);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto voipCopyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(voipCopyrightLabel);
                voipCopyrightLabel->setMultiline(true);
                voipCopyrightLabel->setText(QT_TRANSLATE_NOOP("about_us", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                Testing::setAccessibleName(voipCopyrightLabel, qsl("AS settings about us voipCopyrightLabel"));
                aboutLayout->addWidget(voipCopyrightLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto emojiLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(emojiLabel);
                emojiLabel->setMultiline(true);
                emojiLabel->setText(QT_TRANSLATE_NOOP("about_us", "Emoji provided free by Emoji One"));
                Testing::setAccessibleName(emojiLabel, qsl("AS settings about us emojiLabel"));
                aboutLayout->addWidget(emojiLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto copyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(copyrightLabel);
                const auto aboutUs = QT_TRANSLATE_NOOP("about_us", "© Mail.ru LLC");
                copyrightLabel->setText(aboutUs % ql1s(", ") % QDate::currentDate().toString(qsl("yyyy")));
                Testing::setAccessibleName(copyrightLabel, qsl("AS settings about us copyrightLabel"));
                aboutLayout->addWidget(copyrightLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));

                auto presentedMailru = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(presentedMailru);
                presentedMailru->setText(QT_TRANSLATE_NOOP("about_us", "Presented by Mail.ru"));
                presentedMailru->setVisible(build::is_icq());
                Testing::setAccessibleName(presentedMailru, qsl("AS settings about us presentedMailru"));
                aboutLayout->addWidget(presentedMailru);
            }
            Testing::setAccessibleName(aboutWidget, qsl("AS settings about us aboutWidget"));
            mainLayout->addWidget(aboutWidget);
        }
        Testing::setAccessibleName(mainWidget, qsl("AS settings about us mainWidget"));
        scrollAreaLayout->addWidget(mainWidget);
    }
}
