#pragma once

namespace installer
{
	namespace ui
	{
		class error_page : public QWidget
		{
			Q_OBJECT

		private Q_SLOTS:

			void on_close_button_click(bool);

		Q_SIGNALS:

			void close();

		private:

			installer::error	err_;

		public:

			error_page(QWidget* _parent, installer::error _err);
			virtual ~error_page(void);
		};
	}
}

