#pragma once

#include "controls/DraggableList.h"
#include "types/message.h"

namespace Emoji
{
    class EmojiCode;
}

namespace Ui
{
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

        void setInputData(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions);

        Data::FString getInputText() const;
        int getCursorPos() const;

    private Q_SLOTS:
        void onCancel();
        void onSend();
        void onEmojiSelected(const Emoji::EmojiCode& _code);
        void onEnterPressed();
        void onItemAdded();
        void onItemRemoved();
        void onInputChanged();
        void popupEmojiPicker(bool _on);

    protected:
        void keyPressEvent(QKeyEvent* _event) override;
        void closeEvent(QCloseEvent* _event) override;

    private:
        friend class PollWidgetPrivate;
        std::unique_ptr<class PollWidgetPrivate> d;
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
        void itemAdded();
        void itemRemoved();
        void textChanged();
        void focusIn();

    private Q_SLOTS:
        void onRemoveItem();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        friend class PollItemsListPrivate;
        std::unique_ptr<class PollItemsListPrivate> d;
    };


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
        void focusIn();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;

    private Q_SLOTS:
        void onTextChanged();

    private:
        friend class PollItemPrivate;
        std::unique_ptr<class PollItemPrivate> d;
    };

} // namespace Ui
