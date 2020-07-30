#include "stdafx.h"

#include "fonts.h"
#include "utils/utils.h"
#include "controls/TextUnit.h"
#include "styles/ThemeParameters.h"
#include "animation/animation.h"
#include "core_dispatcher.h"
#include "../MessageStyle.h"
#include "ComplexMessageItem.h"
#include "../MessageStatusWidget.h"
#include "omicron/omicron_helper.h"
#include "utils/InterConnector.h"
#include "../../input_widget/InputWidgetUtils.h"
#include "app_config.h"
#include "../common.shared/omicron_keys.h"

#include "PollBlockLayout.h"
#include "PollBlock.h"

namespace
{
    int32_t itemVerticalPadding()
    {
        return Utils::scale_value(12);
    }

    int32_t itemsGap()
    {
        return Utils::scale_value(8);
    }

    int32_t answersTopMargin()
    {
        return Utils::scale_value(4);
    }

    enum class WithCheckBox
    {
        No,
        Yes
    };

    int32_t answerLeftMargin(WithCheckBox _withCheckBox)
    {
        if (_withCheckBox == WithCheckBox::Yes)
            return Utils::scale_value(12);
        else
            return Utils::scale_value(8);
    }

    QSize checkBoxSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int32_t checkBoxLeftMargin()
    {
        return Utils::scale_value(8);
    }

    int32_t checkBoxLineWidth()
    {
        return Utils::scale_value(2);
    }

    int32_t itemRectBorderRadius()
    {
        return Utils::scale_value(8);
    }

    int32_t itemRectBorderWidth()
    {
        return Utils::scale_value(1);
    }

    const QColor& resultItemBorderColor(bool _outgoing)
    {
        static const auto incomingColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.2);
        static const auto outgoingColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);

        return (_outgoing ? outgoingColor : incomingColor);
    }

    enum class CheckBoxOpacity
    {
        _50,
        _100
    };

    const QColor& checkBoxColor(CheckBoxOpacity _opacity)
    {
        static const auto color50 = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.5);
        static const auto color100 = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);

        if (_opacity == CheckBoxOpacity::_50)
            return color50;
        else
            return color100;
    }

    const QColor& itemFillColor(bool _hovered, bool _pressed, bool _outgoing)
    {
        if (_outgoing)
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
            static const auto hoveredColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT_HOVER);
            static const auto pressedColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT_ACTIVE);
            return (_pressed ? pressedColor : (_hovered ? hoveredColor : color));
        }
        else
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
            static const auto hoveredColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
            static const auto pressedColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
            return (_pressed ? pressedColor : (_hovered ? hoveredColor : color));
        }
    }

    int32_t votesCountTopMargin()
    {
        return Utils::scale_value(2);
    }

    int32_t itemRightPadding()
    {
        return Utils::scale_value(12);
    }

    int32_t minAnswerHeight()
    {
        return Utils::scale_value(42);
    }

    int32_t maxBlockWidth()
    {
        return Utils::scale_value(360);
    }

    int32_t placeholderHeight()
    {
        return 2 * (minAnswerHeight() + itemsGap()) + Utils::scale_value(4);
    }

    int32_t placeholderAnswersCount()
    {
        return 2;
    }

    const QColor& placeholderColor(bool _outgoing)
    {
        if (_outgoing)
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
            return color;
        }
        else
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE);
            return color;
        }
    }

    const QColor& placeholderCheckBoxColor(bool _outgoing)
    {
        if (_outgoing)
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.5);
            return color;
        }
        else
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
            return color;
        }
    }

    QSize checkMarkSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    const QPixmap& checkMarkPixmap()
    {
        static const auto pixmap = Utils::renderSvg(qsl(":/poll/check_mark"), checkMarkSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        return pixmap;
    }

    int32_t checkMarkHorizontalMargin()
    {
        return Utils::scale_value(8);
    }

    const QColor& votesPercentRectColor()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.2);
        return color;
    }

    QSize quotePollIconSize()
    {
        return Utils::scale_value(QSize(44, 44));
    }

    int32_t quoteQuestionLeftMargin()
    {
        return Utils::scale_value(8);
    }

    const QPixmap& quotePixmap()
    {
        static const auto pixmap = [](){
            QPixmap result(Utils::scale_bitmap(quotePollIconSize()));
            Utils::check_pixel_ratio(result);
            result.fill(Qt::transparent);
            QPainter p(&result);
            p.setRenderHint(QPainter::Antialiasing);
            const auto pollIcon = Utils::renderSvgScaled(qsl(":/poll/poll_icon"), QSize(32, 32), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE));
            auto backgroundColor = Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE, 0.05);
            QPainterPath backgroundPath;
            backgroundPath.addEllipse(QRect(QPoint(0, 0), quotePollIconSize()));
            p.fillPath(backgroundPath, backgroundColor);
            p.drawPixmap(Utils::unscale_bitmap((result.width() - pollIcon.width()) / 2), Utils::unscale_bitmap((result.height() - pollIcon.height()) / 2), pollIcon);

            return result;
        }();

        return pixmap;
    }

    int32_t bottomTextTopMargin()
    {
        return Utils::scale_value(12);
    }

    const QColor& bottomTextColor(bool _outgoing)
    {
        static const auto incomingColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        static const auto outgoingColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);

        return (_outgoing ? outgoingColor : incomingColor);
    }

    void sendPollStat(const std::string_view _action)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::polls_action, { {"chat_type", Ui::getStatsChatType() }, { "action", std::string(_action) }});
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

