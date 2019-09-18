#include "stdafx.h"
#include "SelectionContactsForGroupChat.h"

#include "AbstractSearchModel.h"
#include "CustomAbstractListModel.h"
#include "ContactList.h"
#include "ContactListWidget.h"
#include "ContactListModel.h"
#include "IgnoreMembersModel.h"
#include "SearchWidget.h"
#include "../GroupChatOperations.h"
#include "../MainWindow.h"
#include "../friendly/FriendlyContainer.h"
#include "../../core_dispatcher.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/DialogButton.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/CheckboxList.h"
#include "../../gui_settings.h"
#include "../../styles/ThemeParameters.h"

namespace Ui
{
    const double heightPartOfMainWindowForFullView = 0.6;
    const int DIALOG_WIDTH = 360;
    const int search_padding_ver = 8;
    const int search_padding_hor = 4;
    const int avatars_area_top_padding = 12;
    const int avatars_area_bottom_padding = 4;
    const int avatars_area_hor_padding = 16;
    const int avatars_area_spacing = 6;
    const int avatar_size = 40;
    const int avatars_scroll_step = 10;

    AvatarsArea::AvatarsArea(QWidget* _parent)
        : QWidget(_parent)
        , diff_(0)
        , avatarOffset_(0)
    {
        setMouseTracking(true);
    }

    void AvatarsArea::add(const QString& _aimid, QPixmap _avatar)
    {
        AvatarData d;
        d.aimId_ = _aimid;
        d.avatar_ = _avatar;
        Utils::check_pixel_ratio(_avatar);
        avatars_.insert(avatars_.begin(), d);

        if (avatars_.size() == 1)
        {
            auto h = Utils::scale_value(avatars_area_top_padding + avatars_area_bottom_padding + avatar_size);
            heightAnimation_.start([this, h]() {
                const auto cur = heightAnimation_.current();
                setFixedHeight(cur);
                if (h == cur)
                    emit showed();
            }, 0, h, 200, anim::linear, 1);
        }
        else
        {
            if (!avatarAnimation_.isRunning())
            {
                avatarAnimation_.finish();
                avatarOffset_ = Utils::scale_value(avatar_size);
                avatarAnimation_.start([this]() {
                    avatarOffset_ = avatarAnimation_.current();
                    update();
                }, Utils::scale_value(avatar_size), 0, 300, anim::linear, 1);
            }
        }

        update();
    }

    void AvatarsArea::remove(const QString& _aimid)
    {
        if (avatars_.size() == 1 && avatars_[0].aimId_ == _aimid)
        {
            heightAnimation_.start([this, _aimid]() {
                const auto cur = heightAnimation_.current();
                setFixedHeight(cur);
            }, height(), 0, 200, anim::linear, 1);
        }

        avatars_.erase(std::remove_if(avatars_.begin(), avatars_.end(), [&_aimid](const AvatarData& _value) { return _value.aimId_ == _aimid; }), avatars_.end());
        emit removed();
        update();
    }

    void AvatarsArea::paintEvent(QPaintEvent* e)
    {
        if (avatars_.empty())
            return;

        const auto b = Utils::scale_bitmap_ratio();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        Utils::drawBackgroundWithBorders(p, rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT), Qt::AlignTop);

        auto horOffset = Utils::scale_value(avatars_area_hor_padding) + diff_;
        auto verOffset = Utils::scale_value(avatars_area_top_padding);

        horOffset -= avatarOffset_;
        for (const auto& a : avatars_)
        {
            p.drawPixmap(horOffset, verOffset, a.avatar_.width() / b, a.avatar_.height() / b, a.avatar_);
            if (a.hovered_)
            {
                Utils::PainterSaver ps(p);

                auto color = QColor(ql1s("#000000"));
                color.setAlphaF(0.48);
                p.setBrush(color);
                p.setPen(QPen(Qt::transparent, 0));
                const auto radius = Utils::scale_bitmap_with_value(avatar_size / 2);
                p.drawEllipse(QPoint(horOffset + radius / b, verOffset + radius / b), radius / b, radius / b);

                static const QPixmap hover(Utils::renderSvgScaled(qsl(":/controls/close_icon"), QSize(12, 12), qsl("#ffffff")));
                p.drawPixmap(horOffset + radius / b - hover.width() /2 / b, verOffset + radius / b - hover.height() / 2 / b, hover.width() / b, hover.height() / b, hover);
            }

            horOffset += a.avatar_.width() / b;
            horOffset += Utils::scale_value(avatars_area_spacing);
        }

        const QSize gradientSize(Utils::scale_value(avatar_size) / 2, Utils::scale_value(avatar_size));

