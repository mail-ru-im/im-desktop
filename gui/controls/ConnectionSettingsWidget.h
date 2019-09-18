#pragma once

#include "GeneralCreator.h"
#include "../../corelib/enumerations.h"

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    class LineEditEx;
    class GeneralDialog;

    class ConnectionSettingsWidget : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void enterClicked();

    public:
        ConnectionSettingsWidget(QWidget* _parent);
        void show();
    private:
        LineEditEx*                     passwordEdit_;
        LineEditEx*                     usernameEdit_;
        LineEditEx*                     addressEdit_;
        LineEditEx*                     portEdit_;
        QCheckBox*                      showPasswordCheckbox_;
        GeneralCreator::DropperInfo     typeDropper_;
        GeneralCreator::DropperInfo     authDropper_;
        QWidget*                        mainWidget_;
        GeneralDialog*                  generalDialog_;
        std::vector<core::proxy_type>   activeProxyTypes_;
        std::vector<core::proxy_auth>   proxyAuthTypes_;
        size_t                          selectedProxyIndex_;
        size_t                          selectedAuthTypeIndex_;
        std::vector<QString>            typesNames_;
        std::vector<QString>            authNames_;
        QSpacerItem*                    horizontalSpacer_;
        QWidget*                        authTypeWidget_;

        void saveProxy() const;
        size_t proxyTypeToIndex(core::proxy_type _type) const;
        void fillProxyTypesAndNames();
        void fillProxyAuthTypes();
        void setVisibleAuth(bool _useAuth);
        void setVisibleParams(size_t _ix, bool _useAuth);
        void updateVisibleParams(int _ix);
    };
}
