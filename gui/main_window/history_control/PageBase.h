#pragma once

#include "types/message.h"
#include "main_window/EscapeCancellable.h"

namespace hist
{
    enum class scroll_mode_type;
}


namespace  Ui
{
    using highlightsV = std::vector<QString>;

    class PageBase : public QWidget, public IEscapeCancellable
    {
        Q_OBJECT

    Q_SIGNALS:
        void switchToPrevDialog(const bool _byKeyboard) const;

    public:
        PageBase(QWidget* _parent) : QWidget(_parent), IEscapeCancellable(this) {}

        virtual void cancelSelection() {}
        virtual void suspendVisisbleItems() {}
        virtual void pageLeave() {}
        virtual void setPinnedMessage(Data::MessageBuddySptr _msg) {}
        virtual void setUnreadBadgeText(const QString& _text) {}
        virtual const QString& aimId() const { static QString empty; return empty; }
        virtual void resetMessageHighlights() {}
        virtual void setHighlights(const highlightsV& _highlights) {}
        virtual void updateWidgetsTheme() {}

        virtual void setPrevChatButtonVisible(bool _visible) {}
        virtual void setOverlayTopWidgetVisible(bool _visible) {}

        enum class FirstInit
        {
            No,
            Yes
        };
        virtual void initFor(qint64 _id, hist::scroll_mode_type _type, FirstInit _firstInit = FirstInit::No) {}

        virtual void open() {}
        virtual QWidget* getTopWidget() const { return nullptr; }
        virtual void pageOpen() {}
        virtual void updateItems() {}
    };
}
