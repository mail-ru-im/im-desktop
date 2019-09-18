#include "stdafx.h"
#include "ConnectionSettingsWidget.h"

#include "GeneralCreator.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "../core_dispatcher.h"

#include "../controls/LineEditEx.h"
#include "../controls/CheckboxList.h"
#include "../fonts.h"
#include "../main_window/MainWindow.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

#include "styles/ThemeParameters.h"

namespace
{
    size_t authTypeToIndex(core::proxy_auth _type)
    {
        return static_cast<size_t>(_type);
    }
}

namespace Ui
{
    int text_edit_width = -1; // take from showPasswordCheckbox_
    const int SPACER_HEIGHT = 56;
    const int PORT_EDIT_WIDTH = 100;
    constexpr auto auth_type_top_margin = 10;

    ConnectionSettingsWidget::ConnectionSettingsWidget(QWidget* _parent)
        : QWidget(_parent)
        , mainWidget_(new QWidget(this))
        , selectedProxyIndex_(0)
    {
        mainWidget_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        Utils::ApplyStyle( mainWidget_, Styling::getParameters().getGeneralSettingsQss());

        auto userProxy = Utils::get_proxy_settings();

        addressEdit_ = new LineEditEx(mainWidget_);
        portEdit_ = new LineEditEx(mainWidget_);
        usernameEdit_ = new LineEditEx(mainWidget_);
        passwordEdit_ = new LineEditEx(mainWidget_);
        showPasswordCheckbox_ = new Ui::CheckBox(mainWidget_);
        showPasswordCheckbox_->setChecked(userProxy->needAuth_);

        auto layout = new QVBoxLayout(mainWidget_);
        layout->setContentsMargins(Utils::scale_value(16), 0, Utils::scale_value(16), 0);
        mainWidget_->setLayout(layout);

        {
            showPasswordCheckbox_->setText(QT_TRANSLATE_NOOP("settings_connection","Proxy server requires password"));
            showPasswordCheckbox_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            text_edit_width = Utils::unscale_value(showPasswordCheckbox_->sizeHint().width());
        }
        {
            fillProxyTypesAndNames();
            selectedProxyIndex_ = proxyTypeToIndex(userProxy->type_);

            typeDropper_ = GeneralCreator::addDropper(
                mainWidget_,
                layout,
                QT_TRANSLATE_NOOP("settings_connection", "Type:"),
                false,
                typesNames_,
                selectedProxyIndex_,
                -1,
                [this](QString v, int ix, TextEmojiWidget*)
            {
                selectedProxyIndex_ = ix;
                updateVisibleParams(ix);
                addressEdit_->setFocus();
            },
                [](bool) -> QString { return QString(); });
        }

        {
            auto proxyLayout = new QHBoxLayout();

            addressEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("settings_connection", "Hostname"));
            addressEdit_->setAlignment(Qt::AlignLeft);
            addressEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            addressEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            addressEdit_->setFont(Fonts::appFontScaled(18));
            addressEdit_->setText(userProxy->proxyServer_);
            Utils::ApplyStyle(addressEdit_, Styling::getParameters().getLineEditCommonQss());
            Testing::setAccessibleName(addressEdit_, qsl("AS csw addressEdit_"));
            proxyLayout->addWidget(addressEdit_);

            horizontalSpacer_ = new QSpacerItem(0, Utils::scale_value(SPACER_HEIGHT), QSizePolicy::Preferred, QSizePolicy::Minimum);
            proxyLayout->addItem(horizontalSpacer_);

            portEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("settings_connection", "Port"));
            portEdit_->setAlignment(Qt::AlignLeft);
            portEdit_->setFixedWidth(Utils::scale_value(PORT_EDIT_WIDTH));
            portEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            portEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            portEdit_->setFont(Fonts::appFontScaled(18));
            portEdit_->setText(userProxy->port_ == Utils::ProxySettings::invalidPort ? QString() : QString::number(userProxy->port_));
            Utils::ApplyStyle(portEdit_, Styling::getParameters().getLineEditCommonQss());
            portEdit_->setValidator(new QIntValidator(0, 65535));
            Testing::setAccessibleName(portEdit_, qsl("AS csw portEdit_"));
            proxyLayout->addWidget(portEdit_);

