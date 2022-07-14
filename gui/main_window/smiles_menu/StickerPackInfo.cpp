#include "stdafx.h"
#include "StickerPackInfo.h"
#include "../../controls/GeneralDialog.h"

#include "../../controls/TextEditEx.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/DialogButton.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../ReportWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "SmilesMenu.h"
#include "StickerPreview.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/CustomButton.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/RecentsTab.h"
#include "../contact_list/ContactListModel.h"
#include "../GroupChatOperations.h"
#include "../../styles/ThemeParameters.h"
#include "../../url_config.h"

using namespace Ui;

namespace
{

    int32_t getStickerItemSize()
    {
        return Utils::scale_value(80);
    }

    int32_t getStickerSize()
    {
        return Utils::scale_value(72);
    }

    int32_t getTextMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t getButtonMargin()
    {
        return Utils::scale_value(16);
    }

    QSize getMoreButtonSizeScaled()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    int32_t getMoreButtonMargin()
    {
        return Utils::scale_value(12);
    }

    QString getStickerpackUrl(const QString& _storeId)
    {
        return u"https://" % Ui::getUrlConfig().getUrlStickerShare() % u'/' % _storeId;
    }

    QMap<QString, QVariant> makeData(const QString& _command)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        return result;
    }

    QSize getCatSize()
    {
        return { 124, 152 };
    }
}

StickersView::StickersView(QWidget* _parent, Smiles::StickersTable* _stickers)
    : QWidget(_parent)
    , stickers_(_stickers)
    , previewActive_(false)
{
    setMouseTracking(true);

    setCursor(Qt::PointingHandCursor);

    connect(stickers_, &Smiles::StickersTable::stickerPreview, this, &StickersView::onStickerPreview);
    connect(stickers_, &Smiles::StickersTable::stickerSelected, this, &StickersView::onStickerPreview);

    connect(stickers_, &Smiles::StickersTable::stickerHovered, this, &StickersView::stickerHovered);
}

void StickersView::onStickerPreview(const Utils::FileSharingId& _stickerId)
{
    previewActive_ = true;

    Q_EMIT stickerPreview(-1, _stickerId);
}

void StickersView::mouseReleaseEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::MouseButton::LeftButton)
    {
        if (previewActive_)
        {
            stickersPreviewClosed();

            Q_EMIT stickerPreviewClose();
        }
        else
        {
            const auto rc = stickers_->geometry();

            stickers_->mouseReleaseEventInternal(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
        }
    }

    QWidget::mouseReleaseEvent(_e);
}


void StickersView::mousePressEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::MouseButton::LeftButton)
    {
        const auto rc = stickers_->geometry();

        stickers_->mousePressEventInternal(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
    }

    QWidget::mousePressEvent(_e);
}

void StickersView::mouseMoveEvent(QMouseEvent* _e)
{
    const auto rc = stickers_->geometry();

    stickers_->mouseMoveEventInternal(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));

    QWidget::mouseMoveEvent(_e);
}

void StickersView::leaveEvent(QEvent* _e)
{
    stickers_->leaveEventInternal();

    QWidget::leaveEvent(_e);
}

void StickersView::stickersPreviewClosed()
{
    previewActive_ = false;
}



//////////////////////////////////////////////////////////////////////////
// PackErrorWidget
//////////////////////////////////////////////////////////////////////////
PackErrorWidget::PackErrorWidget(QWidget* _parent)
    : QWidget(_parent)
{

}

PackErrorWidget::~PackErrorWidget()
{

}


void PackErrorWidget::setErrorText(const QString& _text)
{
    errorText_ = _text;
}

void PackErrorWidget::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

    static auto catPixmap = Utils::renderSvgScaled(qsl(":/smiles_menu/sad_cat_sticker"), getCatSize());

    const auto catSize = Utils::scale_value(getCatSize());

    QRect rcCat(0, 0, catSize.width(), catSize.height());

    rcCat.moveCenter(rect().center());

    p.drawPixmap(rcCat, catPixmap);

    p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    p.setFont(Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold));

    const int textMargin = Utils::scale_value(24);

    QRect rcText(rect().left() + textMargin, rcCat.bottom() + textMargin, rect().width() - 2 * textMargin, Utils::scale_value(100));

    p.drawText(rcText, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, errorText_);
}




