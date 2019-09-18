#pragma once


class QCompleter;

namespace Ui
{
    class LineEditEx;
    class CustomButton;
    class PictureWidget;

    class SearchComboboxView : public QTreeView
    {
        Q_OBJECT

    public:
        SearchComboboxView(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _event);
    };

    class CountrySearchCombobox : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void selected(const QString&);

    public Q_SLOTS:
        void editClicked();
        void completerActivated(const QString&);
        void editCompleted();
        void editTextChanged(const QString&);
        void setFocusIn();
        void setFocusOut();

    public:
        CountrySearchCombobox(QWidget* _parent);

        void setComboboxViewClass(const char* _className);
        void setClass(const char* _className);

        void setSources(const QMap<QString, QString>&  _sources);
        void setPlaceholder(const QString& _placeholder);
        bool selectItem(QString _item);
        bool containsCode(const QString& _code) const;

    protected:
        void resizeEvent(QResizeEvent *_e) override;

    private:
        void initLayout();
        QString getValue(const QString& _key) const;

    private:
        LineEditEx* Edit_;
        QCompleter* Completer_;
        SearchComboboxView* ComboboxView_;
        PictureWidget* searchGlass_;
        CustomButton* dropDown_;

        QString OldEditValue_;
        QMap<QString, QString> Sources_;
    };
}