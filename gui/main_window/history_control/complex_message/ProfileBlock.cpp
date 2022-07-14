#include "stdafx.h"

#include "ProfileBlock.h"

#include "utils/utils.h"
#include "utils/stat_utils.h"
#include "utils/features.h"
#include "controls/TextUnit.h"
#include "controls/GeneralDialog.h"
#include "ProfileBlockLayout.h"
#include "ComplexMessageItem.h"
#include "../MessageStyle.h"
#include "core_dispatcher.h"
#include "main_window/sidebar/UserProfile.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/AddContactDialogs.h"
#include "previewer/toast.h"
#include "utils/PhoneFormatter.h"
#include "utils/gui_coll_helper.h"
#include "../../contact_list/ContactListModel.h"
#include "controls/UserMiniProfile.h"

namespace
{
    int getToastOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    int getTopProfileMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    void showIntermediateProfile(const QString& _name, const QString& _phone, const QString& _sn)
    {
        auto userProfile = new Ui::UserProfile(Utils::InterConnector::instance().getMainWindow(), _phone, _sn, _name);
        Ui::GeneralDialog d(userProfile, Utils::InterConnector::instance().getMainWindow());

        QObject::connect(userProfile, &Ui::UserProfile::saveClicked, &d, [_name, _phone, &d, _sn]()
        {
            d.close();
            Utils::showAddContactsDialog(_name, PhoneFormatter::formatted(_phone), [_sn](){
                Utils::openDialogWithContact(_sn);
                Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("profile_block", "Contact was saved. Phone is visible in profile."), getToastOffset());
            }, { Utils::AddContactDialogs::Initiator::From::Unspecified });
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharedcontactscr_action, { { "do", "create" } });
        });

        d.addLabel(QT_TRANSLATE_NOOP("profile_block", "About this contact"));
        d.addCancelButton(QT_TRANSLATE_NOOP("profile_block", "Cancel"), true);
        d.execute();
    }
}

using namespace Ui::ComplexMessage;


//////////////////////////////////////////////////////////////////////////
// ProfileBlockBase
//////////////////////////////////////////////////////////////////////////

ProfileBlockBase::ProfileBlockBase(ComplexMessageItem* _parent, const QString& _link)
    : GenericBlock(_parent, Data::FString(_link), MenuFlags::MenuFlagCopyable, false)
    , Layout_(new ProfileBlockLayout())
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageProfile " % QString::number(_parent->getId()));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setLayout(Layout_);
    setMouseTracking(true);

    MiniProfileFlags profileFlags = MiniProfileFlag::None;
    profileFlags.setFlag(MiniProfileFlag::isOutgoing, isOutgoing());
    userProfile_ = new UserMiniProfile(this, sn(), profileFlags);
    Layout_->setSpacing(getTopProfileMargin());
    Layout_->addWidget(userProfile_);

    connect(userProfile_, &UserMiniProfile::showNameTooltip, this, &ProfileBlockBase::onNameTooltipShow);
    connect(userProfile_, &UserMiniProfile::hideNameTooltip, this, &ProfileBlockBase::onNameTooltipHide);
    connect(userProfile_, &UserMiniProfile::onButtonClicked, this, &ProfileBlockBase::onButtonPressed);
    connect(userProfile_, &UserMiniProfile::onClickAreaClicked, this, &ProfileBlockBase::onClickAreaPressed);
}

ProfileBlockBase::~ProfileBlockBase() = default;

IItemBlockLayout* ProfileBlockBase::getBlockLayout() const
{
    return Layout_;
}

void ProfileBlockBase::updateFonts()
{
    userProfile_->updateFonts();
}

int ProfileBlockBase::desiredWidth(int _width) const
{
    Q_UNUSED(_width)
    return MessageStyle::getProfileBlockDesiredWidth();
}

int ProfileBlockBase::getMaxWidth() const
{
    return Utils::scale_value(400);
}

int ProfileBlockBase::updateSizeForWidth(int _width)
{
    setFixedWidth(_width);
    if (_width != cachedWidth_)
    {
        cachedWidth_ = _width;
        userProfile_->updateSize(_width);
    }

    return userProfile_->height() + getTopProfileMargin();
}

QString ProfileBlockBase::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("profile_block", "Contact");
}

Ui::MediaType ProfileBlockBase::getMediaType() const
{
    return Ui::MediaType::mediaTypeContact;
}

bool ProfileBlockBase::pressed(const QPoint &_p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());
    userProfile_->pressed(mappedPoint, Utils::InterConnector::instance().isMultiselect(getChatAimid()));

    update();

    return false;
}

bool ProfileBlockBase::clicked(const QPoint &_p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());
    auto clickHandled = userProfile_->clicked(mappedPoint, Utils::InterConnector::instance().isMultiselect(getChatAimid()));
    update();
    return clickHandled;
}

