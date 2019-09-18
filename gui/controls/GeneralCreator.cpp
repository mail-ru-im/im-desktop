#include "stdafx.h"
#include "GeneralCreator.h"

#include "FlatMenu.h"
#include "TextEmojiWidget.h"
#include "SwitcherCheckbox.h"
#include "CustomButton.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "main_window/sidebar/SidebarUtils.h"

class SliderProxyStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0) const
    {
        switch (metric) {
        case PM_SliderLength: return Utils::scale_value(8);
        case PM_SliderThickness: return Utils::scale_value(24);
        default: return (QProxyStyle::pixelMetric(metric, option, widget));
        }
    }
};


class ComboboxProxyStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0) const
    {
        switch (metric)
        {
            case PM_SmallIconSize: return Utils::scale_value(24);
            default: return (QProxyStyle::pixelMetric(metric, option, widget));
        }
    }
};

namespace Ui
{
    void GeneralCreator::addHeader(QWidget* _parent, QLayout* _layout, const QString& _text, const int _leftMargin)
    {
        auto mainWidget = new QWidget(_parent);
        auto layout = Utils::emptyHLayout(mainWidget);
        if (_leftMargin)
            layout->addSpacing(Utils::scale_value(_leftMargin));
        auto title = new TextEmojiWidget(
            _parent,
            Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        title->setText(_text);
        Testing::setAccessibleName(title, qsl("AS ")+_text);
        layout->addWidget(title);
        _layout->addWidget(mainWidget);
        Utils::grabTouchWidget(title);
    }

    void GeneralCreator::addHotkeyInfo(QWidget* _parent, QLayout* _layout, const QString& _name, const QString& _keys)
    {
        auto infoWidget = new QWidget(_parent);
        auto infoLayout = Utils::emptyHLayout(infoWidget);
        infoLayout->setAlignment(Qt::AlignTop);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(Utils::scale_value(12));

        auto textName = new TextEmojiWidget(infoWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        auto textKeys = new TextEmojiWidget(infoWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        textName->setText(_name);
        textName->setMultiline(true);
        textKeys->setText(_keys, TextEmojiAlign::allign_right);

        QSizePolicy sp(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sp.setHorizontalStretch(1);
        textName->setSizePolicy(sp);
        textKeys->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        infoLayout->addWidget(textName);
        infoLayout->addStretch();
        infoLayout->addWidget(textKeys);

        Testing::setAccessibleName(infoWidget, qsl("AS gc hotkeyInfoWidget") + _keys);
        Utils::grabTouchWidget(infoWidget);
        _layout->addWidget(infoWidget);
    }

    SidebarCheckboxButton* GeneralCreator::addSwitcher(QWidget *_parent, QLayout *_layout, const QString &_text,
                                                       bool _switched, std::function<QString(bool)> _slot, int _height)
    {
        const auto switcherRow = addCheckbox(QString(), _text, _parent, _layout);
        switcherRow->setFixedHeight(_height == -1 ? Utils::scale_value(44) : _height);
        switcherRow->setChecked(_switched);
        switcherRow->setTextOffset(0);
        switcherRow->setContentsMargins(0, 0, 0, 0);
        switcherRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QObject::connect(switcherRow, &SidebarCheckboxButton::clicked, [switcherRow, _slot]()
        {
            _slot(switcherRow->isChecked());
        });

        return switcherRow;
    }

    TextEmojiWidget* GeneralCreator::addChooser(QWidget* _parent, QLayout* _layout, const QString& _info, const QString& _value, std::function< void(TextEmojiWidget*) > _slot)
    {
        auto mainWidget = new QWidget(_parent);

        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->addSpacing(Utils::scale_value(20));
        mainLayout->setAlignment(Qt::AlignLeft);
        mainLayout->setSpacing(Utils::scale_value(4));

        Utils::grabTouchWidget(mainWidget);

        auto info = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        Utils::grabTouchWidget(info);
        info->setSizePolicy(QSizePolicy::Policy::Preferred, info->sizePolicy().verticalPolicy());
        info->setText(_info);
        Testing::setAccessibleName(info, qsl("AS gc info ") + _info);
        mainLayout->addWidget(info);

        auto valueWidget = new QWidget(_parent);
        auto valueLayout = Utils::emptyVLayout(valueWidget);
        Utils::grabTouchWidget(valueWidget);
        valueWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        valueLayout->setAlignment(Qt::AlignBottom);
        valueLayout->setContentsMargins(0, 0, 0, 0);

        auto value = new TextEmojiWidget(valueWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        {
            Utils::grabTouchWidget(value);
            value->setCursor(Qt::PointingHandCursor);
            value->setText(_value);
            value->setEllipsis(true);
            value->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER));
            value->setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
            QObject::connect(value, &TextEmojiWidget::clicked, [value, _slot]()
            {
                _slot(value);
            });
            Testing::setAccessibleName(value, qsl("AS gc ") + _value);
            valueLayout->addWidget(value);
        }
        Testing::setAccessibleName(valueWidget, qsl("AS gc valueWidget"));
        mainLayout->addWidget(valueWidget);

        Testing::setAccessibleName(mainWidget, qsl("AS gc mainWidget"));
        _layout->addWidget(mainWidget);
        return value;
    }

    GeneralCreator::DropperInfo GeneralCreator::addDropper(QWidget* _parent, QLayout* _layout, const QString& _info, bool _is_header, const std::vector< QString >& _values, int _selected, int _width, std::function< void(QString, int, TextEmojiWidget*) > _slot1, std::function< QString(bool) > _slot2)
    {
        TextEmojiWidget* title = nullptr;
        TextEmojiWidget* aw1 = nullptr;
        auto titleWidget = new QWidget(_parent);
        auto titleLayout = Utils::emptyHLayout(titleWidget);
        Utils::grabTouchWidget(titleWidget);
        titleWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        titleLayout->setAlignment(Qt::AlignLeft);
        {
            QFont font;
            if (!_is_header)
                font = Fonts::appFontScaled(15);
            else
                font = Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold);

            title = new TextEmojiWidget(titleWidget, font, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            Utils::grabTouchWidget(title);
            title->setSizePolicy(QSizePolicy::Policy::Preferred, title->sizePolicy().verticalPolicy());
            title->setText(_info);
            Testing::setAccessibleName(title, qsl("AS gc title ") + _info);
            titleLayout->addWidget(title);

            aw1 = new TextEmojiWidget(titleWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            Utils::grabTouchWidget(aw1);
            aw1->setSizePolicy(QSizePolicy::Policy::Preferred, aw1->sizePolicy().verticalPolicy());
            aw1->setText(qsl(" "));
            Testing::setAccessibleName(aw1, qsl("AS gc aw1"));
            titleLayout->addWidget(aw1);
        }
        Testing::setAccessibleName(titleWidget, qsl("AS gc titleWidget"));
        _layout->addWidget(titleWidget);

        auto selectedItemWidget = new QWidget(_parent);
        auto selectedItemLayout = Utils::emptyHLayout(selectedItemWidget);
        Utils::grabTouchWidget(selectedItemWidget);
        selectedItemWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        selectedItemWidget->setFixedHeight(Utils::scale_value(32));
        selectedItemLayout->setAlignment(Qt::AlignLeft);

        DropperInfo di;
        di.currentSelected = nullptr;
        di.menu = nullptr;
        {
            auto selectedItemButton = new QPushButton(selectedItemWidget);
            auto dl = Utils::emptyVLayout(selectedItemButton);
            selectedItemButton->setFlat(true);
            selectedItemButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            auto selectedWidth = _width == -1 ? 280 : _width;
            selectedItemButton->setFixedSize(Utils::scale_value(QSize(selectedWidth, 32)));
            selectedItemButton->setCursor(Qt::CursorShape::PointingHandCursor);
            selectedItemButton->setStyleSheet(qsl("border: none; outline: none;"));
            {
                auto selectedItem = new TextEmojiWidget(selectedItemButton, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), Utils::scale_value(24));
                Utils::grabTouchWidget(selectedItem);

                auto selectedItemText = Utils::getItemSafe(_values, _selected, qsl(" "));
                selectedItem->setText(selectedItemText);
                selectedItem->setFading(true);
                Testing::setAccessibleName(selectedItem, qsl("AS gc selectedItem ") + selectedItemText);
                dl->addWidget(selectedItem);
                di.currentSelected = selectedItem;

                auto lp = new QWidget(selectedItemButton);
                auto lpl = Utils::emptyHLayout(lp);
                Utils::grabTouchWidget(lp);
                lpl->setAlignment(Qt::AlignBottom);
                {
                    auto ln = new QFrame(lp);
                    Utils::grabTouchWidget(ln);
                    ln->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    ln->setFrameShape(QFrame::StyledPanel);
                    Utils::ApplyStyle(ln, qsl("border-width: 1dip; border-bottom-style: solid; border-color: %1; ").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY)));//DROPDOWN_STYLE);
                    Testing::setAccessibleName(ln, qsl("AS gc ln"));
                    lpl->addWidget(ln);
                }
                Testing::setAccessibleName(lp, qsl("AS gc lp"));
                dl->addWidget(lp);

                auto menu = new FlatMenu(selectedItemButton);
                menu->setMinimumWidth(Utils::scale_value(selectedWidth));
                menu->setExpandDirection(Qt::AlignBottom);
                menu->setNeedAdjustSize(true);

                Testing::setAccessibleName(menu, qsl("AS menu FlatMenu"));
                for (const auto& v : _values)
                {
                    auto a = menu->addAction(v);
                    a->setProperty(FlatMenu::sourceTextPropName(), v);
                }
                QObject::connect(menu, &QMenu::triggered, _parent, [menu, aw1, selectedItem, _slot1](QAction* a)
                {
                    int ix = -1;
                    const QList<QAction*> allActions = menu->actions();
                    for (QAction* action : allActions)
                    {
                        ++ix;
                        if (a == action)
                        {
                            const auto text = a->property(FlatMenu::sourceTextPropName()).toString();
                            selectedItem->setText(text);
                            _slot1(text, ix, aw1);
                            break;
                        }
                    }
                });
                selectedItemButton->setMenu(menu);
                di.menu = menu;
            }
            Testing::setAccessibleName(selectedItemButton, qsl("AS gc selectedItemButton"));
            selectedItemLayout->addWidget(selectedItemButton);
        }
        Testing::setAccessibleName(selectedItemWidget, qsl("AS gc selectedItemWidget"));
        _layout->addWidget(selectedItemWidget);

        return di;
    }

    void GeneralCreator::addProgresser(
        QWidget* _parent,
        QLayout* _layout,
        const std::vector< QString >& _values,
        int _selected,
        std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot_finish,
        std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot_progress)
    {
        TextEmojiWidget* w = nullptr;
        TextEmojiWidget* aw = nullptr;
        auto mainWidget = new QWidget(_parent);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->addSpacing(Utils::scale_value(20));
        mainWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        mainLayout->setAlignment(Qt::AlignLeft);
        mainLayout->setSpacing(Utils::scale_value(4));
        {
            w = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            Utils::grabTouchWidget(w);
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(qsl(" "));
            Testing::setAccessibleName(w, qsl("AS gc w"));
            mainLayout->addWidget(w);

            aw = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            Utils::grabTouchWidget(aw);
            aw->setSizePolicy(QSizePolicy::Policy::Preferred, aw->sizePolicy().verticalPolicy());
            aw->setText(qsl(" "));
            Testing::setAccessibleName(aw, qsl("AS gc aw"));
            mainLayout->addWidget(aw);

            _slot_finish(w, aw, _selected);
        }
        Testing::setAccessibleName(mainWidget, qsl("AS gc mainWidget"));
        _layout->addWidget(mainWidget);

        auto sliderWidget = new QWidget(_parent);
        Utils::grabTouchWidget(sliderWidget);
        auto sliderLayout = Utils::emptyVLayout(sliderWidget);
        sliderWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sliderWidget->setFixedWidth(Utils::scale_value(300));
        sliderLayout->setAlignment(Qt::AlignBottom);
        sliderLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(12), 0, 0);
        {
            auto slider = new SettingsSlider(Qt::Orientation::Horizontal, _parent);
            Utils::grabTouchWidget(slider);
            slider->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            slider->setFixedWidth(Utils::scale_value(280));
            slider->setFixedHeight(Utils::scale_value(24));
            slider->setMinimum(0);
            slider->setMaximum((int)_values.size() - 1);
            slider->setValue(_selected);
            slider->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

            QObject::connect(slider, &QSlider::sliderReleased, [w, aw, _slot_finish, slider]()
            {

                _slot_finish(w, aw, slider->value());
                w->update();
                aw->update();
            });

            QObject::connect(slider, &QSlider::valueChanged, [w, aw, _slot_progress, slider]()
            {

                _slot_progress(w, aw, slider->value());
                w->update();
                aw->update();
            });

            Testing::setAccessibleName(slider, qsl("AS gc slider"));
            sliderLayout->addWidget(slider);
        }
        Testing::setAccessibleName(sliderWidget, qsl("AS gc sliderWidget"));
        _layout->addWidget(sliderWidget);
    }

