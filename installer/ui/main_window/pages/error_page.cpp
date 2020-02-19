#include "stdafx.h"
#include "error_page.h"
#include "../../../utils/styles.h"
#include "../../../common.shared/config/config.h"

namespace installer
{
	namespace ui
	{
		error_page::error_page(QWidget* _parent, installer::error _err)
			:	QWidget(_parent),
				err_(_err)
		{
			QVBoxLayout* root_layout = new QVBoxLayout(this);
			{
				root_layout->setSpacing(0);
				root_layout->setContentsMargins(0, 0, 0, 0);
				root_layout->setAlignment(Qt::AlignTop);

				QLabel* red_text = new QLabel(this);

				red_text->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
				red_text->setObjectName(qsl("error_page_caption"));
				red_text->setText(QT_TR_NOOP("Houston, we have a problem"));
				red_text->setFixedHeight(dpi::scale(72));
				root_layout->addWidget(red_text);

				QLabel* comment_text = new QLabel(this);
				comment_text->setObjectName(qsl("error_page_comment"));
				comment_text->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
                comment_text->setText(QApplication::translate("product", config::get().translation(config::translations::installer_failed_win).data()));
				comment_text->setFixedHeight(dpi::scale(32));
				root_layout->addWidget(comment_text);

				root_layout->addSpacing(dpi::scale(28));

				QWidget* image = new QWidget(this);
				image->setFixedHeight(dpi::scale(152));
				QString style_string = qsl("background-image:url(:/images/content_illustration_facepalm_100.jpg);background-repeat: no-repeat;background-position: center;");
				image->setStyleSheet(styles::scale_style(style_string));
				root_layout->addWidget(image);

				root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

				QHBoxLayout* error_layout = new QHBoxLayout();
				error_layout->setAlignment(Qt::AlignHCenter);
				{
					QLabel* error_code = new QLabel(this);
					error_code->setObjectName(qsl("error_page_error_code"));
					error_code->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
					error_code->setText(QT_TR_NOOP("Error code: ") % err_.get_code_string() % ql1s(". "));
					error_layout->addWidget(error_code);

					QLabel* label_support_link = new QLabel(this);
					label_support_link->setObjectName(qsl("error_page_support_link"));
					label_support_link->setText(ql1s("<a href=\"http://www.icq.com/support/?open-form\" style=\"color: #57b359;\">") % QT_TR_NOOP("Write to developer") % ql1s("</a>"));
					label_support_link->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
					label_support_link->setOpenExternalLinks(true);
					error_layout->addWidget(label_support_link);
				}

				root_layout->addLayout(error_layout);

				root_layout->addSpacing(dpi::scale(24));

				QHBoxLayout* button_layout = new QHBoxLayout();
				button_layout->setAlignment(Qt::AlignHCenter);
				{
					QPushButton* button_close = new QPushButton(this);
					button_close->setContentsMargins(40, 0, 40, 0);
					button_close->setFixedHeight(dpi::scale(40));
                    button_close->setObjectName(qsl("custom_button"));
					button_close->setText(QT_TR_NOOP("Close"));
					button_close->setCursor(QCursor(Qt::PointingHandCursor));
					button_layout->addWidget(button_close);

					connect(button_close, SIGNAL(clicked(bool)), this, SLOT(on_close_button_click(bool)), Qt::QueuedConnection);
				}
				root_layout->addLayout(button_layout);

				root_layout->addSpacing(dpi::scale(16));
			}

			setLayout(root_layout);
		}


		error_page::~error_page()
		{
		}

		void error_page::on_close_button_click(bool)
		{
			emit close();
		}


	}
}