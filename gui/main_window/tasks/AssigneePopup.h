#pragma once

namespace Ui
{
    class ContactListWidget;
    namespace TextRendering
    {
        class TextUnit;
        using TextUnitPtr = std::unique_ptr<TextUnit>;
    }

    class AssigneeEdit;
    class AssigneePopupContent;

    class AssigneePopup : public QWidget
    {
        Q_OBJECT
    public:
        explicit AssigneePopup(AssigneeEdit* _assigneeEdit);
        ~AssigneePopup();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void onWindowResized();
        void onSearchPatternChanged(const QString& _searchPattern);
        void selectContact(const QString& _aimId);
        void onSearchResult();
        void setSelectedContact(const QString& _aimId);

    private:
        AssigneeEdit* assigneeEdit_ = nullptr;
        AssigneePopupContent* content_ = nullptr;
    };
}