            layout->addLayout(proxyLayout);
        }
        {
            Testing::setAccessibleName(showPasswordCheckbox_, qsl("AS csw showPasswordCheckbox_"));
            layout->addWidget(showPasswordCheckbox_);
        }
        {
            usernameEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("settings_connection", "Username"));
            usernameEdit_->setAlignment(Qt::AlignLeft);
            usernameEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            usernameEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            usernameEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            usernameEdit_->setFont(Fonts::appFontScaled(18));
            usernameEdit_->setText(userProxy->username_);
            Utils::ApplyStyle(usernameEdit_, Styling::getParameters().getLineEditCommonQss());
            Testing::setAccessibleName(usernameEdit_, qsl("AS csw usernameEdit_"));
            layout->addWidget(usernameEdit_);

            passwordEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("settings_connection", "Password"));
            passwordEdit_->setAlignment(Qt::AlignLeft);
            passwordEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            passwordEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            passwordEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            passwordEdit_->setFont(Fonts::appFontScaled(18));
            passwordEdit_->setText(userProxy->password_);
            Utils::ApplyStyle(passwordEdit_, Styling::getParameters().getLineEditCommonQss());
            passwordEdit_->setEchoMode(QLineEdit::Password);
            Testing::setAccessibleName(passwordEdit_, qsl("AS csw passwordEdit_"));
            layout->addWidget(passwordEdit_);
        }

        layout->addSpacing(Utils::scale_value(auth_type_top_margin));

        {
            authTypeWidget_ = new QWidget(mainWidget_);
            auto authTypeLayout = Utils::emptyVLayout(authTypeWidget_);
            fillProxyAuthTypes();
            selectedAuthTypeIndex_ = authTypeToIndex(userProxy->authType_);

            authDropper_ = GeneralCreator::addDropper(
                mainWidget_,
                authTypeLayout,
                QT_TRANSLATE_NOOP("settings_connection", "Authentication type:"),
                false,
                authNames_,
                selectedAuthTypeIndex_,
                -1,
                [this](QString v, int ix, TextEmojiWidget*)
            {
                selectedAuthTypeIndex_ = ix;
                addressEdit_->setFocus();
            },
                [](bool) -> QString { return QString(); });
        }

        layout->addWidget(authTypeWidget_);

        generalDialog_ = new GeneralDialog(mainWidget_, Utils::InterConnector::instance().getMainWindow());
        Utils::ApplyStyle(mainWidget_, Styling::getParameters().getGeneralSettingsQss());

        connect(showPasswordCheckbox_, &QCheckBox::toggled, this, [this](bool _isVisible)
        {
            setVisibleAuth(_isVisible);
            if (_isVisible)
            {
                usernameEdit_->setFocus();
            }
        });

        QObject::connect(addressEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(portEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(usernameEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(passwordEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);

        generalDialog_->addLabel(QT_TRANSLATE_NOOP("settings_connection", "Connection settings"));
        generalDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "DONE"), true);

        setVisibleParams(selectedProxyIndex_, userProxy->needAuth_);
    }

    void ConnectionSettingsWidget::show()
    {
        if (generalDialog_->showInCenter())
            saveProxy();
        delete generalDialog_;
    }

    void ConnectionSettingsWidget::enterClicked()
    {
        if (!addressEdit_->text().isEmpty() && !portEdit_->text().isEmpty())
        {
            generalDialog_->accept();
        }
    }

    void ConnectionSettingsWidget::saveProxy() const
    {
        auto userProxy = Utils::get_proxy_settings();

        userProxy->type_ = activeProxyTypes_[selectedProxyIndex_];
        userProxy->username_ = usernameEdit_->text();
        userProxy->password_ = passwordEdit_->text();
        userProxy->proxyServer_ = addressEdit_->text();
        userProxy->port_ = portEdit_->text().isEmpty() ? Utils::ProxySettings::invalidPort : portEdit_->text().toInt();
        userProxy->needAuth_ = showPasswordCheckbox_->isChecked();
        userProxy->authType_ = userProxy->needAuth_ ? proxyAuthTypes_[selectedAuthTypeIndex_] : core::proxy_auth::basic;
        userProxy->postToCore();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_set, { { "Proxy_Type", Utils::ProxySettings::proxyTypeStr(userProxy->type_).toStdString() } });
    }

    void ConnectionSettingsWidget::setVisibleAuth(bool _useAuth)
    {
        if (_useAuth)
        {
            usernameEdit_->setVisible(true);
            passwordEdit_->setVisible(true);

            if (activeProxyTypes_[selectedProxyIndex_] == core::proxy_type::http_proxy)
                authTypeWidget_->setVisible(true);
        }
        else
        {
            usernameEdit_->setVisible(false);
            passwordEdit_->setVisible(false);
            authTypeWidget_->setVisible(false);
        }
    }

    void ConnectionSettingsWidget::setVisibleParams(size_t _ix, bool _useAuth)
    {
        if (_ix == 0)
        {
            addressEdit_->setVisible(false);
            portEdit_->setVisible(false);
            showPasswordCheckbox_->setVisible(false);
            setVisibleAuth(false);
            horizontalSpacer_->changeSize(0, Utils::scale_value(1), QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
        else
        {
            addressEdit_->setVisible(true);
            portEdit_->setVisible(true);
            showPasswordCheckbox_->setVisible(true);
            setVisibleAuth(_useAuth);
            horizontalSpacer_->changeSize(0, Utils::scale_value(SPACER_HEIGHT), QSizePolicy::Preferred, QSizePolicy::Minimum);

            if (activeProxyTypes_[selectedProxyIndex_] != core::proxy_type::http_proxy)
                authTypeWidget_->setVisible(false);
        }
    }

    void ConnectionSettingsWidget::updateVisibleParams(int _ix)
    {
        setVisibleParams(_ix, showPasswordCheckbox_->isChecked());
    }

    void ConnectionSettingsWidget::fillProxyTypesAndNames()
    {
        activeProxyTypes_.push_back(core::proxy_type::auto_proxy);
        activeProxyTypes_.push_back(core::proxy_type::http_proxy);
        activeProxyTypes_.push_back(core::proxy_type::socks4);
        activeProxyTypes_.push_back(core::proxy_type::socks5);

        for (auto proxyType : activeProxyTypes_)
            typesNames_.push_back(Utils::ProxySettings::proxyTypeStr(proxyType));
    }

    void ConnectionSettingsWidget::fillProxyAuthTypes()
    {
        proxyAuthTypes_.push_back(core::proxy_auth::basic);
        proxyAuthTypes_.push_back(core::proxy_auth::digest);
        proxyAuthTypes_.push_back(core::proxy_auth::negotiate);
        proxyAuthTypes_.push_back(core::proxy_auth::ntlm);

        for (auto proxyAuth : proxyAuthTypes_)
            authNames_.push_back(Utils::ProxySettings::proxyAuthStr(proxyAuth));
    }

    size_t ConnectionSettingsWidget::proxyTypeToIndex(core::proxy_type _type) const
    {
        auto typeIter = std::find(activeProxyTypes_.begin(), activeProxyTypes_.end(), _type);

        size_t index = 0;
        if (typeIter != activeProxyTypes_.end())
            index =  std::distance(activeProxyTypes_.begin(), typeIter);
        return index;
    }
}