    void GeneralCreator::addBackButton(QWidget* _parent, QLayout* _layout, std::function<void()> _onButtonClick)
    {
        auto backArea = new QWidget(_parent);
        backArea->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        auto backLayout = new QVBoxLayout(backArea);
        backLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

        auto backButton = new CustomButton(backArea, qsl(":/controls/back_icon"), QSize(20, 20));
        Styling::Buttons::setButtonDefaultColors(backButton);
        backButton->setFixedSize(Utils::scale_value(QSize(24, 24)));
        Testing::setAccessibleName(backButton, qsl("AS gc backButton"));
        backLayout->addWidget(backButton);

        auto verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
        backLayout->addItem(verticalSpacer);

        Testing::setAccessibleName(backArea, qsl("AS gc backArea"));
        _layout->addWidget(backArea);
        QObject::connect(backButton, &QPushButton::clicked, _onButtonClick);
    }

    QComboBox* GeneralCreator::addComboBox(QWidget * _parent, QLayout * _layout, const QString & _info, bool _is_header, const std::vector<QString>& _values, int _selected, int _width, std::function<void(QString, int, TextEmojiWidget*)> _slot1, std::function<QString(bool)> _slot2)
    {
        TextEmojiWidget* title = nullptr;
        auto titleWidget = new QWidget(_parent);
        auto titleLayout = Utils::emptyHLayout(titleWidget);
        Utils::grabTouchWidget(titleWidget);
        titleWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        titleLayout->setAlignment(Qt::AlignLeft);
        {
            QFont font;
            if (!_is_header)
                font = Fonts::appFontScaled(15);
            else
                font = Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold);

            title = new TextEmojiWidget(titleWidget, font, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            Utils::grabTouchWidget(title);
            title->setSizePolicy(QSizePolicy::Policy::Preferred, title->sizePolicy().verticalPolicy());
            title->setText(_info);
            Testing::setAccessibleName(title, qsl("AS gc title ") + _info);
            titleLayout->addWidget(title);
        }
        Testing::setAccessibleName(titleWidget, qsl("AS gc titleWidget"));
        _layout->addWidget(titleWidget);

        auto cmbBox = new QComboBox(_parent);
        cmbBox->setFixedWidth(Utils::scale_value(280));
        cmbBox->setStyle(new ComboboxProxyStyle);
        Utils::ApplyStyle(cmbBox, Styling::getParameters().getComboBoxQss());
        cmbBox->setFont(Fonts::appFontScaled(16));
        cmbBox->setMaxVisibleItems(6);
        cmbBox->setCursor(Qt::PointingHandCursor);
        Testing::setAccessibleName(cmbBox, qsl("AS gc cmbBoxProblems"));

        QPixmap pixmap(1, Utils::scale_value(50));
        pixmap.fill(Qt::transparent);
        QIcon icon(pixmap);
        cmbBox->setIconSize(QSize(1, Utils::scale_value(50)));

        auto metrics = cmbBox->fontMetrics();
        auto viewWidth = 0;
        for (const auto& v : _values)
        {
            viewWidth = std::max(viewWidth, metrics.width(v));
            cmbBox->addItem(icon, v);
        }

        const auto maxViewWidth = Utils::scale_value(400);
        viewWidth = std::max(viewWidth + Utils::scale_value(32), cmbBox->width());

        cmbBox->view()->setFixedWidth(std::min(maxViewWidth, viewWidth));
        QObject::connect(cmbBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), _parent, [cmbBox, _slot1](int _idx)
        {
            _slot1(cmbBox->itemText(_idx), _idx, nullptr);
        });

