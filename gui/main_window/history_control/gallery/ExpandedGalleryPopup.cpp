#include "stdafx.h"

#include "ExpandedGalleryPopup.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"
#include "../../../utils/InterConnector.h"
#include "../../../controls/TransparentScrollBar.h"

#include "../../../qml/models/RootModel.h"
#include "../../../qml/helpers/QmlEngine.h"
#include "../../../controls/TextUnit.h"
#include "../../../fonts.h"
#include "../../contact_list/Common.h"

namespace
{
    auto textOffset() noexcept
    {
        return Utils::scale_value(16);
    }

    auto itemHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    auto verticalSpacing() noexcept
    {
        return Utils::scale_value(4);
    }

    auto popupRadius() noexcept
    {
        return Utils::scale_value(8);
    }

    auto borderWidth() noexcept
    {
        return Utils::scale_value(1);
    }

    auto tabsModel()
    {
        auto rootModel = Utils::InterConnector::instance().qmlRootModel();
        return rootModel->gallery()->galleryTabsModel();
    }

    auto popupItemFont(bool _bold)
    {
        return Fonts::appFontScaled(15, _bold ? Fonts::FontWeight::Bold : Fonts::FontWeight::Normal);
    }

    class GalleryPopupDelegate : public QItemDelegate
    {
    public:
        GalleryPopupDelegate(QWidget* _parent);

    protected:
        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

    private:
        bool isFirstItem(const QModelIndex& _index) const;
        bool isLastItem(const QModelIndex& _index) const;
        Ui::TextRendering::TextUnitPtr createTextUnit(bool _bold);

    private:
        Ui::TextRendering::TextUnitPtr selectedName_;
        Ui::TextRendering::TextUnitPtr name_;
    };

    GalleryPopupDelegate::GalleryPopupDelegate(QWidget* _parent)
        : QItemDelegate(_parent)
        , selectedName_ { createTextUnit(true) }
        , name_ { createTextUnit(false) }
    {
    }

    void GalleryPopupDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Utils::PainterSaver ps(*_painter);
        _painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        const auto topAdjust = isFirstItem(_index) ? verticalSpacing() : 0;
        const auto botAdjust = isLastItem(_index) ? -verticalSpacing() : 0;
        const auto itemRect = _option.rect.adjusted(0, topAdjust, 0, botAdjust);
        if (_option.state & QStyle::State_MouseOver)
            Ui::RenderMouseState(*_painter, true, false, itemRect);

        const auto needBold = _index.data(Qml::GalleryTabsModel::TabRoles::CheckedState).toBool();
        auto& unit = needBold ? selectedName_ : name_;
        unit->setText(_index.data(Qml::GalleryTabsModel::TabRoles::Name).toString());

