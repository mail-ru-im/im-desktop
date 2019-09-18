#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

namespace Ui
{
    class MainWindow;
}

namespace Logic
{
    class UnknownsModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void orderChanged();

        void updatedSize();
        void updatedMessages();

        void readStateChanged(const QString&);
        void selectContact(const QString&);
        void dlgStatesHandled(const QVector<Data::DlgState>&);
        void dlgStateChanged(const Data::DlgState&);
        void refreshAll();

    public Q_SLOTS:
        void refresh();

    private Q_SLOTS:
        void activeDialogHide(const QString&);
        void contactChanged(const QString&);
        void selectedContactChanged(const QString& _new, const QString& _prev);
        void dlgStates(const QVector<Data::DlgState>&);
        void sortDialogs();

    public:
        explicit UnknownsModel(QObject* _parent = nullptr);
        ~UnknownsModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        int itemsCount() const;

        void add(const QString& _aimId);

        Data::DlgState getDlgState(const QString& _aimId = QString(), bool _fromDialog = false);

        void sendLastRead(const QString& _aimId = QString());
        void markAllRead();
        void hideChat(const QString& _aimId);

        QModelIndex contactIndex(const QString& _aimId) const;
        bool contains(const QString& _aimId) const override;

        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;

        int unreads(size_t _i) const;
        int totalUnreads() const;
        bool hasAttentionDialogs() const;

        QString firstContact() const;
        QString nextUnreadAimId() const;
        QString nextAimId(const QString& _aimId) const;
        QString prevAimId(const QString& _aimId) const;
        void setHeaderVisible(bool _isVisible);

    private:
        int correctIndex(int _i) const;
        void sendLastReadInternal(const unsigned int _index);

        std::vector<Data::DlgState> dialogs_;
        QHash<QString, int> indexes_;
        QTimer* timer_;
        bool isHeaderVisible_;
    };

    UnknownsModel* getUnknownsModel();
    void ResetUnknownsModel();
}

