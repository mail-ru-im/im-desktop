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
    class BoxModel : public IBoxModel
    {
    public:
        BoxModel(const bool hasLeadLines, const QMargins &bubbleMargins);

        virtual ~BoxModel();

        virtual QMargins getBubbleMargins() const override;

        virtual bool hasLeadLines() const override;

    private:
        const bool HasLeadLines_;

        const QMargins BubbleMargins_;

    };

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

    const IItemBlockLayout::IBoxModel& getBlockBoxModel() const override;

    virtual QRect getBlockGeometry() const override;

    virtual bool onBlockContentsChanged() final override;

    virtual QRect setBlockGeometry(const QRect &ltr) final override;

    virtual void shiftHorizontally(const int _shift) final override;

protected:
    template<class T>
    T* blockWidget();

    template<class T>
    T* blockWidget() const;

    virtual QSize setBlockGeometryInternal(const QRect &geometry) = 0;

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