#include "stdafx.h"

#include "utils/utils.h"
#include "utils/stat_utils.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "controls/TextUnit.h"
#include "controls/GeneralDialog.h"
#include "ProfileBlock.h"
#include "ProfileBlockLayout.h"
#include "ComplexMessageItem.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"
#include "core_dispatcher.h"
#include "cache/avatars/AvatarStorage.h"
#include "main_window/sidebar/UserProfile.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "main_window/GroupChatOperations.h"
#include "main_window/contact_list/AddContactDialogs.h"
#include "previewer/toast.h"
#include "utils/PhoneFormatter.h"

namespace
{
    QPoint getAvatarPos()
    {
        return QPoint(0, 0);
    }

    QPoint getNamePos()
    {
        return QPoint(Utils::scale_value(56), Utils::scale_value(4));
    }

    QPoint getUnderNamePos()
    {
        return QPoint(Utils::scale_value(56), Utils::scale_value(24));
    }

    int getAvatarSize()
    {
        return Utils::scale_value(44);
    }

    QPoint getMessagePos()
    {
        return QPoint(0, Utils::scale_value(52));
    }

    int getButtonHeight()
    {
        return Utils::scale_value(32);
    }

    int getButtonOffset()
    {
        return Utils::scale_value(8);
    }

    QSize getArrowButtonPixmapSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QRect getClickArea(const QRect& _contentRect)
    {
        return QRect(_contentRect.topLeft(), QSize(_contentRect.width(), getAvatarSize()));
    }

    const QColor& getSelectedTextColor()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
        return color;
    }

    QFont getNameFont()
    {
        return Fonts::adjustedAppFont(16);
    }

    QFont getUnderNameFont()
    {
        return Fonts::adjustedAppFont(13);
    }

    QFont getDescriptionFont()
    {
        return Fonts::adjustedAppFont(15);
    }

    QFont getButtonFont()
    {
        return Fonts::adjustedAppFont(14, Fonts::FontWeight::SemiBold);
    }

    const QColor& getNameColor(bool _selected)
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        if (_selected)
            return getSelectedTextColor();
        else
            return color;
    }

    const QColor& getUnderNameColor(bool _selected)
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);
        if (_selected)
            return getSelectedTextColor();
        else
            return color;
    }

    const QColor& getDescriptionColor(bool _selected)
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        if (_selected)
            return getSelectedTextColor();
        else
            return color;
    }

    enum class ButtonState {Normal, Hovered, Active};

    const QColor& getButtonBackgroundColor(ButtonState _state, bool _selected, bool _outgoing)
    {
        static const auto emptyColor = QColor();
        static const auto selectedColor = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);

        if (_state != ButtonState::Normal && _selected)
            return emptyColor;

        if (_selected)
            return selectedColor;

        if (_outgoing)
        {
            static const auto normalColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
            static const auto hoveredColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT_HOVER);
            static const auto activeColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT_ACTIVE);

            switch (_state)
            {
                case ButtonState::Normal:
                    return normalColor;
                case ButtonState::Hovered:
                    return hoveredColor;
                case ButtonState::Active:
                    return activeColor;
            }
        }
        else
        {
            static const auto normalColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
            static const auto hoveredColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
            static const auto activeColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);

            switch (_state)
            {
                case ButtonState::Normal:
                    return normalColor;
                case ButtonState::Hovered:
                    return hoveredColor;
                case ButtonState::Active:
                    return activeColor;
            }
        }

        return emptyColor;
    }

    const QColor& getButtonTextColor(bool _selected)
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE);
        if (_selected)
            return getSelectedTextColor();
        else
            return color;
    }

    const QPixmap& getArrowButtonPixmap(bool _hovered, bool _active, bool _selected, bool _outgoing)
    {
        auto getPixmap = [](const QColor& _color)
        {
            auto pixmap = Utils::renderSvg(qsl(":/controls/back_icon"), getArrowButtonPixmapSize(), _color);
            pixmap = pixmap.transformed(QTransform().rotate(180));
            Utils::check_pixel_ratio(pixmap);
            return pixmap;
        };

        static const auto pixmapSelected = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));

        if (_selected)
            return pixmapSelected;

        if (_outgoing)
        {
            static const auto pixmap = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL));
            static const auto pixmapHovered = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER));
            static const auto pixmapActive = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));

            if (_active)
                return pixmapActive;
            else if (_hovered)
                return pixmapHovered;
            else
                return pixmap;
        }
        else
        {
            static const auto pixmap = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
            static const auto pixmapHovered = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
            static const auto pixmapActive = getPixmap(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));

            if (_active)
                return pixmapActive;
            else if (_hovered)
                return pixmapHovered;
            else
                return pixmap;
        }
    }

    int getToastOffset()
    {
        return Utils::scale_value(10);
    }

    void showIntermediateProfile(const QString& _name, const QString& _phone, const QString& _sn)
    {
        auto userProfile = new Ui::UserProfile(Utils::InterConnector::instance().getMainWindow(), _phone, _sn, _name);
        Ui::GeneralDialog d(userProfile, Utils::InterConnector::instance().getMainWindow(), false, true);

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
        d.addCancelButton(QT_TRANSLATE_NOOP("profile_block", "CANCEL"), true);
        d.showInCenter();
    }

}

