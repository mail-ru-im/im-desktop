#include "stdafx.h"
#include "EmojiButton.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "styles/ThemeParameters.h"
#include "../statuses/StatusCommonUi.h"

namespace
{
    constexpr auto animationDuration() noexcept { return std::chrono::milliseconds(150); }

    QSize defaultEmojiButtonSize() noexcept { return Utils::scale_value(QSize(52, 52)); }

    QSize addEmojiIconSize() noexcept { return Utils::scale_value(QSize(32, 32)); }

    int emojiSize() noexcept { return Utils::scale_value(32); }

    QPoint editIconOffset() noexcept { return Utils::scale_value(QPoint(32, 32)); }

    QColor circleColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.1);
    }

    QPixmap addEmojiIcon()
    {
        static auto pixmap = Utils::StyledPixmap(qsl(":/add_emoji_icon"), addEmojiIconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
        return pixmap.actualPixmap();
    }
}

namespace Ui
{

    class EmojiButtonPrivate
    {
    public:
        EmojiButtonPrivate(QWidget* _q) : qptr_(_q) {}

        void drawEmoji(QPainter& _p)
        {
            Utils::PainterSaver saver(_p);
            _p.setOpacity(emojiOpacity_);
            QPainterPath circle;
            circle.addEllipse(QRect({ 0, 0 }, defaultEmojiButtonSize()));
            _p.fillPath(circle, circleColor());

            _p.drawImage(iconRect_, emoji_);

            Statuses::drawEditIcon(_p, editIconOffset());
        }

        void drawIcon(QPainter& _p)
        {
            Utils::PainterSaver saver(_p);
            _p.setOpacity(iconOpacity_);
            _p.drawPixmap(iconRect_, addEmojiIcon());
        }

        void initAnimation()
        {
            animation_ = new QTimeLine(animationDuration().count(), qptr_);
            animation_->setEasingCurve(QEasingCurve::InOutSine);

            auto onValueChanged = [this](double value)
            {
                emojiOpacity_ = value;
                iconOpacity_ = 1 - emojiOpacity_;
                qptr_->update();
            };
            QObject::connect(animation_, &QTimeLine::valueChanged, qptr_, onValueChanged);
            QObject::connect(animation_, &QTimeLine::finished, qptr_, [this]()
                {
                    if (animation_->direction() == QTimeLine::Backward)
                        emoji_ = QImage();
                });
        }

        void showEmojiAnimated()
        {
            animation_->setDirection(QTimeLine::Forward);
            animation_->start();
        }

        void showEmoji()
        {
            animation_->stop();
            emojiOpacity_ = 1;
            iconOpacity_ = 0;
            qptr_->update();
        }

        void hideEmoji()
        {
            emoji_ = QImage();
            code_.clear();
            emojiOpacity_ = 0;
            iconOpacity_ = 1;
            qptr_->update();
        }

        QRect iconRect_;
        QRect editIconRect_;
        QTimeLine* animation_ = nullptr;
        double emojiOpacity_ = 0;
        double iconOpacity_ = 1;
        QString code_;
        QImage emoji_;
        QWidget* qptr_;
    };

    EmojiButton::EmojiButton(QWidget* _parent)
        : QWidget(_parent)
        , d(std::make_unique<EmojiButtonPrivate>(this))
    {
        setCursor(Qt::PointingHandCursor);
        setFixedSize(defaultEmojiButtonSize());
        d->initAnimation();
    }

    EmojiButton::~EmojiButton() = default;

    void EmojiButton::setCurrent(const Emoji::EmojiCode& _code, SkipAnimation _skipAnimation)
    {
        const auto animate = d->emoji_.isNull() && _skipAnimation != SkipAnimation::Yes;
        d->emoji_ = Emoji::GetEmoji(_code, Utils::scale_bitmap(emojiSize()));
        d->code_ = Emoji::EmojiCode::toQString(_code);

        if (animate)
            d->showEmojiAnimated();
        else
            d->showEmoji();
    }

    const QString& EmojiButton::current() const
    {
        return d->code_;
    }

    void EmojiButton::reset()
    {
        d->hideEmoji();
    }

    void EmojiButton::setFixedSize(QSize _size)
    {
        QWidget::setFixedSize(_size);

        const auto offset = (size() - addEmojiIconSize()) / 2;
        d->iconRect_ = QRect({ offset.width(), offset.height() }, addEmojiIconSize());
    }

    void EmojiButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        d->drawEmoji(p);
        d->drawIcon(p);
    }

    void EmojiButton::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (rect().contains(_event->pos()))
            Q_EMIT clicked(QPrivateSignal());
        else
            QWidget::mouseReleaseEvent(_event);
    }
}