        _painter->translate(itemRect.topLeft());
        unit->draw(*_painter, Ui::TextRendering::VerPosition::MIDDLE);
    }

    QSize GalleryPopupDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto height = itemHeight();
        if (isFirstItem(_index))
            height += verticalSpacing();
        if (isLastItem(_index))
            height += verticalSpacing();
        return { _option.rect.width(), height };
    }

    bool GalleryPopupDelegate::isFirstItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == 0;
    }

    bool GalleryPopupDelegate::isLastItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == _index.model()->rowCount() - 1;
    }

    Ui::TextRendering::TextUnitPtr GalleryPopupDelegate::createTextUnit(bool _bold)
    {
        auto text = Ui::TextRendering::MakeTextUnit(QString());
        Ui::TextRendering::TextUnit::InitializeParameters params{ popupItemFont(_bold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.maxLinesCount_ = 1;
        text->init(params);
        text->evaluateDesiredSize();
        text->setOffsets(textOffset(), itemHeight() / 2);
        return text;
    }
} // namespace

namespace Ui
{
    ExpandedGalleryPopup::ExpandedGalleryPopup(QWidget* _parent)
        : QWidget(_parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , model_{ tabsModel() }
        , listView_{ CreateFocusableViewAndSetTrScrollBar(this) }
    {
        setAttribute(Qt::WA_TranslucentBackground);

        if constexpr (platform::is_apple())
        {
            setCursor(Qt::PointingHandCursor);
            QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, this, &ExpandedGalleryPopup::hide);
        }

        connect(model_, &Qml::GalleryTabsModel::countChanged, this, &ExpandedGalleryPopup::updateSize);

        listView_->setParent(this);
        listView_->setSelectByMouseHover(true);
        listView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        listView_->setFrameShape(QFrame::NoFrame);
        listView_->setSpacing(0);
        listView_->setModelColumn(0);
        listView_->setUniformItemSizes(false);
        listView_->setBatchSize(50);
        const auto style = ql1s("background: %3; border: %1px solid %4; border-radius: %2px;");
        listView_->setStyleSheet(style.arg(QString::number(borderWidth()), QString::number(popupRadius()),
                                      Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                                      Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT)));
        listView_->setMouseTracking(true);
        listView_->setAcceptDrops(false);
        listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        listView_->setAutoScroll(false);
        listView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        listView_->setResizeMode(QListView::Adjust);
        listView_->setAttribute(Qt::WA_MacShowFocusRect, false);
        listView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        listView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        listView_->viewport()->grabGesture(Qt::TapGesture);
        listView_->setListViewGestureHandler(new ListViewGestureHandler(this));
        Utils::grabTouchWidget(listView_->viewport(), true);
        auto styleContainer = new Styling::StyleSheetContainer(listView_->verticalScrollBar());
        styleContainer->setGenerator(Styling::getParameters().getScrollBarQss(4, 0));
        Testing::setAccessibleName(listView_, qsl("AS ExpandedGalleryPopup"));

        connect(listView_, &FocusableListView::clicked, this, &ExpandedGalleryPopup::onItemClick);

        listView_->setModel(model_);
        auto delegate = new GalleryPopupDelegate(this);
        listView_->setItemDelegate(delegate);

        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(listView_);

        updateSize();
    }

    ExpandedGalleryPopup::~ExpandedGalleryPopup()
    {
        if constexpr (platform::is_apple())
            QGuiApplication::restoreOverrideCursor();
    }

    void ExpandedGalleryPopup::hideEvent(QHideEvent* _event)
    {
        Q_EMIT hidden();
    }

    void ExpandedGalleryPopup::enterEvent(QEvent* _event)
    {
        if constexpr (platform::is_apple())
            QGuiApplication::changeOverrideCursor(cursor());
        QWidget::enterEvent(_event);
    }

    void ExpandedGalleryPopup::leaveEvent(QEvent* _event)
    {
        if constexpr (platform::is_apple())
            QGuiApplication::changeOverrideCursor(Qt::ArrowCursor);
        QWidget::leaveEvent(_event);
    }

    int ExpandedGalleryPopup::itemCount() const
    {
        return model_->rowCount();
    }

    int ExpandedGalleryPopup::evaluateWidth() const
    {
        auto model = tabsModel();
        auto width = 0;
        for (const auto tab : model->tabTypes())
        {
            const auto font = popupItemFont(model->selectedTab() == tab);
            const QFontMetrics fm(font);
            const int textWidht = fm.horizontalAdvance(getGalleryTitle(tab));
            width = std::max(width, textWidht);
        }
        width += 2 * textOffset() + 2 * borderWidth();
        return width;
    }

    int ExpandedGalleryPopup::evaluateHeight() const
    {
        return model_->rowCount() * itemHeight() + 2 * verticalSpacing() + 2 * borderWidth();
    }

    void ExpandedGalleryPopup::updateSize()
    {
        setFixedSize({ evaluateWidth(), evaluateHeight() });
    }

    void ExpandedGalleryPopup::onItemClick(const QModelIndex& _current)
    {
        const auto type = _current.data(Qml::GalleryTabsModel::TabRoles::ContentType).value<MediaContentType>();
        Q_EMIT itemClicked(type);
        hide();
    }

} // namespace Ui