using namespace Ui::ComplexMessage;


//////////////////////////////////////////////////////////////////////////
// ProfileBlockBase
//////////////////////////////////////////////////////////////////////////

ProfileBlockBase::ProfileBlockBase(ComplexMessageItem* _parent, const QString& _link)
    : GenericBlock(_parent, _link, MenuFlags::MenuFlagCopyable, false)
    , Layout_(new ProfileBlockLayout())
{
    setLayout(Layout_);
    setMouseTracking(true);

    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ProfileBlockBase::onAvatarChanged);
}

ProfileBlockBase::~ProfileBlockBase() = default;

IItemBlockLayout* ProfileBlockBase::getBlockLayout() const
{
    return Layout_;
}

void ProfileBlockBase::updateFonts()
{
    if (loaded_)
        initTextUnits();
}

int ProfileBlockBase::desiredWidth(int _width) const
{
    Q_UNUSED(_width)
    auto width = MessageStyle::getProfileBlockDesiredWidth();

    if (!isStandalone())
        width += MessageStyle::Files::getHorMargin() * 2;

    return width;
}

int ProfileBlockBase::getHeightForWidth(int _width) const
{
    auto height = getMessagePos().y();

    if (loaded_ && descriptionUnit_)
        height += descriptionUnit_->getHeight(isStandalone() ? _width : _width - MessageStyle::Files::getHorMargin() * 2);
    else
        height += Utils::scale_value(20);

    if (!isInsideQuote())
        height += getButtonOffset() + getButtonHeight();

    if (!isStandalone())
        height += MessageStyle::Files::getVerMargin() * 2;

    return height;
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

    clickAreaPressed_ = getClickArea(Layout_->getContentRect()).contains(mappedPoint);

    if (button_)
        button_->setPressed(button_->rect().contains(mappedPoint));

    update();

    return false;
}

bool ProfileBlockBase::clicked(const QPoint &_p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());

    auto clickHandled = false;

    if (getClickArea(Layout_->getContentRect()).contains(mappedPoint))
    {
        onClickAreaPressed();
        clickHandled = true;
    }

    if (button_ && button_->rect().contains(mappedPoint))
    {
        onButtonPressed();
        clickHandled = true;
    }

    update();

    return clickHandled;
}

void ProfileBlockBase::drawBlock(QPainter& p, const QRect& _rect, const QColor& _quoteColor)
{
    Q_UNUSED(_rect)
    Q_UNUSED(_quoteColor)

    Utils::PainterSaver ps(p);

    auto contentSize = Layout_->getContentRect().size();
    auto namePos = getNamePos();
    auto messagePos = getMessagePos();

    if (!isStandalone())
    {
        if (isSelected())
            drawSelectedFrame(p, QRect(QPoint(0, 0), contentSize));

        p.setTransform(QTransform().translate(MessageStyle::Files::getHorMargin(), MessageStyle::Files::getVerMargin()));
        contentSize.setWidth(contentSize.width() - MessageStyle::Files::getHorMargin() * 2);
        contentSize.setHeight(contentSize.height() - MessageStyle::Files::getVerMargin() * 2);
    }

    if (loaded_)
    {
        auto availableWidthForName = contentSize.width() - namePos.x() - getArrowButtonPixmapSize().width();

        nameUnit_->setOffsets(namePos.x(), namePos.y());
        nameUnit_->getHeight(availableWidthForName);
        nameUnit_->draw(p);

        underNameUnit_->setOffsets(calcUndernamePos());
        underNameUnit_->getHeight(availableWidthForName);
        underNameUnit_->draw(p);

        descriptionUnit_->setOffsets(messagePos.x(), messagePos.y());
        auto messageHeight = descriptionUnit_->getHeight(contentSize.width());
        descriptionUnit_->draw(p);

        if (!isInsideQuote())
        {
            button_->setRect(QRect(messagePos.x(), messagePos.y() + messageHeight + getButtonOffset(), contentSize.width(), getButtonHeight()));
            button_->draw(p);
        }

        auto pixmapSize = getArrowButtonPixmapSize();
        p.drawPixmap(contentSize.width() - pixmapSize.width(), getNamePos().y() + (getAvatarSize() - pixmapSize.height()) / 2, getArrowButtonPixmap(clickAreaHovered_, clickAreaPressed_, isSelected(), isOutgoing()));

        if (avatar_)
            p.drawPixmap(getAvatarPos().x(), getAvatarPos().y(), *avatar_);
        else
            p.drawPixmap(getAvatarPos().x(), getAvatarPos().y(),
                         Utils::roundImage(Utils::getDefaultAvatar(QString(), name_.isEmpty() ? underNameText_ : name_, Utils::scale_bitmap(getAvatarSize())),QString(), false, false));
    }
    else
    {
        QPainterPath path;

        auto underNamePos = getUnderNamePos();

        path.addRect(namePos.x(), namePos.y(), Utils::scale_value(100), Utils::scale_value(18));
        path.addRect(underNamePos.x(), underNamePos.y(), Utils::scale_value(60), Utils::scale_value(18));
        path.addRect(messagePos.x(), messagePos.y(), contentSize.width() - Utils::scale_value(40), Utils::scale_value(18));
        path.addRect(QRect(messagePos.x(), messagePos.y() + Utils::scale_value(18) + getButtonOffset(), contentSize.width(), getButtonHeight()));

        path.addEllipse(getAvatarPos().x(), getAvatarPos().y(), getAvatarSize(), getAvatarSize());
        p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::GHOST_QUATERNARY));
    }
}

