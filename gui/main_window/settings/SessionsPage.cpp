#include "stdafx.h"

#include "SessionsPage.h"
#include "controls/GeneralCreator.h"
#include "controls/LabelEx.h"
#include "controls/CustomButton.h"
#include "controls/TransparentScrollBar.h"
#include "controls/ContextMenu.h"

#include "controls/TextWidget.h"

#include "utils/utils.h"
#include "utils/gui_coll_helper.h"
#include "previewer/toast.h"
#include "styles/ThemeParameters.h"

#include "core_dispatcher.h"
#include "fonts.h"

namespace
{
    int leftMargin() { return Utils::scale_value(20); }
    int buttonHorMargin() { return Utils::scale_value(16); }
    int buttonBotMargin() { return Utils::scale_value(12); }
    QSize closeIconSize() { return Utils::scale_value(QSize(20, 20)); }
    QSize closeButtonSize() { return Utils::scale_value(QSize(24, 24)); }
    int totalHorMargins() { return leftMargin() + 2 * buttonBotMargin() + closeButtonSize().width(); }

    QString getDateString(const Data::SessionInfo& _info)
    {
        if (_info.isCurrent())
            return QT_TRANSLATE_NOOP("settings", "Active session");

        auto res = Utils::GetTranslator()->formatDifferenceToNow(_info.getStartedTime(), false);
        if (!res.isEmpty())
            res[0] = res.at(0).toUpper();
        return res;
    }

    QString sep()
    {
        static QString sep = QChar::Space % QChar(0x2022) % QChar::Space;
        return sep;
    }

    template <typename T>
    QString join(std::initializer_list<QString> _list, const T& _sep)
    {
        QStringList res(_list);
        res.removeAll(QString());
        return res.join(_sep);
    }
}

namespace Ui
{
    SessionWidget::SessionWidget(const Data::SessionInfo& _info, QWidget* _parent)
        : SimpleListItem(_parent)
    {
        init(_info);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void SessionWidget::init(const Data::SessionInfo& _info)
    {
        info_ = _info;

        if (!caption_ || !text_)
        {
            caption_ = new TextWidget(this, getName(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
            TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
            params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
            caption_->init(params);
            caption_->setAttribute(Qt::WA_TransparentForMouseEvents);
            caption_->move(leftMargin(), Utils::scale_value(12));

            text_ = new TextWidget(this, getText(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
            params.setFonts(Fonts::appFontScaled(15, Fonts::FontWeight::Normal));
            params.color_ = Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
            text_->init(params);
            text_->setAttribute(Qt::WA_TransparentForMouseEvents);
            text_->move(leftMargin(), Utils::scale_value(44));

            onResize();
        }
        else
        {
            caption_->setText(getName());
            text_->setText(getText());
        }
    }

    QString SessionWidget::getName() const
    {
        return join( { info_.getOS(), info_.getClientName(), (build::is_debug() ? (u'[' % info_.getHash() % u']') : QString()) }, QChar::Space);
    }

    QString SessionWidget::getText() const
    {
        return join({ getDateString(info_), info_.getLocation(), u"IP: " % info_.getIp() }, sep());
    }

    QSize SessionWidget::sizeHint() const
    {
        if (!caption_ || !text_)
            return SimpleListItem::sizeHint();

        const auto w = std::max(caption_->getDesiredWidth(), text_->getDesiredWidth()) + totalHorMargins();
        const auto h = Utils::scale_value(12) + caption_->height() + Utils::scale_value(12) + text_->height() + Utils::scale_value(16);
        return QSize(w, h);
    }

    void SessionWidget::paintEvent(QPaintEvent*)
    {
        if (!info_.isCurrent() && isHovered())
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

            p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

            const auto x = width() - buttonHorMargin() - closeIconSize().width();
            const auto y = (height() - closeIconSize().height()) / 2;
            static Utils::StyledPixmap pm = Utils::StyledPixmap(qsl(":/history_icon"), closeIconSize(), Styling::Buttons::hoverColorKey());
            p.drawPixmap(x, y, pm.actualPixmap());
        }
    }

    void SessionWidget::resizeEvent(QResizeEvent* _e)
    {
        if (width() != _e->oldSize().width())
            onResize();
    }

    void SessionWidget::onResize()
    {
        const auto maxWidth = width() - totalHorMargins();
        caption_->setMaxWidthAndResize(maxWidth);
        text_->setMaxWidthAndResize(maxWidth);

        setMinimumHeight(sizeHint().height());
    }


    SessionsPage::SessionsPage(QWidget* _parent)
        : QWidget(_parent)
        , activeSessionsWidget_(new QWidget(this))
    {
        auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
        scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));
        scrollArea->setWidgetResizable(true);
        Utils::grabTouchWidget(scrollArea->viewport(), true);

        auto mainWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget);
        Utils::ApplyStyle(mainWidget, Styling::getParameters().getGeneralSettingsQss());

        auto mainLayout = Utils::emptyVLayout(mainWidget);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

        scrollArea->setWidget(mainWidget);

        auto layout = Utils::emptyHLayout(this);
        layout->addWidget(scrollArea);

        GeneralCreator::addHeader(this, mainLayout, QT_TRANSLATE_NOOP("settings", "Current session"), 20);

        mainLayout->addSpacing(Utils::scale_value(12));

        auto curContainer = new SimpleListWidget(Qt::Vertical);
        connect(curContainer, &SimpleListWidget::rightClicked, this, &SessionsPage::copyCurSessionInfo);
        currentSession_ = new SessionWidget(Data::SessionInfo::emptyCurrent());
        currentSession_ ->setHoverStateCursor(Qt::ArrowCursor);
        curContainer->addItem(currentSession_);
        mainLayout->addWidget(curContainer);
        Testing::setAccessibleName(currentSession_, qsl("AS SessionsList currentSession"));

        auto activeSessionsLayout = Utils::emptyVLayout(activeSessionsWidget_);
        activeSessionsLayout->setAlignment(Qt::AlignTop);
        activeSessionsLayout->addSpacing(Utils::scale_value(24));
        {
            auto resetAllLink = new LabelEx(this);
            resetAllLink->setFont(Fonts::appFontScaled(15));
            resetAllLink->setColors(Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION },
                Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_HOVER },
                Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE });
            resetAllLink->setText(QT_TRANSLATE_NOOP("settings", "Close all other sessions"));
            resetAllLink->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(resetAllLink, qsl("AS SessionsList closeAll"));
            Utils::grabTouchWidget(resetAllLink);

