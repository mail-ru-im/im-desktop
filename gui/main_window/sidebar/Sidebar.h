#pragma once
#include "utils/utils.h"
#include "main_window/EscapeCancellable.h"

namespace Ui
{
    class SemitransparentWindowAnimated;
    class SlideController;

    enum class FrameCountMode;

    struct SidebarParams
    {
        bool showFavorites_ = true;
        FrameCountMode frameCountMode_;
        QString parentPageId_;
    };

    class SidebarPage : public QWidget, public IEscapeCancellable
    {
        Q_OBJECT

    public:
        static constexpr std::chrono::milliseconds kLoadDelay = std::chrono::milliseconds(60);
        static constexpr std::chrono::milliseconds kShowDelay = std::chrono::milliseconds(350);
        static constexpr std::chrono::milliseconds kShowDuration = std::chrono::milliseconds(100);

        explicit SidebarPage(QWidget* _parent)
            : QWidget(_parent)
            , IEscapeCancellable(this)
            , isActive_(false)
        {
            Utils::setDefaultBackground(this);
        }
        virtual ~SidebarPage() {}

        virtual const QString& prev() const { return prevAimId_; }
        virtual void setPrev(const QString& _aimId) { prevAimId_ = _aimId; }
        virtual void initFor(const QString& _aimId, SidebarParams _params = {}) = 0;
        virtual void setFrameCountMode(FrameCountMode _mode) = 0;
        virtual void close() = 0;
        virtual void setMembersVisible(bool) { }
        virtual bool isMembersVisible() const { return false; }
        virtual bool isActive() const { return isActive_; }
        virtual QString getSelectedText() const { return {}; }
        virtual void scrollToTop() { }
        virtual QString parentAimId() const { return {}; }
        virtual void onPageBecomeVisible() {}

    Q_SIGNALS:
        void contentsChanged(QPrivateSignal);

    protected:
        void emitContentsChanged()
        {
            Q_EMIT contentsChanged(QPrivateSignal());
        }

    protected:
        QString prevAimId_;
        bool isActive_;
    };

    //////////////////////////////////////////////////////////////////////////

    class Sidebar : public QStackedWidget, public IEscapeCancellable
    {
        Q_OBJECT

    Q_SIGNALS:
        void pageChanged(const QString& _aimId, QPrivateSignal) const;

    public:
        explicit Sidebar(QWidget* _parent = nullptr);
        ~Sidebar();

        void preparePage(const QString& _aimId, SidebarParams _params = {}, bool _selectionChanged = false);
        void showMembers(const QString& _aimId);

        enum class ResolveThread
        {
            No,
            Yes,
        };
        QString currentAimId(ResolveThread _resolve = ResolveThread::No) const;
        QString parentAimId() const;
        void updateSize();
        void changeHeight();

        bool isThreadOpen() const;
        bool wasVisible() const noexcept;

        bool isNeedShadow() const noexcept;
        void setNeedShadow(bool _value);

        void setFrameCountMode(FrameCountMode _mode);

        void setWidth(int _width);

        static int getDefaultWidth();
        static constexpr QStringView getThreadPrefix() { return u"thread/"; }

        QString getSelectedText() const;

    public Q_SLOTS:
        void showAnimated();
        void hideAnimated();

        void show();
        void hide();

    private:
        void moveToWindowEdge();
        void closePage(SidebarPage* _page);

    private Q_SLOTS:
        void animate(const QVariant& _value);
        void onAnimationFinished();
        void onCurrentChanged(int _index);
        void onContentChanged();
        void onIndexChanged(int _index);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;

    private:
        friend class SidebarPrivate;
        std::unique_ptr<class SidebarPrivate> d;
    };
}
