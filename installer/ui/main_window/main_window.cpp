#include "stdafx.h"
#include "main_window.h"
#include "../../utils/dpi.h"

#include "pages/start_page.h"
#include "pages/progress_page.h"
#include "pages/error_page.h"

#include "../../logic/worker.h"
#include "../../logic/bundle_installer.h"
#include "../../logic/tools.h"

namespace installer
{
    namespace ui
    {
        main_window::main_window(main_window_start_page _start_page, const bool _autoInstall)
            : pages_(new QStackedWidget(this))
            , start_page_(nullptr)
            , progress_page_(nullptr)
            , error_page_(nullptr)
        {
            setFixedSize(dpi::scale(520), dpi::scale(456));

            setWindowTitle(build::get_product_variant(QT_TR_NOOP("ICQ Setup"), QT_TR_NOOP("Mail.ru Agent Setup"), QT_TR_NOOP("Myteam Setup"), QT_TR_NOOP("Messenger Setup")));

            // to disable bundle, define NO_BUNDLE in
            // * project properties->C/C++->Preprocessor
            // * project properties->Resources->General
            // * comment pack_bundle() in qt_prebuld.py

#ifndef NO_BUNDLE
            const auto offerBundle = logic::get_translator()->getLang() == qsl("ru");
#else
            constexpr auto offerBundle = false;
#endif
            switch(_start_page)
            {
            case main_window_start_page::page_start:
                start_page_ = new start_page(this, offerBundle);
                pages_->addWidget(start_page_);
                break;

            case main_window_start_page::page_progress:
                progress_page_ = new progress_page(this);
                pages_->addWidget(progress_page_);
                break;

            case main_window_start_page::page_error:
                error_page_ = new error_page(this, installer::error());
                pages_->addWidget(error_page_);
                break;

            default:
                assert(false);
                break;
            }

            setCentralWidget(pages_);

            if (start_page_)
                connect(start_page_, &start_page::start_install, this, &main_window::on_start_install);

            if (_autoInstall)
                QTimer::singleShot(500, this, &main_window::on_start_install);
        }

        main_window::~main_window()
        {
        }

        void main_window::paintEvent(QPaintEvent* _e)
        {
            QStyleOption opt;
            opt.init(this);
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

            return QMainWindow::paintEvent(_e);
        }

        void main_window::on_start_install()
        {
            assert(!progress_page_);

            if (!progress_page_)
            {
                progress_page_ = new progress_page(this);
                pages_->addWidget(progress_page_);
            }

            pages_->setCurrentWidget(progress_page_);

            start_installation();

#ifndef NO_BUNDLE
            installBundle();
#endif
        }

        void main_window::on_finish()
        {
            close();
        }

        void main_window::start_installation()
        {
            auto worker = logic::get_worker();
            connect(worker, &logic::worker::finish, this, &main_window::on_finish);
            connect(worker, &logic::worker::error, this, &main_window::on_error);

            worker->install();
        }

        void main_window::installBundle()
        {
            if (!start_page_)
                return;

            const auto installHP = start_page_->isHomePageSelected();
            const auto installSearch = start_page_->isSearchSelected();

            logic::runBundle(installHP, installSearch);
        }

        void main_window::on_error(installer::error _err)
        {
            if (!error_page_)
            {
                error_page_ = new error_page(this, _err);
                pages_->addWidget(error_page_);

                connect(error_page_, &error_page::close, this, &main_window::on_close);
            }

            pages_->setCurrentWidget(error_page_);
        }

        void main_window::on_close()
        {
            close();
        }
    }
}