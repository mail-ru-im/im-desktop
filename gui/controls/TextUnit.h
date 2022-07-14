#pragma once
#include "textrendering/TextRendering.h"

#include "../styles/StyleVariable.h"
#include "../styles/ThemeColor.h"

namespace Ui
{
    using highlightsV = std::vector<QString>;

    [[nodiscard]] QString getEllipsis();

    enum class ParseBackticksPolicy
    {
        KeepBackticks,
        ParseSingles,
        ParseTriples,
        ParseSinglesAndTriples,
    };

    namespace TextRendering
    {
        class TextUnit
        {
        public:
            struct InitializeParameters
            {
                InitializeParameters(QFont _font, Styling::ColorParameter _colorKey)
                    : font_ { _font }
                    , monospaceFont_ { font_ }
                    , color_ { std::move(_colorKey) }
                {}

                void setFonts(const QFont& _font)
                {
                    font_ = _font;
                    monospaceFont_ = _font;
                }

                QFont font_;
                QFont monospaceFont_;
                Styling::ColorParameter color_;
                Styling::ThemeColorKey linkColor_;                      // color of links
                Styling::ThemeColorKey selectionColor_;                 // color of selection
                Styling::ThemeColorKey highlightColor_;                 // color of highlighted text; currlently disabled
                HorAligment align_ = HorAligment::LEFT;                 // horizontal alignment of the text
                LineBreakType lineBreak_ = LineBreakType::PREFER_WIDTH; // type of line's break, see LineBreakType for details
                EmojiSizeType emojiSizeType_ = EmojiSizeType::REGULAR;
                LinksStyle linksStyle_ = LinksStyle::PLAIN;
                int maxLinesCount_ = -1; // maximum count of lines, last line will be elided by "..."
            };

            TextUnit();
            TextUnit(const Data::FString& _text, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType);

            void setText(const QString& _text, const Styling::ThemeColorKey& _color = {}); // set or replace existing text; also init blocks and evaluate height by desired width
            void setText(const Data::FString& _text, const Styling::ThemeColorKey& _color = {}); // set or replace existing text; also init blocks and evaluate height by desired width

            void setMentions(const Data::MentionMap& _mentions);//set mentions, find they in text and replace by clickable links
            const Data::MentionMap& getMentions() const;//get mentions

            void setTextAndMentions(const QString& _text, const Data::MentionMap& _mentions);//setText() + setMentions()
            void setTextAndMentions(const Data::FString& _text, const Data::MentionMap& _mentions);//setText() + setMentions()

            void elide(int _width); //elide text by specified width;
            [[nodiscard]] bool isElided() const;//check if TextUnit's elided

            //some stuff for replacing blocks in debug mode
            using condition_callback_t = std::function<bool(BaseDrawingBlockPtr&)>;
            using modify_callback_t = std::function<void(BaseDrawingBlockPtr&, size_t)>;

            void forEachBlockOfType(BlockType _blockType, const modify_callback_t& _modifyCallback);
            [[nodiscard]] bool removeBlocks(BlockType _blockType, const condition_callback_t& _removeConditionCallback);
            [[nodiscard]] bool replaceBlock(size_t _atIndex, BaseDrawingBlockPtr&& _newBlock, const condition_callback_t& _replaceConditionCallback);
            [[nodiscard]] bool insertBlock(BaseDrawingBlockPtr&& _block, size_t _atIndex);

            void assignBlock(BaseDrawingBlockPtr&& _block);
            void assignBlocks(std::vector<BaseDrawingBlockPtr>&& _blocks);
            //

            void setOffsets(int _horOffset, int _verOffset);//set vertical and horizontal offsets, they will be used for text's drawing
            inline void setOffsets(const QPoint& _offsets) { setOffsets(_offsets.x(), _offsets.y()); }

            [[nodiscard]] int horOffset() const;//get horizontal offset
            [[nodiscard]] int verOffset() const;//get vertical offset
            [[nodiscard]] QPoint offsets() const;//get offsets as QPoint

            void draw(QPainter& _painter, VerPosition _pos = VerPosition::TOP);//draw text; specify _pos for determinate text's vertilcal position (see VerPosition for details)

