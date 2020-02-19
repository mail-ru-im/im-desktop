#pragma once

#include "IItemBlock.h"
#include "IItemBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;
class GenericBlock;

class GenericBlockLayout
    : public QLayout
    , public IItemBlockLayout
{
public:

    GenericBlockLayout();

    virtual ~GenericBlockLayout() = 0;

    virtual void addItem(QLayoutItem *item) final override;

    virtual QLayoutItem *itemAt(int index) const final override;

    virtual QLayoutItem *takeAt(int index) final override;

    virtual int count() const final override;

    virtual void invalidate() final override;

    virtual void setGeometry(const QRect &r) final override;

    virtual QSize sizeHint() const final override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual QSize blockSizeHint() const override;

    virtual QRect getBlockGeometry() const override;

    virtual bool onBlockContentsChanged() final override;

    virtual QRect setBlockGeometry(const QRect &ltr) final override;

    virtual void shiftHorizontally(const int _shift) final override;

    void resizeBlock(const QSize& _size) override;

protected:
    template<class T>
    T* blockWidget();

    template<class T>
    T* blockWidget() const;

    virtual QSize setBlockGeometryInternal(const QRect &geometry) = 0;

    virtual QSize stretchToWidthInternal(const int _width) { return QSize(); }

private:
    QRect BlockGeometry_;

    QSize BlockSize_;

};

template<class T>
T* GenericBlockLayout::blockWidget()
{
    return ((const GenericBlockLayout*)this)->blockWidget<T>();
}

template<class T>
T* GenericBlockLayout::blockWidget() const
{
    static_assert(
        std::is_base_of<IItemBlock, T>::value,
        "std::is_base_of<IItemBlock, T>::value");

    auto block = qobject_cast<T*>(parentWidget());
    assert(block);

    return block;
}

UI_COMPLEX_MESSAGE_NS_END
