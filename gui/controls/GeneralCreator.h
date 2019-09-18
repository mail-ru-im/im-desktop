#pragma once

namespace Ui
{
    class TextEmojiWidget;
    class SidebarCheckboxButton;

    class SettingsSlider: public QSlider
    {
        Q_OBJECT
    private:

        bool hovered_;
        bool pressed_;

        QPixmap handleNormal_;
        QPixmap handleHovered_;
        QPixmap handlePressed_;

        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void wheelEvent(QWheelEvent* _e) override;

        void paintEvent(QPaintEvent* _event) override;

    public:
        explicit SettingsSlider(Qt::Orientation _orientation, QWidget* _parent = nullptr);
        ~SettingsSlider();
    };

    struct GeneralCreator
    {
        struct DropperInfo
        {
            QMenu* menu = nullptr;
            TextEmojiWidget* currentSelected = nullptr;
        };

        static void addHeader(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _text,
            const int _leftMargin = 0
        );

        static void addHotkeyInfo(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _name,
            const QString& _keys
        );

        static SidebarCheckboxButton* addSwitcher(QWidget *_parent,
                                                  QLayout *_layout,
                                                  const QString &_text,
                                                  bool _switched,
                                                  std::function<QString(bool)> _slot,
                                                  int _height = -1);

        static TextEmojiWidget* addChooser(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            const QString& _value,
            std::function< void(TextEmojiWidget*) > _slot
        );

        static DropperInfo addDropper(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            bool _is_header,
            const std::vector< QString >& _values,
            int _selected,
            int _width,
            std::function< void(QString, int, TextEmojiWidget*) > _slot1,
            std::function< QString(bool) > _slot2
        );

        static void addProgresser(
            QWidget* _parent,
            QLayout* _layout,
            const std::vector< QString >& _values,
            int _selected,
            std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot_finish,
            std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot_progress
        );

        static void addBackButton(
            QWidget* _parent,
            QLayout* _layout,
            std::function<void()> _on_button_click = [](){}
        );

        static QComboBox* addComboBox(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            bool _is_header,
            const std::vector< QString >& _values,
            int _selected,
            int _width,
            std::function< void(QString, int, TextEmojiWidget*) > _slot1,
            std::function< QString(bool) > _slot2
        );
    };
}
