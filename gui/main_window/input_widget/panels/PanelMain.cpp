#include "stdafx.h"

#include "PanelMain.h"
#include "EditMessageWidget.h"

#include "../SubmitButton.h"
#include "../AttachFilePopup.h"
#include "../AttachFileButton.h"
#include "../HistoryTextEdit.h"

#include "core_dispatcher.h"
#include "gui_settings.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/MainPage.h"
#include "controls/CustomButton.h"
#include "styles/ThemeParameters.h"
#include "main_window/MainWindow.h"

namespace
{
    int textEditMinHeightDesigned()
    {
        return Utils::scale_value(20);
    }

    int textEditMinHeight()
    {
        static const auto minH = std::max(Emoji::GetEmojiSizeForCurrentUiScale(), textEditMinHeightDesigned());
        return minH;
    }

    int minHeightEditTopOffset()
    {
        static const auto offset = Emoji::GetEmojiSizeForCurrentUiScale() - textEditMinHeightDesigned();
        return offset;
    }

    int getInputMinHeight()
    {
        return Ui::getDefaultInputHeight() - Ui::getVerMargin();
    }

    constexpr std::chrono::milliseconds heightAnimDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds sideAnimDuration() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds buttonAnimDuration() noexcept { return std::chrono::milliseconds(200); }

    int normalIconSize()
    {
        return Utils::scale_value(32);
    }

    int minIconSize()
    {
        return Utils::scale_value(20);
    }

    class ButtonEffect : public QGraphicsOpacityEffect
    {
    public:
        ButtonEffect(QWidget* _parent) : QGraphicsOpacityEffect(_parent) {}