            auto linkLayout = Utils::emptyHLayout();
            linkLayout->addSpacing(leftMargin());
            linkLayout->addWidget(resetAllLink);
            activeSessionsLayout->addLayout(linkLayout);

            connect(resetAllLink, &LabelEx::clicked, this, &SessionsPage::onResetAllClicked);
        }

        activeSessionsLayout->addSpacing(Utils::scale_value(40));
        GeneralCreator::addHeader(activeSessionsWidget_, activeSessionsLayout, QT_TRANSLATE_NOOP("settings", "Active sessions"), 20);
        activeSessionsLayout->addSpacing(Utils::scale_value(24));

        sessionsList_ = new SimpleListWidget(Qt::Vertical);
        sessionsList_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        connect(sessionsList_, &SimpleListWidget::clicked, this, &SessionsPage::resetSession);
        connect(sessionsList_, &SimpleListWidget::rightClicked, this, &SessionsPage::copySessionInfo);
        activeSessionsLayout->addWidget(sessionsList_);

        mainLayout->addWidget(activeSessionsWidget_);
        mainLayout->addStretch();

        connect(GetDispatcher(), &core_dispatcher::activeSessionsList, this, &SessionsPage::updateSessions);
        connect(GetDispatcher(), &core_dispatcher::sessionResetResult, this, &SessionsPage::onSessionReset);
    }

    void SessionsPage::updateSessions(const std::vector<Data::SessionInfo>& _sessions)
    {
        im_assert(!_sessions.empty());
        if (_sessions.empty())
            return;

        setUpdatesEnabled(false);

        sessionsList_->clear();
        for (const auto& s : _sessions)
        {
            if (s.isCurrent())
            {
                currentSession_->init(s);
            }
            else
            {
                auto session = new SessionWidget(s);
                Testing::setAccessibleName(session, qsl("AS SessionsList session") % s.getOS());
                sessionsList_->addItem(session);
            }
        }
        activeSessionsWidget_->setVisible(sessionsList_->count() > 0);

        updateGeometry();
        setUpdatesEnabled(true);
    }

    void SessionsPage::onSessionReset(const QString&, bool _success)
    {
        Utils::showTextToastOverContactDialog(_success ? QT_TRANSLATE_NOOP("settings", "Session closed") : QT_TRANSLATE_NOOP("settings", "Error occurred"));

        if (_success)
            GetDispatcher()->requestSessionsList();
    }

    void SessionsPage::onResetAllClicked()
    {
        const auto confirm = Utils::GetDeleteConfirmation(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Close"),
            QT_TRANSLATE_NOOP("popup_window", "All sessions except the current one will be closed"),
            QT_TRANSLATE_NOOP("popup_window", "Close sessions"),
            nullptr);

        if (confirm)
            resetSessionImpl(QString());
    }

    void SessionsPage::resetSession(int _idx)
    {
        if (auto w = qobject_cast<SessionWidget*>(sessionsList_->itemAt(_idx)))
        {
            const auto confirm = Utils::GetDeleteConfirmation(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Close"),
                QT_TRANSLATE_NOOP("popup_window", "Session %1 will be closed").arg(w->getName()),
                QT_TRANSLATE_NOOP("popup_window", "Close session"),
                nullptr);

            if (confirm)
                resetSessionImpl(w->getHash());
        }
    }

    void SessionsPage::resetSessionImpl(const QString& _hash)
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("hash", _hash);
        GetDispatcher()->post_message_to_core(_hash.isEmpty() ? "sessions/resetall" : "sessions/reset", collection.get());
    }

    void SessionsPage::copyCurSessionInfo()
    {
        copySessionInfoImpl(currentSession_);
    }

    void SessionsPage::copySessionInfo(int _idx)
    {
        if (auto w = qobject_cast<SessionWidget*>(sessionsList_->itemAt(_idx)))
            copySessionInfoImpl(w);
    }

    void SessionsPage::copySessionInfoImpl(SessionWidget* _widget)
    {
        if (!_widget)
            return;

        auto menu = new ContextMenu(this, 24);
        menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy session info"), _widget, [_widget]()
        {
            QApplication::clipboard()->setText(_widget->getName() % QChar::LineFeed % _widget->getText());
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("settings", "Session info copied"));
        });
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        menu->popup(QCursor::pos());
        Testing::setAccessibleName(menu, qsl("AS General contextMenu"));
    }
}