void ProfileBlockBase::drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor)
{
    Q_UNUSED(_rect)
    Q_UNUSED(_quoteColor)
    Q_UNUSED(_p)

    userProfile_->setFixedWidth(Layout_->getContentRect().width());
    userProfile_->updateFlags(isStandalone(), isInsideQuote());
}

void ProfileBlockBase::initialize()
{
    return GenericBlock::initialize();
}

void ProfileBlockBase::mouseMoveEvent(QMouseEvent* _event)
{
    if (Utils::InterConnector::instance().isMultiselect(getChatAimid()))
        setCursor(Qt::PointingHandCursor);

    GenericBlock::mouseMoveEvent(_event);
}

QString ProfileBlockBase::getButtonText() const
{
    return QString();
}

void ProfileBlockBase::init(const QString& _name, const QString& _underName, const QString& _description)
{
    userProfile_->setFixedWidth(Layout_->getContentRect().width());
    userProfile_->init(sn(), _name, _underName, _description, getButtonText());
    userProfile_->updateSize(width());
}

void ProfileBlockBase::onNameTooltipShow(const QString& _text, const QRect& _nameRect)
{
    if (!isTooltipActivated())
    {
        const QRect ttRect(0, _nameRect.y(), width(), _nameRect.height());
        const auto isFullyVisible = visibleRegion().boundingRect().y() <= ttRect.top();
        const auto arrowDir = isFullyVisible ? Tooltip::ArrowDirection::Down : Tooltip::ArrowDirection::Up;
        const auto arrowPos = isFullyVisible ? Tooltip::ArrowPointPos::Top : Tooltip::ArrowPointPos::Bottom;
        showTooltip(_text, QRect(mapToGlobal(ttRect.topLeft()), ttRect.size()), arrowDir, arrowPos);
    }
}

void ProfileBlockBase::onNameTooltipHide()
{
    hideTooltip();
}

//////////////////////////////////////////////////////////////////////////
// ProfileBlock
//////////////////////////////////////////////////////////////////////////

ProfileBlock::ProfileBlock(ComplexMessageItem* _parent, const QString& _link)
    : ProfileBlockBase(_parent, _link)
    , link_(_link)
{
    connect(GetDispatcher(), &Ui::core_dispatcher::idInfo, this, &ProfileBlock::onIdInfo);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &ProfileBlock::chatInfo);
}

ProfileBlock::~ProfileBlock()
{

}

QString ProfileBlock::extractProfileId(const QString& _link)
{
    const auto parts = _link.splitRef(ql1c('/'));
    if (!parts.empty() && !parts.back().isEmpty())
        return parts.back().toString();

    return QString();
}

bool ProfileBlock::isSelected() const { return userProfile_->isSelected(); }

void ProfileBlock::selectByPos(const QPoint& _from, const QPoint& _to, bool _topToBottom)
{
    Q_UNUSED(_topToBottom)
    if (userProfile_)
    {
        userProfile_->selectDescription(_from, _to);
        setSelected(true);
    }

    update();
}

Data::FString ProfileBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    Q_UNUSED(_isFullSelect)
    if (_dest == IItemBlock::TextDestination::selection)
    {
        return getTextForCopy();
    }
    else if (_dest == IItemBlock::TextDestination::quote)
    {
        return userProfile_->getSelectedDescription();
    }
    
    return {};
}

void ProfileBlock::clearSelection()
{
    userProfile_->clearSelection();
}

void ProfileBlock::releaseSelection()
{
    userProfile_->releaseSelection();
}

void ProfileBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
{
    const auto mappedPoint = mapFromParent(_p, getBlockGeometry());
    userProfile_->doubleClicked(mappedPoint, _callback);
    update();
}

QString ProfileBlock::getButtonText() const
{
    if (info_.type_ == Data::IdInfo::IdType::User)
        return QT_TRANSLATE_NOOP("profile_block", "WRITE");
    else if (info_.type_ == Data::IdInfo::IdType::Chat)
        return QT_TRANSLATE_NOOP("profile_block", "VIEW");

    return QString();
}

QString ProfileBlock::getUnderNameText()
{
    if (info_.type_ == Data::IdInfo::IdType::User)
    {
        QString text;
        if (!info_.sn_.contains(u'@'))
            text += u'@';

        if (info_.nick_.isEmpty())
            text += info_.sn_;
        else
            text += info_.nick_;

        return text;
    }
    else if (info_.type_ == Data::IdInfo::IdType::Chat)
    {
        const auto isChannel = Logic::getContactListModel()->isChannel(info_.sn_) || (chatInfo_ && chatInfo_->isChannel());
        return Utils::getMembersString(info_.memberCount_, isChannel);
    }

    return QString();
}