void ProfileBlockBase::drawSelectedFrame(QPainter& _p, const QRect& _rect)
{
    Utils::PainterSaver ps(_p);
    _p.setRenderHint(QPainter::Antialiasing);

    const auto &bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), isSelected(), getChatAimid());
    _p.setBrush(bodyBrush);
    _p.setPen(Qt::NoPen);

    _p.drawRoundedRect(_rect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
}

void ProfileBlockBase::initialize()
{
    return GenericBlock::initialize();
}

void ProfileBlockBase::mouseMoveEvent(QMouseEvent* _event)
{
    if (loaded_)
    {
        auto clickAreaHovered = getClickArea(Layout_->getContentRect()).contains(_event->pos());
        auto buttonHovered = button_->rect().contains(_event->pos());

        auto needUpdate = buttonHovered != button_->hovered() || clickAreaHovered != clickAreaHovered_;

        button_->setHovered(buttonHovered);
        clickAreaHovered_ = clickAreaHovered;

        setCursor(clickAreaHovered_ || button_->hovered() ? Qt::PointingHandCursor : Qt::ArrowCursor);

        if (needUpdate)
            update();
    }

    GenericBlock::mouseMoveEvent(_event);
}

void ProfileBlockBase::mouseReleaseEvent(QMouseEvent *_event)
{
    clickAreaPressed_ = false;

    if (button_)
        button_->setPressed(false);

    update();

    GenericBlock::mouseReleaseEvent(_event);
}

void ProfileBlockBase::leaveEvent(QEvent* _event)
{
    if (button_)
        button_->setHovered(false);

    clickAreaHovered_ = false;

    setCursor(Qt::ArrowCursor);

    update();

    GenericBlock::leaveEvent(_event);
}

QString ProfileBlockBase::getButtonText() const
{
    return QString();
}

void ProfileBlockBase::onAvatarChanged(const QString& aimId)
{
    if (sn() != aimId)
        return;

    loadAvatar(sn(), name_);
    update();
}

