#include "stdafx.h"
#include "StatusEmojiPicker.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "styles/ThemeParameters.h"


namespace
{
    int maxPickerHeight()
    {
        return Utils::scale_value(260);
    }

    int pickerOffsetFromTop()
    {
        return Utils::scale_value(12);
    }

    int margin()
    {
        return Utils::scale_value(4);
    }

    int pickerBorderRadius()
    {
        return Utils::scale_value(8);
    }

    int arrowHeight()
    {
        return Utils::scale_value(10);
    }
}

namespace Statuses
{
using namespace Ui;

class StatusEmojiPicker_p
{
public:
    Smiles::SmilesMenu* smiles_;
};

StatusEmojiPicker::StatusEmojiPicker(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<StatusEmojiPicker_p>())
{
    auto layout = Utils::emptyVLayout(this);
    layout->setContentsMargins(margin(), margin(), margin(), margin() + arrowHeight());
    d->smiles_ = new Smiles::SmilesMenu(this);
    d->smiles_->setToolBarVisible(false);
    d->smiles_->setRecentsVisible(false);
    d->smiles_->setStickersVisible(false);
    d->smiles_->setHorizontalMargins(Utils::scale_value(12));
    d->smiles_->setDrawTopBorder(false);
    d->smiles_->setTopSpacing(Utils::scale_value(4));
    layout->addWidget(d->smiles_);
    setFixedSize(Utils::scale_value(344), maxPickerHeight());

    auto shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(Utils::scale_value(16));
    shadow->setOffset(0, Utils::scale_value(2));
    shadow->setColor(QColor(0, 0, 0, 255 * 0.2));
    setGraphicsEffect(shadow);

    connect(d->smiles_, &Smiles::SmilesMenu::emojiSelected, this, [this](const Emoji::EmojiCode& _code, const QPoint _pos)
    {
        Q_EMIT emojiSelected(_code, QPrivateSignal());
    });
}

StatusEmojiPicker::~StatusEmojiPicker() = default;

void StatusEmojiPicker::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;

    auto backgroundRect = rect();
    backgroundRect.setBottom(backgroundRect.bottom() - arrowHeight());
    path.addRoundedRect(backgroundRect, pickerBorderRadius(), pickerBorderRadius());
    p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

    p.translate(Utils::scale_value(15), backgroundRect.bottom());

    QPainterPath arrowPath;
    arrowPath.addRoundedRect(QRect({0,0}, Utils::scale_value(QSize(16, 16))), Utils::scale_value(4), Utils::scale_value(4));
    p.rotate(-45);
    p.fillPath(arrowPath, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

    QWidget::paintEvent(_event);
}

class StatusEmojiPickerDialog_p
{
public:
    QWidget* emojiButton_ = nullptr;
    StatusEmojiPicker* picker_ = nullptr;
};

StatusEmojiPickerDialog::StatusEmojiPickerDialog(QWidget* _emojiButton, QWidget* _parent)
    : QWidget(Utils::InterConnector::instance().getMainWindow())
    , d(std::make_unique<StatusEmojiPickerDialog_p>())
{
    setAttribute(Qt::WA_DeleteOnClose);

    d->emojiButton_ = _emojiButton;
    d->picker_ = new StatusEmojiPicker(this);
    Testing::setAccessibleName(d->picker_, qsl("AS Statuses StatusEmojiPicker"));
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &StatusEmojiPickerDialog::onWindowResized, Qt::QueuedConnection);
    connect(d->picker_, &StatusEmojiPicker::emojiSelected, this, [this](const Emoji::EmojiCode& _code){ Q_EMIT emojiSelected(_code, QPrivateSignal()); });
    connect(d->picker_, &StatusEmojiPicker::emojiSelected, this, &StatusEmojiPickerDialog::close);

    onWindowResized();
}

StatusEmojiPickerDialog::~StatusEmojiPickerDialog() = default;

void StatusEmojiPickerDialog::mousePressEvent(QMouseEvent* _event)
{
    close();
}

void StatusEmojiPickerDialog::resizeEvent(QResizeEvent* _event)
{
    onWindowResized();
    QWidget::resizeEvent(_event);
}

void StatusEmojiPickerDialog::onWindowResized()
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    const auto mainWindowSize = mainWindow->size();
    const auto titleHeight = Utils::InterConnector::instance().getMainWindow()->getTitleHeight();
    setGeometry(QRect({0, titleHeight}, QSize(mainWindowSize.width(), mainWindowSize.height() - titleHeight)));

    auto pos = mapFromGlobal(d->emojiButton_->mapToGlobal({0, 0}));
    const auto availableHeight = pos.y() - pickerOffsetFromTop();
    pos.ry() -= d->picker_->height();

    d->picker_->move(pos);
    d->picker_->setFixedHeight(std::min(maxPickerHeight(), availableHeight));
}

}
