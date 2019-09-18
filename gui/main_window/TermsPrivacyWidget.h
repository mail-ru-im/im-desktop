#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "namespaces.h"

#if defined(__APPLE__)
class MacMenuBlocker;
#endif

namespace Ui
{
class PictureWidget;
class TextBrowserEx;

class TermsPrivacyWidget: public QWidget
{
    Q_OBJECT

public:
    struct AcceptParams
    {
        bool accepted_ = false;
    };

    struct Options
    {
        QDialog* containingDialog_ = nullptr;
        bool controlContainingDialog_ = false;
        bool blockUntilAccepted_ = false;
    };

public:
    TermsPrivacyWidget(const QString& _titleHtml,
                       const QString& _descriptionHtml,
                       const Options& _options,
                       QWidget* _parent = nullptr);
    ~TermsPrivacyWidget();
    void setContainingDialog(QDialog* _containingDialog);

Q_SIGNALS:
    void agreementAccepted(const AcceptParams& _acceptParams);

public Q_SLOTS:
    void onAgreeClicked();
    void reportAgreementAccepted(const AcceptParams& _acceptParams);

private Q_SLOTS:
    void onFocusChanged(QWidget *_from, QWidget *_to);

protected:
    void keyPressEvent(QKeyEvent *_event) override;
    void showEvent(QShowEvent *_event) override;

private:
    QPixmap iconPixmap_;

    PictureWidget* icon_ = nullptr;
    TextBrowserEx* title_ = nullptr;
    TextBrowserEx* description_ = nullptr;
    QPushButton* agreeButton_ = nullptr;

    QVBoxLayout* layout_ = nullptr;

    Options options_;

#if defined(__APPLE__)
    std::unique_ptr<MacMenuBlocker> macMenuBlocker_;
#endif
};

}
