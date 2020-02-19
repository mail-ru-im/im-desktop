#pragma once

namespace installer
{
	namespace ui
	{
        class BundleCheckBox;

        class list_item : public QWidget
        {
        public:
            list_item(QWidget* _parent, const QString& _text);
        private:
            QLabel* dot_;
            QLabel* textLabel_;
        };

		class start_page : public QWidget
		{
			Q_OBJECT

		Q_SIGNALS:
			void start_install();

		public:
            explicit start_page(QWidget* _parent);
		};
	}
}
