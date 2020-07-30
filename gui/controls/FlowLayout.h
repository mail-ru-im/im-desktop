#pragma once

namespace Ui
{
    class FlowLayout : public QLayout
    {
    public:

        explicit FlowLayout(QWidget* _parent, int _margin = -1, int _hSpacing = -1, int _vSpacing = -1);
        explicit FlowLayout(int _margin = -1, int _hSpacing = -1, int _vSpacing = -1);
        ~FlowLayout();

        void addItem(QLayoutItem* _item) override;
        int horizontalSpacing() const;
        int verticalSpacing() const;
        Qt::Orientations expandingDirections() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int) const override;
        int count() const override;
        QLayoutItem *itemAt(int _index) const override;
        QSize minimumSize() const override;
        void setGeometry(const QRect& _rect) override;
        QSize sizeHint() const override;
        QLayoutItem *takeAt(int _index) override;

    private:

        enum class Target
        {
            arrange,
            test
        };

        int doLayout(const QRect& _rect, const Target _target) const;
        int smartSpacing(QStyle::PixelMetric _pm) const;

        QList<QLayoutItem *> itemList_;
        int hSpace_;
        int vSpace_;
    };

}