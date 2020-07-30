#pragma once

#include "../core_dispatcher.h"

namespace Utils
{
    enum class CallLinkFrom;
}

namespace Ui
{
    class GeneralDialog;
    class TextLabel;

    class CallLinkWidget : public QDialog
    {
        Q_OBJECT
    public:
        CallLinkWidget(QWidget *_parent, Utils::CallLinkFrom _from, const QString& _url, ConferenceType _type = ConferenceType::Call, const QString& _aimId = QString());
        ~CallLinkWidget();
        virtual bool show();
    private:
        void copyLink() const;
    private:
        std::unique_ptr<GeneralDialog> mainDialog_;
        TextLabel* linkLabel_;
        QString url_;
    };
}