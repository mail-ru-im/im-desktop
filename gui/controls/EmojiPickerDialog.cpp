#include "stdafx.h"
#include "EmojiPickerDialog.h"
#include "EmojiPickerWidget.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "styles/ThemeParameters.h"
#include "../statuses/StatusCommonUi.h"
#include "GeneralDialog.h"

namespace
{
    int pickerMainWindowMargin() noexcept { return Utils::scale_value(12); }
    int pickerFrameShadowWidth() noexcept { return Utils::scale_value(12); }
}

namespace Ui
{

class EmojiPickerDialogPrivate
{
public:
    QPointer<QWidget> emojiButton_;
    QPointer<QWidget> alignWidget_;
    QPointer<QWidget> inputWidget_;
    EmojiPickerWidget* picker_ = nullptr;
    EmojiPickerDialog::PopupDirections allowedDirections_;
    bool isGeneralDialogAncestor;

    QPoint mapPos(QWidget* _source, QWidget* _target, const QPoint& _p = QPoint()) const
    {
        return _target->mapFromGlobal(_source->mapToGlobal(_p));
    }

    EmojiPickerDialog::PopupDirection prefferedDirection(const QRect& _constrainRect, const QRect& _pivotRect, QSize& _s) const
    {
        const int px = _pivotRect.x();
        const int py = _pivotRect.y();

        const int pw = _pivotRect.width();
        const int ph = _pivotRect.height();

        const int cw = _constrainRect.width();
        const int ch = _constrainRect.height();

        std::array<QSize, 4> availablePositionSize =
        {
            QSize{ std::min(EmojiPickerWidget::maxPickerWidth(), cw), std::min(EmojiPickerWidget::maxPickerHeight(), py - pickerMainWindowMargin()) }, // top
            QSize{ std::min(EmojiPickerWidget::maxPickerWidth(), cw), std::min(EmojiPickerWidget::maxPickerHeight(), ch - (py + ph) - pickerMainWindowMargin()) }, // bottom
            QSize{ std::min(EmojiPickerWidget::maxPickerWidth() + Utils::scale_value(16), px - pickerMainWindowMargin()), std::min(2 * EmojiPickerWidget::maxPickerHeight(), ch - 2 * pickerMainWindowMargin()) }, // left
            QSize{ std::min(EmojiPickerWidget::maxPickerWidth() + Utils::scale_value(16), cw - (px + pw) - pickerMainWindowMargin()), std::min(2 * EmojiPickerWidget::maxPickerHeight(), ch - 2 * pickerMainWindowMargin()) } // right
        };

        EmojiPickerDialog::PopupDirection prefferedDirection = EmojiPickerDialog::Top;

        int minimum = 0;
        for (size_t i = 0; i < 4; ++i)
        {
            const EmojiPickerDialog::PopupDirection dir = EmojiPickerDialog::PopupDirection(1 << i);
            if (!(allowedDirections_ & dir))
                continue;

            if ((dir == EmojiPickerDialog::Top || dir == EmojiPickerDialog::Bottom) && availablePositionSize[i].height() > minimum)
            {
                minimum = availablePositionSize[i].height();
                prefferedDirection = (EmojiPickerDialog::PopupDirection)dir;
                _s = availablePositionSize[i];
            }
            if ((dir == EmojiPickerDialog::Left || dir == EmojiPickerDialog::Right) && availablePositionSize[i].width() > minimum)
            {
                minimum = availablePositionSize[i].width();
                prefferedDirection = (EmojiPickerDialog::PopupDirection)dir;
                _s = availablePositionSize[i];
            }
        }

        return prefferedDirection;
    }

    QPoint preferredPosition(const QPoint& _pivotPos, const QPoint& _alignPos, const QSize& _preferredSize, EmojiPickerDialog::PopupDirection _preferredDir) const
    {
        int x = 0, y = 0;
        const int ew = emojiButton_->width(), eh = emojiButton_->height();
        const int aw = alignWidget_->width(), ah = alignWidget_->height();
        const int pw = _preferredSize.width(), ph = _preferredSize.height();

        switch (_preferredDir)
        {
        case EmojiPickerDialog::Top:
            x = _alignPos.x() - ((pw - aw) / 2);
            y = _pivotPos.y() - ph;
            break;
        case EmojiPickerDialog::Bottom:
            x = _alignPos.x() - ((pw - aw) / 2);
            y = _pivotPos.y() + eh;
            break;
        case EmojiPickerDialog::Left:
            x = _pivotPos.x() - pw;
            y = _alignPos.y() - ((ph - ah) / 2);
            break;
        case EmojiPickerDialog::Right:
            x = _pivotPos.x() + ew;
            y = _alignPos.y() - ((ph - ah) / 2);
            break;
        }
        return QPoint(std::max(x, pickerMainWindowMargin()), std::max(y, pickerMainWindowMargin()));
    }

