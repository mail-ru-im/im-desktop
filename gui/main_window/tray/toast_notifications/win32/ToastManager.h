/*#pragma once

#include <roapi.h>
#include <wrl\client.h>
#include <wrl\implements.h>
#include <windows.ui.notifications.h>

namespace Ui
{
    class ToastNotifier;

    class ToastManager : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void messageClicked(QString);

    public:
        ToastManager();
        ~ToastManager();

        void DisplayToastMessage(const QString& aimdId, const QString& message);
        void HideNotifications(const QString& aimId);

        void Activated(const QString& aimId);
        void Remove(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification, const QString& aimId);

    private:
        HRESULT GetFactory(HSTRING activatableClassId, REFIID iid, void ** factory);

        HRESULT CreateToastXml(
            const QString& aimId, const QString& message,
            ABI::Windows::Data::Xml::Dom::IXmlDocument **xml
            );

        HRESULT CreateToast(
            ABI::Windows::Data::Xml::Dom::IXmlDocument *xml,
            const QString& aimId
            );
        HRESULT SetImageSrc(
            const QString& aimId,
            const QString& displayName,
            ABI::Windows::Data::Xml::Dom::IXmlDocument *toastXml
            );
        HRESULT SetTextValues(
            const QStringList values,
            ABI::Windows::Data::Xml::Dom::IXmlDocument *toastXml
            );
        HRESULT SetNodeValueString(
            HSTRING onputString,
            ABI::Windows::Data::Xml::Dom::IXmlNode *node,
            ABI::Windows::Data::Xml::Dom::IXmlDocument *xml
            );

    private:
        std::map<QString, std::list<Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification>>> Notifications_;
        Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> ToastNotifier_;
        Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics> ToastManager_;
    };
}*/