            void drawSmart(QPainter& _painter, int _center);//for history messages; draw with VerPosition::TOP if text has multiple lines, otherwise draw with VerPosition::MIDDLE and _center as vertical offset

            //size calculation; TEXT CAN"T BE PAINTED WITHOUT IT
            int getHeight(int _width, CallType _calltype = CallType::USUAL); //get height of the text for specified width

            void evaluateDesiredSize(); //calc height by text's desired width

            void init(const InitializeParameters& _parameters);

            void setLastLineWidth(int _width); //only for units with _maxLinesCount != -1

            size_t getLinesCount() const; // get actual line count

            void select(QPoint _from, QPoint _to);//select text between _from and _to

            void selectAll();//select all text

            void fixSelection();//temporarily fix the selection; selected text can't be unselected until releaseSelection() is called;

            void clearSelection();//clear selected text

            void releaseSelection();//release fixed selection

            [[nodiscard]] bool isSelected() const;//check text is selected

            void clicked(const QPoint& _p);//call clicked action for a word under cursor specified by _p; open links, mentions etc.

            void doubleClicked(const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback = std::function<void(bool)>());//call double clicked action for a word under cursor specified by _p

            [[nodiscard]] bool isOverLink(const QPoint& _p) const;//check cursor is over link; cursor specified by _p

            [[nodiscard]] Data::LinkInfo getLink(const QPoint& _p) const;//get link under cursor, returns QString() by default

            [[nodiscard]] std::optional<TextWordWithBoundary> getWordAt(QPoint) const; //get word
            [[nodiscard]] bool replaceWordAt(const Data::FString& _old, const Data::FString& _new, QPoint _p);

            [[nodiscard]] Data::FString getSelectedText(TextType _type = TextType::VISIBLE) const;//get selected text, visible or source (depends on _type)

            [[nodiscard]] QString getText() const;//get visible text

            [[nodiscard]] Data::FString getTextInstantEdit() const;//get text for instant edit

            [[nodiscard]] Data::FString getSourceText() const;//get source text

            [[nodiscard]] bool isAllSelected() const;//check all text is selected

            [[nodiscard]] int desiredWidth() const;//get text's desired width

            int sourceTextWidth() const;
            int textWidth() const;

            void applyLinks(const std::map<QString, QString>& _links);//set links; like <a href=second>first</a> (not literally)
            void applyFontToLinks(const QFont& _font); // set custom font to links

            [[nodiscard]] bool contains(const QPoint& _p) const;//check TextUnit's rect contains _p

            [[nodiscard]] QSize cachedSize() const;// get cached size

            void setColor(const Styling::ColorParameter& _color);
            void setLinkColor(const Styling::ThemeColorKey& _color);
            void setSelectionColor(const Styling::ThemeColorKey& _color);
            void setHighlightedTextColor(const Styling::ThemeColorKey& _color); // set color for highlighted text
            void setHighlightColor(const Styling::ThemeColorKey& _color);

            [[nodiscard]] Styling::ThemeColorKey getColorKey() const; // get text color
            [[nodiscard]] QColor getColor() const;                    // get text color

            [[nodiscard]] Styling::ThemeColorKey getLinkColorKey() const;

            [[nodiscard]] HorAligment getAlign() const; // get text alignment

            void setAlign(HorAligment _align);

            void setColorForAppended(const Styling::ThemeColorKey& _color); // set text color for appended blocks

            [[nodiscard]] const QFont& getFont() const;

            void append(std::unique_ptr<TextUnit>&& _other);//append words from _other to TextUnit

            void appendBlocks(std::unique_ptr<TextUnit>&& _other);//append blocks from _other to TextUnit (usually block is one line of a text separated by QChar::LineFeed)

            void setLineSpacing(int _spacing);//add _spacing to line's height; experemental function, don't use it

            void setHighlighted(const bool _isHighlighted); // set all word highlighted attribute
            void setHighlighted(const highlightsV& _entries); // set all matching words and word-parts highlighted

            void setUnderline(const bool _enabled); //set underline

            int getLastLineWidth() const; //get width of last line; getHeight() or evaluateDesiredSize() must be called before

