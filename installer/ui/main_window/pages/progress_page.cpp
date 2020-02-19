#include "stdafx.h"
#include "progress_page.h"
#include "../../../utils/styles.h"
#include "../../../logic/tools.h"
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
			advertising_.reserve(1);
			advertising_.emplace_back(qsl(":/images/install_screen_100.png"), QT_TR_NOOP("High-quality videocalls and screen sharing"));

            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			QVBoxLayout* root_layout = new QVBoxLayout(this);

            root_layout->setSpacing(0);
			root_layout->setContentsMargins(0, 0, 0, 0);
			root_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

			demo_image_->setFixedHeight(dpi::scale(292));
            demo_image_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

			root_layout->addWidget(demo_image_);
            root_layout->addSpacing(dpi::scale(84));

            demo_text_->setObjectName(qsl("progress_page_demo_label"));
			demo_text_->setFixedSize(dpi::scale(307), dpi::scale(58));
			demo_text_->setAlignment(Qt::AlignHCenter);
			demo_text_->setWordWrap(true);
            root_layout->addWidget(demo_text_);
            root_layout->setAlignment(demo_text_, Qt::AlignHCenter);

			root_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

			progress_->setMinimum(0);
			progress_->setMaximum(100);
			progress_->setTextVisible(false);
			progress_->setFixedHeight(dpi::scale(20));
			progress_->setFixedWidth(dpi::scale(380));
            progress_->setValue(0);

			root_layout->addWidget(progress_);
            root_layout->setAlignment(progress_, Qt::AlignHCenter);

			root_layout->addItem(new QSpacerItem(0, dpi::scale(44), QSizePolicy::Fixed, QSizePolicy::Fixed));

			timer_->setInterval(3000);
			timer_->setSingleShot(false);
			connect(timer_, SIGNAL(timeout()), this, SLOT(on_timer()), Qt::QueuedConnection);
			timer_->start();

			setLayout(root_layout);

			connect(logic::get_worker(), SIGNAL(progress(int)), this, SLOT(on_progress(int)));
            draw::animateButton(progress_, true, dpi::scale(160), dpi::scale(380));
			show_advertising();
		}


        progress_page::~progress_page() = default;

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

			const QString style = ql1s("background-image:url(") % advertising_[advertising_index_].image_ % ql1s(");background-repeat: no-repeat;background-position: bottom; background-color: #F5F6FA; padding-top: 36dip;");
			demo_image_->setStyleSheet(styles::scale_style(style));

			++advertising_index_;
			if (advertising_index_ >= advertising_.size())
				advertising_index_ = 0;
		}
	}
}
