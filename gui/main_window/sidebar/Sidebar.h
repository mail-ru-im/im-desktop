#pragma once

#include "utils/utils.h"

namespace Ui
{
    class SemitransparentWindowAnimated;

    enum class FrameCountMode;

    struct SidebarParams
    {
        bool showFavorites_ = true;
    };

    class SidebarPage : public QWidget
    {
    public:
        SidebarPage(QWidget* parent) : QWidget(parent), isActive_(false) {}
        virtual ~SidebarPage() {}

        virtual void setPrev(const QString& aimId) { prevAimId_ = aimId; }
        virtual void initFor(const QString& aimId, SidebarParams _params = {}) = 0;
        virtual void setFrameCountMode(FrameCountMode _mode) = 0;
        virtual void close() = 0;
        virtual void toggleMembersList() { }
        virtual bool isActive() { return isActive_; }
        virtual QString getSelectedText() const { return QString(); }

    protected:
        QString prevAimId_;
        bool isActive_;
    };

    //////////////////////////////////////////////////////////////////////////

    class Sidebar : public QStackedWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void backButtonClicked() const;

    public:
        explicit Sidebar(QWidget* _parent);
        void preparePage(const QString& aimId, SidebarParams _params = {}, bool _selectionChanged = false);
        void showMembers();

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
        void onAnimationFinished();

    private:
        QMap<int, SidebarPage*> pages_;
        QString currentAimId_;
        SemitransparentWindowAnimated* semiWindow_;
        QPropertyAnimation* animSidebar_;
        int anim_;

        bool needShadow_;
        FrameCountMode frameCountMode_;

    private:
        Q_PROPERTY(int anim READ getAnim WRITE setAnim)
        void setAnim(int _val);
        int getAnim() const;
    };
}
