#include "stdafx.h"

#include "TypingWidget.h"
#include "MessageStyle.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../styles/ThemeParameters.h"
#include "../../cache/ColoredCache.h"
#include "../../utils/utils.h"
#include "../../fonts.h"

namespace
{
    constexpr std::chrono::milliseconds animDuration = std::chrono::seconds(1);
    constexpr const auto frameCount = 6;

    struct AnimationFrames
    {
        using frameArray = std::array<QPixmap, frameCount>;
        frameArray frames_;

        void fill(const QColor& _color)
        {
            for (auto i = 0; i < frameCount; ++i)
            {
                frames_[i] = Utils::renderSvgScaled(u":/history/typing_icon_" % QString::number(i + 1), QSize(16, 16), _color);
                im_assert(!frames_[i].isNull());
            }
        }
    };

    const QPixmap& getFrame(const QColor& _color, const int _frame)
    {
        static Utils::ColoredCache<AnimationFrames> cache;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            cache.clear();

        return cache[_color].frames_[_frame % frameCount];
    }
}

namespace Ui
{
    TypingWidget::TypingWidget(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , aimId_(_aimId)
        , anim_(nullptr)
        , frame_(0)
    {
        setFixedHeight(Utils::scale_value(32));

        connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &TypingWidget::onTypingStatus);
    }

    TypingWidget::~TypingWidget()
    {
        stopAnimation();
    }

    void TypingWidget::updateTheme()
    {
        if (textUnit_)
        {
            animColor_ = Styling::getColor(getAnimColor());
            update();
        }
    }

    void TypingWidget::paintEvent(QPaintEvent* _event)
    {
        if (!textUnit_)
            return;

        QPainter p(this);
        textUnit_->draw(p, TextRendering::VerPosition::BASELINE);

        const auto& pm = getFrame(animColor_, frame_);
        const auto x = textUnit_->horOffset() + textUnit_->cachedSize().width() + Utils::scale_value(2);
        const auto y = (height() - pm.height() / Utils::scale_bitmap_ratio()) / 2;
        p.drawPixmap(x, y, pm);
    }

    void TypingWidget::showEvent(QShowEvent * _event)
    {
        startAnimation();
    }

    void TypingWidget::hideEvent(QHideEvent * _event)
    {
        stopAnimation();
    }

    void TypingWidget::onTypingStatus(const Logic::TypingFires& _typing, const bool _isTyping)
    {
        if (_typing.aimId_ != aimId_)
            return;

        QString name = Utils::replaceLine(_typing.getChatterName());
        const auto tokens = name.split(ql1c(' '));

        if (!tokens.isEmpty())
            name = tokens.at(0);

        if (_isTyping)
            typingChattersAimIds_.insert(std::make_pair(_typing.chatterAimId_, name));
        else
            typingChattersAimIds_.erase(_typing.chatterAimId_);

        if (!typingChattersAimIds_.empty())
            updateText();
        setVisible(!typingChattersAimIds_.empty());
    }

    void TypingWidget::updateText()
    {
        if (typingChattersAimIds_.empty())
            return;

        QString names;
        if (Logic::getContactListModel()->isChat(aimId_))
        {
            for (const auto& chatter : std::as_const(typingChattersAimIds_))
            {
                if (!names.isEmpty())
                    names += ql1s(", ");
                names += chatter.second;
            }
        }

        QString text;
        if (!names.isEmpty())
        {
            const auto typing = typingChattersAimIds_.size() == 1 ? QT_TRANSLATE_NOOP("chat_page", "is typing") : QT_TRANSLATE_NOOP("chat_page", "are typing");
            text = names % ql1c(' ') % typing;
        }
        else
        {
            text = QT_TRANSLATE_NOOP("chat_page", "is typing");
        }

        if (!textUnit_)
        {
            textUnit_ = TextRendering::MakeTextUnit(text, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(12), getTextColor() };
            params.maxLinesCount_ = 1;
            textUnit_->init(params);
            textUnit_->setOffsets(Utils::scale_value(24), Utils::scale_value(20));
            textUnit_->evaluateDesiredSize();

            updateTheme();
        }
        else
        {
            textUnit_->setText(text);
            textUnit_->evaluateDesiredSize();
        }

        update();
    }

    void TypingWidget::ensureAnimationInitialized()
    {
        if (anim_)
            return;

        anim_ = new QVariantAnimation(this);
        anim_->setStartValue(0);
        anim_->setEndValue(frameCount);
        anim_->setDuration(animDuration.count());
        anim_->setLoopCount(-1);
        connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            if (value.toInt() % frameCount != frame_ % frameCount)
            {
                frame_++;
                update();
            }
        });
    }


    void TypingWidget::startAnimation()
    {
        frame_ = 0;
        ensureAnimationInitialized();
        anim_->stop();
        anim_->start();
    }

    void TypingWidget::stopAnimation()
    {
        if (anim_)
            anim_->stop();
    }

    Styling::ThemeColorKey TypingWidget::getTextColor() const
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TYPING_TEXT, aimId_ };
    }

    Styling::ThemeColorKey TypingWidget::getAnimColor() const
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TYPING_ANIMATION, aimId_ };
    }
}
