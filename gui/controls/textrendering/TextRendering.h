#pragma once

#include "cache/emoji/Emoji.h"
#include "utils/UrlParser.h"
#include "types/message.h"

namespace Ui
{
    namespace TextRendering
    {
        enum class VerPosition
        {
            TOP = 0,//specified vertical offset leads to the highest point of the text
            MIDDLE = 1,//specified vertical offset leads to the middle of the text
            BASELINE = 2,//specified vertical offset leads to baseline of the text
            BOTTOM = 3,//specified vertical offset leads to the lowest point of the text
        };

        enum class HorAligment
        {
            LEFT = 0,
            CENTER = 1,
            RIGHT = 2,
        };

        enum class BlockType
        {
            TYPE_TEXT = 0,
            TYPE_EMOJI = 1,
            TYPE_NEW_LINE = 2,
            TYPE_SKIP = 3,
            TYPE_DEBUG_TEXT = 4
        };

        enum class Space
        {
            WITHOUT_SPACE_AFTER = 0,
            WITH_SPACE_AFTER = 1
        };

        enum class WordType
        {
            TEXT = 0,
            LINK = 1,
            MENTION = 2,
            EMOJI = 3,
            NICK = 4,
        };

        enum class LinksVisible
        {
            DONT_SHOW_LINKS = 0,
            SHOW_LINKS = 1
        };

        enum class LinksStyle
        {
            PLAIN = 0,
            UNDERLINED = 1
        };

        enum class ProcessLineFeeds
        {
            KEEP_LINE_FEEDS = 0,
            REMOVE_LINE_FEEDS = 1,
        };

        enum class TextType
        {
            VISIBLE = 0,
            SOURCE = 1,
        };

        enum class CallType
        {
            USUAL = 0,
            FORCE = 1,
        };

        enum class SelectionType
        {
            WITH_SPACE_AFTER = 0,
            WITHOUT_SPACE_AFTER = 1,
        };

        enum class EmojiScale
        {
            NORMAL = 0,
            REGULAR = 1,
            MEDIUM = 2,
            BIG = 3,
        };

        enum class EmojiSizeType
        {
            REGULAR = 0,
            ALLOW_BIG = 1,
            TOOLTIP = 2,
        };

        enum class LineBreakType
        {
            PREFER_SPACES = 0,//break line by words (like WordWrap)
            PREFER_WIDTH = 1,//break by width, words can be splitted
        };

        enum class MarkdownType
        {
            ALL = 0,
            FROM = 1,
            TILL = 2,
        };

        enum class ELideType
        {
            ACCURATE = 0,
            FAST = 1,
        };

        enum class WidthMode
        {
            PLAIN = 0,
            FORCE = 1,
        };

        using highlightsV = std::vector<QString>;

        constexpr int defaultEmojiSize() noexcept { return 22; }

        class TextWord
        {
        public:
            TextWord(QString _text, Space _space, WordType _type, LinksVisible _showLinks, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR)
                : code_(0)
                , space_(_space)
                , type_(_type)
                , text_(std::move(_text))
                , emojiSize_(defaultEmojiSize())
                , cachedWidth_(0)
                , selectedFrom_(-1)
                , selectedTo_(-1)
                , spaceWidth_(0)
                , highlight_({0, 0})
                , isSpaceSelected_(false)
                , isSpaceHighlighted_(false)
                , isTruncated_(false)
                , linkDisabled_(false)
                , selectionFixed_(false)
                , showLinks_(_showLinks)
                , emojiSizeType_(_emojiSizeType)
            {
                trimmedText_ = text_;
                trimmedText_.remove(QChar::Space);

                if (type_ != WordType::LINK && _showLinks == LinksVisible::SHOW_LINKS)
                {
                    Utils::UrlParser p;
                    p.process(QStringRef(&trimmedText_));
                    if (p.hasUrl())
                    {
                        type_ = WordType::LINK;
                        originalUrl_ = QString::fromStdString(p.getUrl().original_);
                    }
                }
                assert(type_ != WordType::EMOJI);

                checkSetNick();
            }

