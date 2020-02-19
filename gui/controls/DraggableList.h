#pragma once

namespace Ui
{

    class DraggableItem;

    class DraggableList_p;

    //////////////////////////////////////////////////////////////////////////
    // DraggableList
    //////////////////////////////////////////////////////////////////////////

    class DraggableList : public QWidget
    {
        Q_OBJECT
    public:
        DraggableList(QWidget* _parent = nullptr);
        ~DraggableList();

        void addItem(DraggableItem* _item);
        void removeItem(DraggableItem* _item);
        QWidget* itemAt(int _index);
        int indexOf(QWidget* _item) const;

        int count() const;

        QVariantList orderedData() const;

    private Q_SLOTS:
        void onDragStarted();
        void onDragStopped();
        void onDragPositionChanged();

    private:
        void updateTabOrder();

        std::unique_ptr<DraggableList_p> d;
    };


    class DraggableItem_p;

    //////////////////////////////////////////////////////////////////////////
    // DraggableItem
    //////////////////////////////////////////////////////////////////////////

    class DraggableItem : public QFrame
    {
        Q_OBJECT
    public:
        explicit DraggableItem(QWidget* _parent);
        ~DraggableItem();

        virtual QVariant data() const = 0;

    Q_SIGNALS:
        void dragStarted();
        void dragStopped();
        void dragPositionChanged();

    protected:
        void onDragMove(const QPoint& _pos);
        void onDragStart(const QPoint& _pos);
        void onDragStop();

    private:
        std::unique_ptr<DraggableItem_p> d;
    };

}