//////////////////////////////////////////////////////////////////////////
// PollItemData
//////////////////////////////////////////////////////////////////////////

struct PollItemData
{
    QRect rect_;
    int votes_ = 0;
    double percent_ = 0;
    QPointF checkBoxPos_;
    QString answerId_;
    QPointF checkMarkPos_;
    TextRendering::TextUnitPtr answerUnit_;
    TextRendering::TextUnitPtr percentUnit_;
    TextRendering::TextUnitPtr votesCountUnit_;
};

//////////////////////////////////////////////////////////////////////////
// PollBlock_p
//////////////////////////////////////////////////////////////////////////

class PollBlock_p
{
public:
    enum State
    {
        PendingData,
        PollIdLoaded,
        VoteOptions,
        LoadingResults,
        ResultsLoaded
    };

    enum Mode
    {
        Placeholder,
        Answers,
        Results
    };

    static Mode modeForState(State _state)
    {
        if (_state == PendingData || _state == VoteOptions || _state == LoadingResults)
            return Answers;
        if (_state == ResultsLoaded)
            return Results;

        return Placeholder;
    }

    Mode currentMode()
    {
        return modeForState(state_);
    }

    void updateContent()
    {
        state_ = pollState(poll_);

        if (currentMode() != Placeholder || isQuote_)
            updatePollData();

        updateDebugId();
    }

    QString bottomTextStr()
    {
        const auto pollTypeStr = (poll_.type_ == Data::PollData::PublicPoll ? QT_TRANSLATE_NOOP("poll_block", "Public") : QT_TRANSLATE_NOOP("poll_block", "Anonymous"));

        return Utils::GetTranslator()->getNumberString(
                   poll_.votes_,
                   QT_TRANSLATE_NOOP3("poll_block", "%1 poll - %2 vote", "1"),
                   QT_TRANSLATE_NOOP3("poll_block", "%1 poll - %2 votes", "2"),
                   QT_TRANSLATE_NOOP3("poll_block", "%1 poll - %2 votes", "5"),
                   QT_TRANSLATE_NOOP3("poll_block", "%1 poll - %2 votes", "21")).arg(pollTypeStr).arg(poll_.votes_);
    }

    void updatePollData()
    {
        static const auto commonTextColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        static const auto loadingTextColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID, 0.5);

