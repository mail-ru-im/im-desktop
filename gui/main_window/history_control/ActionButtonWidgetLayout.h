#pragma once

#include "../../namespaces.h"

UI_NS_BEGIN

class ActionButtonWidget;

class ActionButtonWidgetLayout final : public QLayout
{
public:
    ActionButtonWidgetLayout(ActionButtonWidget *parent);

    virtual void setGeometry(const QRect &r) override;

    virtual void addItem(QLayoutItem *item) override;

    const QRect& getIconRect() const;

    const QRect& getProgressRect() const;

    const QRect& getProgressTextRect() const;

    virtual QLayoutItem *itemAt(int index) const override;

    virtual QLayoutItem *takeAt(int index) override;

    virtual int count() const override;

    virtual QSize sizeHint() const override;

    virtual void invalidate() override;

private:
    QRect ContentRect_;

    QRect IconRect_;

    ActionButtonWidget *Item_;

    QRect LastGeometry_;

    QString LastProgressText_;

    QRect ProgressRect_;

    QRect ProgressTextRect_;

    QSize ProgressTextSize_;

    QRect evaluateContentRect(const QSize &iconSize, const QSize &progressTextSize) const;

    QRect evaluateIconRect(const QRect &contentRect, const QSize &iconSize) const;

    QRect evaluateProgressRect(const QRect &iconRect) const;

    QRect evaluateProgressTextRect(const QRect &iconRect, const QSize &progressTextSize) const;

    QSize evaluateProgressTextSize(const QString& progressText) const;

    void setGeometryInternal();

};

UI_NS_END