            TextWord(Emoji::EmojiCode _code, Space _space, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR)
                : code_(std::move(_code))
                , space_(_space)
                , type_(WordType::EMOJI)
                , emojiSize_(defaultEmojiSize())
                , cachedWidth_(0)
                , selectedFrom_(-1)
                , selectedTo_(-1)
                , spaceWidth_(0)
                , highlight_({0, 0})
                , isSpaceSelected_(false)
                , isSpaceHighlighted_(false)
                , isTruncated_(false)
                , linkDisabled_(false)
                , selectionFixed_(false)
                , showLinks_(LinksVisible::SHOW_LINKS)
                , emojiSizeType_(_emojiSizeType)
            {
                assert(!code_.isNull());
            }

            bool operator==(const TextWord& _other) const noexcept
            {
                return text_ == _other.text_;
            }

            WordType getType() const noexcept { return type_; }

            Space getSpace() const noexcept { return space_; }

            void setSpace(const Space& _space) noexcept { space_ = _space; }

            bool isEmoji() const noexcept { return type_ == WordType::EMOJI; }

            bool isNick() const noexcept { return type_ == WordType::NICK; }

            bool isSpaceAfter() const noexcept { return space_ == Space::WITH_SPACE_AFTER; }

            QString getText(TextType _text_type = TextType::VISIBLE) const;

            bool equalTo(const QString& _sl) const;

            QString getLink() const;

            QString getOriginalUrl() const { return originalUrl_; }

            const Emoji::EmojiCode& getCode() const noexcept { return code_; }

            double width(WidthMode _force = WidthMode::PLAIN, int _emojiCount = 0);

            void setWidth(double _width, int _emojiCount = 0);

            int getPosByX(int _x);

            void select(int _from, int _to, SelectionType _type);

            void selectAll(SelectionType _type);

            void clearSelection();

            void fixSelection();

            void releaseSelection();

            void clicked() const;

            int emojiSize() const noexcept { return emojiSize_; }

            double cachedWidth() const;

            int selectedFrom() const noexcept { return selectedFrom_; }

            int selectedTo() const noexcept { return selectedTo_; }

            bool isSelected() const noexcept { return selectedFrom_ != -1 && selectedTo_ != -1; }

            bool isFullSelected() const { return selectedFrom_ == 0 && (isEmoji() ? selectedTo_ == 1 : selectedTo_ == text_.size()); }

            int highlightedFrom() const noexcept { return highlight_.first; }
            int highlightedTo() const noexcept { return highlight_.second; }

            bool isHighlighted() const noexcept { return highlightedTo() - highlightedFrom() > 0; }
            bool isLeftHighlighted() const noexcept { return isHighlighted() && highlightedFrom() == 0; }
            bool isRightHighlighted() const { return isHighlighted() && highlightedTo() == (isEmoji() ? 1 : text_.size()); }
            bool isFullHighlighted() const { return highlightedFrom() == 0 && highlightedTo() == (isEmoji() ? 1 : text_.size()); }

            void setHighlighted(const bool _isHighlighted) noexcept;
            void setHighlighted(const int _from, const int _to) noexcept;
            void setHighlighted(const highlightsV& _entries);

            bool isLink() const noexcept { return type_ == WordType::LINK || isNick() || isMention() || !originalLink_.isEmpty(); }

            bool isMention() const noexcept { return type_ == WordType::MENTION; }

            int spaceWidth() const;

            bool applyMention(const std::pair<QString, QString>& _mention);

            bool applyMention(const QString& _mention, const std::vector<TextWord>& _mentionWords);

            void setOriginalLink(const QString& _link);

            void setTruncated() noexcept { isTruncated_ = true; }

            bool isTruncated() const noexcept { return isTruncated_; }

            bool isSpaceSelected() const noexcept { return isSpaceSelected_; }

            bool isSpaceHighlighted() const noexcept { return isSpaceHighlighted_; }

            void setSpaceHighlighted(const bool _isHighlighted) { isSpaceHighlighted_ = _isHighlighted; }

            LinksVisible getShowLinks() const noexcept { return showLinks_; }

            bool isLinkDisabled() const noexcept { return linkDisabled_; }

            void disableLink();

            void setFont(const QFont& _font);

            const QFont& getFont() const { return font_; }

            void setColor(const QColor& _color)
            {
                color_ = _color;
                for (auto& w : subwords_)
                    w.setColor(_color);
            }

