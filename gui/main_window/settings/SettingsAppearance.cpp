#include "stdafx.h"

#include "GeneralSettingsWidget.h"
#include "RestartLabel.h"

#include "themes/WallpaperDialog.h"
#include "themes/WallpaperPreviewWidget.h"
#include "themes/DialogOpenerWidget.h"
#include "themes/FontSizeSelectorWidget.h"

#include "../../controls/TransparentScrollBar.h"
#include "../../controls/RadioTextRow.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../gui_settings.h"
#include "fonts.h"

#include "../../styles/ThemesContainer.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

namespace
{
    int getBgHeight()
    {
        return Utils::scale_value(180);
    }

    int getItemHeight()
    {
        return Utils::scale_value(44);
    }

    int getItemSpacing()
    {
        return Utils::scale_value(12);
    }

    QString getPreviewContact()
    {
        return QString();
    }
}


void GeneralSettingsWidget::Creator::initAppearance(QWidget* _parent)
{
    auto layout = Utils::emptyHLayout(_parent);

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("background: transparent"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);
    Testing::setAccessibleName(scrollArea, qsl("AS settings appearance scrollArea"));
    layout->addWidget(scrollArea);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);
    Testing::setAccessibleName(scrollAreaWidget, qsl("AS settings scrollAreaWidget"));

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(scrollAreaWidget);

    const PreviewMessagesVector previewMsgs =
    {
        { QT_TRANSLATE_NOOP("wallpaper_select", "Hello! How are you?"),     QTime(12, 44), QString(), true },
        { QT_TRANSLATE_NOOP("wallpaper_select", "Hi :)"),  QTime(16, 44), QT_TRANSLATE_NOOP("wallpaper_select", "Alice") },
    };
    auto preview = new WallpaperPreviewWidget(scrollArea, previewMsgs);
    preview->setFixedHeight(getBgHeight());
    preview->updateFor(getPreviewContact());
    scrollAreaLayout->addWidget(preview);
    scrollAreaLayout->addSpacing(Utils::scale_value(8));

    auto wpSelector = new DialogOpenerWidget(scrollArea, QT_TRANSLATE_NOOP("settings", "Chats wallpaper"));
    QObject::connect(wpSelector, &DialogOpenerWidget::clicked, preview, [preview]()
    {
        showWallpaperSelectionDialog(getPreviewContact());
        preview->updateFor(getPreviewContact());
    });
    scrollAreaLayout->addWidget(wpSelector);
    scrollAreaLayout->addSpacing(getItemSpacing());

    const std::vector<std::pair<int, Fonts::FontSize>> fontSizes =
    {
        { 15, Fonts::FontSize::Small },
        { 18, Fonts::FontSize::Medium },
        { 22, Fonts::FontSize::Large },
    };
    auto fontSelector = new FontSizeSelectorWidget(scrollArea, QT_TRANSLATE_NOOP("settings", "Font size"), fontSizes);
    fontSelector->setSelectedSize(Fonts::getFontSizeSetting());
    QObject::connect(fontSelector, &FontSizeSelectorWidget::sizeSelected, fontSelector, [](const Fonts::FontSize _size)
    {
        if (Fonts::getFontSizeSetting() != _size)
        {
            Fonts::setFontSizeSetting(_size);
            emit Utils::InterConnector::instance().chatFontParamsChanged();
        }
    });
    scrollAreaLayout->addWidget(fontSelector);

    auto gcItemsLayout = Utils::emptyVLayout();
    gcItemsLayout->setContentsMargins(0, Utils::scale_value(12) + getItemSpacing(), 0, 0);
    gcItemsLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->addLayout(gcItemsLayout);

    GeneralCreator::addSwitcher(
            scrollArea,
            gcItemsLayout,
            QT_TRANSLATE_NOOP("settings", "Bold text"),
            Fonts::getFontBoldSetting(),
            [](bool checked) -> QString {
                if (Fonts::getFontBoldSetting() != checked) {
                    Fonts::setFontBoldSetting(checked);
                    emit Utils::InterConnector::instance().chatFontParamsChanged();
                }

                return QString();
            });

    gcItemsLayout->addSpacing(getItemSpacing());

    GeneralCreator::addSwitcher(
            scrollArea,
            gcItemsLayout,
            QT_TRANSLATE_NOOP("settings", "Chat list compact mode"),
            !get_gui_settings()->get_value<bool>(settings_show_last_message, true),
            [](bool checked) -> QString {
                if (get_gui_settings()->get_value<bool>(settings_show_last_message, true) != !checked) {
                    get_gui_settings()->set_value<bool>(settings_show_last_message, !checked);
                    emit Utils::InterConnector::instance().compactModeChanged();
                }

                return QString();
            });

