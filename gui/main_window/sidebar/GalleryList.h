#pragma once
#include "../../types/chat.h"

namespace Ui
{
    class ContextMenu;

    class MediaContentWidget : public QWidget
    {
        Q_OBJECT
    public:

        struct ItemData
        {
            ItemData() = default;

            ItemData(const QString& _link, qint64 _msg, const QString& _sender, time_t _time)
                : msg_(_msg)
                , link_(_link)
                , sender_(_sender)
                , time_(_time)
                , is_video_(false)
                , is_gif_(false)
            {
            }

            bool isNull() { return msg_ == 0 || link_.isEmpty(); }

            qint64 msg_ = -1;
            QString link_;
            QString sender_;
            time_t time_;

            bool is_video_;
            bool is_gif_;
        };

        enum Type
        {
            ImageVideo,
            Video,
            Files,
            Links,
            Audio
        };

        MediaContentWidget(Type _type, QWidget* _parent) : QWidget(_parent), type_(_type) {}
        virtual ~MediaContentWidget() = default;
        virtual void processItems(const QVector<Data::DialogGalleryEntry>& _entries) {}
        virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) {}
        virtual void initFor(const QString& _aimId) { aimId_ = _aimId; }
        virtual void markClosed() {}
        virtual void scrolled() {}

    private Q_SLOTS:
        void onMenuAction(QAction* _action);

    protected:
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

        virtual ItemData itemAt(const QPoint& _pos) { return ItemData(); }
        virtual void onClicked(ItemData _data) {}

        virtual void showContextMenu(ItemData _data, const QPoint& _pos, bool _inverted = false);

        ContextMenu* makeContextMenu(qint64 _msg, const QString& _link, const QString& _sender, time_t _time);

        QString aimId_;
        QPoint pos_;
        Type type_;
    };

    class GalleryList : public QWidget
    {
        Q_OBJECT
    public:
        GalleryList(QWidget* _parent, const QString& _type, MediaContentWidget* _contentWidget = nullptr);
        GalleryList(QWidget* _parent, const QStringList& _types, MediaContentWidget* _contentWidget = nullptr);
        virtual ~GalleryList();

        virtual void initFor(const QString& _aimId);
        virtual void markClosed();

        void setContentWidget(MediaContentWidget* _contentWidget);
        void setType(const QString _type);
        void setTypes(QStringList _types);

    protected:
        QString currentAimId() const;
        bool exhausted() const;

        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted);
        void dialogGalleryUpdate(const QString& _aimid, const QVector<Data::DialogGalleryEntry>& _entries);
        void dialogGalleryHolesDownloaded(const QString& _aimid);
        void scrolled(int value);

    protected:
        MediaContentWidget* contentWidget_;
        QScrollArea* area_;

        QString aimId_;
        QStringList types_;
        qint64 lastMsgId_;
        qint64 lastSeq_;
        qint64 requestId_;
        bool exhausted_;
    };
}