            const QColor& getColor() const { return color_; }

            void applyFontColor(const TextWord& _other);

            void setSubwords(const std::vector<TextWord>& _subwords) { subwords_ = std::move(_subwords); cachedWidth_ = 0; }

            const std::vector<TextWord>& getSubwords() const { return subwords_; }

            bool hasSubwords() const { return !subwords_.empty(); }

            std::vector<TextWord> splitByWidth(int width);

            void setUnderline(bool _enabled);

            bool underline() const;

            void setEmojiSizeType(EmojiSizeType _emojiSizeType);

            EmojiSizeType getEmojiSizeType() const noexcept { return emojiSizeType_; }

            bool isResizable() const { return emojiSizeType_ == EmojiSizeType::ALLOW_BIG; }

            bool isInTooltip() const { return  emojiSizeType_ == EmojiSizeType::TOOLTIP; }

        private:
            void checkSetNick();

            Emoji::EmojiCode code_;
            Space space_;
            WordType type_;
            QString text_;
            QString trimmedText_;
            QString originalMention_;
            QString originalLink_;
            QString originalUrl_;
            int emojiSize_;
            double cachedWidth_;
            int selectedFrom_;
            int selectedTo_;
            double spaceWidth_;
            std::pair<int, int> highlight_;
            bool isSpaceSelected_;
            bool isSpaceHighlighted_;
            bool isTruncated_;
            bool linkDisabled_;
            bool selectionFixed_;
            LinksVisible showLinks_;
            QFont font_;
            QColor color_;
            std::vector<TextWord> subwords_;
            EmojiSizeType emojiSizeType_;
        };

        class BaseDrawingBlock
        {
        public:
            BaseDrawingBlock(BlockType _type)
                : type_(_type)
            {
            }

            virtual ~BaseDrawingBlock() { }

            BlockType getType() const { return type_; }

            virtual void init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR, const LinksStyle _linksStyle = LinksStyle::PLAIN) = 0;

            virtual void draw(QPainter& _p, QPoint& _point, VerPosition _align = VerPosition::TOP) = 0;

            virtual void drawSmart(QPainter& _p, QPoint& _point, int _center) = 0;

            virtual int getHeight(int width, int lineSpacing, CallType type = CallType::USUAL, bool _isMultiline = false) = 0;

            virtual int getCachedHeight() const = 0;

            virtual void setMaxHeight(int height) = 0;

            virtual void select(const QPoint& _from, const QPoint& _to) = 0;

            virtual void selectAll() = 0;

            virtual void fixSelection() = 0;

            virtual void clearSelection() = 0;

            virtual void releaseSelection() = 0;

            virtual bool isSelected() const = 0;

            virtual bool isFullSelected() const = 0;

            virtual QString selectedText(TextType _type = TextType::VISIBLE) const = 0;

            virtual QString getText() const = 0;

            virtual QString getSourceText() const = 0;

            virtual void clicked(const QPoint& _p) const = 0;

            virtual bool doubleClicked(const QPoint& _p, bool _fixSelection) = 0;

            virtual bool isOverLink(const QPoint& _p) const = 0;

            virtual QString getLink(const QPoint& _p) const = 0;

            virtual void applyMentions(const Data::MentionMap& _mentions) = 0;

            virtual int desiredWidth() const = 0;

            virtual void elide(int _width, ELideType _type, bool _prevElided) = 0;

            virtual bool isElided() const = 0;

            virtual void setMaxLinesCount(size_t _count, LineBreakType _lineBreak) = 0;

            virtual void setLastLineWidth(int _width) = 0;

            virtual int getLineCount() const = 0;

            virtual void applyLinks(const std::map<QString, QString>& _links) = 0;

            virtual void applyLinksFont(const QFont& _font) = 0;

            virtual void setColor(const QColor& _color) = 0;

            virtual void setLinkColor(const QColor& _color) = 0;

            virtual void setSelectionColor(const QColor& _color) = 0;

            virtual void setHighlightedTextColor(const QColor& _color) = 0;

            virtual void setColorForAppended(const QColor& _color) = 0;

            virtual void appendWords(const std::vector<TextWord>& _words) = 0;

