#pragma once
#include "utils/utils.h"

namespace Ui
{
    class SemitransparentWindowAnimated;
    class SlideController;

    enum class FrameCountMode;

    struct SidebarParams
    {
        bool showFavorites_ = true;
        FrameCountMode frameCountMode_;
    };

    class SidebarPage : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr std::chrono::milliseconds kLoadDelay = std::chrono::milliseconds(60);
        static constexpr std::chrono::milliseconds kShowDelay = std::chrono::milliseconds(350);
        static constexpr std::chrono::milliseconds kShowDuration = std::chrono::milliseconds(100);

        explicit SidebarPage(QWidget* _parent) : QWidget(_parent), isActive_(false) {}
        virtual ~SidebarPage() {}

        virtual void setPrev(const QString& _aimId) { prevAimId_ = _aimId; }
        virtual void initFor(const QString& _aimId, SidebarParams _params = {}) = 0;
        virtual void setFrameCountMode(FrameCountMode _mode) = 0;
        virtual void close() = 0;
        virtual void setMembersVisible(bool) { }
        virtual bool isMembersVisible() const { return false; }
        virtual bool isActive() { return isActive_; }
        virtual QString getSelectedText() const { return QString(); }
        virtual void scrollToTop() { }

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

    class Sidebar : public QStackedWidget
    {
        Q_OBJECT

    public:
        explicit Sidebar(QWidget* _parent = nullptr);
        ~Sidebar();

        void preparePage(const QString& _aimId, SidebarParams _params = {}, bool _selectionChanged = false);
        void showMembers(const QString& _aimId);

        QString currentAimId() const;
        void updateSize();

        bool isNeedShadow() const noexcept;
        void setNeedShadow(bool _value);

        void setFrameCountMode(FrameCountMode _mode);

        void setWidth(int _width);

        static int getDefaultWidth();

        QString getSelectedText() const;

    public Q_SLOTS:
        void showAnimated();
        void hideAnimated();

    private Q_SLOTS:
        void animate(const QVariant& _value);
        void onAnimationFinished();
        void onCurrentChanged(int _index);
        void onContentChanged();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        friend class SidebarPrivate;
        std::unique_ptr<class SidebarPrivate> d;
    };
}
