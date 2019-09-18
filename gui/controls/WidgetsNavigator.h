#pragma once

namespace Ui
{
    class WidgetsNavigator: public QStackedWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void push(QWidget* _widget);
        void pop();
        void poproot();

    private:
        std::stack<QWidget*> history_;
        std::unordered_map<QWidget*, int> widgetToIndex_;
        std::map<int, std::vector<QWidget*>> widgetsAddedOrderByIndexes_;

        QWidget* getFirstWidget() const;

    public:
        explicit WidgetsNavigator(QWidget* _parent = nullptr);
        void insertWidget(int _index, QWidget* _widget);
        void removeWidget(QWidget* _widget);
        virtual ~WidgetsNavigator();
        int addWidget(QWidget* _widget);
    };
}