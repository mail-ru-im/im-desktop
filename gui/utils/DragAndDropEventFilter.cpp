#include "stdafx.h"
#include "DragAndDropEventFilter.h"
#include "InterConnector.h"
#include "../main_window/MainWindow.h"
#include "utils/MimeDataUtils.h"

namespace
{
    constexpr std::chrono::milliseconds dragActivateDelay = std::chrono::milliseconds(500);
}

namespace Utils
{
    DragAndDropEventFilter::DragAndDropEventFilter(QWidget* _parent)
        : QObject(_parent)
        , dragMouseoverTimer_(new QTimer(this))
    {
        dragMouseoverTimer_->setInterval(dragActivateDelay);
        dragMouseoverTimer_->setSingleShot(true);
        connect(dragMouseoverTimer_, &QTimer::timeout, this, &DragAndDropEventFilter::onTimer);
    }

    void DragAndDropEventFilter::onTimer()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    bool DragAndDropEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::DragMove)
        {
            dragMouseoverTimer_->start();

            QDropEvent* de = static_cast<QDropEvent*>(_event);
            auto acceptDrop = de->mimeData() && (de->mimeData()->hasUrls() || MimeData::isMimeDataWithImage(de->mimeData()));
            if (QApplication::activeModalWidget() == nullptr && acceptDrop)
            {
                de->acceptProposedAction();
                Q_EMIT dragPositionUpdate(de->pos(), QPrivateSignal {});
            }
            else
            {
                de->setDropAction(Qt::IgnoreAction);
            }
            return true;
        }
        else if (_event->type() == QEvent::DragLeave)
        {
            dragMouseoverTimer_->stop();
            Q_EMIT dragPositionUpdate(QPoint(), QPrivateSignal {});
            return true;
        }
        else if (_event->type() == QEvent::Drop)
        {
            dragMouseoverTimer_->stop();
            onTimer();

            QDropEvent* e = static_cast<QDropEvent*>(_event);
            Q_EMIT dropMimeData(e->pos(), e->mimeData(), QPrivateSignal {});
            e->acceptProposedAction();
            Q_EMIT dragPositionUpdate(QPoint(), QPrivateSignal {});
            return true;
        }
        else if (_event->type() == QEvent::MouseButtonDblClick)
        {
            _event->ignore();
            return true;
        }

        return QObject::eventFilter(_obj, _event);
    }
} // namespace Utils
