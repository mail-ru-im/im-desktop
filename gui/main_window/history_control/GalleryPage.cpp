#include "stdafx.h"

#include "GalleryPage.h"
#include "MessageStyle.h"
#include "main_window/sidebar/Sidebar.h"
#include "main_window/sidebar/GalleryList.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "main_window/contact_list/TopPanel.h"
#include "main_window/contact_list/ContactListModel.h"
#include "utils/utils.h"
#include "core_dispatcher.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr QStringView galleryTag() { return u"/gallery"; }
    constexpr QSize headerIconSize() noexcept { return { 24, 24 }; }

    constexpr Ui::MediaContentType supportedTypes[] =
    {
        Ui::MediaContentType::ImageVideo,
        Ui::MediaContentType::Video,
        Ui::MediaContentType::Files,
        Ui::MediaContentType::Links,
        Ui::MediaContentType::Voice
    };
}

namespace Ui
{
    GalleryPage::GalleryPage(const QString& _aimId, QWidget* _parent)
        : PageBase(_parent)
        , aimId_(createGalleryAimId(_aimId))
        , currentType_(MediaContentType::Invalid)
        , gallery_(new GalleryList(this, QString()))
        , galleryPopup_(nullptr)
    {
        gallery_->setMaxContentWidth(MessageStyle::getHistoryWidgetMaxWidth());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(gallery_);

        titleBar_ = new HeaderTitleBar(this);
        titleBar_->setButtonSize(Utils::scale_value(headerIconSize()));
        titleBar_->setArrowVisible(true);
        connect(titleBar_, &HeaderTitleBar::arrowClicked, this, &GalleryPage::onTitleArrowClicked);

        backButton_ = new HeaderTitleBarButton(this);
        backButton_->setDefaultImage(qsl(":/controls/back_icon"), {}, headerIconSize());
        backButton_->setCustomToolTip(QT_TRANSLATE_NOOP("sidebar", "Back"));
        Styling::Buttons::setButtonDefaultColors(backButton_);
        Testing::setAccessibleName(backButton_, qsl("AS GalleryPage backButton"));
        titleBar_->addButtonToLeft(backButton_);
        connect(backButton_, &HeaderTitleBarButton::clicked, this, &GalleryPage::closeGallery);

        Utils::setDefaultBackground(this);
        Utils::setDefaultBackground(titleBar_);

        if (Utils::isChat(getContact()))
            connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &GalleryPage::onRoleChanged);

        initState();
    }

    void GalleryPage::pageOpen()
    {
        const auto it = std::find_if(std::begin(supportedTypes), std::end(supportedTypes), [this](auto t) { return countForType(state_, t); });
        if (it != std::end(supportedTypes))
            setContentType(*it);
    }

    void GalleryPage::pageLeave()
    {
        gallery_->markClosed();
        GetDispatcher()->cancelGalleryHolesDownloading(getContact());
    }

    QWidget* GalleryPage::getTopWidget() const
    {
        return titleBar_;
    }

    QString GalleryPage::createGalleryAimId(const QString& _aimId)
    {
        return _aimId % galleryTag();
    }

    bool GalleryPage::isGalleryAimId(const QString& _aimId)
    {
        return _aimId.endsWith(galleryTag());
    }

    void GalleryPage::resizeEvent(QResizeEvent* _event)
    {
        updatePopupSizeAndPos();
    }

    void GalleryPage::setContentType(MediaContentType _type)
    {
        if (galleryPopup_)
            galleryPopup_->hide();

        titleBar_->setTitle(getGalleryTitle(_type));
        gallery_->openContentFor(getContact(), _type);
        currentType_ = _type;
    }

    void GalleryPage::onTitleArrowClicked()
    {
        initPopup();
        updatePopupCounters();
        updatePopupSizeAndPos();
        galleryPopup_->show();
    }

    void GalleryPage::onRoleChanged(const QString& _aimId)
    {
        if (_aimId != getContact())
            return;

        const auto role = Logic::getContactListModel()->getYourRole(getContact());
        if (role.isEmpty() || role == u"notamember" || role == u"pending")
            closeGallery();
    }

    void GalleryPage::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
    {
        if (_aimId != getContact())
            return;

        state_ = _state;
        updatePopupCounters();

        if (countForType(state_, currentType_) == 0)
            closeGallery();
    }

    void GalleryPage::closeGallery()
    {
        Q_EMIT switchToPrevDialog(backButton_->hasFocus());
    }

    void GalleryPage::initState()
    {
        connect(GetDispatcher(), &core_dispatcher::dialogGalleryState, this, &GalleryPage::dialogGalleryState);
        dialogGalleryState(getContact(), Logic::getContactListModel()->getGalleryState(getContact()));
    }

    void GalleryPage::initPopup()
    {
        if (galleryPopup_)
            return;

        galleryPopup_ = new GalleryPopup();
        connect(galleryPopup_, &GalleryPopup::galleryPhotoClicked, this, [this]() { setContentType(MediaContentType::ImageVideo); });
        connect(galleryPopup_, &GalleryPopup::galleryVideoClicked, this, [this]() { setContentType(MediaContentType::Video); });
        connect(galleryPopup_, &GalleryPopup::galleryFilesClicked, this, [this]() { setContentType(MediaContentType::Files); });
        connect(galleryPopup_, &GalleryPopup::galleryLinksClicked, this, [this]() { setContentType(MediaContentType::Links); });
        connect(galleryPopup_, &GalleryPopup::galleryPttClicked, this, [this]() { setContentType(MediaContentType::Voice); });
    }

    void GalleryPage::updatePopupCounters()
    {
        const auto typesWithContent = std::count_if(std::begin(supportedTypes), std::end(supportedTypes), [this](auto t)
        {
            return countForType(state_, t);
        });
        titleBar_->setArrowVisible(typesWithContent > 1);

        if (galleryPopup_)
            galleryPopup_->setCounters(state_);
    }

    void GalleryPage::updatePopupSizeAndPos()
    {
        if (!galleryPopup_)
            return;

        const auto maxWidth = std::min(width() - GalleryPopup::horOffset() * 2, Sidebar::getDefaultWidth());
        const auto popupWidth = std::max(0, maxWidth);
        galleryPopup_->setFixedWidth(popupWidth);

        const auto x = (width() - popupWidth) / 2;
        const auto y = 0;
        galleryPopup_->move(mapToGlobal({ x, y }));
    }

    QString GalleryPage::getContact() const
    {
        return aimId_.left(aimId_.indexOf(galleryTag()));
    }
}