        if (!isQuote_)
        {
            const auto bottomText = bottomTextStr();

            bottomTextUnit_ = TextRendering::MakeTextUnit(bottomText);
            bottomTextUnit_->init(Fonts::adjustedAppFont(11, Fonts::FontWeight::Normal), bottomTextColor(isOutgoing_));

            items_.clear();
            items_.reserve(poll_.answers_.size());

            const auto loading = !myLocalAnswerId_.isEmpty();

            for (const auto& answer : poll_.answers_)
            {
                PollItemData data;
                data.votes_ = answer->votes_;
                data.answerId_ = answer->answerId_;
                const auto localAnswer = !myLocalAnswerId_.isEmpty() && data.answerId_ == myLocalAnswerId_;

                const auto& textColor = (!loading || localAnswer ? commonTextColor : loadingTextColor);

                data.answerUnit_ = TextRendering::MakeTextUnit(answer->text_, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
                data.answerUnit_->init(Fonts::adjustedAppFont(14), textColor);

                if (currentMode() == Results)
                {
                    data.percent_ = votesPercent(answer->votes_);
                    data.percentUnit_ = TextRendering::MakeTextUnit(qsl("%1%").arg(std::floor(data.percent_)));
                    data.percentUnit_->init(Fonts::adjustedAppFont(14), commonTextColor);

                    data.votesCountUnit_ = TextRendering::MakeTextUnit(QString::number(answer->votes_));
                    data.votesCountUnit_->init(Fonts::adjustedAppFont(10), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL));
                }

                items_.push_back(std::move(data));
            }
        }
        else
        {
            quoteQuestionUnit_ = TextRendering::MakeTextUnit(question_, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
            quoteQuestionUnit_->init(Fonts::adjustedAppFont(14, isQuote_ ? Fonts::FontWeight::Normal : Fonts::FontWeight::Medium), commonTextColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 2);
        }
    }

