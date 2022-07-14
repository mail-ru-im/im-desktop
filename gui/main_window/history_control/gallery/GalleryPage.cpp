#include "stdafx.h"

#include "GalleryPage.h"
#include "../MessageStyle.h"
#include "main_window/sidebar/Sidebar.h"
#include "main_window/sidebar/GalleryList.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "main_window/contact_list/TopPanel.h"
#include "main_window/contact_list/ContactListModel.h"
#include "utils/utils.h"
#include "core_dispatcher.h"
#include "styles/ThemeParameters.h"
#include "../../../utils/InterConnector.h"
#include "../../../qml/helpers/QmlEngine.h"
#include "../../../qml/models/RootModel.h"
#include "../../sidebar/GalleryList.h"
#include "ExpandedGalleryPopup.h"

namespace
{
    constexpr QStringView galleryTag() { return u"/gallery"; }
    constexpr QSize headerIconSize() noexcept { return { 24, 24 }; }

    auto tabsHeight() noexcept
    {
        return Utils::scale_value(48);
    }

    QPoint galleryPopupPosition()
    {
        return Utils::scale_value(QPoint{ 44, 32 });
    }

    auto galleryModel()
    {
        const auto rootModel = Utils::InterConnector::instance().qmlRootModel();
        return rootModel->gallery();
    }

    auto tabsModel()
    {
        return galleryModel()->galleryTabsModel();
    }
} // namespace

namespace Ui
{
    GalleryPage::GalleryPage(const QString& _aimId, QWidget* _parent)
        : PageBase(_parent)
        , aimId_(createGalleryAimId(_aimId))
        , gallery_(new GalleryList(this, QString()))
        , tabs_(new QQuickWidget(Utils::InterConnector::instance().qmlEngine(), this))
    {
        gallery_->setMaxContentWidth(MessageStyle::getHistoryWidgetMaxWidth());
        Testing::setAccessibleName(gallery_, qsl("AS GalleryPage galleryList"));

        tabs_->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
        tabs_->setFixedHeight(tabsHeight());
        tabs_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        const QUrl url(qsl("qrc:/qml_components/expanded_gallery/GalleryTabs.qml"));
        tabs_->setSource(url);
        Utils::InterConnector::instance().qmlEngine()->addQuickWidget(tabs_);
        Testing::setAccessibleName(tabs_, qsl("AS GalleryPage tabs"));

        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->addWidget(tabs_);
        rootLayout->addWidget(gallery_);

        titleBar_ = new HeaderTitleBar(this);
        titleBar_->setButtonSize(Utils::scale_value(headerIconSize()));

        backButton_ = new HeaderTitleBarButton(this);
        backButton_->setDefaultImage(qsl(":/controls/back_icon"), {}, headerIconSize());
        backButton_->setCustomToolTip(QT_TRANSLATE_NOOP("sidebar", "Back"));
        Styling::Buttons::setButtonDefaultColors(backButton_);
        Testing::setAccessibleName(backButton_, qsl("AS GalleryPage backButton"));
        titleBar_->addButtonToLeft(backButton_);
        connect(backButton_, &HeaderTitleBarButton::clicked, this, &GalleryPage::closeGallery);

        updateWidgetsTheme();

        if (Utils::isChat(getContact()))
            connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &GalleryPage::onRoleChanged);
    }

    void GalleryPage::pageOpen()
    {
        tabsModel()->selectContact(getContact());
        setContentType(tabsModel()->selectedTab());
        connect(galleryModel(), &Qml::GalleryModel::popupVisibleChanged, this, &GalleryPage::onShowPopupClicked);
        connect(tabsModel(), &Qml::GalleryTabsModel::selectedTabChanged, this, &GalleryPage::setContentType);
        connect(tabsModel(), &Qml::GalleryTabsModel::countChanged, this, &GalleryPage::onTabsCountChanged);
    }

    void GalleryPage::pageLeave()
    {
        galleryModel()->disconnect(this);
        tabsModel()->disconnect(this);

        if (galleryPopup_)
            galleryPopup_->hide();

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
        updatePopupPosition();
    }

    void GalleryPage::updateWidgetsTheme()
    {
        Utils::setDefaultBackground(this);
        Utils::setDefaultBackground(titleBar_);
    }

    void GalleryPage::setContentType(MediaContentType _type)
    {
        if (tabsModel()->selectedContact() == getContact() && MediaContentType::Invalid != _type)
        {
            titleBar_->setTitle(getGalleryTitle(_type));
            gallery_->openContentFor(getContact(), _type);
        }
    }

    void GalleryPage::onTabsCountChanged()
    {
        if (!tabsModel()->rowCount())
            closeGallery();
    }

    void GalleryPage::onShowPopupClicked(bool _popupNeedVisible)
    {
        if (_popupNeedVisible && !galleryPopup_)
        {
            initPopup();
            updatePopupPosition();
            galleryPopup_->show();
        }
    }

    void GalleryPage::onGalleryPopupHide()
    {
        auto rootModel = Utils::InterConnector::instance().qmlRootModel();
        auto galleryModel = rootModel->gallery();
        galleryModel->setPopupVisible(false);
    }

    void GalleryPage::onRoleChanged(const QString& _aimId)
    {
        if (_aimId != getContact())
            return;

        const auto role = Logic::getContactListModel()->getYourRole(getContact());
        if (role.isEmpty() || role == u"notamember" || role == u"pending")
            closeGallery();
    }

    void GalleryPage::closeGallery()
    {
        onGalleryPopupHide();
        Q_EMIT switchToPrevDialog(backButton_->hasFocus());
    }

    void GalleryPage::initPopup()
    {
        if (galleryPopup_)
            return;

        galleryPopup_ = new ExpandedGalleryPopup(this);
        connect(galleryPopup_, &ExpandedGalleryPopup::itemClicked, tabsModel(), &Qml::GalleryTabsModel::setSelectedTab);
        connect(galleryPopup_, &ExpandedGalleryPopup::hidden, galleryPopup_, &ExpandedGalleryPopup::deleteLater);
        connect(galleryPopup_, &ExpandedGalleryPopup::destroyed, this, &GalleryPage::onGalleryPopupHide, Qt::QueuedConnection);
    }

    void GalleryPage::updatePopupPosition()
    {
        if (!galleryPopup_)
            return;

        galleryPopup_->move(mapToGlobal(galleryPopupPosition()));
    }

    QString GalleryPage::getContact() const
    {
        return aimId_.left(aimId_.indexOf(galleryTag()));
    }
} // namespace Ui
