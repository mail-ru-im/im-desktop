#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../LoginPage.h"
#include "../../controls/GeneralCreator.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAttachPhone(QWidget* _parent)
{
    auto scrollArea = new QScrollArea(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("QWidget{background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(16), 0, Utils::scale_value(16), Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);

    LoginPage* page = new LoginPage(nullptr, false /* is_login */);
    GeneralCreator::addBackButton(scrollArea, layout, [page]()
    {
        page->prevPage();
        emit Utils::InterConnector::instance().attachPhoneBack();
    });
    Testing::setAccessibleName(scrollArea, qsl("AS attachphone scrollArea"));
    layout->addWidget(scrollArea);

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("sidebar", "Attach phone"));
    Utils::ApplyStyle(scrollArea, Styling::getParameters().getLoginPageQss());
    Testing::setAccessibleName(page, qsl("AS attachphone page"));
    mainLayout->addWidget(page);

    connect(page, &LoginPage::attached, [page]()
    {
        if (!page->isVisible())
            return;
        emit Utils::InterConnector::instance().attachPhoneBack();
        emit Utils::InterConnector::instance().profileSettingsUpdateInterface();
    });
}
