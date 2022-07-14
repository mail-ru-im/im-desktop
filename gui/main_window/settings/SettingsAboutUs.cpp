#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/GeneralCreator.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/InterConnector.h"
#include "../../styles/ThemeParameters.h"
#include "../../core_dispatcher.h"
#include "../../url_config.h"
#include "CheckForUpdateButton.h"

namespace
{
    QString betaChatStamp()
    {
        return qsl("desktopbeta");
    }
}

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollAreaWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->setContentsMargins(0, 0, 0, 0);
    Testing::setAccessibleName(scrollArea, qsl("AS AboutUsPage scrollArea"));
    layout->addWidget(scrollArea);

    {
        auto generalWidget = new QWidget(scrollArea);
        auto generalLayout = Utils::emptyVLayout(generalWidget);
        auto mainWidget = new QWidget(generalWidget);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = new QVBoxLayout(mainWidget);
        mainLayout->setContentsMargins(0, Utils::scale_value(36), Utils::scale_value(20), 0);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setSpacing(0);
        {
            const auto textColor = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
            auto aboutWidget = new QWidget(mainWidget);
            auto aboutLayout = Utils::emptyVLayout(aboutWidget);
            Utils::grabTouchWidget(aboutWidget);
            aboutLayout->setAlignment(Qt::AlignTop);
            aboutLayout->setContentsMargins(Utils::scale_value(20), 0, 0, 0);
            {
                auto versionText = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                versionText->setText(Utils::getVersionLabel());
                versionText->setSelectable(true);
                versionText->setMultiline(true);
                Utils::grabTouchWidget(versionText);
                Testing::setAccessibleName(versionText, qsl("AS AboutUsPage versionText"));
                aboutLayout->addWidget(versionText);
            }
            if (Features::hasRegistryAbout())
            {
                aboutLayout->addSpacing(Utils::scale_value(20));
                auto registrylLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(registrylLabel);
                registrylLabel->setMultiline(true);
                registrylLabel->setText(QT_TRANSLATE_NOOP("about_us", "The product is registered in the Unified Register of Russian Software and Databases"));
                aboutLayout->addWidget(registrylLabel);

                auto registryButton = new QPushButton(aboutWidget);
                auto registryLayout = Utils::emptyVLayout(registryButton);
                registryButton->setFlat(true);
                registryButton->setStyleSheet(qsl("background-color: transparent;"));
                registryButton->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                registryButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                registryLayout->setAlignment(Qt::AlignTop);
                {
                    auto registryLink = new TextEmojiWidget(registryButton, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
                    Utils::grabTouchWidget(registryLink);
                    registryLink->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_HOVER });
                    registryLink->setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });
                    registryLink->setText(QT_TRANSLATE_NOOP("about_us", "https://reestr.digital.gov.ru/reestr/307994/"));
                    connect(registryButton, &QPushButton::pressed, [registryLink]()
                        {
                            QDesktopServices::openUrl(registryLink->text());
                        });
                    registryButton->setFixedHeight(registryLink->height());
                    registryLayout->addWidget(registryLink);
                }
                aboutLayout->addWidget(registryButton);
            }
            {
                aboutLayout->addSpacing(Utils::scale_value(20));
                auto opensslLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(opensslLabel);
                opensslLabel->setMultiline(true);
                opensslLabel->setText(QT_TRANSLATE_NOOP("about_us", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
                Testing::setAccessibleName(opensslLabel, qsl("AS AboutUsPage opensslLabel"));
                aboutLayout->addWidget(opensslLabel);
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
                    auto opensslLink = new TextEmojiWidget(opensslButton, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
                    Utils::grabTouchWidget(opensslLink);
                    opensslLink->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_HOVER });
                    opensslLink->setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });
                    opensslLink->setText(QT_TRANSLATE_NOOP("about_us", "https://openssl.org"));
                    connect(opensslButton, &QPushButton::pressed, [opensslLink]()
                    {
                        QDesktopServices::openUrl(opensslLink->text());
                    });
                    opensslButton->setFixedHeight(opensslLink->height());
                    Testing::setAccessibleName(opensslLink, qsl("AS AboutUsPage opensslLink"));
                    opensslLayout->addWidget(opensslLink);
                }
                Testing::setAccessibleName(opensslButton, qsl("AS AboutUsPage opensslButton"));
                aboutLayout->addWidget(opensslButton);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto voipCopyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(voipCopyrightLabel);
                voipCopyrightLabel->setMultiline(true);
                voipCopyrightLabel->setText(QT_TRANSLATE_NOOP("about_us", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                Testing::setAccessibleName(voipCopyrightLabel, qsl("AS AboutUsPage voipCopyrightLabel"));
                aboutLayout->addWidget(voipCopyrightLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }
            {
                auto emojiLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(emojiLabel);
                emojiLabel->setMultiline(true);
                emojiLabel->setText(QT_TRANSLATE_NOOP("about_us", "Emoji provided free by Emoji One"));
                Testing::setAccessibleName(emojiLabel, qsl("AS AboutUsPage emojiLabel"));
                aboutLayout->addWidget(emojiLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }

            if (Features::hasRegistryAbout())
            {
                auto registryCopyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(registryCopyrightLabel);
                const auto aboutUs = QT_TRANSLATE_NOOP("about_us", "registry_copyright");
                registryCopyrightLabel->setText(aboutUs % u", " % QDate::currentDate().toString(u"yyyy"));
                aboutLayout->addWidget(registryCopyrightLabel);
            }
            {
                auto copyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), textColor);
                Utils::grabTouchWidget(copyrightLabel);
                const auto aboutUs = QT_TRANSLATE_NOOP("about_us", "© Mail.ru LLC");
                copyrightLabel->setText(aboutUs % u", " % QDate::currentDate().toString(u"yyyy"));
                Testing::setAccessibleName(copyrightLabel, qsl("AS AboutUsPage copyrightLabel"));
                aboutLayout->addWidget(copyrightLabel);
                aboutLayout->addSpacing(Utils::scale_value(20));
            }

            Testing::setAccessibleName(aboutWidget, qsl("AS AboutUsPage aboutWidget"));
            mainLayout->addWidget(aboutWidget);
            generalLayout->addWidget(mainWidget);

            mainLayout->addSpacing(Utils::scale_value(20));

            if (Features::betaUpdateAllowed())
            {
                GeneralCreator::addSwitcher(
                    mainWidget,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Install beta updates"),
                    Ui::GetDispatcher()->isInstallBetaUpdatesEnabled(),
                    [](bool checked) { Ui::GetDispatcher()->setInstallBetaUpdates(checked); }, -1, qsl("AS AboutUsPage betaSwitcher")
                );

                auto betaWidget = new QWidget(mainWidget);
                auto betaLayout = Utils::emptyVLayout(betaWidget);
                Utils::grabTouchWidget(betaWidget);
                betaLayout->setAlignment(Qt::AlignTop);
                betaLayout->setContentsMargins(Utils::scale_value(20), 0, 0, 0);

                {
                    auto betaDescription = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER });
                    Utils::grabTouchWidget(betaDescription);
                    betaDescription->setMultiline(true);
                    betaDescription->setText(QT_TRANSLATE_NOOP("about_us", "Beta version contains new features, but it is not complete yet.\nYou can leave your feedback or report an error here:"));
                    betaLayout->addWidget(betaDescription);
                }
                {
                    auto betachatButton = new QPushButton(aboutWidget);
                    auto betachatLayout = Utils::emptyVLayout(betachatButton);
                    betachatButton->setFlat(true);
                    betachatButton->setStyleSheet(qsl("background-color: transparent;"));
                    betachatButton->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                    betachatButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                    betachatLayout->setAlignment(Qt::AlignTop);
                    {
                        auto betachatLink = new TextEmojiWidget(betachatButton, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
                        Utils::grabTouchWidget(betachatLink);
                        betachatLink->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_HOVER });
                        betachatLink->setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });
                        betachatLink->setText(QT_TRANSLATE_NOOP("about_us", "https://icq.im/desktopbeta"));
                        connect(betachatButton, &QPushButton::pressed, []()
                        {
                            Utils::openDialogOrProfile(betaChatStamp(), Utils::OpenDOPParam::stamp);
                        });
                        betachatButton->setFixedHeight(betachatLink->height());
                        betachatLayout->addWidget(betachatLink);
                    }
                    betaLayout->addWidget(betachatButton);
                    Testing::setAccessibleName(betachatButton, qsl("AS AboutUsPage betaChatButton"));
                }

                mainLayout->addWidget(betaWidget);
            }

            if ((platform::is_apple() && Utils::supportUpdates()) || (!platform::is_apple() && Features::updateAllowed()))
            {
                auto widget = new QWidget(mainWidget);
                auto l = Utils::emptyHLayout(widget);
                l->setAlignment(Qt::AlignTop);
                l->setSpacing(0);
                l->setContentsMargins(Utils::scale_value(12), 0, 0, 0);
                l->addWidget(new CheckForUpdateButton(mainWidget));
                mainLayout->addWidget(widget);

                const auto updateCheckButtonVisible = [widget = QPointer(widget)]()
                {
                    if (widget)
                        widget->setVisible(!Features::isUpdateFromBackendEnabled() || (Features::isUpdateFromBackendEnabled() && !Ui::getUrlConfig().getUrlAppUpdate().isEmpty()));
                };
                connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, widget, updateCheckButtonVisible);
                connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, widget, updateCheckButtonVisible);

                updateCheckButtonVisible();
            }
        }
        Testing::setAccessibleName(generalWidget, qsl("AS AboutUsPage"));
        scrollAreaLayout->addWidget(generalWidget);
    }
}
