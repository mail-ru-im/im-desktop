#pragma once
#include "../../../types/message.h"

namespace Ui
{

class MessagesLayout_p;

class MessagesLayout : public QLayout
{
public:
    MessagesLayout(QWidget* _parent = nullptr);
    ~MessagesLayout();

    void setSpacing(int _spacing);
    void clear();

    void insertSpace(int _height, int _pos = -1);
    void insertWidget(QWidget* _w, int _pos = -1);
    void replaceWidget(QWidget* _w, int _pos);

    void setGeometry(const QRect& _rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void addItem(QLayoutItem* _item) override;
    QLayoutItem* itemAt(int _index) const override;
    QLayoutItem* takeAt(int _index) override;
    int count() const override;
    void invalidate() override;

    bool tryToEditLastMessage();

    void clearSelection();
    bool selectByPos(const QPoint& _from, const QPoint& _to, const QPoint& _areaFrom, const QPoint& _areaTo);
    Data::FString getSelectedText() const;

private:
    std::unique_ptr<MessagesLayout_p> d;
};

}
