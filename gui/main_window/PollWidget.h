#pragma once

#include "controls/DraggableList.h"
#include "types/message.h"

namespace Ui
{
    class PollWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // PollWidget
    //////////////////////////////////////////////////////////////////////////

    class PollWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit PollWidget(const QString& _contact, QWidget* _parent = nullptr);
        ~PollWidget();

        void setFocusOnQuestion();

        void setInputData(const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions);

        QString getInputText() const;
        int getCursorPos() const;

    private Q_SLOTS:
        void onAdd();
        void onCancel();
        void onSend();
        void onEnterPressed();
        void onItemRemoved();
        void onInputChanged();

    private:
        std::unique_ptr<PollWidget_p> d;
    };


    class PollItem;
    class PollItemsList_p;

    //////////////////////////////////////////////////////////////////////////
    // PollItemsList
    //////////////////////////////////////////////////////////////////////////

    class PollItemsList : public QWidget
    {
        Q_OBJECT
    public:
        explicit PollItemsList(QWidget* _parent = nullptr);
        ~PollItemsList();

        void addItem();
        PollItem* itemAt(int _index);

        int count() const;
        int itemsWithTextCount() const;

        QStringList answers() const;

    Q_SIGNALS:
        void enterPressed();
        void itemRemoved();
        void textChanged();


    private Q_SLOTS:
        void onRemoveItem();

    private:
        std::unique_ptr<PollItemsList_p> d;
    };

    class DraggableItem;

    class PollItem_p;

    //////////////////////////////////////////////////////////////////////////
    // PollItem
    //////////////////////////////////////////////////////////////////////////

    class PollItem : public DraggableItem
    {
        Q_OBJECT
    public:
        explicit PollItem(QWidget* _parent = nullptr);
        ~PollItem();

        QVariant data() const override;

        void setFocusOnInput();
        bool hasFocusOnInput() const;

    Q_SIGNALS:
        void remove();
        void enterPressed();
        void textChanged();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;

    private Q_SLOTS:
        void onTextChanged();

    private:
        std::unique_ptr<PollItem_p> d;
    };

}