        _layout->addWidget(cmbBox);
        return cmbBox;
    }

    SettingsSlider::SettingsSlider(Qt::Orientation _orientation, QWidget* _parent/* = nullptr*/):
        QSlider(_orientation, _parent)
        , hovered_(false)
        , pressed_(false)
    {
        auto icon = qsl(":/controls/selectbar");
        handleNormal_ = Utils::renderSvg(icon, QSize(Utils::scale_value(8), Utils::scale_value(24)), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        handleHovered_ = Utils::renderSvg(icon, QSize(Utils::scale_value(8), Utils::scale_value(24)), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
        handlePressed_ = Utils::renderSvg(icon, QSize(Utils::scale_value(8), Utils::scale_value(24)), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
        setStyle(new SliderProxyStyle);
    }

    SettingsSlider::~SettingsSlider()
    {
        //
    }

    void SettingsSlider::mousePressEvent(QMouseEvent * _event)
    {
        QSlider::mousePressEvent(_event);
#ifndef __APPLE__
        if (_event->button() == Qt::LeftButton)
        {
            setValue(QStyle::sliderValueFromPosition(minimum(), maximum(),
                (orientation() == Qt::Vertical) ? _event->y() : _event->x(),
                (orientation() == Qt::Vertical) ? height() : width()));

            _event->accept();
        }
#endif // __APPLE__
    }

    void SettingsSlider::mouseReleaseEvent(QMouseEvent* _event)
    {
        QSlider::mousePressEvent(_event);
#ifndef __APPLE__
        if (_event->button() == Qt::LeftButton)
        {
            setValue(QStyle::sliderValueFromPosition(minimum(), maximum(),
                (orientation() == Qt::Vertical) ? _event->y() : _event->x(),
                (orientation() == Qt::Vertical) ? height() : width()));

            emit sliderReleased();

            _event->accept();
        }
#endif // __APPLE__
    }

    void SettingsSlider::enterEvent(QEvent * _event)
    {
        hovered_ = true;
        QSlider::enterEvent(_event);
    }

    void SettingsSlider::leaveEvent(QEvent * _event)
    {
        hovered_ = false;
        QSlider::leaveEvent(_event);
    }

    void SettingsSlider::wheelEvent(QWheelEvent* _e)
    {
        // Disable mouse wheel event for sliders
        if (parent())
        {
            parent()->event(_e);
        }
    }

    void SettingsSlider::paintEvent(QPaintEvent * _event)
    {
        QPainter painter(this);
        QStyleOptionSlider opt;
        initStyleOption(&opt);

        QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        handleRect.setSize(QSize(Utils::scale_value(8), Utils::scale_value(24)));
        handleRect.setTop(0);

        QRect grooveOrigRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
        QRect grooveRect = grooveOrigRect;

        grooveRect.setHeight(Utils::scale_value(2));
        grooveRect.moveCenter(grooveOrigRect.center());

        painter.fillRect(grooveRect, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        painter.drawPixmap(handleRect, hovered_ ? handleHovered_ : (pressed_ ? handlePressed_ : handleNormal_));
    }
}
