#include "stdafx.h"
#include "start_page.h"
#include "../../../utils/styles.h"
#include "../checkbox.h"

namespace installer
{
    namespace ui
    {
        start_page::start_page(QWidget* _parent, const bool _offerBundle)
            : QWidget(_parent)
            , cbHomePage_(nullptr)
            , cbSearch_(nullptr)
        {
            auto layout = new QVBoxLayout(this);
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignTop);

            layout->addSpacing(dpi::scale(60));

            auto logo = new QWidget(this);
            logo->setObjectName(build::get_product_variant("logo_icq", "logo_agent", "logo_biz", "logo_dit"));
            logo->setFixedHeight(dpi::scale(100));
            layout->addWidget(logo);

            layout->addSpacing(dpi::scale(28));

            auto welcome = new QLabel(this);
            welcome->setObjectName(build::get_product_variant("start_page_welcome_label_icq", "start_page_welcome_label_agent", "start_page_welcome_label_biz", "start_page_welcome_label_dit"));
            welcome->setText(build::get_product_variant(QT_TR_NOOP("Welcome to ICQ"), QT_TR_NOOP("Welcome to Mail.ru Agent"), QT_TR_NOOP("Welcome to Myteam"), QT_TR_NOOP("Welcome to Messenger")));
            welcome->setAlignment(Qt::AlignHCenter);
            layout->addWidget(welcome);

            if (_offerBundle)
            {
                layout->addSpacing(dpi::scale(72));

                auto makeCheckBox = [&layout, this](const auto& _text)
                {
                    QHBoxLayout* checkboxLayout = new QHBoxLayout();
                    checkboxLayout->setSpacing(0);
                    checkboxLayout->setAlignment(Qt::AlignLeft);
                    checkboxLayout->setContentsMargins(0, 0, 0, 0);
                    checkboxLayout->addSpacing(dpi::scale(88));

                    auto cb = new BundleCheckBox(this, _text);
                    cb->setChecked(true);

                    checkboxLayout->addWidget(cb);
                    layout->addLayout(checkboxLayout);

                    return cb;
                };

                cbHomePage_ = makeCheckBox(QT_TR_NOOP("Make Mail.Ru my homepage and add bookmarks in the browser."));

                layout->addSpacing(dpi::scale(12));

                cbSearch_ = makeCheckBox(QT_TR_NOOP("Make Mail.Ru Search my default search engine."));
            }

            layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

            QHBoxLayout* buttonLayout = new QHBoxLayout();
            buttonLayout->setAlignment(Qt::AlignHCenter);
            {
                auto buttonInstall = new QPushButton(this);
                buttonInstall->setObjectName(build::get_product_variant("custom_button", "custom_button", "custom_button_biz", "custom_button_dit"));
                buttonInstall->setText(QT_TR_NOOP("INSTALL"));
                buttonInstall->setCursor(Qt::PointingHandCursor);
                buttonInstall->setFocus(Qt::MouseFocusReason);
                buttonInstall->setDefault(true);
                Testing::setAccessibleName(buttonInstall, "install_button");

                buttonLayout->addWidget(buttonInstall);

                connect(buttonInstall, &QPushButton::clicked, this, &start_page::start_install);
            }
            layout->addLayout(buttonLayout);

            layout->addSpacing(dpi::scale(20));
        }

        bool start_page::isHomePageSelected() const
        {
            return cbHomePage_ && cbHomePage_->isChecked();
        }

        bool start_page::isSearchSelected() const
        {
            return cbSearch_ && cbSearch_->isChecked();
        }
    }
}
