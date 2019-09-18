#pragma once

#include "TextUnit.h"

namespace Ui
{
    class CheckBox : public QCheckBox
    {
        Q_OBJECT
    public:
        CheckBox(QWidget* _parent);

        enum class Activity
        {
            NORMAL,
            ACTIVE,
            HOVER,
            DISABLED
        };
        void setState(const Activity _state);

        static QPixmap getCheckBoxIcon(const Activity _state);
        static QPixmap getRadioButtonIcon(const Activity _state);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        bool hitButton(const QPoint &pos) const override;

    private:
        Activity state_;
        QPixmap button_;
    };

    /* title | checkbox */
    class CheckboxList : public QWidget
    {
        Q_OBJECT

    public:
        struct Options
        {
            explicit Options(int _separatorHeight = 0,
                             const QColor& _separatorColor = QColor());

            bool hasSeparators() const;

            int checkboxToTextOffset_ = 0; // not scaled
            int separatorHeight_;
            QColor separatorColor_;

            bool overlayGradientBottom_;
            bool overlayWhenHeightDoesNotFit_;
            int overlayHeight_;
            int removeBottomOverlayThreshold_;
        };

    public:
        using CheckboxItem = std::pair<QString, QString>; // value, friendlyText

        CheckboxList(QWidget* _parent, const QFont& _textFont, const QColor& _textColor, const int _horPaddings, const int _itemHeight,
                     const Options& _options = Options());

        void setItems(std::initializer_list<CheckboxItem> _items);
        void addItem(const QString& _value, const QString& _friendlyText);
        void setSeparators(const std::vector<int>& _afterIndexes);

        void setItemSelected(const QString& _itemValue, const bool _isSelected = true);

        void clear();

        int itemsCount() const;

        std::vector<QString> getSelectedItems() const;
        virtual QSize sizeHint() const override;

        void setScrollAreaHeight(int _scrollAreaHeight);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;

        virtual void itemClicked(const int _index);

        void drawSeparatorIfNeeded(QPainter &_p, const int _index);
        void drawOverlayGradientIfNeeded(QPainter &_p);
        int separatorsBefore(const int _index);

    protected:
        struct CheckBoxItem_p
        {
            QString value_;
            QString friendlyText_;
            bool selected_ = false;
            TextRendering::TextUnitPtr textUnit_;
            bool hasSeparator = false;
        };

        std::vector<CheckBoxItem_p> items_;
        std::vector<int> separatorsIndexes_;

        QPixmap buttonNormal_;
        QPixmap buttonHover_;
        QPixmap buttonActive_;

    protected:
        int itemHeight_;
        int hoveredIndex_;
        int horPadding_;

        QFont textFont_;
        QColor textColor_;

        Options options_;
        int scrollAreaHeight_;

    private:
        int getTextWidth() const;
    };

    //----------------------------------------------------------------------

    class ComboBoxList : public CheckboxList
    {
        Q_OBJECT

    public:
        ComboBoxList(QWidget* _parent, const QFont& _textFont, const QColor& _textColor, const int _horPaddings, const int _itemHeight);

        QString getSelectedItem() const;

    protected:
        void itemClicked(const int _index) override;
    };

    //----------------------------------------------------------------------
    /* R as in reversed */
    /* checkbox | title */
    class RCheckboxList : public CheckboxList
    {
        Q_OBJECT

    public:
        RCheckboxList(QWidget* _parent, const QFont& _textFont, const QColor& _textColor, const int _horPaddings, const int _itemHeight,
                      const Options& _options = Options());

    protected:
        void paintEvent(QPaintEvent* _e) override;

    };
}
