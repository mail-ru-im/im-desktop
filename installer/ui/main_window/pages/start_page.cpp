#include "stdafx.h"
#include "start_page.h"
#include "../../../utils/styles.h"
#include "../../../logic/tools.h"
#include "../../../common.shared/config/config.h"

namespace installer
{
    namespace ui
    {
        list_item::list_item(QWidget * _parent, const QString & _text)
            : QWidget(_parent)
        {
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            auto layout = new QHBoxLayout(this);
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

            dot_ = new QLabel(this);
            dot_->setObjectName(qsl("dot_label"));
            dot_->setFixedSize(dpi::scale(8), dpi::scale(8));
            layout->addWidget(dot_);
            layout->addSpacing(dpi::scale(8));

            textLabel_ = new QLabel(this);
            textLabel_->setText(_text);
            textLabel_->setObjectName(qsl("text_label"));
            layout->addWidget(textLabel_);
        }

        start_page::start_page(QWidget* _parent)
            : QWidget(_parent)
        {
            auto layout = new QVBoxLayout(this);
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);

            QWidget* logoWidget = new QWidget();
            logoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            logoWidget->setFixedHeight(dpi::scale(292));
            auto logoLayout = new QVBoxLayout(logoWidget);
            logoWidget->setObjectName(qsl("content_logo"));
            layout->addWidget(logoWidget);

            QWidget* textWidget = new QWidget();
            textWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            auto textLayout = new QVBoxLayout(textWidget);
            textLayout->setAlignment(Qt::AlignHCenter);

            auto welcome = new QLabel(textWidget);
            welcome->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            welcome->setObjectName(qsl("start_page_welcome_label"));
            welcome->setText(QApplication::translate("product", config::get().translation(config::translations::installer_welcome_win).data()));
            welcome->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
            textLayout->addSpacing(dpi::scale(40));
            textLayout->addWidget(welcome);

            QWidget* descriptions = new QWidget();
            descriptions->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            auto descriptionsLayout = new QVBoxLayout(descriptions);
            descriptionsLayout->setAlignment(Qt::AlignHCenter);

            descriptionsLayout->addWidget(new list_item(descriptions, QT_TR_NOOP("Cloud storage of history and files")));
            descriptionsLayout->addWidget(new list_item(descriptions, QT_TR_NOOP("Full sync with mobile")));

            textLayout->addSpacing(dpi::scale(16));
            textLayout->addWidget(descriptions);

            layout->addWidget(textWidget);
            layout->addSpacing(dpi::scale(28));

            QHBoxLayout* buttonLayout = new QHBoxLayout();
            buttonLayout->setAlignment(Qt::AlignHCenter);

            auto buttonInstall = new QPushButton(this);
            buttonInstall->setObjectName(qsl("custom_button"));
            buttonInstall->setText(QT_TR_NOOP("Install"));
            buttonInstall->setCursor(Qt::PointingHandCursor);
            buttonInstall->setFocus(Qt::MouseFocusReason);
            buttonInstall->setDefault(true);
            Testing::setAccessibleName(buttonInstall, qsl("install_button"));
            buttonLayout->addWidget(buttonInstall);


            connect(buttonInstall, &QPushButton::clicked, this, &start_page::start_install);

            layout->addLayout(buttonLayout);

            layout->addSpacing(dpi::scale(40));
        }
    }
}