            virtual std::vector<TextWord> getWords() const = 0;

            virtual bool markdownSingle(const QFont& _font, const QColor& _color) = 0;

            virtual bool markdownMulti(const QFont& _font, const QColor& _color, bool _split, std::pair<std::vector<TextWord>, std::vector<TextWord>>& _splitted) = 0;

            virtual int findForMarkdownMulti() const = 0;

            virtual std::vector<TextWord> markdown(MarkdownType _type, const QFont& _font, const QColor& _color, bool _split) = 0;

            virtual void setHighlighted(const bool _isHighlighted) = 0;
            virtual void setHighlighted(const highlightsV& _entries) = 0;

            virtual void setUnderline(const bool _enabled) = 0;

            virtual double getLastLineWidth() const = 0;

            virtual double getMaxLineWidth() const = 0;

            virtual bool isEmpty() const = 0;

            virtual bool needsEmojiMargin() const = 0;

        protected:
            BlockType type_;
        };

        typedef std::unique_ptr<BaseDrawingBlock> BaseDrawingBlockPtr;

        class TextDrawingBlock : public BaseDrawingBlock
        {
        public:
            TextDrawingBlock(const QStringRef& _text, LinksVisible _showLinks = LinksVisible::SHOW_LINKS, BlockType _blockType = BlockType::TYPE_TEXT);

            TextDrawingBlock(const std::vector<TextWord>& _words, BaseDrawingBlock* _other);

            void setMargins(const QMargins& _margins);

            void init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR, const LinksStyle _linksStyle = LinksStyle::PLAIN) override;

            void draw(QPainter& _p, QPoint& _point, VerPosition _align = VerPosition::TOP) override;

            void drawSmart(QPainter& _p, QPoint& _point, int _center) override;

            int getHeight(int width, int lineSpacing, CallType type = CallType::USUAL, bool _isMultiline = false) override;

            int getCachedHeight() const override;

            void setMaxHeight(int height) override;

            void select(const QPoint& _from, const QPoint& _to) override;

            void selectAll() override;

            void fixSelection() override;

            void clearSelection() override;

            void releaseSelection() override;

            bool isSelected() const override;

            bool isFullSelected() const override;

            QString selectedText(TextType _type) const override;

            QString getText() const override;

            QString getSourceText() const override;

            void clicked(const QPoint& _p) const override;

            bool doubleClicked(const QPoint& _p, bool _fixSelection) override;

            bool isOverLink(const QPoint& _p) const override;

            QString getLink(const QPoint& _p) const override;

            void applyMentions(const Data::MentionMap& _mentions) override;

            int desiredWidth() const override;

            void elide(int _width, ELideType _type, bool _prevElided) override;

            bool isElided() const  override { return elided_ && !words_.empty(); }

            void setMaxLinesCount(size_t _count, LineBreakType _lineBreak) override;

            void setLastLineWidth(int _width) override;

            int getLineCount() const override;

            void applyLinks(const std::map<QString, QString>& _links) override;

            void applyLinksFont(const QFont& _font) override;

            void setColor(const QColor& _color) override;

            void setLinkColor(const QColor& _color) override;

            void setSelectionColor(const QColor& _color) override;

            void setHighlightedTextColor(const QColor& _color) override;

            void setColorForAppended(const QColor& _color) override;

            void appendWords(const std::vector<TextWord>& _words) override;

            std::vector<TextWord> getWords() const override { return words_; }

            bool markdownSingle(const QFont& _font, const QColor& _color) override;

            bool markdownMulti(const QFont& _font, const QColor& _color, bool _split, std::pair<std::vector<TextWord>, std::vector<TextWord>>& _splitted) override;

            int findForMarkdownMulti() const override;

            std::vector<TextWord> markdown(MarkdownType _type, const QFont& _font, const QColor& _color, bool _split) override;

            void setHighlighted(const bool _isHighlighted) override;
            void setHighlighted(const highlightsV& _entries) override;

            void setUnderline(const bool _enabled) override;

            double getLastLineWidth() const override;

            double getMaxLineWidth() const override;

            bool isEmpty() const override { return words_.empty(); }

            bool needsEmojiMargin() const override { return needsEmojiMargin_; }

