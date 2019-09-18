#include "ReleaseNotes.h"
#include "ReleaseNotesText.h"
#include "stdafx.h"
#include "../controls/GeneralDialog.h"

#include "../controls/TooltipWidget.h"
#include "history_control/MessageStyle.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../fonts.h"

#include "../controls/TransparentScrollBar.h"
#include "main_window/MainWindow.h"
#include "../styles/ThemeParameters.h"

namespace {

    auto titleTopMargin()
    {
        return Utils::scale_value(20);
    }

    auto textTopMargin()
    {
        return Utils::scale_value(28);
    }

    auto leftMargin()
    {
        return Utils::scale_value(28);
    }

    auto rightMargin()
    {
        return Utils::scale_value(60);
    }

    auto releaseNotesWidth()
    {
        return Utils::scale_value(400);
    }

    auto releaseNotesHeight()
    {
        return Utils::scale_value(414);
    }
}

namespace Ui
{

namespace ReleaseNotes
{

ReleaseNotesWidget::ReleaseNotesWidget(QWidget *_parent)
    : QWidget(_parent)
{
    setFixedSize(releaseNotesWidth(), releaseNotesHeight());

    auto layout = Utils::emptyVLayout(this);

    layout->addSpacing(titleTopMargin());

    auto titleLayout = Utils::emptyHLayout(this);
    auto title = new TextWidget(this, QT_TRANSLATE_NOOP("release_notes", "What's new"));
    title->init(Fonts::appFontScaled(22, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    titleLayout->addSpacing(leftMargin());
    Testing::setAccessibleName(title, qsl("AS rn title"));
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFocusPolicy(Qt::NoFocus);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("background-color: %1; border: none").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));

    auto text = new TextWidget(this, releaseNotesText());
    text->setMaxWidth(releaseNotesWidth() - leftMargin() - rightMargin());
    text->init(Fonts::appFontScaled(15, Fonts::FontWeight::Normal), MessageStyle::getTextColor());

    auto scrollAreaContent = new QWidget(scrollArea);
    auto scrollAreaContentLayout = Utils::emptyHLayout(scrollAreaContent);
    Testing::setAccessibleName(text, qsl("AS rn text"));
    scrollAreaContentLayout->addWidget(text);
    scrollAreaContent->setLayout(scrollAreaContentLayout);
    scrollAreaContent->setStyleSheet(qsl("background-color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));

    scrollArea->setWidget(scrollAreaContent);

    layout->addSpacing(textTopMargin());
    Testing::setAccessibleName(scrollArea, qsl("AS rn scrollArea"));
    layout->addWidget(scrollArea);
}

void showReleaseNotes()
{
    auto w = new ReleaseNotesWidget();
    GeneralDialog d(w, Utils::InterConnector::instance().getMainWindow(), false, false);
    d.addAcceptButton(QT_TRANSLATE_NOOP("release_notes", "OK"), true);
    d.showInCenter();
}

QString releaseNotesHash()
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(releaseNotesText().toUtf8());
    return QString::fromUtf8(hash.result().toHex());
}

}

}
