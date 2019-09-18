#include "stdafx.h"

#include "CheckboxList.h"

#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "fonts.h"

namespace
{
    const auto buttonSize = QSize(20, 20);

    Utils::SvgLayers getLayers(const Ui::CheckBox::Activity _state)
    {
        const auto bgColor = (_state == Ui::CheckBox::Activity::NORMAL || _state == Ui::CheckBox::Activity::HOVER)
            ? Qt::transparent
            : Styling::getParameters().getColor(
            (_state == Ui::CheckBox::Activity::ACTIVE)
                ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_SECONDARY);

        const auto borderColor = (_state == Ui::CheckBox::Activity::NORMAL || _state == Ui::CheckBox::Activity::HOVER)
            ? Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)
            : Qt::transparent;

        const auto tickColor = (_state == Ui::CheckBox::Activity::NORMAL) ? Qt::transparent : Styling::getParameters().getColor((_state == Ui::CheckBox::Activity::HOVER ? Styling::StyleVariable::BASE_SECONDARY : Styling::StyleVariable::TEXT_SOLID_PERMANENT));

        return
        {
            { qsl("border"), borderColor },
            { qsl("bg"), bgColor },
            { qsl("tick"), tickColor },
        };
    }
}

namespace Ui
{
    CheckboxList::CheckboxList(QWidget* _parent, const QFont& _textFont, const QColor& _textColor, const int _horPaddings, const int _itemHeight, const Options &_options)
        : QWidget(_parent)
        , itemHeight_(_itemHeight)
        , hoveredIndex_(-1)
        , horPadding_(_horPaddings)
        , textFont_(_textFont)
        , textColor_(_textColor)
        , options_(_options)
        , scrollAreaHeight_(-1)
    {
        buttonNormal_ = CheckBox::getCheckBoxIcon(CheckBox::Activity::NORMAL);
        buttonHover_ = CheckBox::getCheckBoxIcon(CheckBox::Activity::NORMAL);
        buttonActive_ = CheckBox::getCheckBoxIcon(CheckBox::Activity::ACTIVE);

        setMouseTracking(true);
    }

    void CheckboxList::setItems(std::initializer_list<CheckboxItem> _items)
    {
        clear();

        for (const auto& item : _items)
            addItem(item.first, item.second);
    }

    void CheckboxList::addItem(const QString& _value, const QString& _friendlyText)
    {
        CheckBoxItem_p item;
        item.value_ = _value;
        item.friendlyText_ = _friendlyText;

        item.textUnit_ = TextRendering::MakeTextUnit(_friendlyText);
        item.textUnit_ ->init(textFont_, textColor_);
        item.textUnit_ ->evaluateDesiredSize();
        item.textUnit_ ->setOffsets(horPadding_, itemsCount() * itemHeight_ + itemHeight_ / 2);

        items_.push_back(std::move(item));

        setFixedHeight(itemsCount() * itemHeight_);
        update();
    }

    void CheckboxList::setSeparators(const std::vector<int> &_afterIndexes)
    {
        separatorsIndexes_ = _afterIndexes;
        setFixedHeight(itemsCount() * itemHeight_ + separatorsIndexes_.size() * options_.separatorHeight_);

        for (decltype(items_)::size_type i = 0; i < items_.size(); ++i)
        {
            auto& item = items_[i];

            const auto height = item.textUnit_->getHeight(getTextWidth());

            const bool isMultiLine = item.textUnit_->desiredWidth() > getTextWidth();

            item.textUnit_->setOffsets(horPadding_, i * itemHeight_
                                                    + separatorsBefore(i) * options_.separatorHeight_
                                                    + (!isMultiLine ? itemHeight_ / 2 : (itemHeight_ - height) / 2));
        }
    }

    void CheckboxList::setItemSelected(const QString& _itemValue, const bool _isSelected)
    {
        const auto it = std::find_if(items_.begin(), items_.end(), [&_itemValue](const auto& _item) { return _item.value_ == _itemValue; });
        if (it != items_.end())
        {
            auto& item = *it;
            if (item.selected_ == _isSelected)
                return;

            item.selected_ = _isSelected;
            update();
        }
    }

    void CheckboxList::clear()
    {
        items_.clear();

        setFixedHeight(0);
        update();
    }

    int CheckboxList::itemsCount() const
    {
        return items_.size();
    }

    std::vector<QString> CheckboxList::getSelectedItems() const
    {
        std::vector<QString> res;
        res.reserve(items_.size());

        for (const auto& item : items_)
        {
            if (item.selected_)
                res.push_back(item.value_);
        }

        return res;
    }

    QSize CheckboxList::sizeHint() const
    {
        return
        {
            width(),
            static_cast<int>(items_.size()) * itemHeight_
                    + static_cast<int>(separatorsIndexes_.size()) * options_.separatorHeight_
        };
    }

