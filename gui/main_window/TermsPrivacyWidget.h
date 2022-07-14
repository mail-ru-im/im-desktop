#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "namespaces.h"
#include "TermsPrivacyWidgetBase.h"

#if defined(__APPLE__)
class MacMenuBlocker;
#endif

namespace Ui
{
    class PictureWidget;
    class TextBrowserEx;
    class DialogButton;

    const QString& legalTermsUrl();
    const QString& privacyPolicyUrl();

    class TermsPrivacyWidget : public TermsPrivacyWidgetBase
    {
        Q_OBJECT

        public:
            struct Options
            {
                bool isGdprUpdate_ = false;
            };

        public:
            TermsPrivacyWidget(const QString& _titleHtml,
                                const QString& _descriptionHtml,
                                const Options& _options,
                                QWidget* _parent = nullptr);
            virtual ~TermsPrivacyWidget();

        Q_SIGNALS:
            void agreementAccepted();

        public Q_SLOTS:
            void onAgreeClicked() override;
            void reportAgreementAccepted() override;

        protected:
            virtual void reportLink(const QString& _link) const override;

        private:
            QSize getImageSize() const override;
            QString getImagePath() const override;
            void addContentToLayout() override;
            void initContent() override;

            void addDescriptionWidget(const QString& _descriptionHtml);
            void showAll() override;

            Options options_;
            const QString titleHtml_;
            const QString descriptionHtml_;
            QWidget* descriptionWidget_ = nullptr;
            TextBrowserEx* title_ = nullptr;
            TextBrowserEx* description_ = nullptr;
            QHBoxLayout* descLayout_ = nullptr;

    #if defined(__APPLE__)
            std::unique_ptr<MacMenuBlocker> macMenuBlocker_;
    #endif
    };

}
