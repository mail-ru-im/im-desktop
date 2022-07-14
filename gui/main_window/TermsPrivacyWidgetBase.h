#pragma once

#include <QObject>
#include <QWidget>
#include "namespaces.h"
#include "controls/TextBrowserEx.h"

#if defined(__APPLE__)
class MacMenuBlocker;
#endif

class QVBoxLayout;
class QHBoxLayout;

namespace Ui
{
    class PictureWidget;
    class DialogButton;
    class CustomButton;


    class TermsPrivacyWidgetBase: public QWidget
    {
        Q_OBJECT

        public:
            TermsPrivacyWidgetBase(const QString& _buttonText,
                                   bool _isActive,
                                   QWidget* _parent = nullptr);
            virtual ~TermsPrivacyWidgetBase();
            void init();

        public Q_SLOTS:
            virtual void onAgreeClicked();
            virtual void reportAgreementAccepted(){};

        protected:
            virtual QString getImagePath() const = 0;
            virtual QSize getImageSize() const = 0;

            virtual void showAll();

            virtual void initIcon(const QString& _iconPath, const QSize& _iconSize);
            virtual void initContent(){};
            virtual void initButton(const QString& _buttonText, bool _isActive);
            virtual void addContentToLayout(){};

            void keyPressEvent(QKeyEvent* _event) override;
            void showEvent(QShowEvent* _event) override;
            virtual void reportLink(const QString& _link) const{};
            virtual void addLink(TextBrowserEx*& _linkLabel, const QString& _link, int _minWidth, int _maxWidth,
                                 QWidget* _parent, const QString& _testName);
            virtual TextBrowserEx::Options getLinkOptions() const;

            void setTestWidgetName(const QString& _name){ testWidgetName_ = _name; }
            void addLinkWithOptions(const TextBrowserEx::Options& _options, TextBrowserEx*& _linkLabel, const QString& _link,
                                int _minWidth, int _maxWidth, QWidget* _parent, const QString& _testName, bool _isClickable = true);
            void addVSpacer(int _height = 0);

        protected Q_SLOTS:
            void onAnchorClicked(const QUrl& _link) const;

        private:
            QWidget* addIcon(const QString& _iconPath, const QSize& _iconSize, QWidget* _parent = nullptr);
            void addAgreeButton(const QString& _text, QWidget* _parent = nullptr);
            void addScrollArea(QWidget* _parent = nullptr);
            void addBottomWidget(QWidget* _parent = nullptr);
            void addAgreeButtonToLayout();
            void addIconToLayout();
            void addMainWidget();
            void addAllToLayout();

            const QString buttonText_;
            bool isActive_;

        protected:
            QVBoxLayout* layout_ = nullptr;
            QVBoxLayout* verticalLayout_ = nullptr;
            QVBoxLayout* bottomLayout_ = nullptr;
            QVBoxLayout* iconLayout_ = nullptr;
            QWidget* mainWidget_ = nullptr;
            DialogButton* agreeButton_ = nullptr;
            PictureWidget* icon_ = nullptr;
            QString testWidgetName_;
            QWidget* iconWidget_ = nullptr;
            QWidget*  bottomWidget_ = nullptr;

    #if defined(__APPLE__)
            std::unique_ptr<MacMenuBlocker> macMenuBlocker_;
    #endif
    };

}