    int arrowPosition(EmojiPickerDialog::PopupDirection _preferredDir, const QRect& _pivotRect) const
    {
        switch (_preferredDir)
        {
        case EmojiPickerDialog::Top:
        case EmojiPickerDialog::Bottom:
            return _pivotRect.x() + _pivotRect.width() / 2;
        case EmojiPickerDialog::Left:
        case EmojiPickerDialog::Right:
            return _pivotRect.y() + _pivotRect.height() / 2;
        default:
            break;
        }
        return 0;
    }
};

EmojiPickerDialog::EmojiPickerDialog(QWidget* _inputWidget, QWidget* _emojiButton, QWidget* _alignWidget)
    : QWidget(Utils::InterConnector::instance().getMainWindow())
    , d(std::make_unique<EmojiPickerDialogPrivate>())
{
    im_assert(_emojiButton != nullptr);
    im_assert(_alignWidget != nullptr);

    d->allowedDirections_ = Top | Bottom;
    d->emojiButton_ = _emojiButton;
    d->alignWidget_ = _alignWidget;
    d->inputWidget_ = _inputWidget;

    if (d->inputWidget_) // need to track input widget
        qApp->installEventFilter(this);

    setAttribute(Qt::WA_DeleteOnClose);

    if (_alignWidget)
        connect(_alignWidget, &QWidget::destroyed, this, &EmojiPickerDialog::close);

    d->picker_ = new EmojiPickerWidget(this);
    Testing::setAccessibleName(d->picker_, qsl("AS EmojiPicker"));
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &EmojiPickerDialog::onWindowResized, Qt::QueuedConnection);
    connect(d->picker_, &EmojiPickerWidget::emojiSelected, this, [this](const Emoji::EmojiCode& _code){ Q_EMIT emojiSelected(_code, QPrivateSignal()); });

    onWindowResized();

   d->isGeneralDialogAncestor = Utils::hasAncestorOf<GeneralDialog*>(_inputWidget);
}

EmojiPickerDialog::~EmojiPickerDialog() = default;

void EmojiPickerDialog::setAllowedDirections(PopupDirections _directions)
{
    d->allowedDirections_ = _directions;
}

EmojiPickerDialog::PopupDirections EmojiPickerDialog::allowedDirections() const
{
    return d->allowedDirections_;
}

void EmojiPickerDialog::popup()
{
    onWindowResized();
    show();
}

void EmojiPickerDialog::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!d->inputWidget_)
        close();
    else
        QWidget::mouseReleaseEvent(_event);
}

void EmojiPickerDialog::closeEvent(QCloseEvent* _event)
{
    qApp->removeEventFilter(this);
    QWidget::closeEvent(_event);
}

void EmojiPickerDialog::focusOutEvent(QFocusEvent* _event)
{
    QWidget::focusOutEvent(_event);
    close();
}

bool EmojiPickerDialog::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == d->emojiButton_ && _event->type() == QEvent::Move)
        onWindowResized();

    if (_object == d->inputWidget_ && _event->type() == QEvent::MouseButtonRelease)
    {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(_event);
        if (mouseEvent->button() == Qt::RightButton)
            close(); // context menu is about to be opened - close the picker
    }

    if (_object == d->inputWidget_ && _event->type() == QEvent::FocusOut)
        close(); // input lost the focus - close the picker

    if (d->isGeneralDialogAncestor && qobject_cast<GeneralDialog*>(_object) && _event->type() == QEvent::MouseButtonRelease)
        close();

    return QWidget::eventFilter(_object, _event);
}

void EmojiPickerDialog::onWindowResized()
{
    im_assert(d->alignWidget_ != nullptr);
    im_assert(d->emojiButton_ != nullptr);

    const auto mainWindow = Utils::InterConnector::instance().getMainWindow();

    const QRect clientRect = mainWindow->rect().adjusted(0, mainWindow->getTitleHeight(), 0, 0);
    const QPoint alignPos = d->mapPos(d->alignWidget_, this);

    QRect pivotRect = d->emojiButton_->rect();
    pivotRect.moveTo(d->mapPos(d->emojiButton_, this));

    QSize preferredSize;
    const auto preferredDir = d->prefferedDirection(clientRect, pivotRect, preferredSize);
    const auto preferredPos = d->preferredPosition(pivotRect.topLeft(), alignPos, preferredSize, preferredDir);

    d->picker_->move(preferredPos);
    d->picker_->setFixedSize(preferredSize);
    d->picker_->repaint();

    pivotRect.moveTo(d->mapPos(d->emojiButton_, d->picker_));
    d->picker_->setArrow((EmojiPickerWidget::ArrowDirection)preferredDir, d->arrowPosition(preferredDir, pivotRect));

    setGeometry(clientRect);

    if (d->inputWidget_)
    {
        int top = 0, bottom = 0, left = 0, right = 0;
        switch (preferredDir)
        {
        case Top:
            top = left = right = pickerFrameShadowWidth();
            break;
        case Bottom:
            bottom = left = right = pickerFrameShadowWidth();
            break;
        case Right:
            bottom = top = left = pickerFrameShadowWidth();
            break;
        case Left:
            bottom = top = right = pickerFrameShadowWidth();
            break;
        }
        setMask(d->picker_->geometry().adjusted(-left, -top, right, bottom));
    }
}

}
