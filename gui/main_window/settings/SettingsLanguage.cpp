#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "RestartLabel.h"

#include "../../controls/TransparentScrollBar.h"
#include "../../controls/SimpleListWidget.h"
#include "../../controls/RadioTextRow.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../styles/ThemeParameters.h"
#include "../../gui_settings.h"

using namespace Ui;

namespace
{
    const std::vector<QString> &getLanguagesStrings()
    {
        static const std::vector<QString> slist =
        {
            QT_TRANSLATE_NOOP("settings", "ru"),
            QT_TRANSLATE_NOOP("settings", "en"),
            QT_TRANSLATE_NOOP("settings", "uk"),
            QT_TRANSLATE_NOOP("settings", "de"),
            QT_TRANSLATE_NOOP("settings", "pt"),
            QT_TRANSLATE_NOOP("settings", "cs"),
            QT_TRANSLATE_NOOP("settings", "fr"),
            QT_TRANSLATE_NOOP("settings", "zh"),
            QT_TRANSLATE_NOOP("settings", "tr"),
            QT_TRANSLATE_NOOP("settings", "vi"),
            QT_TRANSLATE_NOOP("settings", "es")
        };
        assert(slist.size() == Utils::GetTranslator()->getLanguages().size());
        return slist;
    }

    QString languageToString(const QString& _code)
    {
        const auto& codes = Utils::GetTranslator()->getLanguages();
        const auto& strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (1)");
        const auto it = std::find(codes.begin(), codes.end(), _code);
        if (it != codes.end())
            return strs[std::distance(codes.begin(), it)];
        return QString();
    }

    QString stringToLanguage(const QString& _str)
    {
        const auto& codes = Utils::GetTranslator()->getLanguages();
        const auto& strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (2)");
        const auto it = std::find(strs.begin(), strs.end(), _str);
        if (it != strs.end())
            return codes[std::distance(strs.begin(), it)];
        return QString();
    }
}

void GeneralSettingsWidget::Creator::initLanguage(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);
    Testing::setAccessibleName(scrollAreaWidget, qsl("AS settings scrollAreaWidget"));
    scrollArea->setStyleSheet(qsl("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    //scrollAreaWidget->setStyleSheet(scrollAreaWidget->styleSheet() + qsl("background-color: green;"));

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollAreaWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS settings about scrollArea"));
    layout->addWidget(scrollArea);

    const auto& ls = getLanguagesStrings();
    const auto lc = languageToString(get_gui_settings()->get_value(settings_language, QString()));
    const auto it = std::find(ls.begin(), ls.end(), lc);
    const auto li = it != ls.end() ? int(std::distance(ls.begin(), it)) : std::numeric_limits<int>::max();

    {
        auto mainWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = new QVBoxLayout(mainWidget);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setContentsMargins(0, Utils::scale_value(12), 0, 0);
        mainLayout->setSpacing(Utils::scale_value(12));

        auto restartLabel = new RestartLabel;

        auto restartLayout = new QVBoxLayout;
        restartLayout->setContentsMargins(Utils::scale_value(20), 0, Utils::scale_value(20), 0);
        restartLayout->addWidget(restartLabel);
        mainLayout->addLayout(restartLayout);

        restartLabel->hide();

        auto list = new SimpleListWidget(Qt::Vertical);

        for (const auto& l : ls)
        {
            list->addItem(new RadioTextRow(l));
        }

        list->setCurrentIndex(li);

        QObject::connect(list, &SimpleListWidget::clicked, list, [&ls, lc, list, restartLabel](int idx)
        {
            if (idx < 0 && idx >= int(ls.size()))
                return;
            const static auto sl = get_gui_settings()->get_value(settings_language, QString());
            const auto lang = stringToLanguage(ls[idx]);
            get_gui_settings()->set_value(settings_language, lang);
            Utils::GetTranslator()->updateLocale();
            list->setCurrentIndex(idx);
            if (sl != lang)
            {
                restartLabel->show();

                const QString text = QT_TRANSLATE_NOOP("popup_window", "To change the language you must restart the application. Continue?");

                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                    QT_TRANSLATE_NOOP("popup_window", "YES"),
                    text,
                    QT_TRANSLATE_NOOP("popup_window", "Restart"),
                    nullptr
                );

                if (confirmed)
                {
                    Utils::restartApplication();

                    return;
                }
            }
            else
            {
                restartLabel->hide();
            }
        }, Qt::UniqueConnection);

        Testing::setAccessibleName(scrollArea, qsl("AS settings languageList"));
        mainLayout->addWidget(list);

        Testing::setAccessibleName(mainWidget, qsl("AS settings mainWidget"));
        scrollAreaLayout->addWidget(mainWidget);
    }
}