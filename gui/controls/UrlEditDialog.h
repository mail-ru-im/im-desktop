#pragma once

#include "GeneralDialog.h"

namespace Ui
{
    class TextEditEx;

    class UrlEditDialog : public Ui::GeneralDialog
    {
    public:
        static std::unique_ptr<UrlEditDialog> create(QWidget* _mainWindow, QWidget* _parent, QStringView _linkDisplayName, InOut QString& _url);

    public:
        UrlEditDialog(QWidget* _mainWidget, QWidget* _parent, Ui::TextEditEx* _urlEdit, QLabel* _hint);
        QString getUrl() const;
        bool isUrlValid() const;

    protected:
        void updateControls(bool _isCorrect);
        void accept() override;

    protected:
        Ui::TextEditEx* url_;
        QLabel* hint_;
    };
}