    void updateDebugId()
    {
        if (GetAppConfig().IsShowMsgIdsEnabled())
        {
            debugIdTextUnit_ = TextRendering::MakeTextUnit(qsl("Poll Id: %1").arg(poll_.id_));
            debugIdTextUnit_->init(Fonts::adjustedAppFont(14, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        }
        else
        {
            debugIdTextUnit_.reset();
        }
    }

    QSize setQuoteSize(const int _availableWidth, const int _timeWidgetWidth)
    {
        Q_UNUSED(_timeWidgetWidth)

        auto questionAvailableWidth = _availableWidth - quotePollIconSize().width() - quoteQuestionLeftMargin();
        auto height = quotePollIconSize().height();
        if (quoteQuestionUnit_)
        {
            const auto questionHeight = quoteQuestionUnit_->getHeight(questionAvailableWidth);
            const auto yOffset = std::max(0, (quotePollIconSize().height() - questionHeight) / 2);
            quoteQuestionUnit_->setOffsets(quotePollIconSize().width() + quoteQuestionLeftMargin(), yOffset);

            height = std::max(height, questionHeight);
        }

        return QSize(_availableWidth, height);
    }

    QSize setPollContentSize(const int _availableWidth, const int _timeWidgetWidth)
    {
        auto currentY = answersTopMargin();

        const auto drawResults = currentMode() == Results;
        const auto drawCheckBox = !drawResults;

        for (auto& item : items_)
        {
            const auto myLocalAnswer = !poll_.myAnswerId_.isEmpty() && item.answerId_ == poll_.myAnswerId_;
            auto answerAvailableWidth = _availableWidth;

            auto answerX = answerLeftMargin(drawCheckBox ? WithCheckBox::Yes : WithCheckBox::No);

            if (drawCheckBox)
            {
                answerX += checkBoxLeftMargin() + checkBoxSize().width();
            }
            else if (drawResults)
            {
                QFontMetrics percentMetrics(item.percentUnit_->getFont());
                auto percentWidth = percentMetrics.horizontalAdvance(qsl("%1%").arg(std::floor(votesPercent(item.votes_))));

                const auto rightTextWidth = std::max(percentWidth, item.votesCountUnit_ ? item.votesCountUnit_->desiredWidth() : 0);
                answerAvailableWidth -= rightTextWidth + (myLocalAnswer ? checkMarkSize().width() : 0) + 2 * checkMarkHorizontalMargin();
            }

            answerAvailableWidth -= answerX + itemRightPadding();

            const auto textHeight = (item.answerUnit_ ? item.answerUnit_->getHeight(answerAvailableWidth) : 0 );
            const auto rectHeight = std::max(minAnswerHeight(), textHeight + 2 * itemVerticalPadding());

            item.checkBoxPos_ = QPointF(checkBoxLeftMargin() + checkBoxSize().width() / 2., currentY + rectHeight / 2.);

            const auto answerWidth = std::min(item.answerUnit_->cachedSize().width(), item.answerUnit_->desiredWidth());

            if (myLocalAnswer)
                item.checkMarkPos_ = QPointF(answerX + answerWidth + checkMarkHorizontalMargin(), currentY + (rectHeight - checkMarkSize().height()) / 2.);

            item.rect_ = QRect(itemRectBorderWidth(), currentY, _availableWidth - 2 * itemRectBorderWidth(), rectHeight);

            if (item.answerUnit_)
                item.answerUnit_->setOffsets(answerX, currentY + std::ceil((rectHeight - textHeight) / 2.));

            if (drawResults)
            {
                const auto percentHeight = item.percentUnit_->getHeight(_availableWidth);
                const auto votesCountHeight = item.votesCountUnit_->getHeight(_availableWidth);

                const int yOffset = (rectHeight - votesCountHeight - percentHeight - votesCountTopMargin()) / 2;

                if (item.percentUnit_)
                    item.percentUnit_->setOffsets(item.rect_.right() - item.percentUnit_->desiredWidth() - itemRightPadding(), currentY + yOffset);

                if (item.votesCountUnit_)
                    item.votesCountUnit_->setOffsets(item.rect_.right() - item.votesCountUnit_->desiredWidth() - itemRightPadding(), currentY + yOffset + percentHeight + votesCountTopMargin());
            }

            currentY += rectHeight + itemsGap();
        }

        currentY -= itemsGap();

        if (bottomTextUnit_)
        {
            currentY += bottomTextTopMargin();
            const auto bottomTextHeight = bottomTextUnit_->getHeight(_availableWidth - _timeWidgetWidth);
            bottomTextUnit_->setOffsets(0, currentY);
            currentY += bottomTextHeight;
        }

        return QSize(_availableWidth, currentY);
    }

    QSize setPlaceholderSize(const int _availableWidth, const int _timeWidgetWidth)
    {
        Q_UNUSED(_timeWidgetWidth)

        return QSize(_availableWidth, placeholderHeight());
    }

    int setDebugIdSize(const int _availableWidth, int _contentHeight)
    {
        if (debugIdTextUnit_)
        {
            _contentHeight += Utils::scale_value(8);
            debugIdTextUnit_->setOffsets(0, _contentHeight);
            _contentHeight += debugIdTextUnit_->getHeight(_availableWidth);
        }

        return _contentHeight;
    }

    QSize setBlockSize_helper(const QSize& _size, const int _timeWidgetWidth)
    {
        const auto availalbeWidth = std::min(_size.width(), maxBlockWidth());

        QSize result;

        if (isQuote_)
            result = setQuoteSize(availalbeWidth, _timeWidgetWidth);
        else if (currentMode() == Placeholder)
            result = setPlaceholderSize(availalbeWidth, _timeWidgetWidth);
        else
            result = setPollContentSize(availalbeWidth, _timeWidgetWidth);

        if (GetAppConfig().IsShowMsgIdsEnabled())
            result.rheight() = setDebugIdSize(availalbeWidth - _timeWidgetWidth, result.height());

        return result;
    }

    void drawItem(QPainter& _p, const PollItemData& _data)
    {
        Utils::PainterSaver saver(_p);

        if (currentMode() == Answers)
        {
            const auto loading = !myLocalAnswerId_.isEmpty();
            const auto myLocalAnswer = loading && _data.answerId_ == myLocalAnswerId_;
            const auto hoverred = _data.rect_.contains(mousePos_) && enabled() && !loading;
            const auto pressed = _data.rect_.contains(pressPos_) && enabled() && !loading || myLocalAnswer;

            fillAnswerRect(_p, _data.rect_, itemFillColor(hoverred, pressed, isOutgoing_));

            if (myLocalAnswer && loading)
                drawProgressCheckBox(_p, _data.checkBoxPos_, checkBoxColor(CheckBoxOpacity::_100));
            else
                drawCheckBox(_p, _data.checkBoxPos_, checkBoxColor((!loading || myLocalAnswer) ? CheckBoxOpacity::_100 : CheckBoxOpacity::_50));
        }
        else if (currentMode() == Results)
        {
            auto rect = _data.rect_;
            rect.setWidth((rect.width() * _data.percent_) / 100);

            fillPercentRect(_p, rect, _data.rect_, votesPercentRectColor());

            _p.save();
            _p.setPen(QPen(resultItemBorderColor(isOutgoing_), itemRectBorderWidth()));
            _p.drawRoundedRect(_data.rect_, itemRectBorderRadius(), itemRectBorderRadius());
            _p.restore();

            if (_data.percentUnit_)
                _data.percentUnit_->draw(_p);

            if (_data.votesCountUnit_)
                _data.votesCountUnit_->draw(_p);

            if (!_data.checkMarkPos_.isNull())
                _p.drawPixmap(_data.checkMarkPos_, checkMarkPixmap());
        }

        if (_data.answerUnit_)
            _data.answerUnit_->draw(_p);
    }

    void drawQuote(QPainter& _p)
    {
        if (quoteQuestionUnit_)
            quoteQuestionUnit_->draw(_p);

        _p.drawPixmap(0, 0, quotePixmap());
    }

    void drawPollContent(QPainter& _p)
    {
        if (bottomTextUnit_)
            bottomTextUnit_->draw(_p);

        for (auto& item : items_)
            drawItem(_p, item);
    }

    void drawPlaceholder(QPainter& _p)
    {
        Utils::PainterSaver saver(_p);

        auto contentSize = layout_->getBlockGeometry().size();
        auto currentY = 0;

        for (auto i = 0; i < placeholderAnswersCount(); ++i)
        {
            auto answerRect = QRect(0, currentY, contentSize.width(), minAnswerHeight());
            auto checkBoxPos = QPointF(checkBoxLeftMargin() + checkBoxSize().width() / 2., currentY + answerRect.height() / 2.);

            fillAnswerRect(_p, answerRect, placeholderColor(isOutgoing_));
            drawCheckBox(_p, checkBoxPos, placeholderCheckBoxColor(isOutgoing_));

            currentY += answerRect.height() + itemsGap();
        }
    }

    void drawDebugId(QPainter& _p)
    {
        if (debugIdTextUnit_)
            debugIdTextUnit_->draw(_p);
    }

    void drawBlock_helper(QPainter& _p)
    {
        if (isQuote_)
            drawQuote(_p);
        else if (currentMode() == Placeholder)
            drawPlaceholder(_p);
        else
            drawPollContent(_p);

        if (GetAppConfig().IsShowMsgIdsEnabled())
            drawDebugId(_p);
    }

    void fillAnswerRect(QPainter& _p, const QRect& _rect, const QColor& _color)
    {
        QPainterPath path;
        path.addRoundedRect(_rect, itemRectBorderRadius(), itemRectBorderRadius());
        _p.fillPath(path, _color);
    }

    void fillPercentRect(QPainter& _p, const QRect& _percentRect, const QRect& _itemRect, const QColor& _color)
    {
        Utils::PainterSaver saver(_p);
        QPainterPath clipPath;
        clipPath.addRoundedRect(_itemRect, itemRectBorderRadius(), itemRectBorderRadius());
        _p.setClipPath(clipPath);

        if (_percentRect.width() <= itemRectBorderRadius())
            _p.fillRect(_percentRect, votesPercentRectColor());
        else
            fillAnswerRect(_p, _percentRect, _color);
    }

    void drawCheckBox(QPainter& _p, const QPointF& _center, const QColor& _color)
    {
        Utils::PainterSaver saver(_p);
        _p.setPen(QPen(_color, checkBoxLineWidth()));
        _p.drawEllipse(_center, checkBoxSize().width() / 2, checkBoxSize().height() / 2);
    }

    void drawProgressCheckBox(QPainter& _p, const QPointF& _center, const QColor& _color)
    {
        Utils::PainterSaver saver(_p);
        _p.setPen(QPen(_color, checkBoxLineWidth()));
        auto arcRect = QRectF(_center - QPointF(checkBoxSize().width() / 2., checkBoxSize().height() / 2.), checkBoxSize());
        _p.drawArc(arcRect, -voteAnimation_.current(), 260 * 16);
    }

    QString answerAtPos(const QPoint& _pos)
    {
        for (const auto& item : items_)
        {
            if (item.rect_.contains(_pos))
                return item.answerId_;
        }

        return QString();
    }

    static State pollState(const Data::PollData& _poll)
    {
        const auto hasAnswers = !_poll.answers_.empty();
        const auto hasId = !_poll.id_.isEmpty();
        const auto hasMyAnswerId = !_poll.myAnswerId_.isEmpty();

        if (hasAnswers)
        {
            if (hasMyAnswerId || _poll.closed_)
                return ResultsLoaded;
            else if (hasId)
                return VoteOptions;
            else
                return PendingData;
        }

        return PollIdLoaded;
    }

    double votesPercent(const int _votes)
    {
        if (_votes == 0 || poll_.votes_ == 0)
            return 0;

        return _votes / static_cast<double>(poll_.votes_) * 100;
    }

    bool enabled()
    {
        return !requestInProgress_ && !Utils::InterConnector::instance().isMultiselect();
    }

    void executeRequest(std::function<int64_t()> _function)
    {
        seq_ = _function();
        requestInProgress_ = true;
    }

    void onResponse(int64_t _seq)
    {
        if (seq_ == _seq)
            requestInProgress_ = false;
    }

    PollBlockLayout* layout_ = nullptr;
    TextRendering::TextUnitPtr bottomTextUnit_;
    TextRendering::TextUnitPtr quoteQuestionUnit_;
    TextRendering::TextUnitPtr debugIdTextUnit_;
    std::vector<PollItemData> items_;
    anim::Animation resultsAnimation_;
    anim::Animation voteAnimation_;
    QString myLocalAnswerId_;
    QTimer pollUpdateTimer_;
    Data::PollData poll_;
    QString question_;
    State state_ = PollIdLoaded;
    bool requestInProgress_ = false;
    bool isOutgoing_ = false;
    bool isQuote_ = false;
    QPoint mousePos_;
    QPoint pressPos_;
    int64_t seq_ = 0;
};

//////////////////////////////////////////////////////////////////////////
// PollBlock
//////////////////////////////////////////////////////////////////////////

PollBlock::PollBlock(ComplexMessageItem* _parent, const Data::PollData& _poll, const QString& _text)
    : GenericBlock(_parent, QString(), MenuFlags::MenuFlagCopyable, false),
      d(std::make_unique<PollBlock_p>())
{
    d->poll_ = _poll;
    d->question_ = _text;
    d->pollUpdateTimer_.setSingleShot(true);
    d->pollUpdateTimer_.setInterval(Omicron::_o(omicron::keys::poll_subscribe_timeout_ms, feature::default_poll_subscribe_timeout_ms()));

    setMouseTracking(true);
}

PollBlock::~PollBlock()
{

}

IItemBlockLayout *PollBlock::getBlockLayout() const
{
    return d->layout_;
}

QString PollBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    Q_UNUSED(_isFullSelect)
    Q_UNUSED(_dest)

    return QT_TRANSLATE_NOOP("poll_block", "Poll");
}

void PollBlock::updateStyle()
{
    d->updateContent();
    notifyBlockContentsChanged();
}

void PollBlock::updateFonts()
{
    updateStyle();
}

QSize PollBlock::setBlockSize(const QSize& _size)
{
    auto timeWidth = 0;

    if (const auto cm = getParentComplexMessage())
    {
        if (const auto timeWidget = cm->getTimeWidget())
            timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();
    }

    return d->setBlockSize_helper(_size, timeWidth);
}

int PollBlock::desiredWidth(int _width) const
{
    return Utils::scale_value(360);
}

int PollBlock::getMaxWidth() const
{
    return Utils::scale_value(384);
}

QString PollBlock::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("poll_block", "Poll: %1").arg(d->question_);
}

