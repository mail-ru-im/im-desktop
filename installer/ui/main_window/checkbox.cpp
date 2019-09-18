#include "stdafx.h"
#include "checkbox.h"

namespace installer
{
    namespace ui
    {
        BundleCheckBox::BundleCheckBox(QWidget* _parent, const QString& _text)
            : QAbstractButton(_parent)
        {
            auto layout = new QHBoxLayout(this);
            layout->setSpacing(0);
            layout->setAlignment(Qt::AlignLeft);
            layout->setContentsMargins(0, 0, 0, 0);

            auto btnVLayout = new QVBoxLayout();
            btnVLayout->setSpacing(0);
            btnVLayout->setAlignment(Qt::AlignTop);
            btnVLayout->setContentsMargins(0, 0, 0, 0);

            icon_ = new QWidget(this);
            icon_->setFixedSize(dpi::scale(20), dpi::scale(20));
            icon_->setObjectName("bundle_cb_icon");
            icon_->setProperty("chk", false);
            icon_->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnVLayout->addWidget(icon_);
            layout->addLayout(btnVLayout);

            layout->addSpacing(dpi::scale(12));

            label_ = new QLabel(_text, this);
            label_->setFixedSize(dpi::scale(300), dpi::scale(36));
            label_->setWordWrap(true);
            label_->setAttribute(Qt::WA_TransparentForMouseEvents);
            label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            layout->addWidget(label_);

            QFont f(label_->font());
            f.setPixelSize(dpi::scale(13));
            label_->setFont(f);

            setHovered(false);

            setCheckable(true);
            setCursor(Qt::PointingHandCursor);
            setMouseTracking(true);
            setFocusPolicy(Qt::NoFocus);

            connect(this, &QAbstractButton::toggled, this, [this](const bool _checked)
            {
                icon_->setProperty("chk", _checked);
                icon_->setStyle(QApplication::style());

                if (!_checked)
                    setHovered(false);

                update();
            });
        }

        void BundleCheckBox::paintEvent(QPaintEvent* _e)
        {
        }

        void BundleCheckBox::setHovered(const bool _hovered)
        {
            static const QString normalColor = qsl("#666666");
            static const QString hoverColor  = qsl("#444444");
            label_->setStyleSheet(qsl("color:") % (_hovered ? hoverColor : normalColor));

            icon_->setProperty("hvr", _hovered);
            icon_->setStyle(QApplication::style());

            update();
        }

        void BundleCheckBox::enterEvent(QEvent* _e)
        {
            setHovered(true);

            QAbstractButton::enterEvent(_e);
        }

        void BundleCheckBox::leaveEvent(QEvent* _e)
        {
            setHovered(false);

            QAbstractButton::leaveEvent(_e);
        }

    }
}