            int getMaxLineWidth() const; //get max line width; getHeight() or evaluateDesiredSize() must be called before

            void setEmojiSizeType(const EmojiSizeType _emojiSizeType);

            LinksVisible linksVisibility() const;

            bool needsEmojiMargin() const;
            size_t getEmojiCount() const;

            bool isEmpty() const;

            void disableCommands();

            void startSpellChecking(std::function<bool()> isAlive, std::function<void(bool)> onFinish); // start async function to spell check
            static bool isSkippableWordForSpellChecking(const TextWordWithBoundary& w);

            int64_t blockId() const noexcept { return blockId_; }

            [[nodiscard]] bool hasEmoji() const;

            void setShadow(const int _offsetX, const int _offsetY, const QColor& _color);

            std::vector<QRect> getLinkRects() const;

            bool mightStretchForLargerWidth() const;

            const std::vector<BaseDrawingBlockPtr>& blocks() const { return blocks_; }

        protected:
            Data::FString sourceText_;

            TextUnit(std::vector<BaseDrawingBlockPtr>&& _blocks, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType);
            void setSourceText(const Data::FString& _text, ProcessLineFeeds _processLineFeeds);
            void appendToSourceText(TextUnit& _suffix);

        private:
            void initializeBlocks();
            void initializeBlocksAndSelectedTextColor();
            [[nodiscard]] QPoint mapPoint(QPoint) const;
            void updateColors();

        private:
            std::vector<BaseDrawingBlockPtr> blocks_;
            int horOffset_ = 0;
            int verOffset_ = 0;

            Data::MentionMap mentions_;
            LinksVisible showLinks_;
            ProcessLineFeeds processLineFeeds_;
            EmojiSizeType emojiSizeType_;

            QFont font_;
            QFont monospaceFont_;
            Styling::ColorContainer color_;
            Styling::ColorContainer linkColor_;
            Styling::ColorContainer selectionColor_;
            Styling::ColorContainer highlightColor_;
            Styling::ColorContainer highlightTextColor_;
            QSize cachedSize_;
            HorAligment align_ = HorAligment::LEFT;
            int maxLinesCount_ = -1;
            int appended_ = -1;
            LineBreakType lineBreak_ = LineBreakType::PREFER_WIDTH;
            LinksStyle linksStyle_;
            int lineSpacing_ = 0;
            bool needsEmojiMargin_ = false;

            int64_t blockId_ = 0;

            std::shared_ptr<bool> guard_;
            TextRendering::VerPosition lastVerPosition_ = TextRendering::VerPosition::TOP;
        };

        using TextUnitPtr = std::unique_ptr<TextUnit>;

        TextUnitPtr MakeTextUnit(
            const Data::FString& _text,
            const Data::MentionMap& _mentions = Data::MentionMap(),
            LinksVisible _showLinks = LinksVisible::SHOW_LINKS,
            ProcessLineFeeds _processLineFeeds = ProcessLineFeeds::KEEP_LINE_FEEDS,
            EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR,
            ParseBackticksPolicy _backticksPolicy = ParseBackticksPolicy::KeepBackticks);

        TextUnitPtr MakeTextUnit(
            const QString& _text,
            const Data::MentionMap& _mentions = Data::MentionMap(),
            LinksVisible _showLinks = LinksVisible::SHOW_LINKS,
            ProcessLineFeeds _processLineFeeds = ProcessLineFeeds::KEEP_LINE_FEEDS,
            EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR);

        //some stuff for replacing blocks in debug mode
        bool InsertOrUpdateDebugMsgIdBlockIntoUnit(TextUnitPtr& _textUnit, qint64 _id, size_t _atPosition = 0);
        bool InsertDebugMsgIdBlockIntoUnit(TextUnitPtr& _textUnit, qint64 _id, size_t _atPosition = 0);
        bool UpdateDebugMsgIdBlock(TextUnitPtr& _textUnit, qint64 _newId);
        bool RemoveDebugMsgIdBlocks(TextUnitPtr& _textUnit);
        void ExtractLinkWords(TextUnitPtr& _textUnit, std::vector<const TextWord*>& _words);
    } // namespace TextRendering
} // namespace Ui