#ifndef __APPLE__
    gcItemsLayout->addSpacing(Utils::scale_value(20) + getItemSpacing());

    const double currentValue = get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
    const int currentValuePersentage = int(currentValue * 100);

    const std::vector<int> sc_int{ 100, 125, 150, 200, 250, 300 };
    std::vector< QString > sc;

    sc.reserve(sc_int.size());
    for (const int& _val : sc_int)
        sc.emplace_back(QString::number(_val));

    auto on_change_scale = [sc, currentValue](const bool _finish, TextEmojiWidget* w, TextEmojiWidget* aw, int i)
    {
        static auto su = currentValue;
        double r = sc[i].toDouble() / 100.f;
        if (fabs(get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient()) - r) >= 0.25f)
            get_gui_settings()->set_value<double>(settings_scale_coefficient, r);

        w->setText(qsl("%1 %2%").arg(QT_TRANSLATE_NOOP("settings", "Interface scale:"), sc[i]), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        if (fabs(su - r) >= 0.25f)
        {

            if (_finish)
            {
                const QString text = QT_TRANSLATE_NOOP("popup_window", "To change the scale you must restart the application. Continue?");

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

            aw->setText(QT_TRANSLATE_NOOP("settings", "(Application restart required)"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        }
        else if (fabs(Utils::getBasicScaleCoefficient() - r) < 0.05f)
        {
            aw->setText(QT_TRANSLATE_NOOP("settings", "(Recommended)"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        }
        else
        {
            aw->setText(qsl(" "), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        }
    };

    int index = 0;
    auto it_idx = std::find(sc_int.begin(), sc_int.end(), currentValuePersentage);
    if (it_idx == sc_int.end())
    {
        assert(false);
    }
    else
        index = std::distance(sc_int.begin(), it_idx);

    GeneralCreator::addProgresser(
        scrollArea,
        gcItemsLayout,
        sc,
        index,
        [on_change_scale](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_scale(true, w, aw, i); },
        [on_change_scale](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_scale(false, w, aw, i); }
    );
#endif

    {
        static const auto themesList = Styling::getThemesContainer().getAvailableThemes();
        const auto current = Styling::getThemesContainer().getCurrentThemeId();
        const auto it = std::find_if(themesList.begin(), themesList.end(), [&current](const auto& _p) { return _p.first == current; });
        const auto currentIdx = it != themesList.end() ? int(std::distance(themesList.begin(), it)) : std::numeric_limits<int>::max();

        auto list = new SimpleListWidget(Qt::Vertical);

        for (const auto&[_, name] : themesList)
            list->addItem(new RadioTextRow(name));

        list->setCurrentIndex(currentIdx);

        QObject::connect(list, &SimpleListWidget::clicked, list, [currentIdx, current, list](int idx)
        {
            if (idx < 0 && idx >= int(themesList.size()))
                return;

            list->setCurrentIndex(idx);

            if (themesList[idx].first != current)
            {
                const QString text = QT_TRANSLATE_NOOP("popup_window", "To change the appearance you must restart the application. Continue?");

                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                    QT_TRANSLATE_NOOP("popup_window", "YES"),
                    text,
                    QT_TRANSLATE_NOOP("popup_window", "Restart"),
                    nullptr
                );

                if (confirmed)
                {
                    Styling::getThemesContainer().setCurrentTheme(themesList[idx].first);

                    Utils::restartApplication();
                }
                else
                {
                    list->setCurrentIndex(currentIdx);
                }
            }
        }, Qt::UniqueConnection);


        gcItemsLayout->addSpacing(Utils::scale_value(40));
        GeneralCreator::addHeader(scrollArea, gcItemsLayout, QT_TRANSLATE_NOOP("settings", "Themes"), 20);
        gcItemsLayout->addSpacing(Utils::scale_value(12));

        scrollAreaLayout->addWidget(list);
        scrollAreaLayout->addStretch();
    }
}