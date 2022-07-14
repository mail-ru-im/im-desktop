#include "stdafx.h"
#include "SmilesMenu.h"

#include "toolbar.h"
#include "StickerPreview.h"
#include "../input_widget/InputWidgetUtils.h"
#include "../history_control/HistoryControlPage.h"
#include "../history_control/complex_message/FileSharingUtils.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../cache/emoji/Emoji.h"
#include "../../cache/emoji/EmojiDb.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/LottiePlayer.h"

#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/ResizePixmapTask.h"
#include "utils/features.h"

#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"

#include <boost/range/adaptor/reversed.hpp>

namespace
{
    constexpr auto stickerSize() noexcept { return core::sticker_size::small; }
    constexpr size_t max_stickers_count = 20;
    constexpr size_t maxRecentEmoji() noexcept { return 20; }

    int emojiToolbarHeight()
    {
        return Utils::scale_value(36);
    }

    int emojiToolbarTopMargin()
    {
        return Utils::scale_value(4);
    }

    qint32 getEmojiItemSize() noexcept
    {
        return Utils::scale_value(44);
    }

    qint32 getStickerItemSize_() noexcept
    {
        return Utils::scale_value(68);
    }

    qint32 getStickerSize_() noexcept
    {
        return Utils::scale_value(60);
    }

    qint32 getPackHeaderHeight()
    {
        return Utils::scale_value(24);
    }

    auto recentsSettingsDelimiter()
    {
        return ql1c(';');
    }

    auto recentsSettingsSourceDelimiter()
    {
        return ql1c(',');
    }

    QSize gifLabelSize()
    {
        return Utils::scale_value(QSize(18, 12));
    }

    int getDefaultPickerHeight()
    {
        return Utils::scale_value(320);
    }

    int getMinimalMessagesAreaHeight()
    {
        return Utils::scale_value(96);
    }

    int getToolBarButtonSize() noexcept
    {
        return Utils::scale_value(40);
    }

    int getToolBarButtonEmojiWidth() noexcept
    {
        return Utils::scale_value(36);
    }

    int getToolBarButtonEmojiHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    constexpr std::chrono::milliseconds getLongtapTimeout() noexcept
    {
        return std::chrono::milliseconds(500);
    }

    QColor getHoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    void drawGifLabel(QPainter& _p, const QPoint& _topLeft)
    {
        static auto icon = Utils::StyledPixmap(qsl(":/smiles_menu/gif_label_small"), gifLabelSize());
        _p.drawPixmap(_topLeft, icon.actualPixmap());
    }

    QPixmap addGifLabel(QPixmap _icon, int _setId)
    {
        if (auto set = Ui::Stickers::getSet(_setId); set && set->containsGifSticker())
        {
            QPainter p(&_icon);
            const auto labelSize = gifLabelSize();
            const auto iconSize = Utils::unscale_bitmap(_icon.size());
            drawGifLabel(p, { iconSize.width() - labelSize.width(), iconSize.height() - labelSize.height() });
        }
        return _icon;
    }

    void loadSetIcon(Ui::Smiles::TabButton* _button)
    {
        if (!_button)
            return;

        const auto& icon = Ui::Stickers::getSetIcon(_button->getSetId());
        if (icon.isLottie())
        {
            _button->setLottie(icon.getLottiePath());
        }
        else if (icon.isPixmap())
        {
            const auto& pm = icon.getPixmap();
            if (pm.isNull())
                return;

            auto task = new Utils::ResizePixmapTask(pm, Utils::scale_bitmap(Ui::Smiles::TabButton::iconSize()));
            QObject::connect(task, &Utils::ResizePixmapTask::resizedSignal, _button, [_button](QPixmap _result)
            {
                Utils::check_pixel_ratio(_result);
                _button->setPixmap(addGifLabel(std::move(_result), _button->getSetId()));
            });
            QThreadPool::globalInstance()->start(task);
        }
    }

    QColor toolbarButtonHoveredBackground()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    QColor toolbarButtonCheckedBackground()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }

    QColor toolbarButtonCheckedBorder()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    QSize categoryButtonSize()
    {
        return Utils::scale_value(QSize(36, 36));
    }

    QSize categoryIconSize()
    {
        return Utils::scale_value(QSize(32, 32));
    }

    QPixmap categoryButtonIconColored(const QString& _category, const QColor& _color)
    {
        const QString resourceString = u":/smiles_menu/cat_" % _category;
        const auto icon = Utils::renderSvg(resourceString, categoryIconSize(), _color);
        auto pixmap = QPixmap(Utils::scale_bitmap(categoryButtonSize()));
        Utils::check_pixel_ratio(pixmap);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        const auto offset = Utils::unscale_bitmap((pixmap.size() - icon.size()) / 2);
        p.drawPixmap(offset.width(), offset.height(), icon);
        return pixmap;
    }

    QPixmap categoryButtonIcon(const QString& _category)
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        color.setAlphaF(0.6);
        return categoryButtonIconColored(_category, color);
    }

    QPixmap categoryButtonHoveredIcon(const QString& _category)
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        color.setAlphaF(.8);
        return categoryButtonIconColored(_category, color);
    }

    QPixmap categoryButtonCheckedIcon(const QString& _category)
    {
        auto hoveredColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        auto pixmap = categoryButtonIconColored(_category, hoveredColor);
        QPainter p(&pixmap);
        QPainterPath path;
        path.addEllipse(QRect({0, 0}, categoryButtonSize()));
        hoveredColor.setAlphaF(.09);
        p.fillPath(path, hoveredColor);
        return pixmap;
    }

    int pickerTopBorderWidth()
    {
        return Utils::scale_value(1);
    }

    int categoryOffset()
    {
        return Utils::scale_value(4);
    }
}

namespace Ui
{
    using namespace Smiles;

   //////////////////////////////////////////////////////////////////////////
    // class ViewItemModel
    //////////////////////////////////////////////////////////////////////////
    EmojiViewItemModel::EmojiViewItemModel(QWidget* _parent, bool _singleLine)
        : QStandardItemModel(_parent)
        , emojisCount_(0)
        , needHeight_(0)
        , singleLine_(_singleLine)
    {
        emojiCategories_.reserve(10);
    }

    QModelIndex Smiles::EmojiViewItemModel::index(int _row, int _column, const QModelIndex& _parent) const
    {
        if (const auto idx = getLinearIndex(_row, _column); idx < getEmojisCount())
            return QStandardItemModel::index(_row, _column, _parent);

        return QModelIndex();
    }

    const Emoji::EmojiRecord& EmojiViewItemModel::getEmoji(int _col, int _row) const
    {
        if (const auto idx = getLinearIndex(_row, _col); idx < getEmojisCount())
        {
            int count = 0;
            for (const auto& category : emojiCategories_)
            {
                if ((count + (int) category.emojis_.size()) > idx)
                {
                    const int indexInCategory = idx - count;

                    im_assert(indexInCategory >= 0);
                    im_assert(indexInCategory < (int) category.emojis_.size());

                    return category.emojis_[indexInCategory];
                }

                count += category.emojis_.size();
            }
        }

        im_assert(!"invalid emoji number");

        return Emoji::EmojiRecord::invalid();
    }

    int EmojiViewItemModel::getLinearIndex(const int _row, const int _col) const
    {
        return _row * columnCount() + _col;
    }

    QVariant EmojiViewItemModel::data(const QModelIndex& _idx, int _role) const
    {
        if (const auto idx = getLinearIndex(_idx.row(), _idx.column()); idx < getEmojisCount())
        {
            const auto& emoji = getEmoji(_idx.column(), _idx.row());
            if (emoji.isValid())
            {
                if (_role == Qt::DecorationRole)
                {
                    QImage emojiPixmap = Emoji::GetEmoji(emoji.fullCodePoints, int(Emoji::getEmojiSize()));
                    Utils::check_pixel_ratio(emojiPixmap);

                    im_assert(!emojiPixmap.isNull());
                    return emojiPixmap;
                }
                else if (Testing::isAccessibleRole(_role))
                {
                    return qsl("AS EmojiViewItemModel emoji_%1").arg(Emoji::EmojiCode::toQString(emoji.fullCodePoints));
                }
            }
        }

        return QVariant();
    }