bool PollBlock::clicked(const QPoint& _p)
{
    if (d->state_ != PollBlock_p::VoteOptions || d->isQuote_ || !d->enabled())
        return false;

    QPoint mappedPoint = mapFromParent(_p, d->layout_->getBlockGeometry());

    const auto answerId = d->answerAtPos(mappedPoint);

    if (!answerId.isEmpty())
    {
        d->executeRequest([this, answerId]() { return GetDispatcher()->voteInPoll(d->poll_.id_, answerId); });
        sendPollStat("vote");

        d->myLocalAnswerId_ = answerId;
        setCursor(Qt::ArrowCursor);

        d->updateContent();
        notifyBlockContentsChanged();

        d->voteAnimation_.finish();
        d->voteAnimation_.start([this](){ update(); }, 0, 360 * 16, 1000, anim::linear, -1);
    }

    return true;
}

void PollBlock::updateWith(IItemBlock* _other)
{
    if (auto otherPollBlock = dynamic_cast<PollBlock*>(_other))
    {
        const auto otherPoll = otherPollBlock->d->poll_;

        if (otherPoll.answers_.empty())
        {
            if (!otherPoll.id_.isEmpty())
            {
                d->poll_.id_ = otherPoll.id_;
                loadPoll();
            }
        }
        else
        {
            d->poll_ = otherPoll;
            notifyBlockContentsChanged();
        }
    }
}