void ProfileBlockBase::init(const QString& _name, const QString& _underName, const QString& _description)
{
    name_ = _name;
    nameUnit_ = TextRendering::MakeTextUnit(_name, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);

    underNameText_ = _underName;
    underNameUnit_ = TextRendering::MakeTextUnit(_underName, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    descriptionUnit_ = TextRendering::MakeTextUnit(_description, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);

    button_ = std::make_unique<BLabel>();
    auto buttonUnit = TextRendering::MakeTextUnit(getButtonText());
    button_->setTextUnit(std::move(buttonUnit));
    button_->background_ = getButtonBackgroundColor(ButtonState::Normal, false, isOutgoing());
    button_->hoveredBackground_ = getButtonBackgroundColor(ButtonState::Hovered, false, isOutgoing());
    button_->pressedBackground_ = getButtonBackgroundColor(ButtonState::Active, false, isOutgoing());
    button_->setBorderRadius(Utils::scale_value(8));
    button_->setVerticalPosition(TextRendering::VerPosition::MIDDLE);
    button_->setYOffset(getButtonHeight() / 2);

    initTextUnits();
}

void ProfileBlockBase::initTextUnits()
{
    nameUnit_->init(getNameFont(), getNameColor(false), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
    underNameUnit_->init(getUnderNameFont(), getUnderNameColor(false), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
    descriptionUnit_->init(getDescriptionFont(), getDescriptionColor(false), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 2, TextRendering::LineBreakType::PREFER_SPACES);
    button_->initTextUnit(getButtonFont(), getButtonTextColor(false), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
}

void ProfileBlockBase::loadAvatar(const QString& _sn, const QString& _name)
{
    auto isDefault = false;

    avatar_ = Logic::GetAvatarStorage()->GetRounded(
        _sn,
        _name,
        Utils::scale_bitmap(getAvatarSize()),
        QString(),
        Out isDefault,
        false,
        false);
}

void ProfileBlockBase::setSelected(bool _selected)
{
    if (getParentComplexMessage()->getBlockCount() == 1 || getParentComplexMessage()->getQuotes().isEmpty())
       getParentComplexMessage()->setDrawFullBubbleSelected(_selected && isStandalone());

    if (loaded_)
    {
        nameUnit_->setColor(getNameColor(_selected));
        underNameUnit_->setColor(getUnderNameColor(_selected));
        descriptionUnit_->setColor(getDescriptionColor(_selected));

        button_->setDefaultColor(getButtonTextColor(_selected));
        button_->background_ = getButtonBackgroundColor(ButtonState::Normal, _selected, isOutgoing());
        button_->hoveredBackground_ = getButtonBackgroundColor(ButtonState::Hovered, _selected, isOutgoing());
        button_->pressedBackground_ = getButtonBackgroundColor(ButtonState::Active, _selected, isOutgoing());
    }

    GenericBlock::setSelected(_selected);
}

QPoint ProfileBlockBase::calcUndernamePos() const
{
    if (!nameUnit_ || !underNameUnit_)
        return QPoint();

    if (!nameUnit_->getText().isEmpty())
        return getUnderNamePos();
    else
        return QPoint(getUnderNamePos().x(), getAvatarPos().y() + (getAvatarSize() - underNameUnit_->cachedSize().height()) / 2);
}

//////////////////////////////////////////////////////////////////////////
// ProfileBlock
//////////////////////////////////////////////////////////////////////////

ProfileBlock::ProfileBlock(ComplexMessageItem* _parent, const QString& _link)
    : ProfileBlockBase(_parent, _link)
    , link_(_link)
{
    connect(GetDispatcher(), &Ui::core_dispatcher::idInfo, this, &ProfileBlock::onIdInfo);
}

ProfileBlock::~ProfileBlock()
{

}

QString ProfileBlock::extractProfileId(const QString& _link)
{
    const auto parts = _link.splitRef(ql1c('/'));
    if (!parts.empty() && !parts.last().isEmpty())
        return parts.last().toString();

    return QString();
}

QString ProfileBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    Q_UNUSED(_isFullSelect)
    Q_UNUSED(_dest)
    return getTextForCopy();
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
        if (!info_.sn_.contains(ql1c('@')))
            text += ql1c('@');

        if (info_.nick_.isEmpty())
            text += info_.sn_;
        else
            text += info_.nick_;

        return text;
    }
    else if (info_.type_ == Data::IdInfo::IdType::Chat)
    {
        const QString text = QString::number(info_.memberCount_) % ql1c(' ')
            % Utils::GetTranslator()->getNumberString(
                  info_.memberCount_,
                  QT_TRANSLATE_NOOP3("profile_block", "member", "1"),
                  QT_TRANSLATE_NOOP3("profile_block", "members", "2"),
                  QT_TRANSLATE_NOOP3("profile_block", "members", "5"),
                  QT_TRANSLATE_NOOP3("profile_block", "members", "21")
                                  );
        return text;
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
        loaded_ = true;

        init(info_.name_, getUnderNameText(), info_.description_);

        loadAvatar(info_.sn_, info_.name_);
    }

    notifyBlockContentsChanged();
}

void ProfileBlock::onButtonPressed()
{
    if (info_.type_ == Data::IdInfo::IdType::Chat)
    {
        Utils::openDialogOrProfile(info_.stamp_, Utils::OpenDOPParam::stamp);
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
    if (info_.type_ == Data::IdInfo::IdType::Chat)
    {
        Utils::openDialogOrProfile(info_.stamp_, Utils::OpenDOPParam::stamp);
    }
    else
    {
        emit Utils::InterConnector::instance().profileSettingsShow(info_.sn_);
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

QString PhoneProfileBlock::getSourceText() const
{
    return QString();
}

QString PhoneProfileBlock::getTextForCopy() const
{
    return name_ % ql1c(' ') % phone_;
}

QString PhoneProfileBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    Q_UNUSED(_isFullSelect)
    if (_dest == IItemBlock::TextDestination::selection)
        return getTextForCopy();
    else
        return QString();
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
    loaded_ = true;
    init(name_, phone_, QString());

    if (!sn_.isEmpty())
        loadAvatar(sn_, name_);
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

