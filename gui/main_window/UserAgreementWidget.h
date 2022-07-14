#pragma once
#include <QWidget>
#include "namespaces.h"
#include "TermsPrivacyWidgetBase.h"

class QCheckBox;

namespace Ui
{
    class TextBrowserEx;

    class  UserAgreementWidget: public TermsPrivacyWidgetBase
    {
        Q_OBJECT
        public:
            UserAgreementWidget(const QString& _confidentialLink, const QString& _personalDataLink, QWidget* _parent = nullptr);
            virtual ~UserAgreementWidget();

        public Q_SLOTS:
            void onAgreeClicked() override;
            void resetChecks();
            void goBack();

        private:
            void addCheckbox(QCheckBox*& _checkbox, QWidget* _parent, TextBrowserEx* _linkLabel, const QString& _testName);
            void initContent() override;
            QSize getImageSize() const override;
            QString getImagePath() const override;
            void addContentToLayout() override;
            void initButton(const QString& _buttonText, bool _isActive) override;
            TextBrowserEx::Options getLinkOptions() const override;
            void addBackButton();

            QCheckBox* confidentialCheckbox_ = nullptr;
            QCheckBox* personalDataCheckbox_ = nullptr;
            TextBrowserEx* confLabel_ = nullptr;
            TextBrowserEx* pdLabel_ = nullptr;
            QVBoxLayout* checkBoxLayout_ = nullptr;
            QWidget *checkBoxWidget_ = nullptr;
            CustomButton* backButton_ = nullptr;

            const QString confidentialLink_;
            const QString personalDataLink_;

        private Q_SLOTS:
            void check();

        Q_SIGNALS:
            void userAgreementAccepted();

    };
}