void ProfileBlock::initialize()
{
    auto id = extractProfileId(link_);
    if (!id.isEmpty())
        seq_ = GetDispatcher()->getIdInfo(id);
    else
        getParentComplexMessage()->replaceBlockWithSourceText(this);
}

QString ProfileBlock::sn() const
{
    return info_.sn_;
}

void ProfileBlock::onIdInfo(const qint64 _seq, const Data::IdInfo& _idInfo)
{
    if (seq_ != _seq)
        return;

    if (_idInfo.sn_.isEmpty())
    {
        getParentComplexMessage()->replaceBlockWithSourceText(this);
    }
    else
    {
        info_ = _idInfo;
        init(info_.getName(), getUnderNameText(), info_.description_);
    }

    if (info_.isChatInfo())
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        if (!info_.sn_.isEmpty())
            collection.set_value_as_qstring("aimid", info_.sn_);

        collection.set_value_as_qstring("stamp", info_.stamp_);

        collection.set_value_as_int("limit", 0);
        chatInfoSeq_ = Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
    }

    notifyBlockContentsChanged();
}

void ProfileBlock::chatInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo>& _chatInfo, const int)
{
    if (_seq != chatInfoSeq_)
        return;

    chatInfo_ = _chatInfo;
    init(info_.name_, getUnderNameText(), info_.description_);
    update();
}

void ProfileBlock::openChat() const
{
    if (!info_.sn_.isEmpty())
        Utils::openDialogOrProfile(info_.sn_);
    else if (!info_.stamp_.isEmpty())
        Utils::openDialogOrProfile(info_.stamp_, Utils::OpenDOPParam::stamp);
}

void ProfileBlock::onButtonPressed()
{
    if (info_.type_ == Data::IdInfo::IdType::Chat)
    {
        openChat();
    }
    else
    {
        Utils::openDialogWithContact(info_.sn_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_contactinfo_action, { { "chat_type", Utils::chatTypeByAimId(info_.sn_) }, { "do", "write" } });
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharedcontactscr_action, { { "do", "write" } });
    }
}

void ProfileBlock::onClickAreaPressed()
{
    const auto previouslyCommunicated = (info_.type_ == Data::IdInfo::IdType::User && Logic::getContactListModel()->hasContact(info_.sn_));
    if (info_.type_ == Data::IdInfo::IdType::Chat || previouslyCommunicated)
    {
        openChat();
    }
    else
    {
        Q_EMIT Utils::InterConnector::instance().profileSettingsShow(info_.sn_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_contactinfo_action, { { "chat_type", Utils::chatTypeByAimId(info_.sn_) }, { "do", "shared_contact" } });
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharedcontactscr_action, { { "do", "profile" } });
    }
}

//////////////////////////////////////////////////////////////////////////
// PhoneProfileBlock
//////////////////////////////////////////////////////////////////////////

PhoneProfileBlock::PhoneProfileBlock(ComplexMessageItem* _parent, const QString& _name, const QString& _phone, const QString& _sn)
    : ProfileBlockBase(_parent, QString())
    , phone_(PhoneFormatter::formatted(_phone))
    , sn_(_sn)
{
    name_ = _name;
}

PhoneProfileBlock::~PhoneProfileBlock()
{

}

Data::FString PhoneProfileBlock::getSourceText() const
{
    return {};
}

QString PhoneProfileBlock::getTextForCopy() const
{
    auto name = name_;
    if (!Utils::isChat(sn()))
        name = name.replace(ql1c('\n'), ql1c(' '));
    return name % ql1c(' ') % phone_;
}

Data::FString PhoneProfileBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    Q_UNUSED(_isFullSelect)
    if (_dest == IItemBlock::TextDestination::selection)
        return getTextForCopy();
    else
        return {};
}

QString PhoneProfileBlock::getButtonText() const
{
    if (sn_.isEmpty())
        return QT_TRANSLATE_NOOP("profile_block", "VIEW");
    else
        return QT_TRANSLATE_NOOP("profile_block", "WRITE");
}

void PhoneProfileBlock::initialize()
{
    init(name_, phone_, QString());
}

QString PhoneProfileBlock::sn() const
{
    return sn_;
}

void PhoneProfileBlock::onButtonPressed()
{
    if (sn_.isEmpty())
    {
        QTimer::singleShot(0, this, [this]() { showIntermediateProfile(name_, phone_, sn_); });
    }
    else
    {
        Utils::openDialogWithContact(sn_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_contactinfo_action, { { "chat_type", Utils::chatTypeByAimId(sn_) }, { "do", "write" } });
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharedcontactscr_action, { { "do", "write" } });
    }

}

void PhoneProfileBlock::onClickAreaPressed()
{
    QTimer::singleShot(0, this, [this]() { showIntermediateProfile(name_, phone_, sn_); });
}

