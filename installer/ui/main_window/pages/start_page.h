#pragma once

namespace installer
{
	namespace ui
	{
        class BundleCheckBox;

		class start_page : public QWidget
		{
			Q_OBJECT

		Q_SIGNALS:
			void start_install();

		public:
            start_page(QWidget* _parent, const bool _offerBundle);

            bool isHomePageSelected() const;
            bool isSearchSelected() const;

        private:
            BundleCheckBox* cbHomePage_;
            BundleCheckBox* cbSearch_;
		};
	}
}