//////////////////////////////////////////////////////////////////////////
// PackWidget
//////////////////////////////////////////////////////////////////////////
PackWidget::PackWidget(Stickers::StatContext _context, QWidget* _parent)
    : QWidget(_parent)
    , viewArea_(nullptr)
    , stickers_(nullptr)
    , stickersView_(nullptr)
    , errorWidget_(nullptr)
    , addButton_(nullptr)
    , removeButton_(nullptr)
    , setId_(0)
    , moreButton_(nullptr)
    , loadingText_(new QLabel(this))
    , descrControl_(nullptr)
    , dialog_(nullptr)
    , context_(_context)
{
    Utils::ApplyStyle(this, Styling::getParameters().getStickersQss());

    setFixedSize(Utils::scale_value(QSize(360, 400)));

    rootVerticalLayout_ = Utils::emptyVLayout(this);

    loadingText_->setObjectName(qsl("loading_state"));
    loadingText_->setAlignment(Qt::AlignCenter);
    loadingText_->setText(QT_TRANSLATE_NOOP("stickers", "Loading..."));
    loadingText_->setWordWrap(true);

    rootVerticalLayout_->addWidget(loadingText_);
}

PackWidget::~PackWidget() = default;

void PackWidget::setError(const QString& _text)
{
    loadingText_->hide();

    if (!errorWidget_)
    {
        errorWidget_ = new PackErrorWidget(this);
        rootVerticalLayout_->addWidget(errorWidget_);
    }
    errorWidget_->setErrorText(_text);
    errorWidget_->show();

    dialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "Close"), true);
}

void PackWidget::setParentDialog(GeneralDialog* _dialog)
{
    dialog_ = _dialog;
}

