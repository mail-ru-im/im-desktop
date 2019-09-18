#pragma once

#include <QWidget>
#include <QItemDelegate>

class QCompleter;
class QHBoxLayout;

namespace Ui
{

class LineEditEx;


class PhoneLineEdit: public QWidget
{
    Q_OBJECT

public:
    explicit PhoneLineEdit(QWidget *_parent = nullptr);

    QCompleter* getCompleter();
    LineEditEx* getPhoneNumberWidget();
    LineEditEx* getCountryCodeWidget();
    QString getFullPhone() const;
    void resetInputs();

    bool setPhone(const QString& _phone);

Q_SIGNALS:
    void fullPhoneChanged(const QString& _phone);

private Q_SLOTS:
    void onCountryCodeClicked();
    void onCountryCodeChanged(const QString& _code);

    void onPhoneChanged();
    void onPhoneEmptyTextBackspace();

    void onAppDeactivated();

private:
    void focusOnPhoneWidget();
    void setCountryCode(const QString& _code);

private:
    LineEditEx* phoneNumber_;
    LineEditEx* countryCode_;
    QCompleter* completer_;

    QHBoxLayout* globalLayout_;
};

class CountryCodeItemDelegate: public QItemDelegate
{
public:
    CountryCodeItemDelegate(QObject *_parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:

};

}
