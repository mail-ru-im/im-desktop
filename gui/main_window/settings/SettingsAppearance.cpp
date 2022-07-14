#include "stdafx.h"

#include "GeneralSettingsWidget.h"
#include "RestartLabel.h"
#include "PageOpenerWidget.h"

#include "themes/WallpaperDialog.h"
#include "themes/WallpaperPreviewWidget.h"
#include "themes/FontSizeSelectorWidget.h"

#include "styles/ThemesContainer.h"
#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "controls/RadioTextRow.h"
#include "controls/TextEmojiWidget.h"
#include "controls/textrendering/TextRenderingUtils.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "gui_settings.h"
#include "fonts.h"
#include "../MainWindow.h"

#include "../../../common.shared/config/config.h"

using namespace Ui;

namespace
{
    int getBgHeight()
    {
        return Utils::scale_value(180);
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


void GeneralSettingsWidget::Creator::initAppearance(AppearanceWidget* _parent)
{
    auto layout = Utils::emptyHLayout(_parent);

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("background: transparent"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);
    Testing::setAccessibleName(scrollArea, qsl("AS AppearancePage scrollArea"));
    layout->addWidget(scrollArea);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);
    Testing::setAccessibleName(scrollAreaWidget, qsl("AS AppearancePage scrollAreaWidget"));

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(scrollAreaWidget);

    const PreviewMessagesVector previewMsgs =
    {
        { QT_TRANSLATE_NOOP("wallpaper_select", "Hello! How are you?"), QTime(12, 44), QString(), true },
        { QT_TRANSLATE_NOOP("wallpaper_select", "Hi :)"),  QTime(16, 44), QT_TRANSLATE_NOOP("wallpaper_select", "Alice") },
    };
    auto preview = new WallpaperPreviewWidget(scrollArea, previewMsgs);
    preview->setFixedHeight(getBgHeight());
    preview->updateFor(getPreviewContact());
    scrollAreaLayout->addWidget(preview);
    Testing::setAccessibleName(preview, qsl("AS AppearancePage WallpaperPreviewWidget"));
    scrollAreaLayout->addSpacing(Utils::scale_value(8));

    auto wpSelector = new PageOpenerWidget(scrollArea, QT_TRANSLATE_NOOP("settings", "Chats wallpaper"));
    Testing::setAccessibleName(wpSelector, qsl("AS AppearancePage wallpapers"));
    QObject::connect(wpSelector, &PageOpenerWidget::clicked, preview, [preview]()
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
    Testing::setAccessibleName(fontSelector, qsl("AS AppearancePage fonts"));
    QObject::connect(fontSelector, &FontSizeSelectorWidget::sizeSelected, fontSelector, [](const Fonts::FontSize _size)
    {
        if (Fonts::getFontSizeSetting() != _size)
        {
            Fonts::setFontSizeSetting(_size);
            Q_EMIT Utils::InterConnector::instance().chatFontParamsChanged();
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
            [](bool checked)
            {
                if (Fonts::getFontBoldSetting() != checked)
                {
                    Fonts::setFontBoldSetting(checked);
                    Q_EMIT Utils::InterConnector::instance().chatFontParamsChanged();
                }
            }, -1, qsl("AS AppearancePage boldSetting")
    );

    gcItemsLayout->addSpacing(getItemSpacing());

    GeneralCreator::addSwitcher(
            scrollArea,
            gcItemsLayout,
            QT_TRANSLATE_NOOP("settings", "Chat list compact mode"),
            !get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default)),
            [](bool checked)
            {
                if (get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default)) == checked) {
                    get_gui_settings()->set_value<bool>(settings_show_last_message, !checked);
                    Q_EMIT Utils::InterConnector::instance().compactModeChanged();
                }
            }, -1, qsl("AS AppearancePage compactChatSetting")
    );

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

        w->setText(ql1s("%1 %2%").arg(QT_TRANSLATE_NOOP("settings", "Interface scale:"), sc[i]), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });

        if (fabs(su - r) >= 0.25f)
        {

            if (_finish)
            {
                const QString text = QT_TRANSLATE_NOOP("popup_window", "To change the scale you must restart the application. Continue?");

                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    QT_TRANSLATE_NOOP("popup_window", "Yes"),
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

            aw->setText(QT_TRANSLATE_NOOP("settings", "(Application restart required)"), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        }
        else if (fabs(Utils::getBasicScaleCoefficient() - r) < 0.05f)
        {
            aw->setText(QT_TRANSLATE_NOOP("settings", "(Recommended)"), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        }
        else
        {
            aw->setText(TextRendering::spaceAsString(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        }
    };

    int index = 0;
    auto it_idx = std::find(sc_int.begin(), sc_int.end(), currentValuePersentage);
    if (it_idx == sc_int.end())
        im_assert(false);
    else
        index = std::distance(sc_int.begin(), it_idx);

    GeneralCreator::addProgresser(
        scrollArea,
        gcItemsLayout,
        static_cast<int>(sc.size() - 1),
        index,
        [on_change_scale](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_scale(true, w, aw, i); },
        [on_change_scale](TextEmojiWidget* w, TextEmojiWidget* aw, int i) { on_change_scale(false, w, aw, i); }
    );
#endif

    {
        _parent->resetThemesList();

        gcItemsLayout->addSpacing(Utils::scale_value(40));
        GeneralCreator::addHeader(scrollArea, gcItemsLayout, QT_TRANSLATE_NOOP("settings", "Themes"), 20);
        gcItemsLayout->addSpacing(Utils::scale_value(12));

        scrollAreaLayout->addWidget(_parent->getList());
        scrollAreaLayout->addStretch();
    }
}
