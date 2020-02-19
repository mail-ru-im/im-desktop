#include "stdafx.h"
#include "main_window.h"
#include "../../utils/dpi.h"

#include "pages/start_page.h"
#include "pages/progress_page.h"
#include "pages/error_page.h"

#include "../../logic/worker.h"
#include "../../logic/tools.h"
#include "../../../common.shared/config/config.h"

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
            setFixedSize(dpi::scale(792), dpi::scale(592));

            setWindowTitle(QApplication::translate("product", config::get().translation(config::translations::installer_title_win).data()));

            switch(_start_page)
            {
            case main_window_start_page::page_start:
                start_page_ = new start_page(this);

                transitionLabel_ = new QLabel(this);
                transitionLabel_->hide();
                transitionLabel_->setAttribute(Qt::WA_TranslucentBackground);
                transitionLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);

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

        main_window::~main_window() = default;

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
            QRect buttonRect(0, dpi::scale(512), dpi::scale(792), dpi::scale(80));
            QPoint rectTopLeft(buttonRect.topLeft());
            QPixmap pm(buttonRect.size());

            pm.fill(Qt::transparent);
            render(&pm, QPoint(), buttonRect, DrawChildren);

            transitionLabel_->move(rectTopLeft);
            transitionLabel_->setFixedSize(buttonRect.size());
            transitionLabel_->setPixmap(pm);

            auto effect = new QGraphicsOpacityEffect(transitionLabel_);
            effect->setOpacity(1.0);
            transitionLabel_->setGraphicsEffect(effect);

            auto transitionAnim_ = new QPropertyAnimation(effect, "opacity", transitionLabel_);
            transitionAnim_->setDuration(100);

            transitionAnim_->setStartValue(1.0);
            transitionAnim_->setEndValue(0.0);
            transitionAnim_->setEasingCurve(QEasingCurve::InCirc);

            connect(transitionAnim_, &QPropertyAnimation::finished, transitionLabel_, &QLabel::hide);

            transitionLabel_->raise();

            transitionLabel_->show();
            transitionAnim_->start();

            pages_->setCurrentWidget(progress_page_);

            start_installation();
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