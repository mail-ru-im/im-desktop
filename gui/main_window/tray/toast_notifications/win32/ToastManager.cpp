/*#include "stdafx.h"
#include "ToastManager.h"
#include "ToastEventHandler.h"
#include "StringReferenceWrapper.h"
#include "../../../contact_list/ContactListModel.h"
#include "../../../../cache/avatars/AvatarStorage.h"
#include "../../../../utils/utils.h"

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

typedef HRESULT (__stdcall *RoGetActivationFactoryInstance)(
    _In_  HSTRING activatableClassId,
    _In_  REFIID  iid,
    _Out_ void    **factory);

namespace Ui
{
    ToastManager::ToastManager()
    {
    }

    ToastManager::~ToastManager()
    {

    }

    void ToastManager::HideNotifications(const QString& aimId)
    {
        if (Notifications_.find(aimId) == Notifications_.end())
        {
            return;
        }

        if (!ToastManager_)
        {
            if (GetFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_INS_ARGS(&ToastManager_)) != S_OK)
                return;
        }
        if (!ToastNotifier_)
        {
            if (ToastManager_->CreateToastNotifierWithId(StringReferenceWrapper(L"ICQ").Get(), &ToastNotifier_) != S_OK)
                return;
        }

        for (auto notification : Notifications_[aimId])
        {
            ToastNotifier_->Hide(notification.Get());
        }

        Notifications_[aimId].clear();
    }

    void ToastManager::DisplayToastMessage(const QString& aimId, const QString& message)
    {
        ComPtr<IXmlDocument> toastXml;
        if (SUCCEEDED(CreateToastXml(aimId, message, &toastXml)))
        {
            CreateToast(toastXml.Get(), aimId);
        }
    }

    void ToastManager::Activated(const QString& aimId)
    {
        emit messageClicked(aimId);
    }

    void ToastManager::Remove(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification, const QString& aimId)
    {
        if (Notifications_.find(aimId) == Notifications_.end())
        {
            return;
        }

        auto iter = std::find(Notifications_[aimId].begin(), Notifications_[aimId].end(), notification);
        if (iter != Notifications_[aimId].end())
            Notifications_[aimId].erase(iter);
    }

    HRESULT ToastManager::GetFactory(HSTRING activatableClassId, REFIID iid, void ** factory)
    {
        static RoGetActivationFactoryInstance factoryGet;
        if (!factoryGet)
        {
            HINSTANCE dll = LoadLibraryW(L"Combase.dll");
            if (dll)
            {
                factoryGet = (RoGetActivationFactoryInstance)GetProcAddress(dll, "RoGetActivationFactory");
            }
        }

        if (factoryGet)
            return factoryGet(activatableClassId, iid, factory);

        return S_FALSE;
    }

    HRESULT ToastManager::CreateToastXml(const QString& aimId, const QString& message, IXmlDocument** inputXml)
    {
        if (!ToastManager_)
        {
            if (GetFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_INS_ARGS(&ToastManager_)) != S_OK)
                return S_FALSE;
        }

        HRESULT hr = ToastManager_->GetTemplateContent(ToastTemplateType_ToastImageAndText04, inputXml);
        if (SUCCEEDED(hr))
        {
            QString displayName = Logic::getContactListModel()->getDisplayName(aimId);
            hr = SetImageSrc(aimId, displayName, *inputXml);
            if (SUCCEEDED(hr))
            {
                QStringList text;
                text << displayName;//Title
                text << message;//message
                hr = SetTextValues(text, *inputXml);
            }
        }
        return hr;
    }

    HRESULT ToastManager::SetImageSrc(const QString& aimId, const QString& displayName, IXmlDocument *toastXml)
    {
        QString imagePath = Logic::GetAvatarStorage()->GetLocal(aimId, displayName, Utils::scale_value(64), !Logic::getContactListModel()->isChat(aimId));
        ComPtr<IXmlNodeList> nodeList;
        HRESULT hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
        if (SUCCEEDED(hr))
        {
            ComPtr<IXmlNode> imageNode;
            hr = nodeList->Item(0, &imageNode);
            if (SUCCEEDED(hr))
            {
                ComPtr<IXmlNamedNodeMap> attributes;

                hr = imageNode->get_Attributes(&attributes);
                if (SUCCEEDED(hr))
                {
                    ComPtr<IXmlNode> srcAttribute;

                    hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
                    if (SUCCEEDED(hr))
                    {
                        hr = SetNodeValueString(StringReferenceWrapper(imagePath).Get(), srcAttribute.Get(), toastXml);
                    }
                }
            }
        }
        return hr;
    }

    HRESULT ToastManager::SetTextValues(const QStringList values, IXmlDocument *toastXml)
    {
        ComPtr<IXmlNodeList> nodeList;
        HRESULT hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
        if (SUCCEEDED(hr))
        {
            int i = 0;
            for (auto iter : values)
            {
                ComPtr<IXmlNode> textNode;
                hr = nodeList->Item(i, &textNode);
                if (SUCCEEDED(hr))
                {
                    hr = SetNodeValueString(StringReferenceWrapper(iter).Get(), textNode.Get(), toastXml);
                }
                ++i;
            }
        }
        return hr;
    }

    HRESULT ToastManager::SetNodeValueString(HSTRING inputString, IXmlNode *node, IXmlDocument *xml)
    {
        ComPtr<IXmlText> inputText;

        HRESULT hr = xml->CreateTextNode(inputString, &inputText);
        if (SUCCEEDED(hr))
        {
            ComPtr<IXmlNode> inputTextNode;

            hr = inputText.As(&inputTextNode);
            if (SUCCEEDED(hr))
            {
                ComPtr<IXmlNode> pAppendedChild;
                hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
            }
        }

        return hr;
    }

    HRESULT ToastManager::CreateToast(IXmlDocument *xml, const QString& aimId)
    {
        if (!ToastManager_)
        {
            if (GetFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_INS_ARGS(&ToastManager_)) != S_OK)
                return S_FALSE;
        }
        if (!ToastNotifier_)
        {
            if (ToastManager_->CreateToastNotifierWithId(StringReferenceWrapper(L"ICQ").Get(), &ToastNotifier_) != S_OK)
                return S_FALSE;
        }

        ComPtr<IToastNotificationFactory> factory;
        HRESULT hr = GetFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_INS_ARGS(&factory));
        if (SUCCEEDED(hr))
        {
            ComPtr<IToastNotification> toast;
            hr = factory->CreateToastNotification(xml, &toast);
            if (SUCCEEDED(hr))
            {
                Notifications_[aimId].push_back(toast);
                EventRegistrationToken activatedToken, dismissedToken, failedToken;
                ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(this, toast, aimId));

                hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
                if (SUCCEEDED(hr))
                {
                    hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
                    if (SUCCEEDED(hr))
                    {
                        hr = toast->add_Failed(eventHandler.Get(), &failedToken);
                        if (SUCCEEDED(hr))
                        {
                            hr = ToastNotifier_->Show(toast.Get());
                        }
                    }
                }
            }
        }
        return hr;
    }
}*/