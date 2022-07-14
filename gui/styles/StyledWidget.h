#pragma once

#include "styles/ThemeColor.h"

namespace Styling
{
    class StyledWidget : public QWidget
    {
        Q_OBJECT

    public:
        StyledWidget(QWidget* _parent = nullptr);

        void setDefaultBackground();
        void setBackgroundKey(const ThemeColorKey& _key);

        bool event(QEvent* _event) override;

    private:
        void updateBackground();

    private Q_SLOTS:
        void markNeedUpdateBackground();

    private:
        ThemeColorKey backgroundKey_;
        bool needUpdateBackground_ = false;
    };
} // namespace Styling