void PackWidget::onStickersPackInfo(std::shared_ptr<Ui::Stickers::Set> _set, const bool _result, const bool _purchased, bool _fromOtherFederation)
{
    if (!_result || !_set)
        return;

    setId_ = _set->getId();
    storeId_ = _set->getStoreId();
    description_ = _set->getDescription();
    purchased_ = _purchased;
    name_ = _set->getName();

    loadingText_->setVisible(false);

    if (!descrControl_)
    {
        QHBoxLayout* textLayout = new QHBoxLayout(nullptr);
        textLayout->setContentsMargins(getTextMargin(), 0, getButtonMargin(), 0);

        int textWidth = width() - getTextMargin() - getButtonMargin();

        descrControl_ = new TextEditEx(this, Fonts::appFontScaled(12), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY }, false, false);
        descrControl_->setWordWrapMode(QTextOption::WordWrap);
        descrControl_->setObjectName(qsl("description"));
        descrControl_->adjustHeight(textWidth);
        descrControl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        descrControl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        descrControl_->setFrameStyle(QFrame::NoFrame);
        descrControl_->setStyleSheet(qsl("QWidget { background-color:transparent; border: none; }"));

        Testing::setAccessibleName(descrControl_, qsl("AS StickerPack subtitle"));
        textLayout->addWidget(descrControl_);

        rootVerticalLayout_->addLayout(textLayout);
    }

    if (!description_.isEmpty())
        descrControl_->setPlainText(description_, false);

    if (!viewArea_)
    {
        viewArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        viewArea_->setStyleSheet(qsl("background: transparent; border: none;"));
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &PackWidget::touchScrollStateChanged, Qt::QueuedConnection);

        stickers_ = new Smiles::StickersTable(this, setId_, getStickerSize(), getStickerItemSize());
        connect(viewArea_->verticalScrollBar(), &QScrollBar::valueChanged, stickers_, &Smiles::StickersTable::onVisibilityChanged);
        connect(viewArea_->verticalScrollBar(), &QScrollBar::rangeChanged, stickers_, &Smiles::StickersTable::onVisibilityChanged);
        connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, stickers_, &Smiles::StickersTable::onStickerUpdated);

        stickersView_ = new StickersView(this, stickers_);
        stickersView_->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(stickersView_);
        viewArea_->setWidgetResizable(true);

        rootVerticalLayout_->addWidget(viewArea_);

        stickersLayout_ = Utils::emptyVLayout(nullptr);
        stickersLayout_->setContentsMargins(Utils::scale_value(20), 0, 0, 0);
        stickersLayout_->setAlignment(Qt::AlignTop);

        stickersView_->setLayout(stickersLayout_);

        stickersLayout_->addWidget(stickers_);

        connect(stickersView_, &StickersView::stickerPreviewClose, this, &PackWidget::onStickerPreviewClose);
        connect(stickersView_, &StickersView::stickerPreview, this, &PackWidget::onStickerPreview);
        connect(stickersView_, &StickersView::stickerHovered, this, &PackWidget::onStickerHovered);
    }
    stickers_->onStickerAdded();

    // init buttons
    if (!addButton_ || !removeButton_)
    {
        QHBoxLayout* buttonsLayout = new QHBoxLayout(nullptr);

        buttonsLayout->setContentsMargins(getButtonMargin(), getButtonMargin(), getButtonMargin(), getButtonMargin());
        buttonsLayout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        {
            auto closeButton = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "Close"), DialogButtonRole::CANCEL);

            closeButton->setCursor(Qt::PointingHandCursor);
            closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            closeButton->adjustSize();

            QObject::connect(closeButton, &QPushButton::clicked, this, &PackWidget::buttonClicked);
            Testing::setAccessibleName(closeButton, qsl("AS StickerPack closeButton"));

            buttonsLayout->addWidget(closeButton);
        }

        buttonsLayout->addSpacerItem(new QSpacerItem(getButtonMargin(), 0));

        addButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "Add"), DialogButtonRole::CONFIRM);

        addButton_->setCursor(QCursor(Qt::PointingHandCursor));
        addButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        addButton_->adjustSize();
        Testing::setAccessibleName(addButton_, qsl("AS StickerPack addButton"));
        buttonsLayout->addWidget(addButton_);

        QObject::connect(addButton_, &QPushButton::clicked, this, &PackWidget::onAddButton);

        removeButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "Remove"), DialogButtonRole::CONFIRM_DELETE);

        removeButton_->setCursor(QCursor(Qt::PointingHandCursor));
        removeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        removeButton_->adjustSize();

        QObject::connect(removeButton_, &QPushButton::clicked, this, &PackWidget::onRemoveButton);

        Testing::setAccessibleName(removeButton_, qsl("AS StickerPack removeButton"));
        buttonsLayout->addWidget(removeButton_);

        rootVerticalLayout_->addLayout(buttonsLayout);
    }

    addButton_->setVisible(!_purchased && !_fromOtherFederation);
    removeButton_->setVisible(_purchased && !_fromOtherFederation);

    if (!moreButton_)
    {
        moreButton_ = new CustomButton(parentWidget(), qsl(":/controls/more_icon"), Utils::unscale_value(getMoreButtonSizeScaled()), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });

        moreButton_->setFixedSize(getMoreButtonSizeScaled());
        moreButton_->show();
        moreButton_->raise();
        Testing::setAccessibleName(moreButton_, qsl("AS StickerPack moreButton"));

        moveMoreButton();

        static ContextMenu menu(nullptr);
        if (menu.actions().isEmpty())
        {
            menu.addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("popup_window", "Forward"), makeData(qsl("forward")));
            menu.addSeparator();
            menu.addActionWithIcon(qsl(":/context_menu/spam"), QT_TRANSLATE_NOOP("popup_window", "Report"), makeData(qsl("report")));
        }

        connect(&menu, &ContextMenu::triggered, this, &PackWidget::contextMenuAction, Qt::QueuedConnection);

        connect(moreButton_, &QPushButton::clicked, this, []() { menu.popup(QCursor::pos()); });
    }
    if (_fromOtherFederation)
        moreButton_->hide();
}

