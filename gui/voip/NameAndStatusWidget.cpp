#include "stdafx.h"
#include "NameAndStatusWidget.h"

#include "VoipTools.h"
#include "../controls/TextEmojiWidget.h"
#include "../utils/utils.h"
#include "../controls/TextUnit.h"

template<typename __Base>
Ui::ShadowedWidget<__Base>::ShadowedWidget(QWidget* _parent, int _tailLen, double _alphaFrom, double _alphaTo)
    : __Base(_parent)
    , tailLenPx_(_tailLen)
{

    QGradientStops stops;
    QColor voipShadowColor("#000000");
    voipShadowColor.setAlphaF(_alphaFrom);
    stops.append(qMakePair(0.0f, voipShadowColor));
    voipShadowColor.setAlphaF(_alphaTo);
    stops.append(qMakePair(1.0f, voipShadowColor));

    linearGradient_.setStops(stops);
}

template<typename __Base>
Ui::ShadowedWidget<__Base>::~ShadowedWidget()
{
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::resizeEvent(QResizeEvent* _e)
{
    __Base::resizeEvent(_e);

    const QRect rc = __Base::rect();
    linearGradient_.setStart(rc.right() - tailLenPx_, 0.0f);
    linearGradient_.setFinalStop(rc.right(), 0.0f);
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.save();

    const QRect rc = __Base::rect();
    //if (__Base::internalWidth(painter) <= rc.width())
    {
        __Base::internalDraw(painter, rc);
        painter.restore();
        return;
    }

/* Disable for now because it works bad on retina.
    QPixmap pixmap(Utils::scale_bitmap(QSize(rc.width(), rc.height())));
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setPen(painter.pen());
    p.setFont(painter.font());

    __Base::internalDraw(p, Utils::scale_bitmap(rc));
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    p.fillRect(QRect(rc.right() - tailLenPx_ + 1, 0, tailLenPx_, pixmap.rect().height()), linearGradient_);
    p.end();

    painter.drawPixmap(Utils::scale_bitmap(rc), pixmap, pixmap.rect());
*/
    //painter.restore();
}

Ui::NameAndStatusWidget::NameAndStatusWidget(QWidget* _parent, int _nameBaseline, int _statusBaseline, int maxAvalibleWidth)
    : QWidget(_parent)
    , name_(NULL)
    , status_(NULL)
{
    setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignLeft);
    setLayout(rootLayout);

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setPixelSize(Utils::scale_value(16));

    QHBoxLayout* nameLayout = Utils::emptyHLayout();
    nameLayout->addStretch(1);

    {
        Ui::TextRendering::TextUnitPtr text = Ui::TextRendering::MakeTextUnit(QString());
        text->init(font, Qt::white, Qt::white, Qt::white, Qt::white, Ui::TextRendering::HorAligment::CENTER, 2);
        // !std::move
        name_ = new TextUnitLabel(this, std::move(text), Ui::TextRendering::VerPosition::TOP, maxAvalibleWidth);
    }

    nameLayout->addWidget(name_);
    nameLayout->addStretch(1);
    rootLayout->addLayout(nameLayout);

    rootLayout->addSpacing(Utils::scale_value(3));

    QHBoxLayout* statusLayout = Utils::emptyHLayout();
    statusLayout->addStretch(1);
    font.setPixelSize(Utils::scale_value(9));
    font.setCapitalization(QFont::AllUppercase);
    status_ = new voipTools::BoundBox<TextEmojiWidget>(this, font, Qt::white);
    status_->disableFixedPreferred();
    statusLayout->addWidget(status_);
    statusLayout->addStretch(1);
    rootLayout->addLayout(statusLayout);
}

Ui::NameAndStatusWidget::~NameAndStatusWidget()
{

}

void Ui::NameAndStatusWidget::setNameProperty(const char* _propName, bool _val)
{
    name_->setProperty(_propName, _val);
    name_->style()->unpolish(name_);
    name_->style()->polish(name_);
}

void Ui::NameAndStatusWidget::setStatusProperty(const char* _propName, bool _val)
{
    status_->setProperty(_propName, _val);
    status_->style()->unpolish(status_);
    status_->style()->polish(status_);
}

void Ui::NameAndStatusWidget::setName(const char* _name)
{
    assert(!!_name);
    if (_name && name_)
    {
        name_->setText(QString::fromUtf8(_name));
    }
}

void Ui::NameAndStatusWidget::setStatus(const char* _status)
{
    assert(!!_status);
    if (_status && status_)
    {
        status_->setText(QString::fromUtf8(_status));
    }
}
