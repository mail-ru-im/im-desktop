#pragma once

namespace Styling
{
    class QmlHelper;
}

namespace Qml
{
    class RootModel;

    class Engine : public QQmlEngine
    {
        Q_OBJECT

    public:
        explicit Engine(QObject* _parent);
        RootModel* rootModel() const;
        void addQuickWidget(QQuickWidget* _widget);

        QVector<QQuickItem*> rootQuickItems() const;

    private:
        void addRootQuickItem(QQuickItem* _item);

        void registerHelpers();
        void registerModels();

        void registerComponents();

    private Q_SLOTS:
        void onQuickWidgetStatusChanged();
        void registerStyleHelper();

    private:
        RootModel* rootModel_;
        QVector<QQuickItem*> rootItems_;
        Styling::QmlHelper* styleHelper_ = nullptr;
    };

} // namespace Qml
