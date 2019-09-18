#pragma once

#include <QWidget>
#include "controls/TextUnit.h"
#include "types/common_phone.h"

class QVBoxLayout;

namespace Ui
{

class TextBrowserEx;
class LineEditEx;
class PhoneLineEdit;

class AddNewContactWidget: public QWidget
{
    Q_OBJECT

public:
    struct FormData
    {
        QString phoneNumber_;
        QString lastName_;
        QString firstName_;
    };

public:
    explicit AddNewContactWidget(QWidget* _parent = nullptr);

    FormData getFormData() const;
    void clearData();

    void setName(const QString& _name);
    void setPhone(const QString& _phone);

Q_SIGNALS:
    void formDataIncomplete();
    void formDataSufficient();

    void formSubmissionRequested();

private Q_SLOTS:
    void onFormChanged();
    void onFullPhoneChanged(const QString& _fullPhone);
    void onPhoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data);

protected:
    virtual void paintEvent(QPaintEvent* _event) override;
    virtual void keyPressEvent(QKeyEvent *_event) override;
    virtual void showEvent(QShowEvent* _e) override;

private:
    bool isFormComplete() const;
    bool isPhoneComplete() const;
    bool isPhoneCorrect() const;

    QString getPhoneNumber() const;
    void cleanHint();
    void updateHint(const QString& _hint, const QColor& _color = QColor());
    void updatePhoneNumberLine(const QColor& _lineColor);
    void setIsPhoneCorrect(bool _correct);

    void connectToLineEdit(LineEditEx *_lineEdit, LineEditEx *_onUp, LineEditEx *_onDown) const;

private:
    QVBoxLayout* globalLayout_;

    LineEditEx* firstName_;
    LineEditEx* lastName_;
    LineEditEx* rawPhoneInput_;

    PhoneLineEdit* phoneInput_;
    Ui::TextRendering::TextUnitPtr textUnit_;
    Ui::TextRendering::TextUnitPtr hintUnit_;

    std::unordered_set<qint64> coreReqs_;

    bool isPhoneCorrect_ = false;
};

}