        private:
            void calcDesired();
            void parseForWords(const QStringRef& _text, LinksVisible _showLinks, std::vector<TextWord>& _words, WordType _type = WordType::TEXT);
            bool parseWordLink(const TextWord& _wordWithLink, std::vector<TextWord>& _words);
            bool parseWordNickImpl(const TextWord& _word, std::vector<TextWord>& _words, const QStringRef& _text);
            bool parseWordNick(const TextWord& _word, std::vector<TextWord>& _words, const QStringRef& _text = QStringRef());

            std::vector<TextWord> elideWords(const std::vector<TextWord>& _original, int _width, int _desiredWidth, bool _forceElide = false, const ELideType& _type = ELideType::ACCURATE);

        private:
            std::vector<TextWord> words_;
            std::vector<TextWord> originalWords_;
            std::vector<std::vector<TextWord>> lines_;
            std::vector<QRect> linkRects_;
            std::map<int, QString> links_;
            QMargins margins_;
            QColor linkColor_;
            QColor selectionColor_;
            QColor highlightColor_;
            QColor highlightTextColor_;
            int lineHeight_;
            double desiredWidth_;
            double originalDesiredWidth_;
            mutable double cachedMaxLineWidth_;
            QSize cachedSize_;
            bool elided_;
            HorAligment align_;
            size_t maxLinesCount_;
            int appended_;
            LineBreakType lineBreak_;
            int lineSpacing_;
            bool selectionFixed_;
            int lastLineWidth_;
            bool needsEmojiMargin_;
        };

        class DebugInfoTextDrawingBlock : public TextDrawingBlock
        {
        public:
            const qint64 INVALID_MSG_ID = -1;

            enum class Subtype
            {
                MessageId = 1,
            };

        public:
            explicit DebugInfoTextDrawingBlock(qint64 _id, Subtype _subtype = Subtype::MessageId, LinksVisible _showLinks = LinksVisible::DONT_SHOW_LINKS);

            Subtype getSubtype() const;
            qint64 getMessageId() const;

        private:
            void setMessageId(qint64 _id);

        private:
            Subtype subtype_;
            qint64 messageId_ = INVALID_MSG_ID;
        };

        class NewLineBlock : public BaseDrawingBlock
        {
        public:

            NewLineBlock();

            void init(const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR, const LinksStyle _linksStyle = LinksStyle::PLAIN) override;

            void draw(QPainter& _p, QPoint& _point, VerPosition _align = VerPosition::TOP) override;

            void drawSmart(QPainter& _p, QPoint& _point, int _center) override;

            int getHeight(int width, int lineSpacing, CallType type = CallType::USUAL, bool _isMultiline = false) override;

            int getCachedHeight() const override;

            void setMaxHeight(int height) override;

            void select(const QPoint& _from, const QPoint& _to) override;

            void selectAll() override;

            void fixSelection() override;

            void clearSelection() override;

            void releaseSelection() override;

            bool isSelected() const override;

            bool isFullSelected() const override;

            QString selectedText(TextType _type) const override;

            QString getText() const override;

            QString getSourceText() const override;

            void clicked(const QPoint& _p) const override;

            bool doubleClicked(const QPoint& _p, bool _fixSelection) override;

            bool isOverLink(const QPoint& _p) const override;

            QString getLink(const QPoint& _p) const override;

            void applyMentions(const Data::MentionMap& _mentions) override;

            int desiredWidth() const override;

            void elide(int _width, ELideType _type, bool _prevElided) override;

            bool isElided() const override { return false; }

            void setMaxLinesCount(size_t, LineBreakType) override { }

            void setLastLineWidth(int _width) override { }

            int getLineCount() const override { return 0; }

            void applyLinks(const std::map<QString, QString>& _links) override { }

            void applyLinksFont(const QFont& _font) override { }

            void setColor(const QColor&) override { };

            void setLinkColor(const QColor&) override { };

            void setSelectionColor(const QColor&) override { };

            void setHighlightedTextColor(const QColor&) override { };

            void setColorForAppended(const QColor&) override { };

            void appendWords(const std::vector<TextWord>&) override { };

            std::vector<TextWord> getWords() const override { return { }; }

            bool markdownSingle(const QFont&, const QColor&) override { return false; }

            bool markdownMulti(const QFont&, const QColor&, bool, std::pair<std::vector<TextWord>, std::vector<TextWord>>&) override { return false; }

            int findForMarkdownMulti() const override { return 0; }

            std::vector<TextWord> markdown(MarkdownType, const QFont&, const QColor&, bool) override { return {}; }

            void setHighlighted(const bool _isHighlighted) override {}
            void setHighlighted(const highlightsV&) override {}

            void setUnderline(const bool _enabled) override {}

            double getLastLineWidth() const override { return 0; }

            double getMaxLineWidth() const override { return 0; }

            bool isEmpty() const override { return false; }

            bool needsEmojiMargin() const override { return false; }

        private:
            QFont font_;
            int cachedHeight_;
        };

        std::vector<BaseDrawingBlockPtr> parseForBlocks(const QString& _text, const Data::MentionMap& _mentions = Data::MentionMap(), LinksVisible _showLinks = LinksVisible::SHOW_LINKS, ProcessLineFeeds _processLineFeeds = ProcessLineFeeds::KEEP_LINE_FEEDS);

        int getBlocksHeight(const std::vector<BaseDrawingBlockPtr>& _blocks, int width, int lineSpacing, CallType _calltype = CallType::USUAL);

        void drawBlocks(const std::vector<BaseDrawingBlockPtr>&, QPainter& _p, QPoint _point, VerPosition _align = VerPosition::TOP);

        void drawBlocksSmart(const std::vector<BaseDrawingBlockPtr>&, QPainter& _p, QPoint _point, int _center);

        void initBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font, const QColor& _color, const QColor& _linkColor, const QColor& _selectionColor, const QColor& _highlightColor, HorAligment _align, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR, const LinksStyle _linksStyle = LinksStyle::PLAIN);

        void selectBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& from, const QPoint& to);

        void selectAllBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void fixBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void clearBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void releaseBlocksSelection(const std::vector<BaseDrawingBlockPtr>& _blocks);

        bool isBlocksSelected(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void blocksClicked(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p);

        void blocksDoubleClicked(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback);

        bool isOverLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p);

        QString getBlocksLink(const std::vector<BaseDrawingBlockPtr>& _blocks, const QPoint& _p);

        QString getBlocksSelectedText(const std::vector<BaseDrawingBlockPtr>& _blocks, TextType _type = TextType::VISIBLE);

        QString getBlocksText(const std::vector<BaseDrawingBlockPtr>& _blocks);

        QString getBlocksSourceText(const std::vector<BaseDrawingBlockPtr>& _blocks);

        bool isAllBlocksSelected(const std::vector<BaseDrawingBlockPtr>& _blocks);

        int getBlocksDesiredWidth(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void elideBlocks(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width, ELideType _type);

        bool isBlocksElided(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void setBlocksMaxLinesCount(const std::vector<BaseDrawingBlockPtr>& _blocks, int _count, LineBreakType _lineBreak);

        void setBlocksLastLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks, int _width);

        void applyBLocksLinks(const std::vector<BaseDrawingBlockPtr>& _blocks, const std::map<QString, QString>& _links);

        void applyLinksFont(const std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font);

        void setBlocksColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color);

        void setBlocksLinkColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color);

        void setBlocksSelectionColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color);

        void setBlocksColorForAppended(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color, int _appended);

        void appendBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from);

        void appendWholeBlocks(std::vector<BaseDrawingBlockPtr>& _to, std::vector<BaseDrawingBlockPtr>& _from);

        bool markdownBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const QFont& _font, const QColor& _color, ProcessLineFeeds _processLineFeeds = ProcessLineFeeds::KEEP_LINE_FEEDS);

        void highlightBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const bool _isHighlighted);

        void highlightParts(std::vector<BaseDrawingBlockPtr>& _blocks, const highlightsV& _entries);

        void underlineBlocks(std::vector<BaseDrawingBlockPtr>& _blocks, const bool _enabled);

        int getBlocksLastLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks);

        int getBlocksMaxLineWidth(const std::vector<BaseDrawingBlockPtr>& _blocks);

        void setBlocksHighlightedTextColor(const std::vector<BaseDrawingBlockPtr>& _blocks, const QColor& _color);
    }
}
