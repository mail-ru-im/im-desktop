#pragma once
#include "../../types/chat.h"

namespace Tooltip
{
    enum class ArrowDirection;
    enum class ArrowPointPos;
}

namespace Ui
{
    class ContextMenu;
    class ScrollAreaWithTrScrollBar;

    enum class MediaContentType
    {
        Invalid,

        ImageVideo,
        Video,
        Files,
        Links,
        Voice,
    };

    QString getGalleryTitle(MediaContentType _type);
    int countForType(const Data::DialogGalleryState& _state, MediaContentType _type);


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
            {
            }

            bool isNull() { return msg_ == 0 || link_.isEmpty(); }

            qint64 msg_ = -1;
            QString link_;
            QString sender_;
            time_t time_;

            bool is_video_ = false;
            bool is_gif_ = false;
        };

        MediaContentWidget(MediaContentType _type, QWidget* _parent) : QWidget(_parent), type_(_type) {}
        virtual ~MediaContentWidget() = default;
        virtual void processItems(const QVector<Data::DialogGalleryEntry>& _entries) {}
        virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) {}
        virtual void initFor(const QString& _aimId) { aimId_ = _aimId; }
        virtual void markClosed() {}
        virtual void scrolled() {}

    private Q_SLOTS:
        void onMenuAction(QAction* _action);

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

        virtual ItemData itemAt(const QPoint& _pos) { return ItemData(); }
        virtual void onClicked(ItemData _data) {}

        virtual void showContextMenu(ItemData _data, const QPoint& _pos, bool _inverted = false);

        ContextMenu* makeContextMenu(qint64 _msg, const QString& _link, const QString& _sender, time_t _time, const QString& _aimid);

        bool isTooltipActivated() const;
        void showTooltip(QString _text, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos);
        void hideTooltip();

        QString aimId_;
        QPoint pos_;
        MediaContentType type_;

        QTimer* tooltipTimer_ = nullptr;
        bool tooltipActivated_ = false;

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
        void setType(const QString& _type);
        void setTypes(QStringList _types);

        void openContentFor(const QString& _aimId, MediaContentType _type);
        void openContentForType(MediaContentType _type);

        void setMaxContentWidth(int _width);

    protected:
        const QString& currentAimId() const;
        bool exhausted() const;

        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted);
        void dialogGalleryUpdate(const QString& _aimid, const QVector<Data::DialogGalleryEntry>& _entries);
        void dialogGalleryHolesDownloaded(const QString& _aimid);
        void scrolled(int value);

    protected:
        MediaContentWidget* contentWidget_;
        ScrollAreaWithTrScrollBar* area_;

        QString aimId_;
        QStringList types_;
        qint64 lastMsgId_;
        qint64 lastSeq_;
        qint64 requestId_;
        bool exhausted_;
    };
}
