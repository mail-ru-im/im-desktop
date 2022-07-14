#include "stdafx.h"
#include "SelectionPanel.h"
#include "controls/ClickWidget.h"
#include "controls/CustomButton.h"
#include "styles/StyleVariable.h"
#include "styles/ThemeParameters.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "fonts.h"

namespace
{
    constexpr int HOR_OFFSET = 16;
}

namespace Ui
{
    SelectionPanel::SelectionPanel(const QString& _aimId, QWidget* _parent)
        : QFrame(_parent)
        , aimId_(_aimId)
    {
        auto layout = Utils::emptyHLayout(this);
        layout->setContentsMargins(Utils::scale_value(HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), 0);

        label_ = new ClickableTextWidget(this, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        label_->setCursor(Qt::ArrowCursor);
        layout->addWidget(label_);

        layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        cancel_ = new CustomButton(this);
        cancel_->setFont(Fonts::appFontScaled(15));
        cancel_->setShape(Ui::ButtonShape::ROUNDED_RECTANGLE);
        cancel_->setNormalTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        cancel_->setHoveredTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER });
        cancel_->setText(QT_TRANSLATE_NOOP("selection", "Cancel"));
        layout->addWidget(cancel_);

        connect(cancel_, &CustomButton::clicked, this, [this]() { Utils::InterConnector::instance().setMultiselect(false, aimId_); });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectedCount, this, &SelectionPanel::setSelectedCount);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentElementChanged, this, &SelectionPanel::multiSelectCurrentElementChanged);
    }

    void SelectionPanel::setSelectedCount(const QString& _aimId, int _count, int, bool)
    {
        if (aimId_ != _aimId)
            return;

        label_->setText(Utils::GetTranslator()->getNumberString(_count, QT_TRANSLATE_NOOP3("selection", "%1 message selected", "1"),
            QT_TRANSLATE_NOOP3("contactlist", "%1 messages selected", "2"),
            QT_TRANSLATE_NOOP3("contactlist", "%1 messages selected", "5"),
            QT_TRANSLATE_NOOP3("contactlist", "%1 messages selected", "21")).arg(_count));
        update();
    }

    void SelectionPanel::multiSelectCurrentElementChanged()
    {
        if (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Cancel)
        {
            cancel_->forceHover(true);
            cancel_->setBackgroundNormal(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY, 0.18 });
        }
        else
        {
            cancel_->forceHover(false);
            cancel_->setBackgroundNormal(Styling::ColorParameter{ Qt::transparent });
        }
    }
}
