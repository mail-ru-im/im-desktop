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
#include "../../cache/stickers/stickers.h"
#include "../../controls/CustomButton.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"
#include "../GroupChatOperations.h"
#include "../../styles/ThemeParameters.h"
#include "../../app_config.h"

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
        static const QString cicq_org = ql1s("https://") % QString::fromStdString(Ui::GetAppConfig().getUrlCICQOrg()) % ql1s("/s/");

        return cicq_org % _storeId;
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

void StickersView::onStickerPreview(const int32_t _setId, const QString& _stickerId)
{
    previewActive_ = true;

    emit stickerPreview(_setId, _stickerId);
}

void StickersView::mouseReleaseEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::MouseButton::LeftButton)
    {
        if (previewActive_)
        {
            stickersPreviewClosed();

            emit stickerPreviewClose();
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
    , subtitleControl_(nullptr)
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

    errorWidget_ = new PackErrorWidget(this);
    errorWidget_->setVisible(false);

    rootVerticalLayout_->addWidget(errorWidget_);

    setLayout(rootVerticalLayout_);
}

PackWidget::~PackWidget() = default;

void PackWidget::setError(const QString& _text)
{
    loadingText_->setVisible(false);

    errorWidget_->setErrorText(_text);
    errorWidget_->setVisible(true);

    dialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "CLOSE"), true);

}

void PackWidget::onStickerEvent(const qint32 _error, const qint32 _setId, const QString& _stickerId)
{
    if (stickers_)
    {
        stickers_->onStickerUpdated(_setId, _stickerId);
    }
}

void PackWidget::setParentDialog(GeneralDialog* _dialog)
{
    dialog_ = _dialog;
}

void PackWidget::onStickersPackInfo(std::shared_ptr<Ui::Stickers::Set> _set, const bool _result, const bool _purchased)
{
    if (viewArea_)
        return;

    if (_result)
    {
        loadingText_->setVisible(false);

        setId_ = _set->getId();
        storeId_ = _set->getStoreId();
        subtitle_ = _set->getSubtitle();
        purchased_ = _purchased;
        name_ = _set->getName();

        if (!subtitle_.isEmpty())
        {
            QHBoxLayout* textLayout = new QHBoxLayout(nullptr);
            textLayout->setContentsMargins(getTextMargin(), 0, getButtonMargin(), 0);

            int textWidth = width() - getTextMargin() - getButtonMargin();

            subtitleControl_ = new TextEditEx(this, Fonts::appFontScaled(12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), false, false);
            subtitleControl_->setWordWrapMode(QTextOption::WordWrap);
            subtitleControl_->setPlainText(subtitle_, false);
            subtitleControl_->setObjectName(qsl("description"));
            subtitleControl_->adjustHeight(textWidth);
            subtitleControl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            subtitleControl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            subtitleControl_->setFrameStyle(QFrame::NoFrame);
            Utils::ApplyStyle(subtitleControl_, qsl("QWidget { background-color:transparent; border: none; }"));

            Testing::setAccessibleName(subtitleControl_, qsl("AS subtitleControl_"));
            textLayout->addWidget(subtitleControl_);

            rootVerticalLayout_->addLayout(textLayout);

        }

        viewArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        viewArea_->setStyleSheet(qsl("background: transparent; border: none;"));
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &PackWidget::touchScrollStateChanged, Qt::QueuedConnection);

        stickers_ = new Smiles::StickersTable(this, setId_, getStickerSize(), getStickerItemSize());

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

        // init buttons
        QHBoxLayout* buttonsLayout = new QHBoxLayout(nullptr);

        buttonsLayout->setContentsMargins(getButtonMargin(), getButtonMargin(), getButtonMargin(), getButtonMargin());
        buttonsLayout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        {
            auto closeButton = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "CLOSE"), DialogButtonRole::CANCEL);

            closeButton->setCursor(Qt::PointingHandCursor);
            closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            closeButton->adjustSize();

            QObject::connect(closeButton, &QPushButton::clicked, this, &PackWidget::buttonClicked);

            buttonsLayout->addWidget(closeButton);
        }

        buttonsLayout->addSpacerItem(new QSpacerItem(getButtonMargin(), 0));

        if (!_purchased)
        {
            addButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "ADD"), DialogButtonRole::CONFIRM);

            addButton_->setCursor(QCursor(Qt::PointingHandCursor));
            addButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            addButton_->adjustSize();
            Testing::setAccessibleName(addButton_, qsl("AS addButton_"));
            buttonsLayout->addWidget(addButton_);

            QObject::connect(addButton_, &QPushButton::clicked, this, &PackWidget::onAddButton);
        }
        else
        {
            removeButton_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "REMOVE"), DialogButtonRole::CONFIRM_DELETE);

            removeButton_->setCursor(QCursor(Qt::PointingHandCursor));
            removeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            removeButton_->adjustSize();

            QObject::connect(removeButton_, &QPushButton::clicked, this, &PackWidget::onRemoveButton);

            Testing::setAccessibleName(removeButton_, qsl("AS removeButton_"));
            buttonsLayout->addWidget(removeButton_);

        }
        rootVerticalLayout_->addLayout(buttonsLayout);

        moreButton_ = new CustomButton(parentWidget(), qsl(":/controls/more_icon"), Utils::unscale_value(getMoreButtonSizeScaled()), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        moreButton_->setFixedSize(getMoreButtonSizeScaled());
        moreButton_->show();
        moreButton_->raise();

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

        QObject::connect(stickersView_, &StickersView::stickerPreviewClose, this, &PackWidget::onStickerPreviewClose);
        QObject::connect(stickersView_, &StickersView::stickerPreview, this, &PackWidget::onStickerPreview);
        QObject::connect(stickersView_, &StickersView::stickerHovered, this, &PackWidget::onStickerHovered);
    }
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

    emit buttonClicked();
}