QString PollBlock::getSourceText() const
{
    return d->question_;
}

bool PollBlock::onMenuItemTriggered(const QVariantMap& _params)
{
    const auto command = _params[qsl("command")].toString();

    if (command == u"revoke_vote")
    {
        d->executeRequest([this]() { return GetDispatcher()->revokeVote(d->poll_.id_); });
        sendPollStat("revote");
        return true;
    }
    else if(command == u"stop_poll")
    {
        const QString text = QT_TRANSLATE_NOOP("popup_window", "If you stop the poll, nobody will be able to vote anymore. This action can not be undone");

        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Stop"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Stop this poll?"),
            nullptr
        );

        if (confirm)
        {
            d->executeRequest([this]() { return GetDispatcher()->stopPoll(d->poll_.id_); });
            sendPollStat("stop");
        }
        return true;
    }
    else if (command == u"copy_poll_id")
    {
        QApplication::clipboard()->setText(d->poll_.id_);
        showToast(QT_TRANSLATE_NOOP("poll_block", "Poll Id copied to clipboard"));
    }

    return GenericBlock::onMenuItemTriggered(_params);
}

IItemBlock::MenuFlags PollBlock::getMenuFlags(QPoint) const
{
    int flags = MenuFlagNone;

    if (!d->poll_.id_.isEmpty() && !d->poll_.closed_ && d->enabled())
    {
        if (!d->poll_.myAnswerId_.isEmpty())
            flags |= MenuFlagRevokeVote;

        if (d->poll_.canStop_)
            flags |= MenuFlagStopPoll;
    }

    return static_cast<MenuFlags>(flags);
}