        const auto drawGradient = [this, &gradientSize, verOffset](const int _left, const QColor& _start, const QColor& _end)
        {
            QRect grRect(QPoint(_left, verOffset), gradientSize);

            QLinearGradient gradient(grRect.topLeft(), grRect.topRight());
            gradient.setColorAt(0, _start);
            gradient.setColorAt(1, _end);

            QPainter p(this);
            p.fillRect(grRect, gradient);
        };
        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

        if (diff_ != 0)
            drawGradient(0, bg, Qt::transparent);

        if (horOffset > width())
            drawGradient(width() - gradientSize.width(), Qt::transparent, bg);
    }

    void AvatarsArea::mousePressEvent(QMouseEvent* e)
    {
        pressPoint_ = e->pos();
        return QWidget::mousePressEvent(e);
    }

    void AvatarsArea::mouseReleaseEvent(QMouseEvent* e)
    {
        if (Utils::clicked(pressPoint_, e->pos()))
        {
            for (const auto& a : avatars_)
            {
                if (a.hovered_)
                {
                    Logic::getContactListModel()->setChecked(a.aimId_, !Logic::getContactListModel()->getIsChecked(a.aimId_));
                    Tooltip::hide();
                    Tooltip::forceShow(false);
                    remove(a.aimId_);
                    break;
                }
            }
        }

        updateHover(e->pos());
        return QWidget::mouseReleaseEvent(e);
    }

    void AvatarsArea::mouseMoveEvent(QMouseEvent* e)
    {
        updateHover(e->pos());
        return QWidget::mouseMoveEvent(e);
    }

    void AvatarsArea::leaveEvent(QEvent* e)
    {
        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        Tooltip::hide();
        Tooltip::forceShow(false);
        update();
        return QWidget::leaveEvent(e);
    }

    void AvatarsArea::wheelEvent(QWheelEvent* e)
    {
        int value = 0;
#ifdef __APPLE__
        auto numPixels = e->pixelDelta();
        auto numDegrees = e->angleDelta() / 8;
        if (!numPixels.isNull())
        {
            value = numPixels.x();
        }
        else if (!numDegrees.isNull())
        {
            auto numSteps = numDegrees / 15;
            value = numSteps.x();
        }
#else
        const auto numDegrees = e->delta() / 8;
        const auto numSteps = numDegrees / 15;
        value = numSteps * Utils::scale_value(avatars_scroll_step);
#endif //__APPLE__

        value /= Utils::scale_bitmap(1);

        auto avatarsWidth = Utils::scale_value(avatars_area_hor_padding);
        for (const auto& a : avatars_)
        {
            avatarsWidth += Utils::scale_value(avatars_area_spacing);
            avatarsWidth += a.avatar_.width() / Utils::scale_bitmap(1);
        }

        if (avatarsWidth < width())
            return;

        diff_ += value;
        diff_ = std::max(diff_, width() - avatarsWidth);
        diff_ = std::min(diff_, 0);

        update();
    }

    void AvatarsArea::resizeEvent(QResizeEvent* e)
    {
        QWidget::resizeEvent(e);
        emit resized(QPrivateSignal());
    }

    void AvatarsArea::focusInEvent(QFocusEvent* _event)
    {
        updateFocus(UpdateFocusOrder::Current);
        return QWidget::focusInEvent(_event);
    }

    void AvatarsArea::focusOutEvent(QFocusEvent* _event)
    {
        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        Tooltip::hide();
        Tooltip::forceShow(false);
        update();
        return QWidget::focusOutEvent(_event);
    }

    void AvatarsArea::keyPressEvent(QKeyEvent* _event)
    {
        switch (_event->key())
        {
        case Qt::Key_Right:
            updateFocus(UpdateFocusOrder::Next);
            break;
        case Qt::Key_Left:
            updateFocus(UpdateFocusOrder::Previous);
            break;
        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            const auto hoveredAvatar = std::find_if(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });
            if (hoveredAvatar == avatars_.end())
                break;

            Logic::getContactListModel()->setChecked(hoveredAvatar->aimId_, !Logic::getContactListModel()->getIsChecked(hoveredAvatar->aimId_));

            if (avatars_.size() > 1)
            {
                if (hoveredAvatar == std::prev(avatars_.end()))
                    std::prev(hoveredAvatar)->hovered_ = true;
                else
                    std::next(hoveredAvatar)->hovered_ = true;
            }

            Tooltip::hide();
            Tooltip::forceShow(false);
            remove(hoveredAvatar->aimId_);

            updateFocus(UpdateFocusOrder::Current);
        }
        break;
        default:
            break;
        }

        QWidget::keyPressEvent(_event);
    }

    void AvatarsArea::updateHover(const QPoint& _pos)
    {
        bool prevHovered = std::any_of(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });

        std::for_each(avatars_.begin(), avatars_.end(), [](AvatarData& _data) { _data.hovered_ = false; });
        auto x = _pos.x();
        x -= diff_;
        x -= Utils::scale_value(avatars_area_hor_padding);
        auto avatarsWidth = Utils::scale_value(avatars_area_hor_padding);
        avatarsWidth += diff_;
        auto space = 0;
        for (auto& a : avatars_)
        {
            x -= a.avatar_.width() / Utils::scale_bitmap(1);
            x -= Utils::scale_value(avatars_area_spacing);
            avatarsWidth += Utils::scale_value(avatars_area_spacing);
            avatarsWidth += a.avatar_.width() / Utils::scale_bitmap(1);
            if (x <= 0 && avatarsWidth <= width())
            {
                a.hovered_ = true;
                setCursor(Qt::PointingHandCursor);
                Tooltip::show(Logic::GetFriendlyContainer()->getFriendly(a.aimId_),
                              QRect(mapToGlobal(QPoint(Utils::scale_value(avatars_area_hor_padding) + space + diff_, Utils::scale_value(avatars_area_top_padding))), Utils::scale_value(QSize(avatar_size, avatar_size))));
                update();
                return;
            }

            space += Utils::scale_value(avatar_size + avatars_area_spacing);
        }

        if (prevHovered)
        {
            Tooltip::hide();
            Tooltip::forceShow(false);
            prevHovered = false;
        }

        setCursor(Qt::ArrowCursor);
        update();
    }

    void AvatarsArea::updateFocus(const UpdateFocusOrder& _order)
    {
        if (avatars_.size() == 0)
            return;

        auto hoveredAvatar = std::find_if(avatars_.begin(), avatars_.end(), [](const auto& _avatar) { return _avatar.hovered_; });
        if (hoveredAvatar != avatars_.end())
        {
            switch (_order)
            {
            case UpdateFocusOrder::Current:
                break;
            case UpdateFocusOrder::Next:
                hoveredAvatar->hovered_ = false;
                if (++hoveredAvatar == avatars_.end())
                    hoveredAvatar = avatars_.begin();
                break;
            case UpdateFocusOrder::Previous:
                hoveredAvatar->hovered_ = false;
                if (hoveredAvatar == avatars_.begin())
                    hoveredAvatar = std::prev(avatars_.end());
                else
                    --hoveredAvatar;
                break;
            }
        }
        else
        {
            hoveredAvatar = avatars_.begin();
        }

        hoveredAvatar->hovered_ = true;

        const auto aWidth = hoveredAvatar->avatar_.width() / Utils::scale_bitmap_ratio();
        const auto aAreaWidth = aWidth + Utils::scale_value(avatars_area_spacing);
        const auto horOffset = Utils::scale_value(avatars_area_hor_padding) + diff_ + std::distance(avatars_.begin(), hoveredAvatar) * aAreaWidth;
        const auto xL = rect().x() + Utils::scale_value(avatars_area_hor_padding);
        const auto xR = rect().width() - aAreaWidth;
        auto correctTooltipX = 0;

        if (horOffset < xL)
        {
            auto val = xL - horOffset;
            diff_ += val;
            correctTooltipX = val;
        }
        else if (horOffset > xR)
        {
            auto val = horOffset - xR;
            diff_ -= val;
            correctTooltipX = -val;
        }

        Tooltip::show(Logic::GetFriendlyContainer()->getFriendly(hoveredAvatar->aimId_),
                      QRect(mapToGlobal(QPoint(horOffset + correctTooltipX, Utils::scale_value(avatars_area_top_padding))), QSize(aWidth, aWidth)));
        Tooltip::forceShow(true);

        update();
    }

    void SelectContactsWidget::itemClicked(const QString& _current)
    {
        searchWidget_->setFocus();

        // Restrict max selected elements.
        if (regim_ == Logic::VIDEO_CONFERENCE && maximumSelectedCount_ >= 0)
        {
            bool isForCheck = !Logic::getContactListModel()->getIsChecked(_current);

            int selectedItemCount = Logic::getContactListModel()->GetCheckedContacts().size();
            if (isForCheck && selectedItemCount >= maximumSelectedCount_)
            {
                // Disable selection.
                return;
            }
        }

        if (Logic::is_members_regim(regim_))
        {
            auto globalCursorPos = mainDialog_->mapFromGlobal(QCursor::pos());
            auto removeFrame = ::Ui::GetContactListParams().removeContactFrame();

            if (removeFrame.left() <= globalCursorPos.x() && globalCursorPos.x() <= removeFrame.right())
            {
                deleteMemberDialog(chatMembersModel_, chatAimId_, _current, regim_, this);
                return;
            }

            if (regim_ != Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                emit ::Utils::InterConnector::instance().profileSettingsShow(_current);
                if constexpr (platform::is_apple())
                {
                    mainDialog_->close();
                }
                else
                {
                    show();
                }
            }

            return;
        }

        // Disable delete for video conference list.
        if (chatMembersModel_ && !Logic::is_video_conference_regim(regim_) && regim_ != Logic::CONTACT_LIST_POPUP)
        {
            if (chatMembersModel_->contains(_current))
                return;
        }

        if (regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT)
            Logic::getContactListModel()->clearChecked();

        Logic::getContactListModel()->setChecked(_current, !Logic::getContactListModel()->getIsChecked(_current));
        contactList_->update();

        if (regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT)
            mainDialog_->accept();
        else
            mainDialog_->setButtonActive(!Logic::getContactListModel()->GetCheckedContacts().empty());

        if (avatarsArea_)
        {
            if (Logic::getContactListModel()->getIsChecked(_current))
            {
                bool isDefault = false;
                auto avatar = Logic::GetAvatarStorage()->GetRounded(_current, Logic::GetFriendlyContainer()->getFriendly(_current), Utils::scale_bitmap(Utils::scale_value(avatar_size)), QString(), isDefault, false, false);
                avatarsArea_->show();
                avatarsArea_->raise();
                avatarsArea_->add(_current, *avatar);
            }
            else
            {
                avatarsArea_->remove(_current);
            }
        }
    }

    class AuthorWidget_p
    {
    public:

        int getCheckBoxSize() const
        {
            return Utils::scale_value(20);
        }
        QRect getCheckBoxRect() const
        {
            return {Utils::scale_value(320), Utils::scale_value(14), getCheckBoxSize(), getCheckBoxSize()};
        }
        QSize getSize() const
        {
            return {Utils::scale_value(DIALOG_WIDTH), Utils::scale_value(48)};
        }
        int getTextLeftMargin() const
        {
            return Utils::scale_value(16);
        }
        int getAuthorTopMargin() const
        {
            return Utils::scale_value(4);
        }
        int getChatNameTopMargin() const
        {
            return Utils::scale_value(24);
        }
        int getCheckBoxTextBaselineTopMargin() const
        {
            return Utils::scale_value(28);
        }
        int getCheckBoxTextRightMargin() const
        {
            return Utils::scale_value(10);
        }
        QRect getCheckBoxClickRect() const
        {
            const int checkBoxTextWidth = checkBoxTextUnit_->desiredWidth();
            const int checkBoxTextLeftOffset = getCheckBoxRect().left() - getCheckBoxTextRightMargin() - checkBoxTextWidth;

            return {checkBoxTextLeftOffset,
                    Utils::scale_value(9),
                    checkBoxTextWidth + getCheckBoxTextRightMargin() + getCheckBoxSize() + Utils::scale_value(4),
                    Utils::scale_value(28)};
        }

        TextRendering::TextUnitPtr authorUnit_;
        TextRendering::TextUnitPtr chatNameUnit_;
        TextRendering::TextUnitPtr checkBoxTextUnit_;
        AuthorWidget::Mode         mode_;
        QString                    chatName_;
        QString                    authors_;
        bool                       checked_;
        QPoint                     pos_;
        bool                       hovered_;
    };

    AuthorWidget::AuthorWidget(QWidget *_parent)
        : QWidget(_parent)
        , d(std::make_unique<AuthorWidget_p>())
    {
        setFixedSize(d->getSize());
        setMouseTracking(true);

        const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        const auto nameFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));

        d->checkBoxTextUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("popup_window", "show author"));
        d->checkBoxTextUnit_->init(nameFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        d->checked_ = get_gui_settings()->get_value(setting_show_forward_author, true);
        d->hovered_ = false;
    }

    AuthorWidget::~AuthorWidget()
    {

    }

    void AuthorWidget::setAuthors(const QSet<QString> &_authors)
    {
        d->authors_.clear();
        for (auto it = _authors.begin(); it != _authors.end(); ++it)
        {
            if (it != _authors.begin())
                d->authors_ += qsl(", ");
            d->authors_ += *it;
        }

        update();
    }

    void AuthorWidget::setChatName(const QString &_chatName)
    {
        d->chatName_ = _chatName;
        update();
    }

    void AuthorWidget::setMode(AuthorWidget::Mode _mode)
    {
        d->mode_ = _mode;
        update();
    }

    bool AuthorWidget::isAuthorsEnabled() const
    {
        return d->checked_;
    }

    void AuthorWidget::paintEvent(QPaintEvent *_e)
    {
        QPainter p(this);

        const auto textColor = Styling::getParameters().getColor(d->checked_ ? Styling::StyleVariable::TEXT_SOLID : Styling::StyleVariable::BASE_PRIMARY);

        if (hasFocus() || d->hovered_)
        {
            QColor color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
            p.fillRect(rect(), color);
        }

        if (!d->chatNameUnit_ && d->mode_ == Mode::Group)
        {
            static auto nameFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));
            d->chatNameUnit_ = TextRendering::MakeTextUnit(d->chatName_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            d->chatNameUnit_->init(nameFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }
        if (!d->authorUnit_)
        {
            static auto authorFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
            d->authorUnit_ = TextRendering::MakeTextUnit(d->mode_ == Mode::Channel ? d->chatName_ : d->authors_, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            d->authorUnit_->init(authorFont, textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        }

        d->checkBoxTextUnit_->getHeight(width());
        const int checkBoxTextLeftOffset = d->getCheckBoxRect().left() - d->getCheckBoxTextRightMargin() - d->checkBoxTextUnit_->desiredWidth();

        d->checkBoxTextUnit_->setOffsets(checkBoxTextLeftOffset, d->getCheckBoxTextBaselineTopMargin());
        d->checkBoxTextUnit_->setColor(textColor);
        d->checkBoxTextUnit_->draw(p, TextRendering::VerPosition::BASELINE);

        const int maxTextWidth = checkBoxTextLeftOffset - d->getTextLeftMargin() - Utils::scale_value(12);

        if (d->mode_ == Mode::Group)
        {
            d->authorUnit_->setOffsets(d->getTextLeftMargin(), d->getAuthorTopMargin());
            d->authorUnit_->getHeight(maxTextWidth);
            d->authorUnit_->setColor(textColor);
            d->authorUnit_->draw(p);

            d->chatNameUnit_->setOffsets(d->getTextLeftMargin(), d->getChatNameTopMargin());
            d->chatNameUnit_->getHeight(maxTextWidth);
            d->chatNameUnit_->setColor(textColor);
            d->chatNameUnit_->draw(p);
        }
        else if (d->mode_ == Mode::Channel || d->mode_ == Mode::Contact)
        {
            d->authorUnit_->setOffsets(d->getTextLeftMargin(), d->getCheckBoxTextBaselineTopMargin());
            d->authorUnit_->getHeight(maxTextWidth);
            d->authorUnit_->setColor(textColor);
            d->authorUnit_->draw(p, TextRendering::VerPosition::BASELINE);
        }

        const auto cbSize = QSize(20, 20);
        const auto state = d->checked_ ? CheckBox::Activity::ACTIVE : CheckBox::Activity::NORMAL;
        const auto& img = Ui::CheckBox::getCheckBoxIcon(state);

        p.drawPixmap(d->getCheckBoxRect(), img);
    }

    void AuthorWidget::mousePressEvent(QMouseEvent *_e)
    {
        d->pos_ = _e->pos();
        QWidget::mousePressEvent(_e);
    }

    void AuthorWidget::mouseReleaseEvent(QMouseEvent *_e)
    {
        if (rect().contains(_e->pos()) && rect().contains(d->pos_))
        {
            d->checked_ = !d->checked_;
            get_gui_settings()->set_value(setting_show_forward_author, d->checked_);
            update();
        }
        QWidget::mouseReleaseEvent(_e);
    }

    void AuthorWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        if (rect().contains(_e->pos()))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        updateHover(_e->pos());

        QWidget::mouseMoveEvent(_e);
    }

    void AuthorWidget::leaveEvent(QEvent* _e)
    {
        updateHover(QPoint(-1, -1));

        QWidget::leaveEvent(_e);
    }

    void AuthorWidget::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Space || _e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            d->checked_ = !d->checked_;
            get_gui_settings()->set_value(setting_show_forward_author, d->checked_);
            update();
        }

        QWidget::keyPressEvent(_e);
    }

    void AuthorWidget::updateHover(const QPoint& _pos)
    {
        d->hovered_ = false;

        if (rect().contains(_pos))
            d->hovered_ = true;

        update();
    }


    SelectContactsWidget::SelectContactsWidget(const QString& _labelText, QWidget* _parent)
        : QDialog(_parent)
        , regim_(Logic::MembersWidgetRegim::SHARE)
        , chatMembersModel_(nullptr)
        , mainWidget_(new QWidget(this))
        , sortCL_(true)
        , handleKeyPressEvents_(true)
        , enableAuthorWidget_(false)
        , maximumSelectedCount_(-1)
        , searchModel_(nullptr)
        , cancelButton_(nullptr)
        , acceptButton_(nullptr)
    {
        init(_labelText);
    }

    SelectContactsWidget::SelectContactsWidget(Logic::CustomAbstractListModel* _chatMembersModel, int _regim, const QString& _labelText,
        const QString& _buttonText, QWidget* _parent, bool _handleKeyPressEvents/* = true*/,
        Logic::AbstractSearchModel* searchModel /*= nullptr*/, bool _enableAuthorWidget)
        : QDialog(_parent)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , mainWidget_(new QWidget(this))
        , handleKeyPressEvents_(_handleKeyPressEvents)
        , enableAuthorWidget_(_enableAuthorWidget)
        , maximumSelectedCount_(-1)
        , searchModel_(searchModel)
        , avatarsArea_(nullptr)
        , cancelButton_(nullptr)
        , acceptButton_(nullptr)
    {
        init(_labelText, _buttonText);
    }

    void SelectContactsWidget::init(const QString& _labelText, const QString& _buttonText)
    {
        globalLayout_ = Utils::emptyVLayout(mainWidget_);
        mainWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        mainWidget_->setFixedWidth(Utils::scale_value(DIALOG_WIDTH));
        mainWidget_->installEventFilter(this);

        if (enableAuthorWidget_)
        {
            authorWidget_ = new AuthorWidget(this);
            globalLayout_->addSpacing(Utils::scale_value(10));
            Testing::setAccessibleName(authorWidget_, qsl("AS scfgc authorWidget_"));
            globalLayout_->addWidget(authorWidget_);
            focusWidget_[authorWidget_] = FocusPosition::Author;
        }

        searchWidget_ = new SearchWidget(this, Utils::scale_value(search_padding_hor), Utils::scale_value(search_padding_ver));
        searchWidget_->setEditEventFilter(this);
        Testing::setAccessibleName(searchWidget_, qsl("AS scfgc CreateGroupChat"));
        globalLayout_->addWidget(searchWidget_);
        focusWidget_[searchWidget_] = FocusPosition::Search;

        contactList_ = new ContactListWidget(this, (Logic::MembersWidgetRegim)regim_, chatMembersModel_, searchModel_);
        Testing::setAccessibleName(contactList_, qsl("AS scfgc contactList_"));
        contactList_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        contactList_->setWidthForDelegate(Utils::scale_value(DIALOG_WIDTH));
        searchModel_ = contactList_->getSearchModel();

        clHost_ = new QWidget(this);
        auto clLayout = Utils::emptyHLayout(clHost_);

        clSpacer_ = new QSpacerItem(0, calcListHeight(), QSizePolicy::Fixed, QSizePolicy::Expanding);
        clLayout->addSpacerItem(clSpacer_);
        clLayout->addWidget(contactList_);
        globalLayout_->addWidget(clHost_);

        // TODO : use SetView here
        auto dialogParent = isVideoConference() ? parentWidget() : Utils::InterConnector::instance().getMainWindow();
        mainDialog_ = std::make_unique<Ui::GeneralDialog>(mainWidget_, dialogParent, handleKeyPressEvents_, !isShareMode());
        mainDialog_->installEventFilter(this);
        mainDialog_->addLabel(_labelText);

        if (isShareMode())
        {
            avatarsArea_ = new AvatarsArea(clHost_);
            avatarsArea_->hide();
            connect(avatarsArea_, &AvatarsArea::removed, this, [this]()
            {
                contactList_->getView()->update();

                if (avatarsArea_->avatars() < 1)
                {
                    avatarsArea_->hide();

                    const auto curFocus = currentFocusPosition();

                    mainDialog_->setButtonActive(false);

                    if (curFocus == FocusPosition::Avatars || curFocus == FocusPosition::Accept)
                        searchWidget_->setFocus();
                }
            });
            connect(avatarsArea_, &AvatarsArea::showed, this, [this]()
            {
                auto selectedIndexes = contactList_->getView()->selectionModel()->selectedIndexes();
                if (!selectedIndexes.isEmpty())
                    contactList_->getView()->scrollTo(selectedIndexes.first());
            });
            connect(avatarsArea_, &AvatarsArea::resized, this, &SelectContactsWidget::recalcAvatarArea);

            focusWidget_[avatarsArea_] = FocusPosition::Avatars;
        }

        if (isPopupWithCloseBtn())
        {
            cancelButton_ = mainDialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "CLOSE"), true);
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }
        else if (isPopupWithCancelBtn())
        {
            cancelButton_ = mainDialog_->addCancelButton(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), true);
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }
        else if (!Logic::is_members_regim(regim_) && !Logic::is_video_conference_regim(regim_))
        {
            auto buttonPair = mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), _buttonText, false, true, true);
            acceptButton_ = buttonPair.first;
            cancelButton_ = buttonPair.second;
            focusWidget_[acceptButton_] = FocusPosition::Accept;
            focusWidget_[cancelButton_] = FocusPosition::Cancel;
        }

        Testing::setAccessibleName(mainDialog_.get(), qsl("AS scfgc SelectContactsWidget"));

        connect(contactList_, &ContactListWidget::searchEnd, searchWidget_, &SearchWidget::searchCompleted);
        connect(contactList_, &ContactListWidget::itemSelected, this, &SelectContactsWidget::itemClicked);

        connect(searchWidget_, &SearchWidget::escapePressed, this, &SelectContactsWidget::escapePressed);
        connect(searchWidget_, &SearchWidget::enterPressed, this, &SelectContactsWidget::enterPressed);
        contactList_->connectSearchWidget(searchWidget_);

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
        {
            connect(contactList_, &ContactListWidget::itemSelected, this, &QDialog::reject, Qt::QueuedConnection);
            connect(contactList_, &ContactListWidget::itemSelected, GetDispatcher(), []()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat, { { "From", "CL_Popup" } });
            });
        }

        for (auto&[w, _] : focusWidget_)
            w->installEventFilter(this);
    }

    SelectContactsWidget::~SelectContactsWidget()
    {
    }

    bool SelectContactsWidget::isCheckboxesVisible() const
    {
        if (Logic::is_members_regim(regim_))
        {
            return false;
        }

        if (isShareMode())
        {
            return false;
        }

        return true;
    }

    bool SelectContactsWidget::isShareMode() const
    {
        return (regim_ == Logic::MembersWidgetRegim::SHARE);
    }

    bool SelectContactsWidget::isVideoConference() const
    {
        return (regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE);
    }

    bool SelectContactsWidget::isPopupWithCloseBtn() const
    {
        return  regim_ == Logic::MembersWidgetRegim::IGNORE_LIST ||
                regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP;
    }

    bool SelectContactsWidget::isPopupWithCancelBtn() const
    {
        return regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT;
    }

    void SelectContactsWidget::updateFocus(bool _order)
    {
        auto reason = Qt::FocusReason::TabFocusReason;
        const auto begin = static_cast<int>(FocusPosition::begin);
        const auto end = static_cast<int>(FocusPosition::end);
        auto pos = static_cast<int>(currentFocusPosition());

        while (1)
        {
            if (pos == begin)
            {
                ++pos;
            }
            else
            {
                if (_order)
                    pos = (pos == end - 1) ? begin + 1 : pos + 1;
                else
                    pos = (pos == begin + 1) ? end - 1 : pos - 1;
            }

            switch (static_cast<FocusPosition>(pos))
            {
            case FocusPosition::Search:
                searchWidget_->setFocus(reason);
                return;
            case FocusPosition::Accept:
                if (acceptButton_ && acceptButton_->isEnabled())
                {
                    acceptButton_->setFocus(reason);
                    return;
                }
                break;
            case FocusPosition::Cancel:
                cancelButton_->setFocus(reason);
                return;
            case FocusPosition::Avatars:
                if (avatarsArea_ && avatarsArea_->avatars() > 0)
                {
                    avatarsArea_->setFocus(reason);
                    return;
                }
                break;
            case FocusPosition::Author:
                if (authorWidget_)
                {
                    authorWidget_->setFocus(reason);
                    return;
                }
                break;
            default:
                return;
            }
        }
    }

    void SelectContactsWidget::updateSpacer()
    {
        clSpacer_->changeSize(0, calcListHeight(), QSizePolicy::Fixed, QSizePolicy::Expanding);
        clHost_->layout()->invalidate();
    }

    SelectContactsWidget::FocusPosition SelectContactsWidget::currentFocusPosition() const
    {
        auto fw = qApp->focusWidget();
        while (fw)
        {
            auto curFocus = focusWidget_.find(fw);
            if (curFocus != focusWidget_.end())
                return curFocus->second;

            fw = fw->parentWidget();
        }

        return FocusPosition::begin;
    }

    void SelectContactsWidget::searchEnd()
    {
        emit contactList_->searchEnd();
        // contactList_->setSearchMode(false);
    }

    void SelectContactsWidget::escapePressed()
    {
        mainDialog_->close();
    }

    void SelectContactsWidget::enterPressed()
    {
        if (isShareMode())
            return;

        if (searchModel_->getSearchPattern().isEmpty() &&
            regim_ != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP &&
            regim_ != Logic::MembersWidgetRegim::SELECT_MEMBERS)
            mainDialog_->accept();
    }

    void SelectContactsWidget::recalcAvatarArea()
    {
        if (avatarsArea_)
        {
            avatarsArea_->move(0, clHost_->height() - avatarsArea_->height());
            avatarsArea_->setFixedWidth(clHost_->width());
        }
    }

    int SelectContactsWidget::calcListHeight() const
    {
        const auto parentWdg = isVideoConference() ? parentWidget() : Utils::InterConnector::instance().getMainWindow();
        const auto newHeight = ::Ui::ItemLength(false, heightPartOfMainWindowForFullView, 0, parentWdg);

        auto extraHeight = searchWidget_->sizeHint().height();
        if (!Logic::is_members_regim(regim_))
            extraHeight += Utils::scale_value(42);

        const auto itemHeight = ::Ui::GetContactListParams().itemHeight();
        int count = (newHeight - extraHeight) / itemHeight;

        int clHeight = (count + 0.5) * itemHeight;
        if (Logic::is_members_regim(regim_))
        {
            const auto ignoreModel = qobject_cast<const Logic::IgnoreMembersModel*>(chatMembersModel_);
            if (ignoreModel)
                count = ignoreModel->getMembersCount();
            else
                count = chatMembersModel_->rowCount();

            if (count != 0)
                clHeight = std::min(clHeight, count * itemHeight + Utils::scale_value(1));
        }

        return clHeight;
    }

    bool SelectContactsWidget::show()
    {
        Logic::getContactListModel()->setIsWithCheckedBox(!isShareMode());

        if (!isPopupWithCloseBtn() && !isPopupWithCancelBtn())
            mainDialog_->setButtonActive(!Logic::getContactListModel()->GetCheckedContacts().empty());

        searchWidget_->setFocus();

        auto result = true;

        if (isShareMode())
            Logic::getContactListModel()->clearChecked();

        result = mainDialog_->showInCenter();

        if (result)
        {
            if (Logic::getContactListModel()->GetCheckedContacts().empty())
            {
                result = false;
            }
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }

        Logic::getContactListModel()->setIsWithCheckedBox(false);
        searchWidget_->clearInput();
        return result;
    }

    void SelectContactsWidget::setAuthors(const QSet<QString> &_authors)
    {
        if (authorWidget_)
            authorWidget_->setAuthors(_authors);
    }

    void SelectContactsWidget::setChatName(const QString &_chatName)
    {
        if (authorWidget_)
            authorWidget_->setChatName(_chatName);
    }

    void SelectContactsWidget::setAuthorWidgetMode(AuthorWidget::Mode _mode)
    {
        if (authorWidget_)
            authorWidget_->setMode(_mode);
    }

    bool SelectContactsWidget::isAuthorsEnabled() const
    {
        return authorWidget_ && authorWidget_->isAuthorsEnabled();
    }

    void SelectContactsWidget::UpdateViewForIgnoreList(bool _isEmptyIgnoreList)
    {
        contactList_->setEmptyIgnoreLabelVisible(_isEmptyIgnoreList);
        updateSpacer();
    }

    void SelectContactsWidget::UpdateMembers()
    {
        mainDialog_->update();
    }

    void SelectContactsWidget::setMaximumSelectedCount(int number)
    {
        maximumSelectedCount_ = number;
    }

    void SelectContactsWidget::setChatAimId(const QString& _chatId)
    {
        chatAimId_ = _chatId;
    }

    void Ui::SelectContactsWidget::rewindToTop()
    {
        contactList_->rewindToTop();
    }

    bool SelectContactsWidget::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::ShortcutOverride)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                keyEvent->accept();
                return true;
            }
        }
        else if (_event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                keyEvent->accept();
                updateFocus(keyEvent->key() == Qt::Key_Tab ? true : false);
                return true;
            }
        }
        else if (_obj == mainWidget_ && _event->type() == QEvent::Resize)
        {
            recalcAvatarArea();
        }

        return false;
    }

    void SelectContactsWidget::UpdateContactList()
    {
        contactList_->update();
    }

    void SelectContactsWidget::reject()
    {
        if (mainDialog_)
        {
            mainDialog_->reject();
        }
    }

}