void PackWidget::touchScrollStateChanged(QScroller::State _state)
{
    stickers_->blockSignals(_state != QScroller::Inactive);
}

void PackWidget::onDialogAccepted()
{
    if (purchased_)
        onRemoveButton(true);
    else
        onAddButton(true);
}

void PackWidget::onAddButton(bool _checked)
{
    GetDispatcher()->addStickersPack(setId_, storeId_);

    sendAddPackStats();

    Q_EMIT buttonClicked();
}

void PackWidget::onRemoveButton(bool _checked)
{
    GetDispatcher()->removeStickersPack(setId_, storeId_);

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_pack_delete);

    Q_EMIT buttonClicked();
}

void PackWidget::onStickerPreviewClose()
{
    if (!stickerPreview_)
        return;

    stickerPreview_->hide();
    stickerPreview_.reset();
}

void PackWidget::onStickerPreview(const int32_t _setId, const Utils::FileSharingId& _stickerId)
{
    if (!stickerPreview_)
    {
        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        if (!mainWindow)
            return;

        stickerPreview_ = std::make_unique<Smiles::StickerPreview>(mainWindow, _setId, _stickerId, Smiles::StickerPreview::Context::Popup);

        auto previewRect = dialog_->getMainHost()->rect();
        previewRect.moveTo(dialog_->getMainHost()->mapTo(mainWindow, previewRect.topLeft()));

        stickerPreview_->setGeometry(previewRect);
        stickerPreview_->show();
        stickerPreview_->raise();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_longtap_preview);

        connect(stickerPreview_.get(), &Smiles::StickerPreview::needClose, this, [this]()
        {
            stickersView_->stickersPreviewClosed();

            onStickerPreviewClose();
        }, Qt::QueuedConnection); // !!! must be Qt::QueuedConnection

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &PackWidget::onWindowResized);
    }
}

void PackWidget::onStickerHovered(const int32_t _setId, const Utils::FileSharingId& _stickerId)
{
    if (stickerPreview_)
        stickerPreview_->showSticker(_stickerId);
}

void PackWidget::contextMenuAction(QAction* _action)
{
    const auto params = _action->data().toMap();
    const auto command = params[qsl("command")].toString();
    if (command == u"forward")
        Q_EMIT forward();
    else if (command == u"report")
        Q_EMIT report();
}

void PackWidget::onWindowResized()
{
    if (stickerPreview_)
    {
        if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
        {
            auto previewRect = dialog_->getMainHost()->rect();
            previewRect.moveTo(dialog_->getMainHost()->mapTo(mainWindow, previewRect.topLeft()));
            stickerPreview_->setGeometry(previewRect);
        }
    }
}

void PackWidget::paintEvent(QPaintEvent* _e)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    return QWidget::paintEvent(_e);
}

void PackWidget::moveMoreButton()
{
    if (moreButton_)
    {
        QRect rc(
            QPoint(
                rect().right() - getMoreButtonMargin() - getMoreButtonSizeScaled().width(),
                getMoreButtonMargin()
            ),
            getMoreButtonSizeScaled()
        );

        moreButton_->setGeometry(rc);
    }
}

void PackWidget::sendAddPackStats()
{
    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_download_pack, {{"Place", Stickers::getStatContextStr(context_)}});

    if (context_ == Stickers::StatContext::Chat)
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_addpack_button_tap_in_chat);
    else if (context_ == Stickers::StatContext::Discover)
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_pack_download);
    else if (context_ == Stickers::StatContext::Search)
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_search_pack_download);
}

void PackWidget::resizeEvent(QResizeEvent* _e)
{
    moveMoreButton();

    QWidget::resizeEvent(_e);
}




