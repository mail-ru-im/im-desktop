#include "ContactNotRegisteredWidget.h"

#include "controls/TextBrowserEx.h"
#include "controls/PictureWidget.h"
#include "utils/utils.h"

namespace
{
constexpr int WIDGET_WIDTH = 380;
constexpr int TEXT_OFFSET = 24;
constexpr int CAT_WIDTH = 120;
constexpr int CAT_HEIGHT = 148;
}

namespace Ui
{

ContactNotRegisteredWidget::ContactNotRegisteredWidget(QWidget *_parent)
    : QWidget(_parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedWidth(Utils::scale_value(WIDGET_WIDTH));

    setInfo(QString(), QString());
    catPixmap_ = Utils::loadPixmap(qsl(":/add_new_contact/disappointed_cat_sticker_100"));
}

void ContactNotRegisteredWidget::setInfo(const QString &_phoneNumber, const QString &_firstName)
{
    const auto text = QT_TRANSLATE_NOOP("add_new_contact_dialogs", "Unfortunately, %1 with phone number %2 has not joined ICQ yet.")
            .arg(_firstName, _phoneNumber);

    auto w = Utils::scale_value(WIDGET_WIDTH - 2 * TEXT_OFFSET /* right offset */);

    if (!textUnit_)
    {
        textUnit_ = TextRendering::MakeTextUnit(text);
        textUnit_->init(Fonts::appFontScaled(16),
                        Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                        QColor(),
                        QColor(),
                        QColor(),
                        TextRendering::HorAligment::CENTER);
        textUnit_->getHeight(w);
        textUnit_->setOffsets(Utils::scale_value(24), Utils::scale_value(228));
        return;
    }

    textUnit_->setText(text);
    textUnit_->getHeight(w);
}

void ContactNotRegisteredWidget::paintEvent(QPaintEvent *_event)
{
    QWidget::paintEvent(_event);

    QPainter p(this);
    p.drawPixmap(Utils::scale_value(130), Utils::scale_value(47),
                 Utils::scale_value(CAT_WIDTH), Utils::scale_value(CAT_HEIGHT),
                 catPixmap_);

    textUnit_->draw(p, TextRendering::VerPosition::TOP);
}

void ContactNotRegisteredWidget::keyPressEvent(QKeyEvent *_event)
{
    QWidget::keyPressEvent(_event);

    if (_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter)
        emit enterPressed();
}

}