    protected:
        void draw(QPainter* _painter)
        {
            const auto r = boundingRect();
            const auto val = opacity();
            const auto range = normalIconSize() - minIconSize();
            const auto iconWidth = val * range + minIconSize();
            QRectF pmRect(0, 0, iconWidth, iconWidth);
            pmRect.moveCenter(r.center());

            _painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            _painter->setOpacity(val);

            const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates);
            _painter->drawPixmap(pmRect, pixmap, pixmap.rect());
        }
    };

    void animateButton(QWidget* _button, const Ui::InputSide::SideState _state)
    {
        const auto startOpacity = _state == Ui::InputSide::SideState::Normal ? 0.0 : 1.0;
        const auto endOpacity = _state == Ui::InputSide::SideState::Normal ? 1.0 : 0.0;

        auto effect = new ButtonEffect(_button);
        effect->setOpacity(startOpacity);
        _button->setGraphicsEffect(effect);
        _button->setVisible(true);

        auto opacityAnim = new QPropertyAnimation(effect, "opacity");
        opacityAnim->setDuration(buttonAnimDuration().count());
        opacityAnim->setEasingCurve(QEasingCurve::InOutQuad);
        opacityAnim->setStartValue(startOpacity);
        opacityAnim->setEndValue(endOpacity);
        QObject::connect(opacityAnim, &QPropertyAnimation::finished, _button, [_button, _state, effect]()
        {
            if (_button->graphicsEffect() == effect)
            {
                _button->setVisible(_state == Ui::InputSide::SideState::Normal);
                _button->setGraphicsEffect(nullptr);
                _button->update();
            }
        });
        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

namespace Ui
{
    TextEditViewport::TextEditViewport(QWidget* _parent, InputPanelMain* _panelMain)
        : QWidget(_parent)
        , textEdit_(_panelMain->getTextEdit())
        , panelMain_(_panelMain)
    {
        assert(panelMain_);
        assert(textEdit_);

        textEdit_->setParent(this);
        textEdit_->move(QPoint());

        const auto margin = (minHeight() - textEditMinHeight()) / 2;
        textEdit_->document()->setDocumentMargin(margin);
    }

    void TextEditViewport::adjustHeight(const int _curEditHeight)
    {
        const auto h = std::clamp(_curEditHeight, minHeight(), maxHeight());
        setFixedHeight(h);
        adjustTextEditPos();
    }

    void TextEditViewport::resizeEvent(QResizeEvent* _event)
    {
        textEdit_->setFixedWidth(width() + textEdit_->document()->documentMargin());
        adjustTextEditPos();
    }

    int TextEditViewport::topOffset() const
    {
        if (platform::is_apple() && height() == minHeight())
            return minHeightEditTopOffset();

        return 0;
    }

    int TextEditViewport::minHeight() const
    {
        return panelMain_->viewportMinHeight();
    }

    int TextEditViewport::maxHeight() const
    {
        return panelMain_->viewportMaxHeight();
    }

    void TextEditViewport::adjustTextEditPos()
    {
        textEdit_->move(-textEdit_->document()->documentMargin(), topOffset());
    }



    InputSide::InputSide(QWidget* _parent)
        : QWidget(_parent)
        , state_(SideState::Normal)
    {
        setFixedWidth(getStateWidth(state_));
    }

    InputSide::SideState InputSide::getState() const noexcept
    {
        return state_;
    }

    void InputSide::resizeAnimated(const SideState _state)
    {
        if (state_ != _state)
        {
            state_ = _state;

            anim_.start([this]()
            {
                setFixedWidth(anim_.current());
            }, width(), getStateWidth(_state), sideAnimDuration().count(), anim::sineInOut);
        }
    }

    int InputSide::getStateWidth(const SideState _state) const
    {
        return _state == SideState::Normal ? getHorMargin() : getEditHorMarginLeft();
    }



    InputPanelMain::InputPanelMain(QWidget* _parent, SubmitButton* _submit)
        : QWidget(_parent)
        , topHost_(new QWidget(this))
        , emptyTop_(new QWidget(this))
        , edit_(nullptr)
        , buttonSubmit_(_submit)
        , curEditHeight_(viewportMinHeight())
        , forceSendButton_(false)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto rootLayout = Utils::emptyVLayout(this);
        Testing::setAccessibleName(topHost_, qsl("AS ChatInput topHost"));
        rootLayout->addWidget(topHost_);
        rootLayout->setAlignment(Qt::AlignTop);

        auto topRowLayout = Utils::emptyVLayout(topHost_);
        topRowLayout->setAlignment(Qt::AlignTop);

        emptyTop_->setFixedHeight(getVerMargin());
        Testing::setAccessibleName(emptyTop_, qsl("AS ChatInput emptyTop"));
        topRowLayout->addWidget(emptyTop_);
        topHost_->setFixedHeight(getVerMargin());

        currentTopWidget_ = emptyTop_;

        const QSize buttonSize(32, 32);

        bottomHost_ = new QWidget(this);
        bottomHost_->setFixedHeight(getInputMinHeight());
        auto inputRowLayout = Utils::emptyHLayout(bottomHost_);
        inputRowLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
        {
            leftSide_ = new InputSide(this);
            auto leftLayout = Utils::emptyVLayout(leftSide_);
            leftLayout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
            leftLayout->setContentsMargins(getHorSpacer(), 0, 0, 0);

            buttonAttach_ = new AttachFileButton(leftSide_);
            connect(buttonAttach_, &AttachFileButton::clicked, this, &InputPanelMain::onAttachClicked);

            Testing::setAccessibleName(buttonAttach_, qsl("AS ChatInput plusButton"));
            leftLayout->addWidget(buttonAttach_);
            leftLayout->addSpacing((getDefaultInputHeight() - buttonAttach_->height()) / 2);

            Testing::setAccessibleName(leftSide_, qsl("AS ChatInput leftHost"));
            inputRowLayout->addWidget(leftSide_);
        }

        {
            auto centerHost = new BubbleWidget(this, 0, getVerMargin(), Styling::StyleVariable::CHAT_PRIMARY);
            centerHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            centerHost->setMinimumHeight(getInputMinHeight());

            auto centerLayout = Utils::emptyHLayout(centerHost);

            auto leftLayout = Utils::emptyVLayout();
            leftLayout->setAlignment(Qt::AlignBottom);

            auto leftWidget = new ClickableWidget(this);
            leftWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            leftWidget->setFocusPolicy(Qt::NoFocus);
            leftWidget->setCursor(Qt::IBeamCursor);
            leftWidget->setFixedWidth(getInputTextLeftMargin());
            leftLayout->addWidget(leftWidget);
            leftLayout->addSpacing(getVerMargin());
            centerLayout->addLayout(leftLayout);

            {
                textEdit_ = new HistoryTextEdit(this);
                textEdit_->setFixedHeight(curEditHeight_);
                Testing::setAccessibleName(textEdit_, qsl("AS ChatInput input"));
                updateTextEditViewportMargins();

                editViewport_ = new TextEditViewport(centerHost, this);
                editViewport_->setFixedHeight(curEditHeight_);

                auto inputLayout = Utils::emptyVLayout();
                inputLayout->setAlignment(Qt::AlignBottom);
                inputLayout->addWidget(editViewport_);
                inputLayout->addSpacing(getVerMargin());

                centerLayout->addLayout(inputLayout);

                connect(leftWidget, &ClickableWidget::pressed, this, [this]()
                {
                    const auto target = textEdit_->viewport();
                    auto mousePos = target->mapFromGlobal(QCursor::pos());
                    mousePos.rx() = 1;
                    mousePos.ry() = std::clamp(mousePos.y(), 1, target->height() - 1);
                    QMouseEvent pressEvent(QEvent::MouseButtonPress, mousePos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(target, &pressEvent);
                });

                connect(leftWidget, &ClickableWidget::released, this, [this]()
                {
                    const auto target = textEdit_->viewport();
                    auto mousePos = target->mapFromGlobal(QCursor::pos());
                    mousePos.rx() = 1;
                    mousePos.ry() = std::clamp(mousePos.y(), 1, target->height() - 1);
                    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, mousePos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(target, &releaseEvent);
                });

                connect(leftWidget, &ClickableWidget::moved, this, [this]()
                {
                    if (QApplication::mouseButtons() & Qt::LeftButton)
                    {
                        const auto target = textEdit_->viewport();
                        auto mousePos = target->mapFromGlobal(QCursor::pos());
                        QMouseEvent moveEvent(QEvent::MouseMove, mousePos, Qt::LeftButton, Qt::LeftButton,
                            Qt::NoModifier);
                        QApplication::sendEvent(target, &moveEvent);
                    }
                });
            }

            buttonEmoji_ = new CustomButton(centerHost, qsl(":/smile"), buttonSize);
            buttonEmoji_->setFixedSize(Utils::scale_value(buttonSize));
            buttonEmoji_->setCustomToolTip(QT_TRANSLATE_NOOP("tooltips", "Smileys and stickers"));
            buttonEmoji_->setCheckable(true);
            buttonEmoji_->setChecked(false);
            buttonEmoji_->setFocusPolicy(Qt::TabFocus);
            buttonEmoji_->setFocusColor(focusColorPrimary());

            updateButtonColors(buttonEmoji_, InputStyleMode::Default);
            connect(buttonEmoji_, &CustomButton::clicked, this, [this]()
            {
                const auto fromKb = buttonEmoji_->hasFocus();

                setFocusOnInput();
                Q_EMIT emojiButtonClicked(fromKb, QPrivateSignal());
            });

            auto emojiLayout = Utils::emptyVLayout();
            emojiLayout->setAlignment(Qt::AlignBottom);
            Testing::setAccessibleName(buttonEmoji_, qsl("AS ChatInput emojiAndStickerPicker"));
            emojiLayout->addWidget(buttonEmoji_);
            emojiLayout->addSpacing((getDefaultInputHeight() - buttonEmoji_->height()) / 2);

            centerLayout->addLayout(emojiLayout);
            centerLayout->addSpacing(Utils::scale_value(6));

            Testing::setAccessibleName(centerHost, qsl("AS ChatInput inputHost"));
            inputRowLayout->addWidget(centerHost);
        }

        {
            rightSide_ = new InputSide(this);
            Testing::setAccessibleName(rightSide_, qsl("AS ChatInput pttButton"));
            inputRowLayout->addWidget(rightSide_);
        }

        rootLayout->addWidget(bottomHost_);

        connect(textEdit_, &HistoryTextEdit::clicked, this, &InputPanelMain::onTextEditClicked);
        connect(textEdit_->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this, &InputPanelMain::onDocumentSizeChanged);
        connect(textEdit_->document(), &QTextDocument::contentsChanged, this, &InputPanelMain::onDocumentContentsChanged);

        setButtonsTabOrder();
    }

    InputPanelMain::~InputPanelMain()
    {
        animResize_.finish();
    }

    void InputPanelMain::setSubmitButton(SubmitButton* _button)
    {
        assert(_button);
        buttonSubmit_ = _button;

        setButtonsTabOrder();
    }

    void InputPanelMain::showEdit(Data::MessageBuddySptr _message, const MediaType _type)
    {
        initEdit();

        setLeftSideState(InputSide::SideState::Minimized);
        setRightSideState(InputSide::SideState::Normal);

        edit_->setMediaType(_type);
        edit_->setMessage(_message);

        setCurrentTopWidget(edit_);
    }

    void InputPanelMain::hidePopups()
    {
        setLeftSideState(InputSide::SideState::Normal);
        setRightSideState(InputSide::SideState::Normal);

        AttachFilePopup::hidePopup();

        setCurrentTopWidget(emptyTop_);
    }

    void InputPanelMain::initEdit()
    {
        if (!edit_)
        {
            edit_ = new EditMessageWidget(this);
            edit_->updateStyle(currentStyle());

            Testing::setAccessibleName(edit_, qsl("AS ChatInput edit"));
            topHost_->layout()->addWidget(edit_);

            connect(edit_, &EditMessageWidget::cancelClicked, this, [this]()
            {
                Q_EMIT editCancelClicked(QPrivateSignal());
            });

            connect(edit_, &EditMessageWidget::messageClicked, this, [this]()
            {
                Q_EMIT editedMessageClicked(QPrivateSignal());
            });

            connect(textEdit_->document(), &QTextDocument::contentsChanged, this, &InputPanelMain::editContentChanged);
        }
    }

    void InputPanelMain::onAttachClicked(const ClickType _type)
    {
        if (buttonAttach_->isActive())
        {
            AttachFilePopup::hidePopup();
        }
        else
        {
            const auto fromKB = _type == ClickType::ByKeyboard;
            AttachFilePopup::showPopup(fromKB ? AttachFilePopup::ShowMode::Persistent : AttachFilePopup::ShowMode::Normal);
            if (fromKB)
                AttachFilePopup::instance().selectFirstItem();
            setFocusOnInput();
        }
    }

    void InputPanelMain::onTextEditClicked()
    {
        if (Ui::get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default()))
            Q_EMIT Utils::InterConnector::instance().searchEnd();
    }

    void InputPanelMain::resizeAnimated(const int _height, const ResizeCondition _condition)
    {
        if (_condition == ResizeCondition::IfChanged && curEditHeight_ == _height)
            return;

        const auto th = std::clamp(_height, viewportMinHeight(), viewportMaxHeight());
        textEdit_->setFixedHeight(th);

        const auto animateStep = [this]()
        {
            setEditHeight(animResize_.current());
        };
        const auto finishStep = [this]()
        {
            const auto newPolicy = curEditHeight_ >= viewportMaxHeight() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
            if (newPolicy != textEdit_->verticalScrollBarPolicy())
                textEdit_->setVerticalScrollBarPolicy(newPolicy);
        };

        animResize_.finish();
        animResize_.start(animateStep, finishStep, curEditHeight_, _height, heightAnimDuration().count(), anim::sineInOut);

        curEditHeight_ = _height;
    }

    void InputPanelMain::setEditHeight(const int _newHeight)
    {
        editViewport_->adjustHeight(_newHeight);
        bottomHost_->setFixedHeight(editViewport_->height() + getVerMargin());

        updateGeometry();
        recalcHeight();
    }

    void InputPanelMain::onDocumentSizeChanged(const QSizeF& _newSize)
    {
        updateTextEditViewportMargins();
        scrollToBottomIfNeeded();

        const auto targetH = std::clamp(_newSize.toSize().height(), viewportMinHeight(), viewportMaxHeight());
        if (targetH != curEditHeight_)
            resizeAnimated(targetH);
    }

    void InputPanelMain::onDocumentContentsChanged()
    {
        auto scrollbar = textEdit_->verticalScrollBar();
        if (curEditHeight_ < viewportMaxHeight())
            scrollbar->setValue(scrollbar->minimum());
        else
            scrollToBottomIfNeeded();
    }

    void InputPanelMain::updateTextEditViewportMargins()
    {
        const auto isScrollVisible = textEdit_->verticalScrollBar()->isVisible();
        const auto rightMargin = Utils::scale_value(isScrollVisible ? 16 : 24) - textEdit_->document()->documentMargin();
        textEdit_->setViewportMargins(0, 0, rightMargin, 0);
    }

    void InputPanelMain::setCurrentTopWidget(QWidget* _widget)
    {
        if (_widget)
        {
            const auto widgets = topHost_->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (auto wdg : widgets)
                wdg->setVisible(wdg == _widget);

            topHost_->setFixedHeight(_widget->height());
        }
        else
        {
            topHost_->setFixedHeight(0);
        }

        currentTopWidget_ = _widget;
        recalcHeight();
    }

    int InputPanelMain::viewportMinHeight() const
    {
        return getDefaultInputHeight() - 2 * getVerMargin();
    }

    int InputPanelMain::viewportMaxHeight() const
    {
        if (const auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            if (mw->height() - mw->getTitleHeight() <= mw->minimumHeight())
            {
                const auto maxHeight = Utils::scale_value(156);
                if (currentTopWidget_)
                    return maxHeight - currentTopWidget_->height() - getVerMargin();

                return maxHeight;
            }
        }

        return Utils::scale_value(260) - 2 * getVerMargin();
    }

    void InputPanelMain::setLeftSideState(const InputSide::SideState _state)
    {
        if (_state != leftSide_->getState())
        {
            animateButton(buttonAttach_, _state);
            leftSide_->resizeAnimated(_state);
        }
    }

    void InputPanelMain::setRightSideState(const InputSide::SideState _state)
    {
        if (_state != rightSide_->getState())
        {
            animateButton(buttonSubmit_, _state);
            rightSide_->resizeAnimated(_state);
        }
    }

    void InputPanelMain::editContentChanged()
    {
        if (edit_ && currentTopWidget_ == edit_)
        {
            const auto newState = (!forceSendButton_ && textEdit_->document()->isEmpty()) ? InputSide::SideState::Minimized : InputSide::SideState::Normal;
            if (!forceSendButton_)
                setRightSideState(newState);
        }
    }

    void InputPanelMain::scrollToBottomIfNeeded()
    {
        auto scrollbar = textEdit_->verticalScrollBar();
        if (textEdit_->cursorRect().bottom() >= textEdit_->rect().bottom())
            scrollbar->setValue(scrollbar->maximum());
    }

    void InputPanelMain::setButtonsTabOrder()
    {
        QWidget* widgets[] = { buttonAttach_, textEdit_, buttonEmoji_, buttonSubmit_ };
        for (size_t i = 0; i < std::size(widgets) - 1; ++i)
        {
            assert(widgets[i]);
            assert(widgets[i + 1]);
            setTabOrder(widgets[i], widgets[i + 1]);
        }
    }

    void InputPanelMain::recalcHeight()
    {
        if (const auto h = bottomHost_->height() + topHost_->height(); h != height())
        {
            setFixedHeight(h);
            Q_EMIT panelHeightChanged(h, QPrivateSignal());
        }
    }

    void InputPanelMain::updateStyleImpl(const InputStyleMode _mode)
    {
        buttonAttach_->updateStyle(_mode);
        updateButtonColors(buttonEmoji_, _mode);

        if (edit_)
            edit_->updateStyle(_mode);
    }

    QRect InputPanelMain::getAttachFileButtonRect() const
    {
        auto rect = buttonAttach_->geometry();
        rect.moveTopLeft(leftSide_->mapToGlobal(rect.topLeft()));
        return rect;
    }

    void InputPanelMain::setUnderQuote(const bool _underQuote)
    {
        if (_underQuote)
            setCurrentTopWidget(nullptr);
        else if (!currentTopWidget_)
            setCurrentTopWidget(emptyTop_);
    }

    void InputPanelMain::forceSendButton(const bool _force)
    {
        forceSendButton_ = _force;
    }

    void InputPanelMain::setFocusOnInput()
    {
        textEdit_->setFocus();
    }

    void InputPanelMain::setFocusOnEmoji()
    {
        buttonEmoji_->setFocus(Qt::TabFocusReason);
    }

    void InputPanelMain::setFocusOnAttach()
    {
        buttonAttach_->setFocus(Qt::TabFocusReason);
    }

    void InputPanelMain::setEmojiButtonChecked(const bool _checked)
    {
        buttonEmoji_->setChecked(_checked);
    }

    HistoryTextEdit* InputPanelMain::getTextEdit() const
    {
        return textEdit_;
    }
}