#include "stdafx.h"

#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "TextEditEx.h"
#include "SemitransparentWindowAnimated.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/MainWindow.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "../controls/DialogButton.h"

namespace
{
    constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(100);
    constexpr auto defaultDialogWidth()
    {
        return 360;
    }

    int getMargin()
    {
        return Utils::scale_value(16);
    }
}

namespace Ui
{
    bool GeneralDialog::inExec_ = false;

    GeneralDialog::GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents/* = false*/, bool _fixed_size/* = true*/, bool _rejectable/* = true*/, bool _withSemiwindow/*= true*/, const Options& _options)
        : QDialog(nullptr)
        , mainWidget_(_mainWidget)
        , nextButton_(nullptr)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, animationDuration.count()))
        , headerLabelHost_(nullptr)
        , areaWidget_(nullptr)
        , ignoreKeyPressEvents_(_ignoreKeyPressEvents)
        , shadow_(true)
        , leftButtonDisableOnClicked_(false)
        , rightButtonDisableOnClicked_(false)
        , rejectable_(_rejectable)
        , withSemiwindow_(_withSemiwindow)
        , options_(_options)
    {
        setParent(semiWindow_);

        semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
        semiWindow_->hide();

        shadowHost_ = new QWidget(this);
        shadowHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        mainHost_ = new QWidget(this);
        Utils::setDefaultBackground(mainHost_);
        Testing::setAccessibleName(mainHost_, qsl("AS mainHost_"));

        if (_fixed_size)
        {
            if (options_.preferredSize_.isNull())
                mainHost_->setFixedWidth(Utils::scale_value(defaultDialogWidth()));
            else
                mainHost_->setFixedWidth(options_.preferredSize_.width());

            shadowHost_->setFixedWidth(mainHost_->width() + Ui::get_gui_settings()->get_shadow_width() * 2);
        }
        else
        {
            mainHost_->setMaximumSize(Utils::GetMainRect().size() - 2 * Utils::scale_value(QSize(8, 8)));
            shadowHost_->setMaximumWidth(mainHost_->maximumWidth() + Ui::get_gui_settings()->get_shadow_width() * 2);
        }

        auto globalLayout = Utils::emptyVLayout(mainHost_);
        globalLayout->setAlignment(Qt::AlignTop);

        headerLabelHost_ = new QWidget(mainHost_);
        headerLabelHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        headerLabelHost_->setVisible(false);
        Testing::setAccessibleName(headerLabelHost_, qsl("AS gd headerLabelHost_"));
        globalLayout->addWidget(headerLabelHost_);

        errorHost_ = new QWidget(mainHost_);
        errorHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(errorHost_, qsl("height: 1dip;"));
        errorHost_->setVisible(false);
        Testing::setAccessibleName(errorHost_, qsl("AS errorHost_"));
        globalLayout->addWidget(errorHost_);

        textHost_ = new QWidget(mainHost_);
        textHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(textHost_, qsl("height: 1dip;"));
        textHost_->setVisible(false);
        Testing::setAccessibleName(textHost_, qsl("AS textHost_"));
        globalLayout->addWidget(textHost_);

        if (mainWidget_)
        {
            Testing::setAccessibleName(mainWidget_, qsl("AS mainWidget_"));
            if constexpr (platform::is_apple())
                globalLayout->addSpacing(Utils::scale_value(5));
            globalLayout->addWidget(mainWidget_);
        }

        areaWidget_ = new QWidget(mainHost_);
        areaWidget_->setVisible(false);
        Testing::setAccessibleName(areaWidget_, qsl("AS areaWidget_"));
        globalLayout->addWidget(areaWidget_);

        bottomWidget_ = new QWidget(mainHost_);
        bottomWidget_->setVisible(false);
        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::setDefaultBackground(bottomWidget_);
        Testing::setAccessibleName(bottomWidget_, qsl("AS bottomWidget_"));
        globalLayout->addWidget(bottomWidget_);

        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);

        if (rejectable_)
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &GeneralDialog::rejectDialog);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupWindow, this, &GeneralDialog::rejectDialog);
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &GeneralDialog::updateSize);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::acceptGeneralDialog, this, &GeneralDialog::acceptDialog);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, &GeneralDialog::close);

        auto mainLayout = Utils::emptyVLayout(this);
        mainLayout->addWidget(shadowHost_);

        auto shadowLayout = Utils::emptyVLayout(shadowHost_);
        shadowLayout->addWidget(mainHost_);

        auto parentLayout = Utils::emptyVLayout(parentWidget());
        parentLayout->setAlignment(Qt::AlignHCenter);
        parentLayout->addWidget(this);

        setAttribute(Qt::WA_QuitOnClose, false);
        installEventFilter(this);

        if (auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject()))
        {
            connect(this, &GeneralDialog::finished, prevFocused, [prevFocused]()
            {
                prevFocused->setFocus();
            });
        }
        setFocus();
    }

    void GeneralDialog::rejectDialog(const Utils::CloseWindowInfo& _info)
    {
        if (std::any_of(
                options_.ignoreRejectDlgPairs_.begin(),
                options_.ignoreRejectDlgPairs_.end(),
                [&_info](const auto& _p) { return _p.first == _info.initiator_ && _p.second == _info.reason_; })
            )
            return;

        rejectDialogInternal();
    }

    void GeneralDialog::rejectDialogInternal()
    {
        if (!rejectable_)
            return;

        if (withSemiwindow_)
            semiWindow_->Hide();

        QDialog::reject();
    }

    void GeneralDialog::acceptDialog()
    {
        if (withSemiwindow_)
            semiWindow_->Hide();

        QDialog::accept();
    }

    void GeneralDialog::done(int r)
    {
        window()->setFocus();
        QDialog::done(r);
    }

    void GeneralDialog::addLabel(const QString& _text)
    {
        headerLabelHost_->setVisible(true);
        auto hostLayout = Utils::emptyHLayout(headerLabelHost_);
        hostLayout->setContentsMargins(getMargin(), 0, 0, 0);
        hostLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        auto label = new TextEmojiWidget(headerLabelHost_, Fonts::appFontScaled(22), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Utils::scale_value(32));
        label->setText(_text);
        label->setEllipsis(true);

        Testing::setAccessibleName(label, qsl("AS addlabel label ") + _text);
        hostLayout->addWidget(label);
    }

    void GeneralDialog::addText(const QString& _messageText, int _upperMarginPx)
    {
        auto text = Ui::TextRendering::MakeTextUnit(_messageText);
        text->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        const auto maxWidth = mainHost_->maximumWidth() - 2 * getMargin();
        auto label = new TextUnitLabel(textHost_, std::move(text), Ui::TextRendering::VerPosition::TOP, maxWidth);
        label->setFixedWidth(maxWidth);
        label->sizeHint();
        Testing::setAccessibleName(label, qsl("AS addtext label"));

        auto textLayout = Utils::emptyHLayout(textHost_);
        textLayout->setContentsMargins(getMargin(), _upperMarginPx, getMargin(), 0);
        textLayout->addWidget(label);
        textHost_->setVisible(true);
    }

    DialogButton* GeneralDialog::addAcceptButton(const QString& _buttonText, const bool _isEnabled)
    {
        {
            nextButton_ =  new DialogButton(bottomWidget_, _buttonText, _isEnabled ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_isEnabled);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS addaccept nextButton_"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    DialogButton* GeneralDialog::addCancelButton(const QString& _buttonText, const bool _setActive)
    {
        {
            nextButton_ = new DialogButton(bottomWidget_, _buttonText, DialogButtonRole::CANCEL);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_setActive);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS addcancel nextButton_ ") + _buttonText);

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    QPair<DialogButton*, DialogButton*> GeneralDialog::addButtonsPair(const QString& _buttonTextLeft, const QString& _buttonTextRight, bool _isActive, bool _rejectable, bool _acceptable, QWidget* _area)
    {
        QPair<DialogButton*, DialogButton*> result;
        {
            auto cancelButton = new DialogButton(bottomWidget_, _buttonTextLeft, DialogButtonRole::CANCEL);
            cancelButton->setObjectName(qsl("left_button"));
            cancelButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            if (_rejectable)
                connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            Testing::setAccessibleName(nextButton_, qsl("AS addbuttonpair cancelButton"));

            nextButton_ = new DialogButton(bottomWidget_, _buttonTextRight, DialogButtonRole::CONFIRM);
            setButtonActive(_isActive);
            nextButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            if (_acceptable)
                connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            Testing::setAccessibleName(nextButton_, qsl("AS addbuttonpair nextButton_"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(cancelButton);
            bottomLayout->addSpacing(getMargin());
            bottomLayout->addWidget(nextButton_);

            result = { nextButton_, cancelButton };
        }

        if (_area)
        {
            auto v = Utils::emptyVLayout(areaWidget_);
            Testing::setAccessibleName(_area, qsl("AS _area"));
            v->addWidget(_area);
            areaWidget_->setVisible(true);
        }

        bottomWidget_->setVisible(true);

        return result;
    }

    void GeneralDialog::setButtonsAreaMargins(const QMargins& _margins)
    {
        getBottomLayout()->setContentsMargins(_margins);
    }

    DialogButton* GeneralDialog::takeAcceptButton()
    {
        if (nextButton_)
            QObject::disconnect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept);

        return nextButton_;
    }

    void GeneralDialog::setAcceptButtonText(const QString& _text)
    {
        if (!nextButton_)
            return;

        nextButton_->setText(_text);
    }

    GeneralDialog::~GeneralDialog()
    {
        semiWindow_->close();
    }

    bool GeneralDialog::showInCenter()
    {
        if (shadow_)
        {
            shadow_ = false;
            Utils::addShadowToWindow(shadowHost_, true);
        }

        if (!semiWindow_->isVisible() && withSemiwindow_)
            semiWindow_->Show();

        show();
        inExec_ = true;
        const auto result = (exec() == QDialog::Accepted);
        inExec_ = false;
        close();

        if constexpr (platform::is_apple())
            semiWindow_->parentWidget()->activateWindow();
        if (semiWindow_->isVisible())
            semiWindow_->Hide();
        return result;
    }

    QWidget* GeneralDialog::getMainHost()
    {
        return mainHost_;
    }

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (!nextButton_)
            return;

        nextButton_->setEnabled(_active);
    }

    QHBoxLayout* GeneralDialog::getBottomLayout()
    {
        assert(bottomWidget_);
        if (bottomWidget_->layout())
        {
            assert(qobject_cast<QHBoxLayout*>(bottomWidget_->layout()));
            return qobject_cast<QHBoxLayout*>(bottomWidget_->layout());
        }

        auto bottomLayout = Utils::emptyHLayout(bottomWidget_);
        bottomLayout->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
        bottomLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        return bottomLayout;
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* _e)
    {
        QDialog::mousePressEvent(_e);

        if (rejectable_ && !mainHost_->rect().contains(mainHost_->mapFrom(this, _e->pos())))
            close();
        else
            _e->accept();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent* _e)
    {
        if (ignoredKeys_.find(static_cast<Qt::Key>(_e->key())) != ignoredKeys_.end())
            return;

        if (_e->key() == Qt::Key_Escape)
        {
            close();
            return;
        }
        else if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            if (!ignoreKeyPressEvents_ && (!nextButton_ || nextButton_->isEnabled()))
                accept();
            else
                _e->ignore();
            return;
        }

        QDialog::keyPressEvent(_e);
    }

    bool GeneralDialog::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == this)
        {
            if (_event->type() == QEvent::ShortcutOverride || _event->type() == QEvent::KeyPress)
            {
                auto keyEvent = static_cast<QKeyEvent*>(_event);
                if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
                {
                    keyEvent->accept();
                    return true;
                }
            }
        }

        return false;
    }

    void GeneralDialog::showEvent(QShowEvent *event)
    {
        QDialog::showEvent(event);
        emit shown(this);
    }

    void GeneralDialog::hideEvent(QHideEvent *event)
    {
        emit hidden(this);
        QDialog::hideEvent(event);
    }

    void GeneralDialog::moveEvent(QMoveEvent *_event)
    {
        QDialog::moveEvent(_event);
        emit moved(this);
    }

    void GeneralDialog::resizeEvent(QResizeEvent* _event)
    {
        QDialog::resizeEvent(_event);
        updateSize();

        emit resized(this);
    }

    void GeneralDialog::addError(const QString& _messageText)
    {
        errorHost_->setVisible(true);
        errorHost_->setContentsMargins(0, 0, 0, 0);
        errorHost_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

        auto textLayout = Utils::emptyVLayout(errorHost_);

        auto upperSpacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        const QString backgroundStyle = qsl("background-color: #fbdbd9; ");
        const QString labelStyle = ql1s("QWidget { ") % backgroundStyle % ql1s("border: none; padding-left: 16dip; padding-right: 16dip; padding-top: 0dip; padding-bottom: 0dip; }");

        auto upperSpacerRedUp = new QLabel();
        upperSpacerRedUp->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upperSpacerRedUp, backgroundStyle);
        Testing::setAccessibleName(upperSpacerRedUp, qsl("AS adderror upperSpacerRedUp"));
        textLayout->addWidget(upperSpacerRedUp);

        auto errorLabel = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), true, true);
        errorLabel->setContentsMargins(0, 0, 0, 0);
        errorLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorLabel->setPlaceholderText(QString());
        errorLabel->setAutoFillBackground(false);
        errorLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorLabel, labelStyle);

        errorLabel->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        Testing::setAccessibleName(errorLabel, qsl("AS adderror errorLabel"));
        textLayout->addWidget(errorLabel);

        auto errorText = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
        errorText->setContentsMargins(0, 0, 0, 0);
        errorText->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorText->setPlaceholderText(QString());
        errorText->setAutoFillBackground(false);
        errorText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorText, labelStyle);

        errorText->setText(_messageText);
        Testing::setAccessibleName(errorText, qsl("AS adderror errorText"));
        textLayout->addWidget(errorText);

        auto upperSpacerRedBottom = new QLabel();
        Utils::ApplyStyle(upperSpacerRedBottom, backgroundStyle);
        upperSpacerRedBottom->setFixedHeight(Utils::scale_value(16));
        Testing::setAccessibleName(upperSpacerRedBottom, qsl("AS adderror upperSpacerRedBottom"));
        textLayout->addWidget(upperSpacerRedBottom);

        auto upperSpacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer2);

    }

    void GeneralDialog::leftButtonClick()
    {
        if (auto leftButton = bottomWidget_->findChild<DialogButton*>(qsl("left_button")))
            leftButton->setEnabled(!leftButtonDisableOnClicked_);
        emit leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        nextButton_->setEnabled(!rightButtonDisableOnClicked_);
        emit rightButtonClicked();
    }

    void GeneralDialog::updateSize()
    {
        if (semiWindow_)
            semiWindow_->updateSize();
    }

    void GeneralDialog::setIgnoreClicksInSemiWindow(bool _ignore)
    {
        if (semiWindow_)
            semiWindow_->setForceIgnoreClicks(_ignore);
    }

    void GeneralDialog::setIgnoredKeys(const GeneralDialog::KeysContainer &_keys)
    {
        ignoredKeys_ = _keys;
    }

    bool GeneralDialog::isActive()
    {
        return inExec_;
    }

    int GeneralDialog::getHeaderHeight() const
    {
        return headerLabelHost_ ? headerLabelHost_->sizeHint().height() : 0;
    }
}
