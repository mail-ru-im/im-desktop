#pragma once

#include "CustomAbstractListModel.h"
#include "../../types/message.h"

namespace Logic
{
    enum ServiceItems
    {
        NoItems = 0,
        TopSpace = (1 << 0),
        GroupCall = (1 << 1),
        CallByLink = (1 << 2),
        Webinar = (1 << 3),
        BottomSpace = (1 << 4),
    };


    class CallsModel : public CustomAbstractListModel
    {
        Q_OBJECT

    public:
        explicit CallsModel(QObject* _parent = nullptr);
        ~CallsModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        bool isServiceItem(const QModelIndex& _index) const override;
        void setTopSpaceVisible(bool _visible);

    private Q_SLOTS:
        void callLog(const Data::CallInfoVec& _calls);
        void newCall(const Data::CallInfo& _call);
        void callRemoveMessages(const QString& _aimid, const std::vector<qint64> _messages);
        void callDelUpTo(const QString& _aimid, qint64 _del_up_to);
        void callRemoveContact(const QString& _contact);
        void updateServiceItems();

    private:
        void sortCalls();
        void groupCalls();
        Data::CallInfoVec getCalls() const;

        int getServiceItemsSize() const;
        int getServiceItemIndex(const ServiceItems _item) const;

    private:
        Data::CallInfoVec calls_;
        int serviceItems_;

        friend class CallsSearchModel;
    };

    CallsModel* GetCallsModel();
    void ResetCallsModel();
}

