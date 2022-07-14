#include "stdafx.h"
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
    double pickerArrowRadius() noexcept { return Utils::scale_value(6); }

    int pickerFrameRadius() noexcept { return Utils::scale_value(8); }

    int margin() noexcept { return Utils::scale_value(4); }

    QSize arrowSize(Ui::EmojiPickerWidget::ArrowDirection _d) noexcept
    {
        switch (_d)
        {
        case Ui::EmojiPickerWidget::ArrowDirection::Top:
        case Ui::EmojiPickerWidget::ArrowDirection::Bottom:
            return QSize(Utils::scale_value(18), Utils::scale_value(10));
        case Ui::EmojiPickerWidget::ArrowDirection::Left:
        case Ui::EmojiPickerWidget::ArrowDirection::Right:
            return QSize(Utils::scale_value(10), Utils::scale_value(18));
        default:
            break;
        }
        return QSize();
    }

    double manhattanDistance(const QPointF& _p1, const QPointF& _p2)
    {
        return std::abs(_p1.x() - _p2.x()) + std::abs(_p1.y() - _p2.y());
    }

    QPointF lineStart(const QPointF& _curr, const QPointF& _next, double _radius)
    {
        QPointF pt;
        double r = _radius / manhattanDistance(_curr, _next);
        r = std::min(r, 0.5);
        pt.setX((1.0 - r) * _curr.x() + r * _next.x());
        pt.setY((1.0 - r) * _curr.y() + r * _next.y());
        return pt;
    }

    QPointF lineEnd(const QPointF& _curr, const QPointF& _next, double _radius)
    {
        QPointF pt;
        double r = _radius / manhattanDistance(_curr, _next);
        r = std::min(r, 0.5);
        pt.setX(r * _curr.x() + (1.0 - r) * _next.x());
        pt.setY(r * _curr.y() + (1.0 - r) * _next.y());
        return pt;
    }

    QPainterPath arrowPath(Ui::EmojiPickerWidget::ArrowDirection _arrowDir, const QRect& _rect, double _radius)
    {
        QPolygonF polygon(3);
        const double x = _rect.x();
        const double y = _rect.y();
        const double w = _rect.width();
        const double h = _rect.height();
        switch (_arrowDir)
        {
        case Ui::EmojiPickerWidget::ArrowDirection::Top:
            polygon[0] = { x, y + h };
            polygon[1] = { x + w / 2, y };
            polygon[2] = { x + w, y + h };
            break;
        case Ui::EmojiPickerWidget::ArrowDirection::Bottom:
            polygon[0] = { x, y };
            polygon[1] = { x + w / 2, y + h };
            polygon[2] = { x + w, y };
            break;
        case Ui::EmojiPickerWidget::ArrowDirection::Left:
            polygon[0] = { x + w, y };
            polygon[1] = { x, y + h / 2 };
            polygon[2] = { x + w, y + h };
            break;
        case Ui::EmojiPickerWidget::ArrowDirection::Right:
            polygon[0] = { x, y };
            polygon[1] = { x + w, y + h / 2 };
            polygon[2] = { x, y + h };
            break;
        default:
            break;
        }

        QPainterPath path;
        QPointF pt1, pt2;
        path.moveTo(polygon[0]);
        pt2 = lineEnd(polygon[0], polygon[1], _radius);
        path.lineTo(pt2);
        pt1 = lineStart(polygon[1], polygon[2], _radius);
        path.quadTo(polygon[1], pt1);
        pt2 = lineEnd(polygon[1], polygon[2], _radius);
        path.lineTo(polygon[2]);
        return path;
    }
}

namespace Ui
{
    class EmojiPickerPrivate
    {
    public:
        QPainterPath framePath_;
        Smiles::SmilesMenu* smiles_;
        QVBoxLayout* layout_;
        EmojiPickerWidget::ArrowDirection arrowDir_;
        int arrowOffset_;

        EmojiPickerPrivate()
            : arrowDir_(EmojiPickerWidget::ArrowDirection::Top)
            , arrowOffset_(Utils::scale_value(14))
        {}

