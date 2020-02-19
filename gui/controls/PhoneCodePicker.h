#pragma once

namespace Ui
{
    class LabelEx;

    class PhoneCodePicker : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void countrySelected(const QString&);
    public:
        PhoneCodePicker(QWidget* _parent);
        const QString& getCode() const;
        void setCode(const QString& _code);
        bool isKnownCode(const QString& _code = QString()) const;
    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent *_event) override;
    private:
        void setIcon(const QString& _country);
        void openCountryPicker();
    private:
        QImage icon_;
        QString code_;
        bool isKnownCode_;
    };
}