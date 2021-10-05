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
#include "cache/stickers/stickers.h"
#include "DraftVersionWidget.h"

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
        if (!_button)
            return;

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
        im_assert(panelMain_);
        im_assert(textEdit_);

        textEdit_->setParent(this);
        textEdit_->move(QPoint());
        textEdit_->document()->setDocumentMargin((minHeight() - textEditMinHeight()) / 2);
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
        textEdit_->move(-textEdit_->document()->documentMargin() * textEdit_->getMarginScaleCorrection(), topOffset());
    }



    InputSide::InputSide(QWidget* _parent, SideState _initialState)
        : QWidget(_parent)
        , state_(_initialState)
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

            initAnimation();
            anim_->stop();
            anim_->setStartValue(width());
            anim_->setEndValue(getStateWidth(_state));
            anim_->start();
        }
    }

    int InputSide::getStateWidth(const SideState _state) const
    {
        return _state == SideState::Normal ? getHorMargin() : getEditHorMarginLeft();
    }

    void InputSide::initAnimation()
    {
        if (anim_)
            return;

        anim_ = new QVariantAnimation(this);
        anim_->setDuration(sideAnimDuration().count());
        anim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            setFixedWidth(value.toInt());
        });
    }


    InputPanelMain::InputPanelMain(QWidget* _parent, SubmitButton* _submit, InputFeatures _features)
        : QWidget(_parent)
        , IEscapeCancellable(this)
        , topHost_(new QWidget(this))
        , emptyTop_(new QWidget(this))
        , draftVersionWidget_(nullptr)
        , buttonSubmit_(_submit)
        , curEditHeight_(viewportMinHeight())
        , features_(_features)
    {
        //for sticker suggests
        Stickers::getCache().requestStickersMeta();

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

        bottomHost_ = new QWidget(this);
        bottomHost_->setFixedHeight(getInputMinHeight());
        auto inputRowLayout = Utils::emptyHLayout(bottomHost_);
        inputRowLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        {
            leftSide_ = new InputSide(this, isAttachEnabled() ? InputSide::SideState::Normal : InputSide::SideState::Minimized);
            Testing::setAccessibleName(leftSide_, qsl("AS ChatInput leftHost"));
            inputRowLayout->addWidget(leftSide_);

            if (isAttachEnabled())
            {
                auto leftLayout = Utils::emptyVLayout(leftSide_);
                leftLayout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
                leftLayout->setContentsMargins(getHorSpacer(), 0, 0, 0);

                buttonAttach_ = new AttachFileButton(leftSide_);
                connect(buttonAttach_, &AttachFileButton::clickedWithButton, this, &InputPanelMain::onAttachClicked);

                Testing::setAccessibleName(buttonAttach_, qsl("AS ChatInput plusButton"));
                leftLayout->addWidget(buttonAttach_);
                leftLayout->addSpacing((getDefaultInputHeight() - buttonAttach_->height()) / 2);
            }
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

            const QSize buttonSize(32, 32);
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
        if (animResize_)
            animResize_->stop();

        if (attachPopup_)
            attachPopup_->deleteLater();
    }

    void InputPanelMain::setSubmitButton(SubmitButton* _button)
    {
        im_assert(_button);
        buttonSubmit_ = _button;

        setButtonsTabOrder();
    }

    void InputPanelMain::showEdit(Data::MessageBuddySptr _message, const MediaType _type)
    {
        initEdit();

        setLeftSideState(InputSide::SideState::Minimized);
        setRightSideState(InputSide::SideState::Normal);

        edit_->setMediaType(_type);
        edit_->setMessage(std::move(_message));

        setCurrentTopWidget(edit_);
    }

    void InputPanelMain::showDraftVersion(const Data::Draft& _draft)
    {
        if (!draftVersionWidget_)
        {
            draftVersionWidget_ = new DraftVersionWidget(this);
            topHost_->layout()->addWidget(draftVersionWidget_);
            connect(draftVersionWidget_, &DraftVersionWidget::cancel, this, [this]()
            {
                hidePopups();
                Q_EMIT draftVersionCancelled(QPrivateSignal());
            });
            connect(draftVersionWidget_, &DraftVersionWidget::accept, this, [this]()
            {
                hidePopups();
                Q_EMIT draftVersionAccepted(QPrivateSignal());
            });
        }

        draftVersionWidget_->setDraft(_draft);
        setCurrentTopWidget(draftVersionWidget_);
    }

    void InputPanelMain::cancelEdit()
    {
        Q_EMIT editCancelClicked(QPrivateSignal());
    }

    void InputPanelMain::hidePopups()
    {
        setLeftSideState(isAttachEnabled() ? InputSide::SideState::Normal : InputSide::SideState::Minimized);
        setRightSideState(InputSide::SideState::Normal);

        hideAttachPopup();

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

            connect(edit_, &EditMessageWidget::cancelClicked, this, &InputPanelMain::cancelEdit);
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
            hideAttachPopup();
        }
        else
        {
            initAttachPopup();

            const auto fromKB = _type == ClickType::ByKeyboard;
            attachPopup_->setPersistent(fromKB);
            attachPopup_->showAnimated(getAttachFileButtonRect());

            if (fromKB)
                attachPopup_->selectFirstItem();
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

        initResizeAnimation();
        animResize_->stop();
        animResize_->setStartValue(curEditHeight_);
        animResize_->setEndValue(_height);
        animResize_->start();

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

        if (!isPttEnabled())
            checkHideSubmit();
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

            if (_widget == edit_)
            {
                escCancel_->addChild(edit_, [this]() { cancelEdit(); });
            }
            else if (_widget == draftVersionWidget_)
            {
                escCancel_->addChild(draftVersionWidget_, [this]() { Q_EMIT draftVersionCancelled(QPrivateSignal()); });
            }
            else
            {
                escCancel_->removeChild(edit_);
                escCancel_->removeChild(draftVersionWidget_);
            }
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
        if (leftSide_ && _state != leftSide_->getState())
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

    void InputPanelMain::checkHideSubmit()
    {
        if (!forceSendButton_)
        {
            const auto isEmptyOrOnlyWhitespaces = QStringView(textEdit_->toPlainText()).trimmed().isEmpty();
            const auto newState = isEmptyOrOnlyWhitespaces ? InputSide::SideState::Minimized : InputSide::SideState::Normal;
            setRightSideState(newState);
        }
    }

    void InputPanelMain::editContentChanged()
    {
        if (edit_ && currentTopWidget_ == edit_)
            checkHideSubmit();
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
        auto it = std::stable_partition(std::begin(widgets), std::end(widgets), [](auto w) { return !w; });
        if (it != std::end(widgets))
        {
            for (; it != std::prev(std::end(widgets)); ++it)
                setTabOrder(*it, *std::next(it));
        }
    }

    void InputPanelMain::initResizeAnimation()
    {
        if (animResize_)
            return;

        animResize_ = new QVariantAnimation(this);
        animResize_->setDuration(heightAnimDuration().count());
        animResize_->setEasingCurve(QEasingCurve::InOutSine);
        connect(animResize_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            setEditHeight(value.toInt());
        });
        connect(animResize_, &QVariantAnimation::finished, this, [this]()
        {
            const auto newPolicy = curEditHeight_ >= viewportMaxHeight() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
            if (newPolicy != textEdit_->verticalScrollBarPolicy())
                textEdit_->setVerticalScrollBarPolicy(newPolicy);
        });
    }

    void InputPanelMain::initAttachPopup()
    {
        if (attachPopup_)
            return;

        attachPopup_ = new AttachFilePopup(parentForPopup_ ? parentForPopup_ : window());
        attachPopup_->setPttEnabled(isPttEnabled());

        connect(attachPopup_, &AttachFilePopup::itemClicked, this, [this](AttachMediaType _mediaType)
        {
            Q_EMIT attachMedia(_mediaType, QPrivateSignal());
        });

        connect(attachPopup_, &AttachFilePopup::visiblityChanged, this, [this](bool _visible)
        {
            if (buttonAttach_)
                buttonAttach_->onAttachVisibleChanged(_visible);

            if (_visible)
                escCancel_->addChild(attachPopup_, [this]() { hideAttachPopup(); });
            else
                escCancel_->removeChild(attachPopup_);

            if (isVisibleTo(parentWidget()))
            {
                const auto mp = Utils::InterConnector::instance().getMessengerPage();
                if (mp && !mp->isSemiWindowVisible())
                {
                    if (attachPopup_->isPersistent())
                        setFocusOnAttach();
                    else
                        setFocusOnInput();
                }
            }
        });

        connect(textEdit_, &HistoryTextEdit::textChanged, this, [this]()
        {
            attachPopup_->setPersistent(false);
            hideAttachPopup();
        });
    }

    bool InputPanelMain::isAttachEnabled() const
    {
        return features_ & InputFeature::AttachFile;
    }

    bool InputPanelMain::isPttEnabled() const
    {
        return features_ & InputFeature::Ptt;
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
        if (buttonAttach_)
            buttonAttach_->updateStyle(_mode);

        updateButtonColors(buttonEmoji_, _mode);

        if (edit_)
            edit_->updateStyle(_mode);
    }

    void InputPanelMain::hideEvent(QHideEvent* _e)
    {
        hideAttachPopup();
    }

    QRect InputPanelMain::getAttachFileButtonRect() const
    {
        if (buttonAttach_)
        {
            auto rect = buttonAttach_->geometry();
            rect.moveTopLeft(leftSide_->mapToGlobal(rect.topLeft()));
            return rect;
        }

        return {};
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

    void InputPanelMain::hideAttachPopup()
    {
        if (attachPopup_)
            attachPopup_->hideAnimated();

        escCancel_->removeChild(attachPopup_);
    }
    
    bool InputPanelMain::isDraftVersionVisible() const
    {
        return draftVersionWidget_ && draftVersionWidget_->isVisible();
    }

    void InputPanelMain::setParentForPopup(QWidget* _parent)
    {
        parentForPopup_ = _parent;
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
        if (buttonAttach_)
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