        void updatePainterPath(const QRect& rect, const QMargins& margins)
        {
            static constexpr int penWidth = 1;
            const QRect frameRect = rect.marginsRemoved(margins);
            QRect arrowRect(QPoint(), arrowSize(arrowDir_));
            switch (arrowDir_)
            {
            case EmojiPickerWidget::ArrowDirection::Top:
                arrowRect.moveTo(arrowOffset_ - arrowRect.width() / 2, penWidth);
                break;
            case EmojiPickerWidget::ArrowDirection::Bottom:
                arrowRect.moveTo(arrowOffset_ - arrowRect.width() / 2, frameRect.bottom() - penWidth);
                break;
            case EmojiPickerWidget::ArrowDirection::Left:
                arrowRect.moveTo(arrowRect.height() / 2 - penWidth, arrowOffset_ - arrowRect.width() / 2);
                break;
            case EmojiPickerWidget::ArrowDirection::Right:
                arrowRect.moveTo(frameRect.right() + penWidth, arrowOffset_ - arrowRect.width() / 2);
                break;
            }

            framePath_.clear();
            framePath_.addRoundedRect(frameRect.adjusted(0, 0, -penWidth, -penWidth), pickerFrameRadius(), pickerFrameRadius());
            framePath_ |= arrowPath(arrowDir_, arrowRect, pickerArrowRadius());
        }

        int minimumOffset() const
        {
            return arrowSize(arrowDir_).width() / 2 + pickerFrameRadius();
        }
    };

    EmojiPickerWidget::EmojiPickerWidget(QWidget* _parent)
        : QWidget(_parent)
        , d(std::make_unique<EmojiPickerPrivate>())
    {
        d->smiles_ = new Smiles::SmilesMenu(this);
        d->smiles_->setToolBarVisible(false);
        d->smiles_->setRecentsVisible(false);
        d->smiles_->setStickersVisible(false);
        d->smiles_->setHorizontalMargins(Utils::scale_value(12));
        d->smiles_->setDrawTopBorder(false);
        d->smiles_->setTopSpacing(Utils::scale_value(4));

        d->layout_ = Utils::emptyVLayout(this);
        d->layout_->addWidget(d->smiles_);
        setFixedSize(maxPickerWidth(), maxPickerHeight());
        setArrow(ArrowDirection::Bottom, arrowSize(ArrowDirection::Bottom).width() / 2 + pickerFrameRadius());

        auto shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(Utils::scale_value(16));
        shadow->setOffset(0, Utils::scale_value(2));
        shadow->setColor(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_OVERLAY, 0.05));
        setGraphicsEffect(shadow);

        connect(d->smiles_, &Smiles::SmilesMenu::emojiSelected, this, [this](const Emoji::EmojiCode& _code)
            {
                Q_EMIT emojiSelected(_code, QPrivateSignal());
            });
    }

    EmojiPickerWidget::~EmojiPickerWidget() = default;

    int EmojiPickerWidget::maxPickerHeight() noexcept { return Utils::scale_value(260); }
    int EmojiPickerWidget::maxPickerWidth() noexcept { return Utils::scale_value(344); }

    void EmojiPickerWidget::setArrow(ArrowDirection _direction, int _offset)
    {
        if (_direction == d->arrowDir_ && _offset == d->arrowOffset_)
            return;

        d->arrowDir_ = _direction;

        int maximumOffset = d->minimumOffset();
        switch (d->arrowDir_)
        {
        case EmojiPickerWidget::ArrowDirection::Top:
        case EmojiPickerWidget::ArrowDirection::Bottom:
            maximumOffset = width() - pickerFrameRadius();
            break;
        case EmojiPickerWidget::ArrowDirection::Left:
        case EmojiPickerWidget::ArrowDirection::Right:
            maximumOffset = height() - pickerFrameRadius();
            break;
        }

        d->arrowOffset_ = std::clamp(_offset, d->minimumOffset(), maximumOffset);

        const int arrowHeight = arrowSize(d->arrowDir_).height();
        QMargins frameMargins;
        switch (d->arrowDir_)
        {
        case ArrowDirection::Left:
            frameMargins.setLeft(arrowHeight);
            break;
        case ArrowDirection::Top:
            frameMargins.setTop(arrowHeight);
            break;
        case ArrowDirection::Right:
            frameMargins.setRight(arrowHeight);
            break;
        case ArrowDirection::Bottom:
            frameMargins.setBottom(arrowHeight);
            break;
        }

        d->updatePainterPath(rect(), frameMargins);
        d->layout_->setContentsMargins(frameMargins += margin());

        update();
    }

    void EmojiPickerWidget::paintEvent(QPaintEvent * _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_BORDER_INVERSE));
        p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        p.drawPath(d->framePath_);
    }

    void EmojiPickerWidget::resizeEvent(QResizeEvent * _event)
    {
        QWidget::resizeEvent(_event);
        d->updatePainterPath(rect(), d->layout_->contentsMargins() -= margin());
        update();
    }
}