//////////////////////////////////////////////////////////////////////////
// StickerPackInfo
//////////////////////////////////////////////////////////////////////////
StickerPackInfo::StickerPackInfo(QWidget* _parent, const int32_t _set_id, const QString& _store_id, const Utils::FileSharingId& _file_id, Stickers::StatContext _context)
    : QWidget(_parent)
    , setId_(_set_id)
    , storeId_(_store_id)
    , fileSharingId_(_file_id)
    , context_(_context)
{
    Utils::ApplyStyle(this, Styling::getParameters().getStickersQss());

    pack_ = new PackWidget(_context, this);
    parentDialog_ = std::make_unique<GeneralDialog>(pack_, Utils::InterConnector::instance().getMainWindow());
    pack_->setParentDialog(parentDialog_.get());

    connect(pack_, &PackWidget::buttonClicked, parentDialog_.get(), &GeneralDialog::close);
    connect(pack_, &PackWidget::forward, this, &StickerPackInfo::onShareClicked);
    connect(pack_, &PackWidget::report, this, &StickerPackInfo::onReportClicked);
    connect(parentDialog_.get(), &GeneralDialog::accepted, pack_, &PackWidget::onDialogAccepted);


    Stickers::setSptr stickerSet;
    if (setId_ != -1)
    {
        stickerSet = Stickers::getSet(setId_);
        if (!stickerSet)
            stickerSet = Stickers::getStoreSet(setId_);
    }
    else if (!fileSharingId_.fileId_.isEmpty())
    {
        stickerSet = Stickers::findSetByFsId(fileSharingId_);
    }

    if (stickerSet)
    {
        pack_->onStickersPackInfo(stickerSet, true, stickerSet->isPurchased(), fileSharingId_.sourceId_.has_value());
        if (const auto& name = stickerSet->getName(); !name.isEmpty())
            parentDialog_->addLabel(name);
    }

    Ui::GetDispatcher()->getStickersPackInfo(setId_, _store_id, _file_id);
    connect(Ui::GetDispatcher(), &core_dispatcher::onStickerpackInfo, this, &StickerPackInfo::onStickerpackInfo);
}

StickerPackInfo::~StickerPackInfo() = default;

void StickerPackInfo::onShareClicked()
{
    parentDialog_->hide();

    Q_EMIT Utils::InterConnector::instance().searchEnd();

    QString sourceText = getStickerpackUrl(storeId_);
    forwardMessage(sourceText, QT_TRANSLATE_NOOP("stickers", "Share"), QT_TRANSLATE_NOOP("popup_window", "Send"), false);

    parentDialog_->close();
}

void StickerPackInfo::onReportClicked()
{
    auto h = parentDialog_->height();
    parentDialog_->setFixedHeight(0);
    if (ReportStickerPack(setId_, QT_TRANSLATE_NOOP("stickers", "Stickerpack: ") + pack_->getName()))
        parentDialog_->close();
    else
        parentDialog_->setFixedHeight(h);
}

void StickerPackInfo::show()
{
    parentDialog_->execute();
}

void StickerPackInfo::onStickerpackInfo(const bool _result, const bool _exist, std::shared_ptr<Ui::Stickers::Set> _set)
{
    if (!_result)
    {
        if (!_exist)
            pack_->setError(QT_TRANSLATE_NOOP("stickers", "This sticker pack was deleted or no longer exists."));

        return;
    }

    if (!_set)
        return;

    if (setId_ != -1 && _set->getId() != setId_)
        return;

    if (!storeId_.isEmpty() && storeId_ != _set->getStoreId())
        return;

    setId_ = _set->getId();
    storeId_ = _set->getStoreId();

    pack_->onStickersPackInfo(_set, _result, _set->isPurchased(), fileSharingId_.sourceId_.has_value());

    if (_result)
        parentDialog_->addLabel(_set->getName());
}

void StickerPackInfo::paintEvent(QPaintEvent* _e)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    return QWidget::paintEvent(_e);
}