    void CheckboxList::setScrollAreaHeight(int _scrollAreaHeight)
    {
        scrollAreaHeight_ = _scrollAreaHeight;
    }

    void CheckboxList::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);
        setSeparators(separatorsIndexes_);
    }

    int CheckboxList::getTextWidth() const
    {
        return width() - horPadding_ - Utils::scale_value(buttonSize).width() - buttonHover_.width();
    }

    void CheckboxList::paintEvent(QPaintEvent* _e)
    {
        const auto s = Utils::scale_value(buttonSize);

        QPainter p(this);
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        int i = 0;
        for (auto& item : items_)
        {
            if (i == hoveredIndex_)
                p.fillRect(QRect(0, i * itemHeight_  + separatorsBefore(i) * options_.separatorHeight_, width(), itemHeight_), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));


            const auto& pixmap = item.selected_ ? buttonActive_
                                                : (i == hoveredIndex_ ? buttonHover_
                                                                      : buttonNormal_);

            p.drawPixmap(QPoint(width() - horPadding_ - s.width(),  i * itemHeight_ + itemHeight_ / 2 - s.height() / 2), pixmap);

            const auto textWidth = getTextWidth();
            if (item.textUnit_->desiredWidth() > textWidth)
                item.textUnit_->draw(p, TextRendering::VerPosition::TOP);
            else
                item.textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);

            drawSeparatorIfNeeded(p, i);
            ++i;
        }

        drawOverlayGradientIfNeeded(p);
    }

    void CheckboxList::mouseMoveEvent(QMouseEvent* _e)
    {
        hoveredIndex_ = -1;
        for (int i = 0; i < (int)items_.size(); ++i)
        {
            if ((i * itemHeight_) + (separatorsBefore(i) * options_.separatorHeight_)
                    <= _e->pos().y() && ((i + 1) * itemHeight_ + separatorsBefore(i) * options_.separatorHeight_) > _e->pos().y())
            {
                hoveredIndex_ = i;
                break;
            }
        }

        setCursor(hoveredIndex_ == -1 ? Qt::ArrowCursor : Qt::PointingHandCursor);

        update();
        QWidget::mouseMoveEvent(_e);
    }

    void CheckboxList::mouseReleaseEvent(QMouseEvent* _e)
    {
        for (int i = 0; i < (int)items_.size(); ++i)
        {
            if ((i * itemHeight_) <= _e->pos().y() && (i + 1) * itemHeight_ > _e->pos().y())
            {
                itemClicked(i);
                break;
            }
        }

        update();
        QWidget::mouseReleaseEvent(_e);
    }

    void CheckboxList::leaveEvent(QEvent* _e)
    {
        hoveredIndex_ = -1;
        setCursor(Qt::ArrowCursor);

        update();
        QWidget::leaveEvent(_e);
    }

    void CheckboxList::itemClicked(const int _index)
    {
        if (_index < 0 || _index >= (int)items_.size())
            return;

        items_[_index].selected_ = !items_[_index].selected_;
        update();
    }

    void CheckboxList::drawSeparatorIfNeeded(QPainter &_p, const int _index)
    {
        if (options_.hasSeparators() &&
            std::find(separatorsIndexes_.cbegin(), separatorsIndexes_.cend(), _index) != separatorsIndexes_.cend())
        {
            // add separator
            _p.fillRect(QRect(0, _index * itemHeight_  + separatorsBefore(_index) * options_.separatorHeight_,
                             width(), options_.separatorHeight_), options_.separatorColor_);
        }
    }

    void CheckboxList::drawOverlayGradientIfNeeded(QPainter &_p)
    {
        if (!options_.overlayGradientBottom_)
            return;

        if (options_.overlayWhenHeightDoesNotFit_ && scrollAreaHeight_ == rect().height())
            return;

        auto visibleHeight = scrollAreaHeight_ > 0 ? scrollAreaHeight_
                                                   : rect().height();

        QRect origin = visibleRegion().boundingRect();
        if ((rect().height() - (origin.y() + visibleHeight)) < options_.removeBottomOverlayThreshold_)
            return;

        QRect bottomRect = QRect(QPoint(origin.x(), origin.y() + visibleHeight - options_.overlayHeight_),
                                 QPoint(origin.width(), origin.y() + visibleHeight));

        QLinearGradient overlay(bottomRect.x(), bottomRect.y(), bottomRect.x(), bottomRect.y() + bottomRect.height());

        overlay.setColorAt(0, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        overlay.setColorAt(1, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        _p.fillRect(bottomRect, overlay);
    }

    int CheckboxList::separatorsBefore(const int _index)
    {
        int cnt = 0;
        std::for_each(separatorsIndexes_.cbegin(), separatorsIndexes_.cend(), [&cnt, _index](const int& i){
            if (i < _index)
                ++cnt;
        });

        return cnt;
    }

    //----------------------------------------------------------------------

    ComboBoxList::ComboBoxList(QWidget* _parent, const QFont& _textFont, const QColor& _textColor, const int _horPaddings, const int _itemHeight)
        : CheckboxList(_parent, _textFont, _textColor, _horPaddings, _itemHeight)
    {
        buttonNormal_ = CheckBox::getRadioButtonIcon(CheckBox::Activity::NORMAL);
        buttonHover_ = CheckBox::getRadioButtonIcon(CheckBox::Activity::HOVER);
        buttonActive_ = CheckBox::getRadioButtonIcon(CheckBox::Activity::ACTIVE);
    }

    QString ComboBoxList::getSelectedItem() const
    {
        const auto selected = getSelectedItems();
        if (!selected.empty())
            return selected.front();

        return QString();
    }

    void ComboBoxList::itemClicked(const int _index)
    {
        if (_index < 0 || _index >= (int)items_.size())
            return;

        for (auto& item : items_)
            item.selected_ = false;

        items_[_index].selected_ = true;

        update();
    }


    // Options
    CheckboxList::Options::Options(int _separatorHeight, const QColor &_separatorColor)
        : separatorHeight_(_separatorHeight),
          separatorColor_(_separatorColor),
          overlayGradientBottom_(false),
          overlayWhenHeightDoesNotFit_(false),
          overlayHeight_(0),
          removeBottomOverlayThreshold_(0)
    {

    }

    bool CheckboxList::Options::hasSeparators() const
    {
        return (separatorHeight_ > 0);
    }

    // Reversed checkbox list

    RCheckboxList::RCheckboxList(QWidget *_parent, const QFont &_textFont, const QColor &_textColor, const int _horPaddings, const int _itemHeight, const CheckboxList::Options &_options)
        : CheckboxList(_parent, _textFont, _textColor, _horPaddings, _itemHeight, _options)
    {

    }

    void RCheckboxList::paintEvent(QPaintEvent *_e)
    {
        const auto s = Utils::scale_value(buttonSize);

        QPainter p(this);
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        int i = 0;

        for (auto& item : items_)
        {
            if (i == hoveredIndex_)
                p.fillRect(QRect(0, i * itemHeight_ + separatorsBefore(i) * options_.separatorHeight_, width(), itemHeight_),
                           Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

            const auto& pixmap = item.selected_ ? buttonActive_
                                                : (i == hoveredIndex_ ? buttonHover_
                                                                      : buttonNormal_);

            p.drawPixmap(QPoint(horPadding_, item.textUnit_->verOffset() - s.height() / 2), pixmap);

            {
                Utils::PainterSaver ps(p);

                p.translate(
                    (pixmap.width() / Utils::scale_bitmap_ratio()) + Utils::unscale_value(options_.checkboxToTextOffset_),
                    0.);
                item.textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
            }

            drawSeparatorIfNeeded(p, i);
            ++i;
        }

        drawOverlayGradientIfNeeded(p);
    }

    CheckBox::CheckBox(QWidget* _parent)
        : QCheckBox(_parent)
        , state_(Activity::NORMAL)
    {
        setFixedHeight(Utils::scale_value(40));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button_ = getCheckBoxIcon(state_);
        setFont(Fonts::appFont(Utils::scale_value(15), Fonts::FontWeight::Light));

        QPalette pal(palette());
        pal.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        setPalette(pal);

        setCursor(Qt::PointingHandCursor);

        connect(this, &QCheckBox::stateChanged, this, [this]() {
            if (this->isChecked())
                state_ = Activity::ACTIVE;
            else
                state_ = Activity::NORMAL; }
        );
    };

    void CheckBox::setState(const Activity _state)
    {
        state_ = _state;
        update();
    }

    QPixmap CheckBox::getCheckBoxIcon(const Activity _state)
    {
        return Utils::renderSvgLayered(qsl(":/controls/checkbox_icon"), getLayers(_state));
    }

    QPixmap CheckBox::getRadioButtonIcon(const Activity _state)
    {
        return Utils::renderSvgLayered(qsl(":/controls/radio_button"), getLayers(_state));
    }

    void CheckBox::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto indSize = Utils::scale_value(buttonSize);

        const QRect indicatorRect(QPoint(0, (height() - indSize.height()) / 2), indSize);
        p.drawPixmap(indicatorRect, getCheckBoxIcon(state_));

        const auto textPadding = Utils::scale_value(12);
        const auto textLeft = indicatorRect.right() + 1 + textPadding;
        const QRect textRect(textLeft, 0, width() - textLeft, height());
        p.drawText(textRect, text(), Qt::AlignVCenter | Qt::AlignLeft);
    }

    bool CheckBox::hitButton(const QPoint&) const
    {
        return true;
    }
}