void PackWidget::onRemoveButton(bool _checked)
{
    GetDispatcher()->removeStickersPack(setId_, storeId_);

    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_pack_delete);

    emit buttonClicked();
}

void PackWidget::onStickerPreviewClose()
{
    if (!stickerPreview_)
        return;

    stickerPreview_->hide();
    stickerPreview_.reset();
}

void PackWidget::onStickerPreview(const int32_t _setId, const QString& _stickerId)
{
    if (!stickerPreview_)
    {
        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        stickerPreview_ = std::make_unique<Smiles::StickerPreview>(mainWindow, _setId, _stickerId, Smiles::StickerPreview::Context::Popup);

        auto previewRect = dialog_->getMainHost()->rect();
        previewRect.moveTo(dialog_->getMainHost()->mapTo(mainWindow, previewRect.topLeft()));

        stickerPreview_->setGeometry(previewRect);
        stickerPreview_->show();
        stickerPreview_->raise();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_longtap_preview);

        QObject::connect(stickerPreview_.get(), &Smiles::StickerPreview::needClose, this, [this]()
        {
            stickersView_->stickersPreviewClosed();

            onStickerPreviewClose();
        }, Qt::QueuedConnection); // !!! must be Qt::QueuedConnection
    }
}

void PackWidget::onStickerHovered(const int32_t _setId, const QString& _stickerId)
{
    if (stickerPreview_)
    {
        stickerPreview_->showSticker(_setId, _stickerId);
    }
}

void PackWidget::contextMenuAction(QAction* _action)
{
    const auto params = _action->data().toMap();
    const auto command = params[qsl("command")].toString();
    if (command == ql1s("forward"))
        emit forward();
    else if (command == ql1s("report"))
        emit report();
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
StickerPackInfo::StickerPackInfo(QWidget* _parent, const int32_t _set_id, const QString& _store_id, const QString& _file_id, Stickers::StatContext _context)
    : QWidget(_parent)
    , set_id_(_set_id)
    , store_id_(_store_id)
    , file_id_(_file_id)
    , context_(_context)
{
    setStyleSheet(Styling::getParameters().getStickersQss());
    pack_ = new PackWidget(_context, this);


    parentDialog_ = std::make_unique<GeneralDialog>(pack_, Utils::InterConnector::instance().getMainWindow());

    pack_->setParentDialog(parentDialog_.get());

    Ui::GetDispatcher()->getStickersPackInfo(set_id_, _store_id, _file_id);

    connect(Ui::GetDispatcher(), &core_dispatcher::onStickerpackInfo, this, &StickerPackInfo::onStickerpackInfo);
    connect(Ui::GetDispatcher(), &core_dispatcher::onSticker, this, [this](qint32 _error, qint32 _setId, qint32, const QString& _fsId) { stickerEvent(_error, _setId, _fsId); });

    connect(pack_, &PackWidget::buttonClicked, parentDialog_.get(), &GeneralDialog::close);

    connect(pack_, &PackWidget::forward, this, &StickerPackInfo::onShareClicked);
    connect(pack_, &PackWidget::report, this, &StickerPackInfo::onReportClicked);
    connect(parentDialog_.get(), &GeneralDialog::accepted, pack_, &PackWidget::onDialogAccepted);
}

StickerPackInfo::~StickerPackInfo()
{
}

void StickerPackInfo::onShareClicked()
{
    parentDialog_->hide();

    emit Utils::InterConnector::instance().searchEnd();

    QString sourceText = getStickerpackUrl(store_id_);
    forwardMessage(sourceText, QT_TRANSLATE_NOOP("stickers", "Share"), QT_TRANSLATE_NOOP("popup_window", "SEND"), false);

    parentDialog_->close();
}

void StickerPackInfo::onReportClicked()
{
    auto h = parentDialog_->height();
    parentDialog_->setFixedHeight(0);
    if (ReportStickerPack(set_id_, QT_TRANSLATE_NOOP("stickers", "Stickerpack: ") + pack_->getName()))
        parentDialog_->close();
    else
        parentDialog_->setFixedHeight(h);
}

void StickerPackInfo::stickerEvent(const qint32 _error, const qint32 _setId, const QString& _stickerId)
{
    pack_->onStickerEvent(_error, _setId, _stickerId);
}


void StickerPackInfo::show()
{
    parentDialog_->showInCenter();
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

    if (set_id_ != -1 && _set->getId() != set_id_)
        return;

    if (!store_id_.isEmpty() && store_id_ != _set->getStoreId())
        return;

    set_id_ = _set->getId();
    store_id_ = _set->getStoreId();

    pack_->onStickersPackInfo(_set, _result, _set->isPurchased());

    if (_result)
    {
        parentDialog_->addLabel(_set->getName());
    }
}

void StickerPackInfo::paintEvent(QPaintEvent* _e)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    return QWidget::paintEvent(_e);
}
