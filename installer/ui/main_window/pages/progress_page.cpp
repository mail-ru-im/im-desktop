#include "stdafx.h"
#include "progress_page.h"
#include "../../../utils/styles.h"
#include "../../../logic/worker.h"

namespace installer
{
	namespace ui
	{
		progress_page::progress_page(QWidget* _parent)
			:	QWidget(_parent),
				demo_text_(new QLabel(this)),
				demo_image_(new QWidget(this)),
				progress_(new QProgressBar(this)),
				timer_(new QTimer(this)),
				advertising_index_(0)

		{
			advertising_.reserve(3);
			advertising_.emplace_back(":/images/content_illustration_calls_100.jpg", QT_TR_NOOP("Free video and voice calls"));
			advertising_.emplace_back(":/images/content_illustration_stickers_100.jpg", QT_TR_NOOP("Hundreds of stickers are always free"));
			advertising_.emplace_back(":/images/content_illustration_stickers_groups_100.jpg", QT_TR_NOOP("Unlimited and free group chats"));

			QVBoxLayout* root_layout = new QVBoxLayout(this);
			{
				root_layout->setSpacing(0);
				root_layout->setContentsMargins(0, 0, 0, 0);
				root_layout->setAlignment(Qt::AlignTop);

				demo_text_->setObjectName("progress_page_demo_label");
				demo_text_->setFixedHeight(dpi::scale(76));
				demo_text_->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
				demo_text_->setWordWrap(true);
				root_layout->addWidget(demo_text_);

				root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

				demo_image_->setFixedHeight(dpi::scale(256));

				root_layout->addWidget(demo_image_);

				root_layout->addSpacing(dpi::scale(12));

                progress_->setObjectName(build::get_product_variant("base", "base", "biz", "dit"));
				progress_->setMinimum(0);
				progress_->setMaximum(100);
				progress_->setTextVisible(false);
				progress_->setFixedHeight(dpi::scale(16));

				root_layout->addWidget(progress_);
				root_layout->addSpacing(dpi::scale(16));

				timer_->setInterval(3000);
				timer_->setSingleShot(false);
				connect(timer_, SIGNAL(timeout()), this, SLOT(on_timer()), Qt::QueuedConnection);
				timer_->start();

			}

			setLayout(root_layout);

			connect(logic::get_worker(), SIGNAL(progress(int)), this, SLOT(on_progress(int)));

			show_advertising();
		}


		progress_page::~progress_page()
		{
		}

		void progress_page::on_progress(int _progress)
		{
			progress_->setValue(_progress);
		}

		void progress_page::on_timer()
		{
			show_advertising();
		}

		void progress_page::show_advertising()
		{
			demo_text_->setText(advertising_[advertising_index_].text_);

			QString style = "background-image:url(" + advertising_[advertising_index_].image_ + ");background-repeat: no-repeat;background-position: center;";
			demo_image_->setStyleSheet(styles::scale_style(style));

			++advertising_index_;
			if (advertising_index_ >= advertising_.size())
				advertising_index_ = 0;
		}
	}
}
