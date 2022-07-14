#pragma once

#include "utils/SvgUtils.h"

namespace Ui
{
    class TextEmojiWidget;
    class SidebarCheckboxButton;

    class SettingsSlider: public QSlider
    {
        Q_OBJECT

    private:
        Utils::StyledPixmap handleNormal_;
        Utils::StyledPixmap handleHovered_;
        Utils::StyledPixmap handlePressed_;
        Utils::StyledPixmap handleDisabled_;

        bool hovered_;
        bool pressed_;

    private:
        bool setPosition(QMouseEvent* _event);
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

        struct ProgresserDescriptor
        {
            QWidget* title_ = nullptr;
            QWidget* slider_ = nullptr;

            void setEnabled(bool _isEnabled)
            {
                title_->setEnabled(_isEnabled);
                slider_->setEnabled(_isEnabled);
            }
        };

        static void addHeader(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _text,
            const int _leftMargin = 0
        );

        static QWidget* addHotkeyInfo(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _name,
            const QString& _keys
        );

        static SidebarCheckboxButton* addSwitcher(QWidget *_parent,
                                                  QLayout *_layout,
                                                  const QString &_text,
                                                  bool _switched,
                                                  std::function<void(bool)> _slot = {},
                                                  int _height = -1,
                                                  const QString& _accName = QString()
        );

        static TextEmojiWidget* addChooser(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            const QString& _value,
            std::function< void(TextEmojiWidget*) > _slot,
            const QString& _accName = QString()
        );

        static DropperInfo addDropper(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            bool _isHeader,
            const std::vector< QString >& _values,
            int _selected,
            int _width,
            std::function< void(const QString&, int, TextEmojiWidget*) > _slot
        );

        static ProgresserDescriptor addProgresser(
            QWidget* _parent,
            QLayout* _layout,
            int _markCount,
            int _selected,
            std::function<void(TextEmojiWidget*, TextEmojiWidget*, int)> _slotFinish,
            std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slotProgress
        );

        static void addBackButton(
            QWidget* _parent,
            QLayout* _layout,
            std::function<void()> _on_button_click = {}
        );

        static QComboBox* addComboBox(
            QWidget* _parent,
            QLayout* _layout,
            const QString& _info,
            bool _isHeader,
            const std::vector< QString >& _values,
            int _selected,
            int _width,
            std::function< void(const QString&, int) > _slot
        );
    };
}