MediaType PollBlock::getMediaType() const
{
    return MediaType::mediaTypePoll;
}

IItemBlock::ContentType PollBlock::getContentType() const
{
    return ContentType::Poll;
}

bool PollBlock::isSharingEnabled() const
{
    return false;
}

void PollBlock::drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor)
{
    Q_UNUSED(_rect)
    Q_UNUSED(_quoteColor)

    d->drawBlock_helper(_p);
}

void PollBlock::initialize()
{
    d->layout_ = new PollBlockLayout();
    d->isOutgoing_ = isOutgoing();
    d->isQuote_ = isInsideQuote();
    setLayout(d->layout_);

    d->updateContent();

    if (!d->isQuote_)
    {
        connect(&d->pollUpdateTimer_, &QTimer::timeout, this, &PollBlock::loadPoll);
        connect(GetDispatcher(), &core_dispatcher::pollLoaded, this, &PollBlock::onPollLoaded);
        connect(GetDispatcher(), &core_dispatcher::voteResult, this, &PollBlock::onVoteResult);
        connect(GetDispatcher(), &core_dispatcher::revokeVoteResult, this, &PollBlock::onRevokeResult);
        connect(GetDispatcher(), &core_dispatcher::stopPollResult, this, &PollBlock::onStopPollResult);
        connect(GetDispatcher(), &core_dispatcher::appConfigChanged, this, [this](){ d->updateContent(); notifyBlockContentsChanged(); });

        if (d->state_ == PollBlock_p::PollIdLoaded)
            loadPoll();
    }
}

