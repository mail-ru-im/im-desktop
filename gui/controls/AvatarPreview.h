#pragma once

namespace Ui
{
    class AvatarPreview : public QWidget
    {
        Q_OBJECT

    protected:
        virtual void paintEvent(QPaintEvent* _e) override;

    public:
        AvatarPreview(QPixmap _img, QWidget* _parent);
        ~AvatarPreview();
        QSize sizeHint() const override;
    private:
        QPixmap img_;
    };
}