    int EmojiViewItemModel::addCategory(QLatin1String _category)
    {
        const Emoji::EmojiRecordVec& emojisVector = Emoji::GetEmojiInfoByCategory(_category);
        emojiCategories_.emplace_back(_category, emojisVector);

        emojisCount_ += emojisVector.size();

        resize(prevSize_);

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::addCategory(const emoji_category& _category)
    {
        emojiCategories_.push_back(_category);

        emojisCount_ += _category.emojis_.size();

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::getEmojisCount() const
    {
        return emojisCount_;
    }

    int EmojiViewItemModel::getNeedHeight() const
    {
        return needHeight_;
    }

    int EmojiViewItemModel::getCategoryPos(int _index)
    {
        const int columnWidth = getEmojiItemSize();
        int columnCount = prevSize_.width() / columnWidth;

        int emojiCountBefore = 0;

        for (int i = 0; i < _index; i++)
            emojiCountBefore += emojiCategories_[i].emojis_.size();

        if (columnCount == 0)
            return 0;

        int rowCount = (emojiCountBefore / columnCount) + (((emojiCountBefore % columnCount) > 0) ? 1 : 0);

        return (((rowCount == 0) ? 0 : (rowCount - 1)) * getEmojiItemSize());
    }

    const std::vector<emoji_category>& EmojiViewItemModel::getCategories() const
    {
        return emojiCategories_;
    }

    bool EmojiViewItemModel::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = getEmojiItemSize();
        int emojiCount = getEmojisCount();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            int columnCount = _size.width()/ columnWidth;
            if (columnCount > emojiCount)
                columnCount = emojiCount;

            int rowCount  = 0;
            if (columnCount > 0)
            {
                rowCount = (emojiCount / columnCount) + (((emojiCount % columnCount) > 0) ? 1 : 0);
                if (singleLine_ && rowCount > 1)
                    rowCount = 1;
            }

            setColumnCount(columnCount);
            setRowCount(rowCount);

            needHeight_ = getEmojiItemSize() * rowCount;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    void EmojiViewItemModel::onEmojiAdded()
    {
        emojisCount_ = 0;

        for (const auto& cat : emojiCategories_)
            emojisCount_ += cat.emojis_.size();
    }

    //////////////////////////////////////////////////////////////////////////
    // TableView class
    //////////////////////////////////////////////////////////////////////////
    EmojiTableView::EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model)
        : QTableView(_parent)
        , model_(_model)
        , itemDelegate_(new EmojiTableItemDelegate(this))
    {
        setModel(model_);
        setItemDelegate(itemDelegate_);
        setShowGrid(false);
        verticalHeader()->hide();
        horizontalHeader()->hide();
        setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalHeader()->setMinimumSectionSize(getEmojiItemSize());
        verticalHeader()->setDefaultSectionSize(getEmojiItemSize());
        horizontalHeader()->setMinimumSectionSize(getEmojiItemSize());
        horizontalHeader()->setDefaultSectionSize(getEmojiItemSize());

        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFocusPolicy(Qt::NoFocus);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setCursor(Qt::PointingHandCursor);

        setMouseTracking(true);
        viewport()->setAttribute(Qt::WA_Hover);
    }

    void EmojiTableView::resizeEvent(QResizeEvent * _e)
    {
        if (model_->resize(_e->size()))
            setFixedHeight(model_->getNeedHeight());

        QTableView::resizeEvent(_e);
    }

    void EmojiTableView::leaveEvent(QEvent* _event)
    {
        clearSelection();
    }

    void EmojiTableView::selectIndex(const QModelIndex & _index)
    {
        itemDelegate_->setHoverEnabled(false);
        setCurrentIndex(_index);
    }

    void EmojiTableView::mouseMoveEvent(QMouseEvent* _e)
    {
        itemDelegate_->setHoverEnabled(true);

        Q_EMIT mouseMoved(QPrivateSignal());

        clearSelection();

        if (const auto idx = indexAt(_e->pos()); idx.isValid())
        {
            selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }

        QTableView::mouseMoveEvent(_e);
    }

    int EmojiTableView::addCategory(QLatin1String _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::addCategory(const emoji_category& _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::getCategoryPos(int _index)
    {
        return model_->getCategoryPos(_index);
    }

    const std::vector<emoji_category>& EmojiTableView::getCategories() const
    {
        return model_->getCategories();
    }

    const Emoji::EmojiRecord& EmojiTableView::getEmoji(int _col, int _row) const
    {
        return model_->getEmoji(_col, _row);
    }

    void EmojiTableView::onEmojiAdded()
    {
        model_->onEmojiAdded();

        if (model_->resize(size(), true))
            setFixedHeight(model_->getNeedHeight());
    }

    bool EmojiTableView::selectUp()
    {
        if (const auto cur = currentIndex(); cur.isValid())
        {
            if (const auto newIdx = model()->index(cur.row() - 1, cur.column()); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
        }

        return false;
    }

    bool EmojiTableView::selectDown()
    {
        if (const auto cur = currentIndex(); cur.isValid())
        {
            if (const auto newIdx = model()->index(cur.row() + 1, cur.column()); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
        }

        return false;
    }

    bool EmojiTableView::selectLeft()
    {
        if (const auto cur = currentIndex(); cur.isValid())
        {
            if (auto newIdx = model()->index(cur.row(), cur.column() - 1); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
            else if (newIdx = model()->index(cur.row() - 1, model()->columnCount() - 1); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
        }

        return false;
    }

    bool EmojiTableView::selectRight()
    {
        if (const auto cur = currentIndex(); cur.isValid())
        {
            if (auto newIdx = model()->index(cur.row(), cur.column() + 1); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
            else if (newIdx = model()->index(cur.row() + 1, 0); newIdx.isValid())
            {
                selectIndex(newIdx);
                return true;
            }
        }

        return false;
    }

    void EmojiTableView::selectFirst()
    {
        selectFirstRowAtColumn(0);
    }

    void EmojiTableView::selectLast()
    {
        selectLastRowAtColumn(model()->columnCount());
    }

    bool EmojiTableView::hasSelection() const
    {
        return currentIndex().isValid() && !itemDelegate_->isHoverEnabled();
    }

    int EmojiTableView::selectedItemColumn() const
    {
        if (auto cur = currentIndex(); cur.isValid())
            return cur.column();
        return 0;
    }

    void EmojiTableView::selectFirstRowAtColumn(const int _column)
    {
        selectIndex(model()->index(0, _column));
    }

    void EmojiTableView::selectLastRowAtColumn(const int _column)
    {
        const auto row = model()->rowCount() - 1;
        auto col = std::clamp(_column, 0, model()->columnCount());

        auto idx = model()->index(row, col);
        while (!idx.isValid() && col >= 0)
            idx = model()->index(row, col--);

        if (idx.isValid())
            selectIndex(idx);
    }

    void EmojiTableView::clearSelection()
    {
        QTableView::clearSelection();
        setCurrentIndex(QModelIndex());
    }

    bool EmojiTableView::isKeyboardActive()
    {
        return !itemDelegate_->isHoverEnabled();
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiTableItemDelegate
    //////////////////////////////////////////////////////////////////////////
    EmojiTableItemDelegate::EmojiTableItemDelegate(QObject* parent)
        : QItemDelegate(parent)
        , Prop_(0)
        , hoverEnabled_(true)
    {
        Animation_ = new QPropertyAnimation(this, QByteArrayLiteral("prop"), this);
    }

    void EmojiTableItemDelegate::animate(const QModelIndex& index, int start, int end, int duration)
    {
        AnimateIndex_ = index;
        Animation_->setStartValue(start);
        Animation_->setEndValue(end);
        Animation_->setDuration(duration);
        Animation_->start();
    }

    void EmojiTableItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        const auto data = _index.data(Qt::DecorationRole).value<QImage>();
        if (data.isNull())
            return;

        const int col = _index.column();
        const int row = _index.row();
        const int emoji_size = (int)Emoji::getEmojiSize() / Utils::scale_bitmap_ratio();
        const auto itemSize = getEmojiItemSize();
        const int margin = (itemSize - emoji_size) / 2;

        const QRect itemRect(col * itemSize, row * itemSize, itemSize, itemSize);
        const auto imageRect = itemRect.marginsRemoved({ margin, margin, margin, margin });

        const auto isHovered = (_option.state & QStyle::State_MouseOver) && hoverEnabled_;
        const auto isSelected = _option.state & QStyle::State_Selected;

        if (isHovered || isSelected)
        {
            Utils::PainterSaver ps(*_painter);

            if (isHovered)
                _painter->setBrush(getHoverColor());
            else
                _painter->setBrush(focusColorPrimary());

            _painter->setPen(Qt::NoPen);

            _painter->setRenderHint(QPainter::Antialiasing);

            const qreal radius = qreal(Utils::scale_value(8));
            _painter->drawRoundedRect(itemRect, radius, radius);
        }

        _painter->drawImage(imageRect, data);
    }

    QSize EmojiTableItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
    {
        const int size = (int)Emoji::getEmojiSize() / Utils::scale_bitmap_ratio();
        return QSize(size, size);
    }

    void EmojiTableItemDelegate::setProp(int val)
    {
        Prop_ = val;
        if (AnimateIndex_.isValid())
            Q_EMIT ((QAbstractItemModel*)AnimateIndex_.model())->dataChanged(AnimateIndex_, AnimateIndex_);
    }

    void EmojiTableItemDelegate::setHoverEnabled(const bool _enabled)
    {
        hoverEnabled_ = _enabled;
    }

    bool EmojiTableItemDelegate::isHoverEnabled() const noexcept
    {
        return hoverEnabled_;
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiViewWidget
    //////////////////////////////////////////////////////////////////////////
    EmojisWidget::EmojisWidget(QWidget* _parent)
        : QWidget(_parent)
        , toolbarAtBottom_(true)
    {
        auto vLayout = Utils::emptyVLayout(this);
        setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "EMOJI"));
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
        setHeader->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });

        Testing::setAccessibleName(setHeader, qsl("AS EmojiAndStickerPicker emojiHeader"));
        vLayout->addWidget(setHeader);

        view_ = new EmojiTableView(this, new EmojiViewItemModel(this));
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Utils::grabTouchWidget(view_);

        toolbar_ = new Toolbar(this, Toolbar::buttonsAlign::center);
        toolbar_->setFixedHeight(emojiToolbarHeight() + emojiToolbarTopMargin());
        toolbar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        toolbar_->setObjectName(qsl("smiles_cat_selector"));
        toolbar_->setContentsMargins(0, emojiToolbarTopMargin(), 0, 0);

        TabButton* firstButton = nullptr;

        const auto& emojiCategories = Emoji::GetEmojiCategories();

        buttons_.reserve(emojiCategories.size());

        for (const auto& category : emojiCategories)
        {
            if (category == u"regional" || category == u"modifier") // skip these categories
                continue;

            TabButton* button = toolbar_->addButton(categoryButtonIcon(category));
            Testing::setAccessibleName(button, qsl("AS EmojiAndStickerPicker category") % category);
            button->setFixedSize(categoryButtonSize());
            button->setProperty("underline", false);
            button->setHoveredPixmap(categoryButtonHoveredIcon(category));
            button->setCheckedPixmap(categoryButtonCheckedIcon(category));
            buttons_.push_back(button);

            const int categoryIndex = view_->addCategory(category);

            if (!firstButton)
                firstButton = button;

            connect(button, &TabButton::clicked, this, [this, categoryIndex]()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_cathegory_click);
                Q_EMIT scrollToGroup(view_->geometry().top() + view_->getCategoryPos(categoryIndex), QPrivateSignal());
            });
        }

        Testing::setAccessibleName(view_, qsl("AS EmojiAndStickerPicker emojiListView"));
        vLayout->addWidget(view_);

        QWidget* toolbarPlace = new QWidget(this);
        toolbarPlace->setFixedHeight(toolbar_->height());
        Testing::setAccessibleName(toolbarPlace, qsl("AS EmojiAndStickerPicker emojiCatagoriesToolbar"));
        vLayout->addWidget(toolbarPlace);

        if (firstButton)
            firstButton->toggle();

        connect(view_, &EmojiTableView::clicked, this, [this](const QModelIndex& _index)
        {
            sendEmoji(_index, EmojiSendSource::click);
        });

        connect(view_, &EmojiTableView::mouseMoved, this, [this]()
        {
            Q_EMIT emojiMouseMoved(QPrivateSignal());
        });

        toolbar_->raise();
    }

    void EmojisWidget::onViewportChanged(const QRect& _viewRect, bool _blockToolbarSwitch)
    {
        if (!_blockToolbarSwitch)
            updateToolbar(_viewRect);

        placeToolbar(_viewRect);
    }

    void EmojisWidget::updateToolbar(const QRect& _viewRect)
    {
        const auto& categories = view_->getCategories();
        for (uint32_t i = 0; i < categories.size(); i++)
        {
            const int pos = view_->getCategoryPos(i);
            if (_viewRect.top() - categoryOffset() < pos && pos < (_viewRect.top() + _viewRect.width() / 2))
            {
                buttons_[i]->setChecked(true);
                break;
            }
        }
    }

    void EmojisWidget::placeToolbar(const QRect& _viewRect)
    {
        const int h = toolbar_->height();

        const auto thisRect = rect();

        int y = _viewRect.bottom() + 1 - h;
        if (_viewRect.bottom() > thisRect.bottom() || (-_viewRect.top() > (_viewRect.height() / 2)))
        {
            y = thisRect.bottom() + 1 - h;
            toolbarAtBottom_ = true;
        }
        else
        {
            toolbarAtBottom_ = false;
        }

        toolbar_->setGeometry(0, y, thisRect.width(), h);
    }

    void EmojisWidget::selectFirstButton()
    {
        toolbar_->selectFirst();
    }

    EmojiTableView* EmojisWidget::getView() const noexcept
    {
        return view_;
    }

    bool EmojisWidget::sendSelectedEmoji()
    {
        if (const auto cur = view_->currentIndex(); cur.isValid())
        {
            sendEmoji(cur, EmojiSendSource::keyboard);
            return true;
        }
        return false;
    }

    bool EmojisWidget::toolbarAtBottom() const
    {
        return toolbarAtBottom_;
    }

    void EmojisWidget::updateToolbarSelection()
    {
        updateToolbar(visibleRegion().boundingRect());
    }

    bool EmojisWidget::isKeyboardActive() const
    {
        return view_->isKeyboardActive();
    }

    void Ui::Smiles::EmojisWidget::updateTheme()
    {
        const auto& categories = view_->getCategories();
        for (size_t i = 0; i < categories.size(); ++i)
        {
            const auto& category = categories[i].name_;
            buttons_[i]->setPixmap(categoryButtonIcon(category));
            buttons_[i]->setHoveredPixmap(categoryButtonHoveredIcon(category));
            buttons_[i]->setCheckedPixmap(categoryButtonCheckedIcon(category));
        }
    }

    void EmojisWidget::resizeEvent(QResizeEvent* _event)
    {
        placeToolbar(visibleRegion().boundingRect());
        QWidget::resizeEvent(_event);
    }

    void EmojisWidget::sendEmoji(const QModelIndex& _index, const EmojiSendSource _src) const
    {
        if (const auto& emoji = view_->getEmoji(_index.column(), _index.row()); emoji.isValid())
        {
            QPoint pt;
            if (_src == EmojiSendSource::click)
                pt = view_->mapToGlobal(QPoint(view_->columnViewportPosition(_index.column()), view_->rowViewportPosition(_index.row())));

            Q_EMIT emojiSelected(emoji, pt, QPrivateSignal());

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_picker);
        }
    }


    StickerWidget::StickerWidget(QWidget* _parent, const Utils::FileSharingId& _stickerId, int _itemSize, int _stickerSize)
        : QWidget(_parent)
        , stickerId_(_stickerId)
        , stickerSize_(_stickerSize)
    {
        setFixedSize(_itemSize, _itemSize);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
    }

    void StickerWidget::clearCache()
    {
        delete lottie_;
        lottie_ = nullptr;
    }

    void StickerWidget::paintEvent(QPaintEvent*)
    {
        const auto& data = Stickers::getStickerData(stickerId_, stickerSize());
        if (data.isLottie())
        {
            if (!lottie_ && !data.getLottiePath().isEmpty())
            {
                QRect playerRect({}, QSize(stickerSize_, stickerSize_));
                playerRect.moveCenter(rect().center());

                lottie_ = new LottiePlayer(this, data.getLottiePath(), playerRect, cached_);
                connect(lottie_, &LottiePlayer::firstFrameReady, this, [this]()
                {
                    cached_ = lottie_->getPreview();
                });
            }
            return;
        }
        else if (data.isPixmap())
        {
            if (cached_.isNull())
            {
                const QSize imageSize(Utils::scale_bitmap(stickerSize_), Utils::scale_bitmap(stickerSize_));
                cached_ = data.getPixmap().scaled(imageSize.width(), imageSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (cached_.isNull())
        {
            static const auto radius = Utils::fscale_value(8.);
            static const QMargins margins(Utils::scale_value(4), Utils::scale_value(4), Utils::scale_value(4), Utils::scale_value(4));
            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            p.drawRoundedRect(rect().marginsRemoved(margins), radius, radius);
        }
        else
        {
            const int sticker_margin = (width() - stickerSize_) / 2;
            const QRect imageRect(sticker_margin, sticker_margin, stickerSize_, stickerSize_);

            QRect targetRect(imageRect.topLeft(), Utils::unscale_bitmap(cached_.size()));
            targetRect.moveCenter(imageRect.center());

            p.drawPixmap(targetRect, cached_, cached_.rect());

            const auto sticker = Stickers::getSticker(stickerId_);
            if (sticker && sticker->isGif())
            {
                const auto size = gifLabelSize();

                const auto x = imageRect.bottomRight().x() - size.width() - Utils::scale_value(1);
                const auto y = imageRect.bottomRight().y() - size.height() - Utils::scale_value(1);

                drawGifLabel(p, QPoint(x, y));
            }
        }
    }

    void StickerWidget::onVisibilityChanged(bool _visible)
    {
        if (lottie_)
            lottie_->onVisibilityChanged(_visible);
    }


    StickersTable::StickersTable(
        QWidget* _parent,
        const int32_t _stickersSetId,
        const int32_t _stickerSize,
        const int32_t _itemSize,
        const bool _trasparentForMouse)
        : QWidget(_parent)
        , longtapTimer_(nullptr)
        , stickersSetId_(_stickersSetId)
        , needHeight_(0)
        , columnCount_(0)
        , rowCount_(0)
        , stickerSize_(_stickerSize)
        , itemSize_(_itemSize)
        , hoveredSticker_(-1, {})
        , keyboardActive_(false)
    {
        if (_trasparentForMouse)
            setAttribute(Qt::WA_TransparentForMouseEvents);

        layout_ = new FlowLayout(0, 0, 0);
        layout_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        setLayout(layout_);
    }

    StickersTable::~StickersTable()
    {
        clear();
    }

    void StickersTable::resizeEvent(QResizeEvent * _e)
    {
        if (resize(_e->size()))
        {
            setFixedHeight(getNeedHeight());

            // call on the next loop to ensure all resize stuff is done
            QMetaObject::invokeMethod(this, &StickersTable::onVisibilityChanged, Qt::QueuedConnection);
        }
    }

    void StickersTable::onStickerAdded()
    {
        if (resize(size(), true))
            setFixedHeight(getNeedHeight());

        if (visibleRegion().isEmpty())
            clearWidgets();
        else
            populateStickerWidgets();
    }

    void StickersTable::onVisibilityChanged()
    {
        const auto visibleRect = visibleRegion().boundingRect();
        const auto isTableVisible = !visibleRect.isEmpty();

        if (isTableVisible && layout_->isEmpty())
            populateStickerWidgets();

        const auto widgets = findChildren<StickerWidget*>();

        std::vector<Utils::FileSharingId> toCancel;
        toCancel.reserve(widgets.size());
        for (auto w : widgets)
        {
            const auto isVisible = isTableVisible && visibleRect.intersects(w->geometry());
            w->onVisibilityChanged(isVisible);

            if (!isVisible)
                toCancel.push_back(w->getId());
        }

        if (!toCancel.empty())
            Stickers::cancelDataRequest(std::move(toCancel), stickerSize());
    }

    bool StickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = itemSize_;

        int stickersCount = (int) Stickers::getSetStickersCount(stickersSetId_);

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
            }

            needHeight_ = itemSize_ * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    int StickersTable::getNeedHeight() const
    {
        return needHeight_;
    }

    int32_t StickersTable::getStickerPosInSet(const Utils::FileSharingId& _stickerId) const
    {
        return Stickers::getStickerPosInSet(stickersSetId_, _stickerId);
    }

    const Ui::Stickers::stickersArray& StickersTable::getStickerIds() const
    {
        return Stickers::getStickers(stickersSetId_);
    }

    void StickersTable::onStickerUpdated(int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        redrawSticker(_setId, _stickerId);
    }

    QRect StickersTable::getStickerRect(const int _index) const
    {
        if (columnCount_ > 0 && rowCount_ > 0)
        {
            int itemY = _index / columnCount_;
            int itemX = _index % columnCount_;

            int x = itemSize_ * itemX;
            int y = itemSize_ * itemY;

            return QRect(x, y, itemSize_, itemSize_);
        }

        return QRect();
    }

    QRect StickersTable::getSelectedStickerRect() const
    {
        return getStickerRect(getStickerPosInSet(getSelected().second));
    }

    void StickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        p.setPen(Qt::NoPen);

        const qreal radius = qreal(Utils::scale_value(8));
        const auto& stickersList = getStickerIds();

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            const auto stickerRect = getStickerRect(i);
            if (!_e->rect().intersects(stickerRect))
                continue;

            if (hoveredSticker_.second == stickersList[i])
            {
                if (keyboardActive_)
                    p.setBrush(focusColorPrimary());
                else
                    p.setBrush(getHoverColor());

                p.drawRoundedRect(stickerRect, radius, radius);
            }
        }
    }

    std::pair<int32_t, Utils::FileSharingId> StickersTable::getStickerFromPos(const QPoint& _pos) const
    {
        const auto& stickersList = getStickerIds();

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            if (getStickerRect(i).contains(_pos))
                return std::make_pair(stickersSetId_, stickersList[i]);
        }

        return std::make_pair(-1, Utils::FileSharingId());
    }

    void StickersTable::selectFirst()
    {
        if (const auto& stickersList = getStickerIds(); !stickersList.empty())
        {
            keyboardActive_ = true;
            setSelected({ stickersSetId_, stickersList.front() });
        }
    }

    void StickersTable::selectLast()
    {
        if (const auto& stickersList = getStickerIds(); !stickersList.empty())
        {
            keyboardActive_ = true;
            setSelected({ stickersSetId_, stickersList.back() });
        }
    }

    bool StickersTable::hasSelection() const
    {
        return !getSelected().second.fileId_.isEmpty();
    }

    int StickersTable::selectedItemColumn() const
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            const auto& stickersList = getStickerIds();
            auto it = std::find_if(stickersList.begin(), stickersList.end(), [&sel](const auto _s) { return _s == sel.second; });
            if (it != stickersList.end())
                return std::distance(stickersList.begin(), it) % columnCount_;
        }