void PollBlock::mouseMoveEvent(QMouseEvent* _event)
{
    d->mousePos_ = _event->pos();

    auto handCursor = false;

    if (d->isQuote_ || Utils::InterConnector::instance().isMultiselect())
        handCursor = true;
    else if (d->currentMode() == PollBlock_p::Answers && d->enabled())
        handCursor = std::any_of(d->items_.begin(), d->items_.end(), [_event](const auto& _item) { return _item.rect_.contains(_event->pos()); });

    setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if (d->enabled())
        update();

    GenericBlock::mouseMoveEvent(_event);
}

void PollBlock::leaveEvent(QEvent* _event)
{
    d->mousePos_ = QPoint();

    if (d->enabled())
        update();

    GenericBlock::leaveEvent(_event);
}

void PollBlock::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        d->pressPos_ = _event->pos();

    GenericBlock::mousePressEvent(_event);

    update();
}

void PollBlock::mouseReleaseEvent(QMouseEvent* _event)
{
    d->pressPos_ = QPoint();
    GenericBlock::mouseReleaseEvent(_event);

    update();
}

void PollBlock::onPollLoaded(const Data::PollData& _poll)
{
    if (_poll.id_ != d->poll_.id_)
        return;

    d->poll_ = _poll;

    if (!d->resultsAnimation_.animating())
    {
        d->updateContent();
        notifyBlockContentsChanged();
    }
}

void PollBlock::loadPoll()
{
    GetDispatcher()->getPoll(d->poll_.id_);

    if (!d->poll_.closed_)
        d->pollUpdateTimer_.start();
}

void PollBlock::onVoteResult(int64_t _seq, const Data::PollData& _poll, bool _success)
{
    d->onResponse(_seq);

    if (d->poll_.id_ != _poll.id_)
        return;

    d->myLocalAnswerId_.clear();
    d->voteAnimation_.finish();

    if (!_success)
    {
        if (d->seq_ == _seq)
            showToast(QT_TRANSLATE_NOOP("poll_block", "Vote: an error occurred"));

        d->updateContent();
        notifyBlockContentsChanged();
        return;
    }

    for (auto& answer : d->poll_.answers_)
    {
        if (auto it = _poll.answersIndex_.find(answer->answerId_); it != _poll.answersIndex_.end())
            answer->votes_ = it->second->votes_;
    }

    d->poll_.myAnswerId_ = _poll.myAnswerId_;
    d->poll_.votes_ =  _poll.votes_;

    d->updateContent();

    d->resultsAnimation_.finish();
    d->resultsAnimation_.start([this]()
    {
        for (auto& item : d->items_)
        {
            const auto votes = item.votes_ * d->resultsAnimation_.current();
            const auto percent = votes / d->poll_.votes_ * 100;

            item.percent_ = percent;
            if (item.percentUnit_)
                item.percentUnit_->setText(qsl("%1%").arg((int)percent));
            if (item.votesCountUnit_)
                item.votesCountUnit_->setText(QString::number((int)votes));
            notifyBlockContentsChanged();
        }
    }, 0, 1, 350, anim::easeOutCubic);
}

void PollBlock::onRevokeResult(int64_t _seq, const QString& _pollId, bool _success)
{
    d->onResponse(_seq);

    if (d->poll_.id_ != _pollId)
        return;

    if (!_success)
    {
        if (d->seq_ == _seq)
            showToast(QT_TRANSLATE_NOOP("poll_block", "Revoke vote: an error occurred"));
        return;
    }

    d->poll_.myAnswerId_.clear();
    d->updateContent();
    notifyBlockContentsChanged();
}

void PollBlock::onStopPollResult(int64_t _seq, const QString& _pollId, bool _success)
{
    d->onResponse(_seq);

    if (d->poll_.id_ != _pollId)
        return;

    if (!_success)
    {
        if (d->seq_ == _seq)
            showToast(QT_TRANSLATE_NOOP("poll_block", "Stop poll: an error occurred"));
        return;
    }

    d->poll_.closed_ = true;
    d->updateContent();
    notifyBlockContentsChanged();
}

UI_COMPLEX_MESSAGE_NS_END
