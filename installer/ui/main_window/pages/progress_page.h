#pragma once

namespace installer
{
	namespace ui
	{
		class progress_page : public QWidget
		{
			Q_OBJECT
			
		private Q_SLOTS:

			void on_progress(int _progress);
			void on_timer();

		private:

			QLabel*			demo_text_;
			QWidget*		demo_image_;
			QProgressBar*	progress_;
			QTimer*			timer_;

			struct image_text
			{
				const QString	image_;
				const QString	text_;

				image_text(
					const QString& _image, 
					const QString& _text)
					: 
					image_(_image), 
					text_(_text)
				{
				}
			};

			std::vector<image_text>		advertising_;

			unsigned int				advertising_index_;


			void show_advertising();

		public:

			progress_page(QWidget* _parent);
			virtual ~progress_page();
		};

	}
}