        return 0;
    }

    void StickersTable::selectFirstRowAtColumn(const int _column)
    {
        if (const auto& stickersList = getStickerIds(); !stickersList.empty())
        {
            keyboardActive_ = true;

            const auto idx = std::min(int(stickersList.size()) - 1, _column);
            setSelected({ stickersSetId_, stickersList[idx] });
        }
    }

    void StickersTable::selectLastRowAtColumn(const int _column)
    {
        if (rowCount_ == 1)
        {
            selectFirstRowAtColumn(_column);
        }
        else if (const auto& stickersList = getStickerIds(); !stickersList.empty())
        {
            keyboardActive_ = true;

            const auto lastRowIdx = columnCount_ * (rowCount_ - 1);
            const auto idx = std::min(lastRowIdx + _column, int(stickersList.size()) - 1);
            setSelected({ stickersSetId_, stickersList[idx] });
        }
    }

    bool StickersTable::isKeyboardActive()
    {
        return keyboardActive_;
    }

    void StickersTable::cancelStickerRequests()
    {
        const auto widgets = findChildren<StickerWidget*>();
        if (!widgets.isEmpty())
        {
            std::vector<Utils::FileSharingId> toCancel;
            toCancel.reserve(widgets.size());
            for (auto w : widgets)
                toCancel.push_back(w->getId());

            Stickers::cancelDataRequest(std::move(toCancel), stickerSize());
        }
    }

    void StickersTable::clearCache()
    {
        const auto widgets = findChildren<StickerWidget*>();
        for (auto w : widgets)
        {
            w->clearCache();
            if (auto s = Stickers::getSticker(w->getId()))
                s->clearCache();
        }
        Stickers::clearSetCache(stickersSetId_);
    }

    void StickersTable::clearWidgets()
    {
        layout_->clearItems();
    }

    void StickersTable::clear()
    {
        cancelStickerRequests();
        clearCache();
        clearWidgets();
    }

    bool StickersTable::selectRight()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            const auto& stickersList = getStickerIds();
            auto it = std::find_if(stickersList.begin(), stickersList.end(), [&sel](const auto _s) { return _s == sel.second; });
            if (it != stickersList.end())
            {
                it = std::next(it);
                if (it != stickersList.end())
                {
                    keyboardActive_ = true;
                    setSelected({ stickersSetId_, *it });
                    return true;
                }
            }
        }

        return false;
    }

    bool StickersTable::selectLeft()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            const auto& stickersList = getStickerIds();
            auto it = std::find_if(stickersList.begin(), stickersList.end(), [&sel](const auto _s) { return _s == sel.second; });
            if (it != stickersList.end() && it != stickersList.begin())
            {
                it = std::prev(it);
                keyboardActive_ = true;
                setSelected({ stickersSetId_, *it });
                return true;
            }
        }
        return false;
    }

    bool StickersTable::selectUp()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            const auto& stickersList = getStickerIds();
            auto it = std::find_if(stickersList.begin(), stickersList.end(), [&sel](const auto _s) { return _s == sel.second; });
            if (it != stickersList.end() && it != stickersList.begin())
            {
                const auto dist = std::distance(stickersList.begin(), it);
                if (dist >= columnCount_)
                {
                    it = std::prev(it, columnCount_);
                    keyboardActive_ = true;
                    setSelected({ stickersSetId_, *it });
                    return true;
                }
            }
        }
        return false;
    }

    bool StickersTable::selectDown()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            const auto& stickersList = getStickerIds();
            auto it = std::find_if(stickersList.begin(), stickersList.end(), [&sel](const auto _s) { return _s == sel.second; });
            if (it != stickersList.end())
            {
                const int dist = std::distance(stickersList.begin(), it);
                const int maxDist = stickersList.size() - 1;

                if (dist != maxDist)
                {
                    const auto inc = std::min(columnCount_, maxDist - dist);
                    const auto curRow = dist / columnCount_;
                    const auto newRow = (dist + inc) / columnCount_;
                    if (curRow != newRow)
                    {
                        it = std::next(it, inc);
                        if (it != stickersList.end())
                        {
                            keyboardActive_ = true;
                            setSelected({ stickersSetId_, *it });
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    std::pair<int32_t, Utils::FileSharingId> StickersTable::getSelected() const
    {
        return hoveredSticker_;
    }

    void StickersTable::setSelected(const std::pair<int32_t, Utils::FileSharingId>& _sticker)
    {
        const auto prevHovered = hoveredSticker_;

        hoveredSticker_ = _sticker;

        redrawSticker(prevHovered.first, prevHovered.second);
        redrawSticker(hoveredSticker_.first, hoveredSticker_.second);
    }

    bool StickersTable::sendSelected()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            Q_EMIT stickerSelected(sel.second);
            return true;
        }
        return false;
    }

    void StickersTable::clearIfNotSelected(const Utils::FileSharingId& _stickerId)
    {
        if (hasSelection() && getSelected().second != _stickerId)
            clearSelection();
    }

    void StickersTable::clearSelection()
    {
        setSelected({ -1, {} });
    }

    void StickersTable::mouseReleaseEventInternal(const QPoint& _pos)
    {
        if (longtapTimer_)
            longtapTimer_->stop();

        const auto sticker = getStickerFromPos(_pos);
        if (!sticker.second.fileId_.isEmpty())
            Q_EMIT stickerSelected(sticker.second);
    }

    void StickersTable::redrawSticker(const int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        if (!_stickerId.fileId_.isEmpty())
        {
            if (const auto stickerPosInSet = getStickerPosInSet(_stickerId); stickerPosInSet >= 0)
                if (auto item = layout_->itemAt(stickerPosInSet))
                    item->widget()->update();
        }
    }

    void StickersTable::populateStickerWidgets()
    {
        const auto& ids = getStickerIds();
        const auto widgets = findChildren<StickerWidget*>();

        auto areMatching = [&ids, &widgets]()
        {
            im_assert((size_t)widgets.size() == ids.size());
            auto res = std::mismatch(ids.begin(), ids.end(), widgets.begin(), [](const auto& id, auto w) { return w->getId() == id; });
            return res.first == ids.end() && res.second == widgets.end();
        };

        if ((size_t)widgets.size() != ids.size() || !areMatching())
        {
            clearWidgets();

            auto visibleRect = visibleRegion().boundingRect();
            int i = 0;
            for (const auto& s : ids)
            {
                layout_->addWidget(new StickerWidget(this, s, itemSize_, stickerSize_));
                if (visibleRect.intersects(getStickerRect(i++)))
                    Stickers::getStickerData(s, stickerSize());
            }

            update();
        }
    }

    void StickersTable::mouseMoveEventInternal(const QPoint& _pos)
    {
        const auto sticker = getStickerFromPos(_pos);

        if (hoveredSticker_ != sticker || keyboardActive_)
        {
            keyboardActive_ = false;

            const auto prevSticker = hoveredSticker_;
            hoveredSticker_ = sticker;

            redrawSticker(sticker.first, sticker.second);
            redrawSticker(prevSticker.first, prevSticker.second);

            if (!sticker.second.fileId_.isEmpty())
                Q_EMIT stickerHovered(sticker.first, sticker.second);
        }
    }

    void StickersTable::leaveEventInternal()
    {
        keyboardActive_ = false;
        clearSelection();
    }

    void StickersTable::longtapTimeout()
    {
        const auto pos = mapFromGlobal(QCursor::pos());
        const auto sticker = getStickerFromPos(pos);
        if (!sticker.second.fileId_.isEmpty())
            Q_EMIT stickerPreview(sticker.second);
    }

    void StickersTable::mousePressEventInternal(const QPoint& _pos)
    {
        if (const auto sticker = getStickerFromPos(_pos); !sticker.second.fileId_.isEmpty())
        {
            if (!longtapTimer_)
            {
                longtapTimer_ = new QTimer(this);
                longtapTimer_->setSingleShot(true);
                longtapTimer_->setInterval(getLongtapTimeout());
                connect(longtapTimer_, &QTimer::timeout, this, &StickersTable::longtapTimeout);
            }
            longtapTimer_->start();
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentsStickersTable
    //////////////////////////////////////////////////////////////////////////
    RecentsStickersTable::RecentsStickersTable(QWidget* _parent, const qint32 _stickerSize, const qint32 _itemSize)
        : StickersTable(_parent, -1, _stickerSize, _itemSize)
        , maxRowCount_(-1)
    {
    }

    int32_t RecentsStickersTable::getStickerPosInSet(const Utils::FileSharingId& _stickerId) const
    {
        return Utils::indexOf(recentStickersArray_.begin(), recentStickersArray_.end(), _stickerId);
    }

    const Ui::Stickers::stickersArray& RecentsStickersTable::getStickerIds() const
    {
        return recentStickersArray_;
    }

    void RecentsStickersTable::selectLast()
    {
        if (rowCount_ * columnCount_ < max_stickers_count)
            selectLastRowAtColumn(columnCount_ - 1);
        else
            StickersTable::selectLast();
    }

    bool RecentsStickersTable::selectRight()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            if (rowCount_ * columnCount_ < max_stickers_count && getStickerPosInSet(sel.second) >= rowCount_ * columnCount_ - 1)
                return false;
            else
                return StickersTable::selectRight();
        }
        return false;
    }

    bool RecentsStickersTable::selectDown()
    {
        if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
        {
            if (getStickerPosInSet(sel.second) / columnCount_ == rowCount_ - 1)
                return false;
            else
                return StickersTable::selectDown();
        }
        return false;
    }

    void RecentsStickersTable::setMaxRowCount(int _val)
    {
        maxRowCount_ = _val;
    }

    bool RecentsStickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = itemSize_;

        const int stickersCount = recentStickersArray_.size();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
                if (maxRowCount_ > 0 && rowCount_ > maxRowCount_)
                    rowCount_ = maxRowCount_;
            }

            needHeight_ = itemSize_ * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        if (resized)
        {
            if (const auto sel = getSelected(); !sel.second.fileId_.isEmpty())
            {
                if (rowCount_ * columnCount_ < max_stickers_count && getStickerPosInSet(sel.second) >= rowCount_ * columnCount_ - 1)
                    selectLast();
            }
        }

        return resized;
    }

    void RecentsStickersTable::clear()
    {
        recentStickersArray_.clear();
    }

    bool RecentsStickersTable::addSticker(const Utils::FileSharingId& _stickerId)
    {
        for (auto iter = recentStickersArray_.begin(); iter != recentStickersArray_.end(); ++iter)
        {
            if (*iter == _stickerId)
            {
                if (iter == recentStickersArray_.begin())
                    return false;

                recentStickersArray_.erase(iter);
                break;
            }
        }

        recentStickersArray_.emplace(recentStickersArray_.begin(), _stickerId);

        if (recentStickersArray_.size() > max_stickers_count)
            recentStickersArray_.pop_back();

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // class StickersWidget
    //////////////////////////////////////////////////////////////////////////


    StickersWidget::StickersWidget(QWidget* _parent, Toolbar* _toolbar)
        : QWidget(_parent)
        , vLayout_(nullptr)
        , toolbar_(_toolbar)
        , previewActive_(false)
    {
        setCursor(Qt::PointingHandCursor);
        setMouseTracking(true);
    }

    void StickersWidget::insertNextSet(int32_t _setId)
    {
        if (setTables_.find(_setId) != setTables_.end())
            return;

        auto stickersView = new StickersTable(this, _setId, getStickerSize_(), getStickerItemSize_());

        auto button = toolbar_->addButton(_setId);
        loadSetIcon(button);

        Testing::setAccessibleName(button, qsl("AS EmojiAndStickerPicker headerButton") + Stickers::getSetName(_setId));
        button->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());
        button->AttachView(AttachedView(stickersView, this));
        button->setHoveredBackgroundColor(toolbarButtonHoveredBackground());
        button->setCheckedBackgroundColor(toolbarButtonCheckedBackground());
        button->setCheckedBorderColor(toolbarButtonCheckedBorder());

        connect(button, &TabButton::clicked, this, [this, _setId]()
        {
            Q_EMIT scrollTo(setTables_[_setId]->geometry().top() - getPackHeaderHeight());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
        setHeader->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        setHeader->setText(Stickers::getSetName(_setId));
        setHeader->setFixedHeight(getPackHeaderHeight());
        setHeader->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        setHeader->setElideMode(Qt::ElideRight);
        Testing::setAccessibleName(setHeader, qsl("AS EmojiAndStickerPicker packHeader ") + Stickers::getSetName(_setId));
        vLayout_->addWidget(setHeader);


        Testing::setAccessibleName(stickersView, qsl("AS EmojiAndStickerPicker stickersView") + Stickers::getSetName(_setId));
        vLayout_->addWidget(stickersView);
        setTables_[_setId] = stickersView;
        Utils::grabTouchWidget(stickersView);

        connect(stickersView, &StickersTable::stickerSelected, this, [this](const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerSelected(_stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_picker);
        });

        connect(stickersView, &StickersTable::stickerHovered, this, [this](int32_t _setId, const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerHovered(_setId, _stickerId);
        });

        connect(stickersView, &StickersTable::stickerPreview, this, [this](const Utils::FileSharingId& _stickerId)
        {
            previewActive_ = true;
            Q_EMIT stickerPreview(-1, _stickerId);
        });
    }

    void StickersWidget::init()
    {
        if (!vLayout_)
            vLayout_ = Utils::emptyVLayout(this);

        clear();

        const auto lottieAllowed = Features::isAnimatedStickersInPickerAllowed();
        for (const auto _setId : Stickers::getStickersSets())
        {
            if (const auto s = Stickers::getSet(_setId); s && s->isPurchased())
            {
                if (lottieAllowed || !s->containsLottieSticker())
                    insertNextSet(_setId);
            }
        }
    }

    void StickersWidget::clear()
    {
        setTables_.clear();

        while (vLayout_->count())
        {
            QLayoutItem* childItem = vLayout_->itemAt(0);

            QWidget* childWidget = childItem->widget();

            vLayout_->removeItem(childItem);

            delete childWidget;
        }

        toolbar_->Clear();
    }

    void StickersWidget::onStickerUpdated(int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        auto iterSet = setTables_.find(_setId);
        if (iterSet == setTables_.end())
            return;

        iterSet->second->onStickerUpdated(_setId, _stickerId);
    }

    void StickersWidget::resetPreview()
    {
        previewActive_ = false;

        Q_EMIT stickerPreviewClose();
    }

    void StickersWidget::scrollToSticker(int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        auto iterSet = setTables_.find(_setId);
        if (iterSet == setTables_.end())
            return;

        const auto stickerPosInSet = Stickers::getStickerPosInSet(_setId, _stickerId);
        const auto r = iterSet->second->getStickerRect(stickerPosInSet);
        Q_EMIT scrollTo(setTables_[_setId]->geometry().top() + r.top() - getPackHeaderHeight());
    }

    void StickersWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            if (previewActive_)
            {
                resetPreview();
            }
            else
            {
                for (auto& [_, table] : setTables_)
                {
                    if (const auto rc = table->geometry(); rc.contains(_e->pos()))
                        table->mouseReleaseEventInternal(_e->pos() - rc.topLeft());
                }
            }
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void StickersWidget::mousePressEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            for (auto& [_, table] : setTables_)
            {
                if (const auto rc = table->geometry(); rc.contains(_e->pos()))
                    table->mousePressEventInternal(_e->pos() - rc.topLeft());
            }
        }

        QWidget::mousePressEvent(_e);
    }

    void StickersWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        onMouseMoved(_e->pos());

        QWidget::mouseMoveEvent(_e);
    }

    void StickersWidget::leaveEvent(QEvent* _e)
    {
        for (auto& [_, table] : setTables_)
            table->leaveEventInternal();

        QWidget::leaveEvent(_e);
    }

    void StickersWidget::wheelEvent(QWheelEvent* _e)
    {
        onMouseMoved(_e->pos());

        QWidget::wheelEvent(_e);
    }

    void StickersWidget::onMouseMoved(const QPoint& _pos)
    {
        for (auto& [_, table] : setTables_)
        {
            const auto& rc = table->geometry();
            table->mouseMoveEventInternal(_pos - rc.topLeft());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Recents widget
    //////////////////////////////////////////////////////////////////////////
    RecentsWidget::RecentsWidget(QWidget* _parent)
        : QWidget(_parent)
        , vLayout_(nullptr)
        , emojiView_(nullptr)
        , stickersView_(nullptr)
        , previewActive_(false)
        , mouseIntoStickers_(false)
    {
        initEmojisFromSettings();

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::onStickers, this, &RecentsWidget::stickers_event);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &RecentsWidget::stickers_event);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::onStickerMigration, this, &RecentsWidget::onStickerMigration);

        setMouseTracking(true);

        setCursor(Qt::PointingHandCursor);
    }

    RecentsWidget::~RecentsWidget() = default;

    void RecentsWidget::stickers_event()
    {
        if (stickersView_)
            stickersView_->clear();

        initStickersFromSettings();
    }

    void RecentsWidget::init()
    {
        if (vLayout_)
            return;

        vLayout_ = Utils::emptyVLayout(this);

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
        setHeader->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "RECENTS"));
        vLayout_->addWidget(setHeader);

        stickersView_ = new RecentsStickersTable(this, getStickerSize_(), getStickerItemSize_());
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        stickersView_->setMaxRowCount(2);
        vLayout_->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiView_ = new EmojiTableView(this, new EmojiViewItemModel(this, true));
        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emojiView_->addCategory(emoji_category(ql1s("recents"), emojis_));
        vLayout_->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        connect(emojiView_, &EmojiTableView::clicked, this, [this](const QModelIndex& _index)
        {
            sendEmoji(_index, EmojiSendSource::click);
        });

        connect(stickersView_, &RecentsStickersTable::stickerSelected, this, [this](const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerSelected(_stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_recents);
        });

        connect(stickersView_, &RecentsStickersTable::stickerHovered, this, [this](qint32 _setId, const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerHovered(_setId, _stickerId);
        });

        connect(stickersView_, &RecentsStickersTable::stickerPreview, this, [this](const Utils::FileSharingId& _stickerId)
        {
            previewActive_ = true;
            Q_EMIT stickerPreview(-1, _stickerId);
        });

        connect(emojiView_, &EmojiTableView::mouseMoved, this, &RecentsWidget::emojiMouseMoved);
    }

    void RecentsWidget::addSticker(const Utils::FileSharingId& _stickerId)
    {
        addStickers({ _stickerId });
    }

    void RecentsWidget::storeStickers()
    {
        const auto& recentsStickers = stickersView_->getStickerIds();

        QStringList vStickers;
        vStickers.reserve(recentsStickers.size());

        for (const auto& s : recentsStickers)
            if (!s.fileId_.isEmpty())
                vStickers.push_back(s.fileId_ % recentsSettingsSourceDelimiter() % (s.sourceId_ ? *s.sourceId_ : QString()));

        get_gui_settings()->set_value<QString>(Ui::Stickers::recentsStickerSettingsPath().data(), vStickers.join(recentsSettingsDelimiter()));
    }

    void RecentsWidget::onStickerMigration(const std::vector<Ui::Stickers::stickerSptr>& _stickers)
    {
        disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::onStickerMigration, this, &RecentsWidget::onStickerMigration);
        const auto sticks = get_gui_settings()->get_value<std::vector<int32_t>>(settings_old_recents_stickers, {});
        if (!sticks.empty())
            get_gui_settings()->set_value<std::vector<int32_t>>(settings_old_recents_stickers, {});

        if (!sticks.empty() && (sticks.size() % 2 == 0))
        {
            std::vector<Utils::FileSharingId> fs_stickers;
            fs_stickers.reserve(sticks.size() / 2);
            for (size_t i = 0; i < sticks.size(); i += 2)
            {
                const auto it = std::find_if(_stickers.begin(), _stickers.end(), [setId = sticks[i], stickerId = sticks[i + 1]](const auto& s)
                {
                    if (auto id = s->getId(); id.obsoleteId_)
                        return id.obsoleteId_->setId_ == setId && id.obsoleteId_->id_ == stickerId;
                    return false;
                });

                if (it != _stickers.end() && (*it)->getId().fsId_)
                    fs_stickers.push_back(*(*it)->getId().fsId_);
            }
            if (!fs_stickers.empty())
                addStickers(fs_stickers);
        }
    }

    void RecentsWidget::initStickersFromSettings()
    {
        const auto sticks = get_gui_settings()->get_value<std::vector<int32_t>>(settings_old_recents_stickers, std::vector<int32_t>());
        if (!sticks.empty() && (sticks.size() % 2 == 0))
            GetDispatcher()->getFsStickersByIds(sticks);


        auto stikersFromSettings = get_gui_settings()->get_value<QString>(Ui::Stickers::recentsStickerSettingsPath().data(), QString());
        if (stikersFromSettings.isEmpty())
        {
            // migrate recent stickers from old version
            stikersFromSettings = get_gui_settings()->get_value<QString>(settings_recents_fs_stickers, QString());
            get_gui_settings()->set_value<QString>(settings_recents_fs_stickers, QString());
        }
        if (stikersFromSettings.isEmpty())
            return;

        const auto stickers = stikersFromSettings.splitRef(recentsSettingsDelimiter(), Qt::SkipEmptyParts);

        std::vector<Utils::FileSharingId> fs_stickers;
        fs_stickers.reserve(stickers.size());
        for (const auto& s : boost::adaptors::reverse(stickers))
        {
            const auto stickerData = s.split(recentsSettingsSourceDelimiter(), QString::SkipEmptyParts);
            const auto size = stickerData.size();
            if (size == 1)
                fs_stickers.push_back({ stickerData.at(0).toString(), std::nullopt });
            else if (size == 2)
                fs_stickers.push_back({ stickerData.at(0).toString(), stickerData.at(1).toString() });
        }
        addStickers(fs_stickers);
    }

    void RecentsWidget::addEmoji(const Emoji::EmojiRecord& _emoji)
    {
        init();

        if (!emojis_.empty())
        {
            const auto it = std::find_if(emojis_.begin(), emojis_.end(), [&_emoji](const auto& _e) { return _emoji.fullCodePoints == _e.fullCodePoints; });
            if (it == emojis_.begin())
                return;
            else if (it != emojis_.end())
                emojis_.erase(it);
        }

        emojis_.insert(emojis_.begin(), _emoji);
        if (emojis_.size() > maxRecentEmoji())
            emojis_.pop_back();

        emojiView_->onEmojiAdded();
        emojiView_->update();

        saveEmojiToSettings();
    }

    void RecentsWidget::saveEmojiToSettings()
    {
        QStringList str;
        str.reserve(emojis_.size());
        for (const auto& x : std::as_const(emojis_))
            str << x.fullCodePoints.serialize2();

        get_gui_settings()->set_value<QString>(settings_recents_emojis2, str.join(recentsSettingsDelimiter()));
    }

    bool RecentsWidget::selectionInStickers() const
    {
        return stickersView_ && !stickersView_->getSelected().second.fileId_.isEmpty();
    }

    bool RecentsWidget::sendSelectedEmoji()
    {
        if (emojiView_)
        {
            if (const auto cur = emojiView_->currentIndex(); cur.isValid())
            {
                sendEmoji(cur, EmojiSendSource::keyboard);
                return true;
            }
        }
        return false;
    }

    void RecentsWidget::sendEmoji(const QModelIndex& _index, const EmojiSendSource _src)
    {
        if (!emojiView_)
            return;

        if (const auto& emoji = emojiView_->getEmoji(_index.column(), _index.row()); emoji.isValid())
        {
            QPoint pt;
            if (_src == EmojiSendSource::click)
                pt = emojiView_->mapToGlobal(emojiView_->visualRect(_index).topLeft());
            Q_EMIT emojiSelected(emoji, pt);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_from_recents);
        }
    }

    void RecentsWidget::addStickers(const std::vector<Utils::FileSharingId>& _stickers)
    {
        if (_stickers.empty())
            return;

        init();

        const auto lottieAllowed = Features::isAnimatedStickersInPickerAllowed();
        auto added = false;
        for (const auto& s : _stickers)
        {
            if (!s.fileId_.isEmpty())
            {
                const auto sticker = Stickers::getSticker(s);
                const auto isLottie = sticker && sticker->isLottie();

                if (!isLottie || lottieAllowed)
                    added |= stickersView_->addSticker(s);
            }
        }

        if (added)
        {
            stickersView_->onStickerAdded();
            storeStickers();
        }
    }

    void RecentsWidget::initEmojisFromSettings()
    {
        emojis_.reserve(maxRecentEmoji());
        if (const auto emojis = get_gui_settings()->get_value<QString>(settings_recents_emojis2, QString()); !emojis.isEmpty())
        {
            init();
            const auto split = emojis.splitRef(recentsSettingsDelimiter(), Qt::SkipEmptyParts);

            for (const auto& str : split)
            {
                if (const auto emojiCode = Emoji::EmojiCode::unserialize2(str); !emojiCode.isNull())
                {
                    if (const auto& emoji = Emoji::GetEmojiInfoByCodepoint(emojiCode); emoji.isValid())
                        emojis_.push_back(emoji);
                }
            }
        }

        if (!emojis_.empty())
            emojiView_->onEmojiAdded();
    }

    void RecentsWidget::onStickerUpdated(int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        if (stickersView_)
            stickersView_->onStickerUpdated(_setId, _stickerId);
    }

    void RecentsWidget::resetPreview()
    {
        previewActive_ = false;

        Q_EMIT stickerPreviewClose();
    }

    bool RecentsWidget::sendSelected()
    {
        if (stickersView_ && selectionInStickers())
            return stickersView_->sendSelected();
        else if (emojiView_)
            return sendSelectedEmoji();
        return false;
    }

    bool RecentsWidget::selectUp()
    {
        if (stickersView_ && emojiView_)
        {
            if (!stickersView_->getSelected().second.fileId_.isEmpty())
            {
                emojiView_->clearSelection();
                return stickersView_->selectUp();
            }
            else
            {
                if (!emojiView_->selectUp())
                {
                    emojiView_->clearSelection();
                    stickersView_->selectLastRowAtColumn(0);
                }
                return true;
            }
        }
        else if (stickersView_)
        {
            return stickersView_->selectUp();
        }
        else if (emojiView_)
        {
            return emojiView_->selectUp();
        }

        return false;
    }

    bool RecentsWidget::selectDown()
    {
        if (stickersView_ && emojiView_)
        {
            if (selectionInStickers())
            {
                if (!stickersView_->selectDown())
                {
                    stickersView_->clearSelection();
                    emojiView_->selectFirst();
                }
                return true;
            }
            else
            {
                return emojiView_->selectDown();
            }
        }
        else if (stickersView_)
        {
            return stickersView_->selectDown();
        }
        else if (emojiView_)
        {
            return emojiView_->selectDown();
        }

        return false;
    }

    bool RecentsWidget::selectLeft()
    {
        if (stickersView_ && emojiView_)
        {
            if (selectionInStickers())
            {
                emojiView_->clearSelection();
                return stickersView_->selectLeft();
            }
            else
            {
                if (!emojiView_->selectLeft())
                {
                    emojiView_->clearSelection();
                    stickersView_->selectLast();
                }
                return true;
            }
        }
        else if (stickersView_)
        {
            return stickersView_->selectLeft();
        }
        else if (emojiView_)
        {
            return emojiView_->selectLeft();
        }

        return false;
    }

    bool RecentsWidget::selectRight()
    {
        if (stickersView_ && emojiView_)
        {
            if (selectionInStickers())
            {
                if (!stickersView_->selectRight())
                {
                    stickersView_->clearSelection();
                    emojiView_->selectFirst();
                }
                return true;
            }
            else
            {
                return emojiView_->selectRight();
            }
        }
        else if (stickersView_)
        {
            return stickersView_->selectRight();
        }
        else if (emojiView_)
        {
            return emojiView_->selectRight();
        }

        return false;
    }

    void RecentsWidget::selectFirst()
    {
        clearSelection();

        if (stickersView_)
            stickersView_->selectFirst();
        else if (emojiView_)
            emojiView_->selectFirst();
    }

    void RecentsWidget::selectLast()
    {
        clearSelection();

        if (emojiView_)
            emojiView_->selectLast();
        else if (stickersView_)
            stickersView_->selectLast();
    }

    bool RecentsWidget::hasSelection() const
    {
        return
            (stickersView_ && stickersView_->hasSelection()) ||
            (emojiView_ && emojiView_->hasSelection());
    }

    int RecentsWidget::selectedItemColumn() const
    {
        if (stickersView_ && selectionInStickers())
            return stickersView_->selectedItemColumn();
        else if (emojiView_)
            return emojiView_->selectedItemColumn();

        return 0;
    }

    void RecentsWidget::selectFirstRowAtColumn(const int _column)
    {
        clearSelection();

        if (stickersView_)
            stickersView_->selectFirstRowAtColumn(_column);
        else if (emojiView_)
            emojiView_->selectFirstRowAtColumn(_column);
    }

    void RecentsWidget::selectLastRowAtColumn(const int _column)
    {
        clearSelection();

        if (emojiView_)
            emojiView_->selectLastRowAtColumn(_column);
        else if (stickersView_)
            stickersView_->selectLastRowAtColumn(_column);
    }

    void RecentsWidget::clearSelection()
    {
        if (stickersView_)
            stickersView_->clearSelection();
        if (emojiView_)
            emojiView_->clearSelection();
    }

    void RecentsWidget::clearIfNotSelected(const Utils::FileSharingId& _stickerId)
    {
        if (stickersView_)
            stickersView_->clearIfNotSelected(_stickerId);
        if (emojiView_)
            emojiView_->clearSelection();
    }

    bool RecentsWidget::isKeyboardActive()
    {
        return ((stickersView_&& stickersView_->isKeyboardActive()) || (emojiView_ && emojiView_->isKeyboardActive()));
    }

    void RecentsWidget::onVisibilityChanged()
    {
        if (stickersView_)
            stickersView_->onVisibilityChanged();
    }

    void RecentsWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (stickersView_)
        {
            if (_e->button() == Qt::MouseButton::LeftButton)
            {
                if (previewActive_)
                {
                    resetPreview();
                }
                else
                {
                    const auto& rc = stickersView_->geometry();

                    if (rc.contains(_e->pos()))
                        stickersView_->mouseReleaseEventInternal(_e->pos() - rc.topLeft());
                }
            }
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void RecentsWidget::mousePressEvent(QMouseEvent* _e)
    {
        if (stickersView_)
        {
            if (_e->button() == Qt::MouseButton::LeftButton)
            {
                const auto& rc = stickersView_->geometry();

                if (rc.contains(_e->pos()))
                    stickersView_->mousePressEventInternal(_e->pos() - rc.topLeft());
            }
        }

        QWidget::mousePressEvent(_e);
    }

    void RecentsWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        if (stickersView_)
        {
            const auto& rc = stickersView_->geometry();

            if (rc.contains(_e->pos()))
            {
                stickersView_->mouseMoveEventInternal(_e->pos() - rc.topLeft());

                mouseIntoStickers_ = true;

                if (emojiView_)
                    emojiView_->clearSelection();
            }
            else if (mouseIntoStickers_)
            {
                stickersView_->leaveEventInternal();

                mouseIntoStickers_ = false;
            }
        }

        QWidget::mouseMoveEvent(_e);
    }

    void RecentsWidget::leaveEvent(QEvent* _e)
    {
        if (stickersView_)
        {
            stickersView_->leaveEventInternal();
            mouseIntoStickers_ = false;
        }

        clearSelection();

        QWidget::leaveEvent(_e);
    }

    void RecentsWidget::emojiMouseMoved()
    {
        if (stickersView_)
        {
            stickersView_->leaveEventInternal();
            mouseIntoStickers_ = false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // SmilesMenu class
    //////////////////////////////////////////////////////////////////////////
    SmilesMenu::SmilesMenu(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , lottieAllowed_(Features::isAnimatedStickersInPickerAllowed())
        , aimId_(_aimId)
    {
        setContentsMargins(0, pickerTopBorderWidth(), 0, 0);
        rootVerticalLayout_ = Utils::emptyVLayout(this);

        Utils::ApplyStyle(this, Styling::getParameters().getSmilesQss());

        connect(&Stickers::getCache(), &Stickers::Cache::setIconUpdated, this, &SmilesMenu::onSetIconChanged);

        InitSelector();
        InitStickers();
        InitRecents();

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &SmilesMenu::updateTheme);

        stickersView_->init();
        recentsView_->initStickersFromSettings();
    }

    SmilesMenu::~SmilesMenu() = default;

    void SmilesMenu::onSetIconChanged(int _setId)
    {
        im_assert(_setId >= 0);
        if (_setId < 0)
            return;

        if (auto button = topToolbar_->getButton(_setId))
            loadSetIcon(button);
    }

    void SmilesMenu::touchScrollStateChanged(QScroller::State state)
    {
        recentsView_->blockSignals(state != QScroller::Inactive);
        emojiView_->blockSignals(state != QScroller::Inactive);
        stickersView_->blockSignals(state != QScroller::Inactive);
    }

    void SmilesMenu::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.1));

        if (drawTopBorder_)
        {
            p.setPen(QPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT), pickerTopBorderWidth()));
            p.drawLine(QPoint(0, 0), QPoint(width(), 0));
        }
    }

    void SmilesMenu::focusOutEvent(QFocusEvent* _event)
    {
        Qt::FocusReason reason = _event->reason();

        if (reason != Qt::FocusReason::ActiveWindowFocusReason)
            hideAnimated();

        QWidget::focusOutEvent(_event);
    }

    void SmilesMenu::showAnimated()
    {
        if (isVisible_)
            return;

        showHideAnimated();
    }

    void SmilesMenu::hideAnimated()
    {
        if (!isVisible_)
            return;

        showHideAnimated();
    }

    void SmilesMenu::showHideAnimated(const bool _fromKeyboard)
    {
        im_assert(!aimId_.isEmpty());

        isVisible_ = !isVisible_;

        auto pickerHeight = getDefaultPickerHeight();
        if (const auto page = Utils::InterConnector::instance().getHistoryPage(aimId_))
        {
            const auto messagesAreaHeight = page->getMessagesAreaHeight();
            pickerHeight = ((messagesAreaHeight - getDefaultPickerHeight()) > getMinimalMessagesAreaHeight()) ? getDefaultPickerHeight() : messagesAreaHeight - getMinimalMessagesAreaHeight();
        }

        const auto startValue = isVisible_ ? 0 : pickerHeight;
        const auto endValue = isVisible_ ? pickerHeight : 0;

        if (!animHeight_)
        {
            animHeight_ = new QPropertyAnimation(this, QByteArrayLiteral("currentHeight"), this);
            animHeight_->setEasingCurve(QEasingCurve::InQuad);

            constexpr std::chrono::milliseconds duration(200);
            animHeight_->setDuration(duration.count());

            connect(animHeight_, &QPropertyAnimation::finished, this, [this]()
            {
                if (!isVisible_)
                    clearCache();
            });
        }
        else
        {
           if (animHeight_->state() == QAbstractAnimation::Running)
               return;
        }

        animHeight_->stop();
        animHeight_->setStartValue(startValue);
        animHeight_->setEndValue(endValue);
        animHeight_->start();

        if (isVisible_)
        {
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = connect(animHeight_, &QPropertyAnimation::finished, this, [this, conn, _fromKeyboard]()
            {
                if (_fromKeyboard)
                {
                    if (!hasSelection())
                        selectFirst();
                    ensureSelectedVisible();
                }
                disconnect(*conn);
            });
        }

        Q_EMIT visibilityChanged(isVisible_, QPrivateSignal());
    }

    bool SmilesMenu::isHidden() const
    {
        return currentHeight_ <= 0;
    }

    void SmilesMenu::ScrollTo(const int _pos)
    {
        if (viewArea_->verticalScrollBar()->value() == _pos ||
            (animScroll_ && animScroll_->state() == QAbstractAnimation::State::Running && animScroll_->endValue() == _pos))
            return;

        if (!animScroll_)
        {
            constexpr std::chrono::milliseconds duration(200);

            animScroll_ = new QPropertyAnimation(viewArea_->verticalScrollBar(), QByteArrayLiteral("value"), this);
            animScroll_->setEasingCurve(QEasingCurve::InQuad);
            animScroll_->setDuration(duration.count());
            connect(animScroll_, &QPropertyAnimation::finished, this, [this]()
            {
                blockToolbarSwitch_ = false;
            });
        }

        blockToolbarSwitch_ = true;
        animScroll_->stop();
        animScroll_->setStartValue(viewArea_->verticalScrollBar()->value());
        animScroll_->setEndValue(_pos);
        animScroll_->start();
    }

    void SmilesMenu::InitRecents()
    {
        connect(recentsView_, &RecentsWidget::emojiSelected, this, [this](const Emoji::EmojiRecord& _emoji, const QPoint _pos)
        {
            Q_EMIT emojiSelected(_emoji.fullCodePoints, _pos);
        });

        connect(recentsView_, &RecentsWidget::stickerSelected, this, [this](const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerSelected(_stickerId);
        });

        connect(recentsView_, &RecentsWidget::stickerHovered, this, &SmilesMenu::onStickerHovered);

        connect(recentsView_, &RecentsWidget::stickerPreview, this, [this](qint32 _setId, const Utils::FileSharingId& _stickerId)
        {
            showStickerPreview(_setId, _stickerId);
        });

        connect(recentsView_, &RecentsWidget::stickerPreviewClose, this, [this]()
        {
            hideStickerPreview();
        });
    }

    void SmilesMenu::selectItemUp()
    {
        if (auto smTable = getCurrentMenuTable())
        {
            const auto selUp = smTable->selectUp();
            if (!selUp && !topToolbar_->isFirstSelected())
            {
                const auto col = smTable->selectedItemColumn();
                clearSelection();
                topToolbar_->selectPrevious();

                const auto toEmoji = getCurrentView() == emojiView_;
                if (toEmoji)
                    selectLast();
                else
                    selectLastAtColumn(col);
            }
        }
    }

    void SmilesMenu::selectItemDown()
    {
        if (auto smTable = getCurrentMenuTable())
        {
            const auto sel = smTable->selectDown();
            if (!sel && !topToolbar_->isLastSelected())
            {
                const auto col = smTable->selectedItemColumn();
                const auto fromEmoji = getCurrentView() == emojiView_;

                clearSelection();
                topToolbar_->selectNext();

                if (fromEmoji)
                    selectFirst();
                else
                    selectFirstAtColumn(col);
            }
        }
    }

    void SmilesMenu::selectItemLeft()
    {
        if (auto smTable = getCurrentMenuTable())
        {
            const auto sel = smTable->selectLeft();
            if (!sel && !topToolbar_->isFirstSelected())
            {
                clearSelection();
                topToolbar_->selectPrevious();
                selectLast();
            }
        }
    }

    void SmilesMenu::selectItemRight()
    {
        if (auto smTable = getCurrentMenuTable())
        {
            const auto sel = smTable->selectRight();
            if (!sel && !topToolbar_->isLastSelected())
            {
                clearSelection();
                topToolbar_->selectNext();
                selectFirst();
            }
        }
    }

    void SmilesMenu::selectLastAtColumn(const int _column)
    {
        if (auto smTable = getCurrentMenuTable())
            smTable->selectLastRowAtColumn(_column);
    }

    void SmilesMenu::selectFirstAtColumn(const int _column)
    {
        if (auto smTable = getCurrentMenuTable())
            smTable->selectFirstRowAtColumn(_column);
    }

    void SmilesMenu::selectFirst()
    {
        if (auto smTable = getCurrentMenuTable())
            smTable->selectFirst();
    }

    void SmilesMenu::selectLast()
    {
        if (auto smTable = getCurrentMenuTable())
            smTable->selectLast();
    }

    void SmilesMenu::clearTablesSelection()
    {
        for (const auto btn : topToolbar_->GetButtons())
        {
            if (const auto view = btn->GetAttachedView().getView())
            {
                if (auto table = qobject_cast<StickersTable*>(view))
                    table->clearSelection();
            }
        }
    }

    void SmilesMenu::clearSelection()
    {
        recentsView_->clearSelection();
        emojiView_->getView()->clearSelection();
        clearTablesSelection();
    }

    bool SmilesMenu::hasSelection() const
    {
        for (const auto btn : topToolbar_->GetButtons())
        {
            if (const auto view = btn->GetAttachedView().getView())
            {
                if (auto table = qobject_cast<StickersTable*>(view); table && table->hasSelection())
                    return true;
            }
        }

        return recentsView_->hasSelection() || emojiView_->getView()->hasSelection();
    }

    void SmilesMenu::setToolBarVisible(bool _visible)
    {
        topToolbar_->setVisible(false);
    }

    void SmilesMenu::setRecentsVisible(bool _visible)
    {
        recentsView_->setVisible(_visible);
    }

    void SmilesMenu::setStickersVisible(bool _visible)
    {
        stickersView_->setVisible(_visible);
    }

    void SmilesMenu::setAddButtonVisible(bool _visible)
    {
        topToolbar_->setAddButtonVisible(_visible);
    }

    void SmilesMenu::setHorizontalMargins(int _margin)
    {
        viewAreaLayout_->setContentsMargins(_margin, 0, _margin, 0);
    }

    void SmilesMenu::setDrawTopBorder(bool _draw)
    {
        if (drawTopBorder_ != _draw)
        {
            auto margins = contentsMargins();
            margins.setTop(margins.top() + pickerTopBorderWidth() * (_draw ? 1 : -1));
            setContentsMargins(margins);
        }

        drawTopBorder_ = _draw;
    }

    void SmilesMenu::setTopSpacing(int _spacing)
    {
        topSpacer_->changeSize(0, _spacing);
        viewAreaLayout_->invalidate();
    }

    void Smiles::SmilesMenu::setPreviewAreaAdditions(QMargins _margins)
    {
        previewMargins_ = std::move(_margins);
    }

    bool SmilesMenu::sendSelectedItem()
    {
        if (hasSelection())
        {
            if (const auto view = getCurrentView())
            {
                if (view == recentsView_)
                    return recentsView_->sendSelected();
                else if (view == emojiView_)
                    return emojiView_->sendSelectedEmoji();
                else if (auto table = qobject_cast<StickersTable*>(view))
                    return table->sendSelected();
            }
        }

        return false;
    }

    void SmilesMenu::ensureSelectedVisible()
    {
        if (const auto view = getCurrentView())
        {
            const auto vp = viewArea_->viewport();
            const auto vpRect = QRect(vp->mapToGlobal(vp->pos()), vp->size());

            QScopedValueRollback scoped(blockToolbarSwitch_, true);
            if (view == recentsView_)
            {
                showRecents();
            }
            else if (view == emojiView_)
            {
                const auto ew = emojiView_->getView();
                if (const auto idx = ew->currentIndex(); idx.isValid())
                {
                    auto r = ew->visualRect(idx);
                    r.translate(0, emojiView_->geometry().top() + ew->geometry().top());

                    auto rg = r;
                    rg.moveTopLeft(viewArea_->widget()->mapToGlobal(r.topLeft()));

                    const auto botAdj = emojiView_->toolbarAtBottom() ? 0 : -emojiToolbarHeight();
                    const auto vpRectAdj = vpRect.adjusted(0, 0, 0, botAdj);
                    if (!vpRectAdj.contains(rg))
                    {
                        const auto yAdj = rg.bottom() >= vpRectAdj.bottom() ? botAdj : 0;
                        viewArea_->ensureVisible(0, r.center().y() - yAdj, 0, getEmojiItemSize());
                        emojiView_->updateToolbarSelection();
                    }
                }
            }
            else if (auto table = qobject_cast<StickersTable*>(view))
            {
                if (auto r = table->getSelectedStickerRect(); r.isValid())
                {
                    r.translate(0, stickersView_->geometry().top() + table->geometry().top());

                    auto rg = r;
                    rg.moveTopLeft(viewArea_->widget()->mapToGlobal(r.topLeft()));

                    if (!vpRect.contains(rg))
                        viewArea_->ensureVisible(0, r.center().y(), 0, getStickerItemSize_());
                }
            }
        }
    }

    SmilesMenuTable* SmilesMenu::getCurrentMenuTable() const
    {
        SmilesMenuTable* smTable = nullptr;
        if (const auto view = getCurrentView())
        {
            if (view == recentsView_)
                smTable = static_cast<SmilesMenuTable*>(recentsView_);
            else if (view == emojiView_)
                smTable = static_cast<SmilesMenuTable*>(emojiView_->getView());
            else if (auto table = qobject_cast<StickersTable*>(view))
                smTable = static_cast<SmilesMenuTable*>(table);
        }

        return smTable;
    }

    QWidget* SmilesMenu::getCurrentView() const
    {
        if (const auto btn = topToolbar_->selectedButton())
            if (const auto view = btn->GetAttachedView().getView())
                return view;

        return nullptr;
    }

    void SmilesMenu::stickersMetaEvent()
    {
        stickersView_->init();
    }

    void SmilesMenu::stickerEvent(const qint32 _error, const qint32 _setId, const Utils::FileSharingId& _stickerId)
    {
        stickersView_->onStickerUpdated(_setId, _stickerId);
        recentsView_->onStickerUpdated(_setId, _stickerId);
    }

    void SmilesMenu::InitStickers()
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::onStickers, this, &SmilesMenu::stickersMetaEvent);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &SmilesMenu::onOmicronUpdate);
        connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, [this](qint32 _error, const Utils::FileSharingId& _stickerId, qint32 _setId) { stickerEvent(_error, _setId, _stickerId); });

        connect(stickersView_, &StickersWidget::stickerSelected, this, [this](const Utils::FileSharingId& _stickerId)
        {
            Q_EMIT stickerSelected(_stickerId);

            recentsView_->addSticker(_stickerId);
        });

        connect(stickersView_, &StickersWidget::stickerHovered, this, &SmilesMenu::onStickerHovered);

        connect(stickersView_, &StickersWidget::stickerPreview, this, [this](qint32 _setId, const Utils::FileSharingId& _stickerId)
        {
            showStickerPreview(_setId, _stickerId);
        });

        connect(stickersView_, &StickersWidget::stickerPreviewClose, this, [this]()
        {
            hideStickerPreview();
        });

        connect(stickersView_, &StickersWidget::scrollTo, this, [this](int _pos)
        {
            ScrollTo(stickersView_->geometry().top() + _pos);
        });
    }

    void SmilesMenu::addStickerToRecents(const qint32 _setId, const Utils::FileSharingId& _stickerId)
    {
        recentsView_->addSticker(_stickerId);
    }

    void SmilesMenu::InitSelector()
    {
        topToolbar_ = new Toolbar(this, Toolbar::buttonsAlign::left);
        recentsView_ = new RecentsWidget(this);
        emojiView_ = new EmojisWidget(this);

        topToolbar_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        topToolbar_->setFixedHeight(getToolBarButtonSize() + Utils::scale_value(1));

        recentsButton_ = topToolbar_->addButton(Utils::renderSvgScaled(qsl(":/smiles_menu/recent"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        Testing::setAccessibleName(recentsButton_, qsl("AS EmojiAndStickerPicker recentsButton"));
        recentsButton_->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());
        recentsButton_->AttachView(recentsView_);
        recentsButton_->setFixed(true);
        recentsButton_->setHoveredBackgroundColor(toolbarButtonHoveredBackground());
        recentsButton_->setCheckedBackgroundColor(toolbarButtonCheckedBackground());
        recentsButton_->setCheckedBorderColor(toolbarButtonCheckedBorder());

        connect(recentsButton_, &TabButton::clicked, this, &SmilesMenu::showRecents);

        auto makeEmojiImage = []()
        {
            auto emojiPixmap = QPixmap::fromImage(Emoji::GetEmojiImage(Emoji::EmojiCode(0x1f603), Utils::scale_bitmap_with_value(24)));
            Utils::check_pixel_ratio(emojiPixmap);
            return emojiPixmap;
        };

        auto emojiButton = topToolbar_->addButton(makeEmojiImage());
        Testing::setAccessibleName(emojiButton, qsl("AS EmojiAndStickerPicker emojiButton"));
        emojiButton->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());
        emojiButton->AttachView(AttachedView(emojiView_));
        emojiButton->setFixed(true);
        emojiButton->setHoveredBackgroundColor(toolbarButtonHoveredBackground());
        emojiButton->setCheckedBackgroundColor(toolbarButtonCheckedBackground());
        emojiButton->setCheckedBorderColor(toolbarButtonCheckedBorder());

        connect(emojiButton, &TabButton::clicked, this, [this]()
        {
            ScrollTo(emojiView_->geometry().top());
            emojiView_->selectFirstButton();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        Testing::setAccessibleName(topToolbar_, qsl("AS EmojiAndStickerPicker topToolbar"));
        rootVerticalLayout_->addWidget(topToolbar_);

        viewArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &SmilesMenu::touchScrollStateChanged, Qt::QueuedConnection);
        connect(viewArea_->verticalScrollBar(), &QScrollBar::valueChanged, this, &SmilesMenu::onScroll, Qt::QueuedConnection);

        QWidget* scroll_area_widget = new QWidget(viewArea_);
        Testing::setAccessibleName(scroll_area_widget, qsl("AS EmojiAndStickerPicker scrollAreaWidget"));
        scroll_area_widget->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(scroll_area_widget);
        viewArea_->setWidgetResizable(true);
        Testing::setAccessibleName(viewArea_, qsl("AS EmojiAndStickerPicker viewArea"));
        rootVerticalLayout_->addWidget(viewArea_);

        viewAreaLayout_ = Utils::emptyVLayout(scroll_area_widget);

        topSpacer_ = new QSpacerItem(0, 0);
        viewAreaLayout_->addSpacerItem(topSpacer_);
        recentsView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(recentsView_, qsl("AS EmojiAndStickerPicker recentsView"));
        viewAreaLayout_->addWidget(recentsView_);
        Utils::grabTouchWidget(recentsView_);

        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        Testing::setAccessibleName(emojiView_, qsl("AS EmojiAndStickerPicker emojiView"));
        viewAreaLayout_->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        stickersView_ = new StickersWidget(this, topToolbar_);
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        Testing::setAccessibleName(stickersView_, qsl("AS EmojiAndStickerPicker stickersView"));
        viewAreaLayout_->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiButton->toggle();

        connect(emojiView_, &EmojisWidget::emojiSelected, this, [this](const Emoji::EmojiRecord& _emoji, const QPoint _pos)
        {
            Q_EMIT emojiSelected(_emoji.fullCodePoints, _pos);

            recentsView_->addEmoji(_emoji);
        });

        connect(emojiView_, &EmojisWidget::scrollToGroup, this, [this](int _pos)
        {
            ScrollTo(emojiView_->geometry().top() + _pos);
        });

        connect(emojiView_, &EmojisWidget::emojiMouseMoved, this, &SmilesMenu::onEmojiHovered);

        HookScroll();
    }

    void SmilesMenu::HookScroll()
    {
        connect(viewArea_->verticalScrollBar(), &QAbstractSlider::valueChanged, this, [this](int _value)
        {
            const auto& emojiRect = emojiView_->geometry();
            QRect viewPortRect = viewArea_->viewport()->geometry();
            QRect viewPortRectForEmoji(0, _value - emojiRect.top(), viewPortRect.width(), viewPortRect.height());

            emojiView_->onViewportChanged(viewPortRectForEmoji, blockToolbarSwitch_);

            viewPortRect.adjust(0, _value, 0, _value);

            if (blockToolbarSwitch_)
                return;

            for (TabButton* button : topToolbar_->GetButtons())
            {
                auto view = button->GetAttachedView();
                if (view.getView())
                {
                    QRect rcView = view.getView()->geometry();

                    if (view.getViewParent())
                    {
                        QRect rcViewParent = view.getViewParent()->geometry();
                        rcView.adjust(0, rcViewParent.top(), 0, rcViewParent.top());
                    }

                    QRect intersectedRect = rcView.intersected(viewPortRect);

                    if (intersectedRect.height() > (viewPortRect.height() / 2))
                    {
                        button->setChecked(true);
                        topToolbar_->scrollToButton(button);
                        break;
                    }
                }
            }
        });
    }

    void SmilesMenu::showRecents()
    {
        QScopedValueRollback scoped(blockToolbarSwitch_, true);

        viewArea_->verticalScrollBar()->setValue(recentsView_->geometry().top());
        emojiView_->selectFirstButton();
    }

    void SmilesMenu::showStickerPreview(const int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        if (!stickerPreview_)
        {
            stickerPreview_ = new StickerPreview(window(), _setId, _stickerId, StickerPreview::Context::Picker);

            // !!! must be Qt::QueuedConnection
            connect(stickerPreview_, &StickerPreview::needClose, recentsView_, &RecentsWidget::resetPreview, Qt::QueuedConnection);
            connect(stickerPreview_, &StickerPreview::needClose, stickersView_, &StickersWidget::resetPreview, Qt::QueuedConnection);

            auto previewRect = rect().marginsAdded(previewMargins_);
            previewRect.moveTo(mapTo(window(), previewRect.topLeft()));

            stickerPreview_->setGeometry(previewRect);
            stickerPreview_->show();
            stickerPreview_->raise();

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_longtap_picker);
        }
    }

    void SmilesMenu::updateStickerPreview(const int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        if (stickerPreview_)
            stickerPreview_->showSticker(_stickerId);
    }

    void SmilesMenu::onStickerHovered(const int32_t _setId, const Utils::FileSharingId& _stickerId)
    {
        emojiView_->getView()->clearSelection();

        recentsView_->clearIfNotSelected(_stickerId);

        for (const auto btn : topToolbar_->GetButtons())
        {
            if (auto table = qobject_cast<StickersTable*>(btn->GetAttachedView().getView()))
                table->clearIfNotSelected(_stickerId);
        }

        updateStickerPreview(_setId, _stickerId);
    }

    void SmilesMenu::onEmojiHovered()
    {
        recentsView_->clearSelection();
        clearTablesSelection();
    }

    bool SmilesMenu::iskeyboardActive() const
    {
        if (const auto view = getCurrentView())
        {
            if (view == recentsView_)
                return recentsView_->isKeyboardActive();
            else if (view == emojiView_)
                return emojiView_->isKeyboardActive();
            else if (auto table = qobject_cast<StickersTable*>(view))
                return table->isKeyboardActive();
        }
        return false;
    }

    void SmilesMenu::clearCache()
    {
        Stickers::clearCache();

        for (const auto btn : topToolbar_->GetButtons())
        {
            if (const auto view = btn->GetAttachedView().getView())
            {
                if (auto table = qobject_cast<StickersTable*>(view))
                    table->clear();
            }
        }
    }

    void SmilesMenu::updateStickersVisibility()
    {
        const auto visibleRect = stickersView_->visibleRegion().boundingRect();
        const auto unloadDistance = getStickerItemSize_() * 3;
        for (const auto btn : topToolbar_->GetButtons())
        {
            if (const auto view = btn->GetAttachedView().getView())
            {
                if (auto table = qobject_cast<StickersTable*>(view))
                {
                    const auto& tableRect = table->geometry();
                    const auto unload =
                        visibleRect.isEmpty() ||
                        tableRect.top() > visibleRect.bottom() + unloadDistance ||
                        tableRect.bottom() < visibleRect.top() - unloadDistance;

                    if (unload)
                    {
                        table->cancelStickerRequests();
                        table->clearCache();
                    }
                    else
                    {
                        table->onVisibilityChanged();
                    }
                }
            }
        }

        recentsView_->onVisibilityChanged();
    }

    void SmilesMenu::onOmicronUpdate()
    {
        const auto lottieAllowed = Features::isAnimatedStickersInPickerAllowed();
        if (lottieAllowed_ != lottieAllowed)
        {
            lottieAllowed_ = lottieAllowed;
            stickersMetaEvent();
        }
    }

    void SmilesMenu::hideStickerPreview()
    {
        if (!stickerPreview_)
            return;

        stickerPreview_->hide();
        delete stickerPreview_;
        stickerPreview_ = nullptr;
    }

    void Smiles::SmilesMenu::updateTheme()
    {
        Utils::ApplyStyle(this, Styling::getParameters().getSmilesQss());

        recentsButton_->setPixmap(Utils::renderSvgScaled(qsl(":/smiles_menu/recent"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));

        for (const auto btn : topToolbar_->GetButtons())
        {
            btn->setHoveredBackgroundColor(toolbarButtonHoveredBackground());
            btn->setCheckedBackgroundColor(toolbarButtonCheckedBackground());
            btn->setCheckedBorderColor(toolbarButtonCheckedBorder());
        }

        emojiView_->updateTheme();
    }

    void SmilesMenu::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (!viewArea_ || !emojiView_)
            return;

        const auto& viewPortRect = viewArea_->viewport()->geometry();
        const auto& emojiRect = emojiView_->geometry();

        emojiView_->onViewportChanged(QRect(0, viewArea_->verticalScrollBar()->value() - emojiRect.top(), viewPortRect.width(), viewPortRect.height() + 1),
            blockToolbarSwitch_);
    }

    void SmilesMenu::keyPressEvent(QKeyEvent* _event)
    {
        _event->ignore();

        const auto selectNextTable = [this]()
        {
            clearSelection();

            if (topToolbar_->isLastSelected())
                topToolbar_->selectFirst();
            else
                topToolbar_->selectNext();
            topToolbar_->selectedButton()->click();

            selectFirst();
        };

        const auto selectPrevTable = [this]()
        {
            clearSelection();

            if (topToolbar_->isFirstSelected())
                topToolbar_->selectLast();
            else
                topToolbar_->selectPrevious();
            topToolbar_->selectedButton()->click();

            selectFirst();
        };

        if (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down || _event->key() == Qt::Key_Left || _event->key() == Qt::Key_Right)
        {
            if (auto smTable = getCurrentMenuTable(); smTable && !smTable->hasSelection())
                return;

            switch (_event->key())
            {
            case Qt::Key_Up:
                selectItemUp();
                break;

            case Qt::Key_Down:
                selectItemDown();
                break;

            case Qt::Key_Left:
                selectItemLeft();
                break;

            case Qt::Key_Right:
                selectItemRight();
                break;

            default:
                break;
            }

            ensureSelectedVisible();

            _event->accept();
        }
        else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
        {
            if (iskeyboardActive() && sendSelectedItem())
            {
                hideAnimated();
                _event->accept();
            }
        }
        else if (_event->key() == Qt::Key_Tab || _event->key() == Qt::Key_Backtab)
        {
            if (_event->key() == Qt::Key_Tab)
                selectNextTable();
            else
                selectPrevTable();

            _event->accept();
        }
    }

    bool SmilesMenu::focusNextPrevChild(bool)
    {
        return false;
    }

    void SmilesMenu::setCurrentHeight(const int _val)
    {
        setFixedHeight(_val);
        currentHeight_ = _val;
        setVisible(currentHeight_ > 0);

        updateStickersVisibility();
    }

    int SmilesMenu::getCurrentHeight() const
    {
        return currentHeight_;
    }

    void SmilesMenu::onScroll(int _value)
    {
        Q_EMIT scrolled();

        updateStickersVisibility();
    }
}
