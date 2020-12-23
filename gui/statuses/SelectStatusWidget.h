#pragma once

#include "Status.h"

namespace Statuses
{

    class SelectStatusWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // SelectStatusWidget
    //////////////////////////////////////////////////////////////////////////

    class SelectStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        SelectStatusWidget(QWidget* _parent = nullptr);
        ~SelectStatusWidget();

    protected:
        void showEvent(QShowEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;

    private:
        std::unique_ptr<SelectStatusWidget_p> d;
    };

    class StatusesList_p;
    class AccessibleStatusesList;

    //////////////////////////////////////////////////////////////////////////
    // StatusesList
    //////////////////////////////////////////////////////////////////////////

    class StatusesList : public QWidget
    {
        friend class StatusesList_p;
        friend class AccessibleStatusesList;

        Q_OBJECT
    public:
        StatusesList(QWidget* _parent);
        ~StatusesList();

        void filter(const QString& _searchPattern);
        void updateUserStatus(const Status& _status);
        int itemsCount() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onLeaveTimer();

    private:
        std::unique_ptr<StatusesList_p> d;
    };

    class AccessibleStatusesList : public QAccessibleWidget
    {
    public:
        AccessibleStatusesList(StatusesList* _list) : QAccessibleWidget(_list), list_(_list)  {}

        int childCount() const override { return list_->itemsCount(); }
        QAccessibleInterface* child(int index) const override;
        int indexOfChild(const QAccessibleInterface* child) const override;
        QString	text(QAccessible::Text type) const override;

    private:
        StatusesList* list_ = nullptr;
    };
}
