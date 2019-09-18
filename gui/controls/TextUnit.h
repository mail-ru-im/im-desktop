#pragma once
#include "TextRendering.h"

namespace Styling
{
    enum class StyleVariable;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit
        {
        public:
            TextUnit();
            TextUnit(const QString& _text, std::vector<BaseDrawingBlockPtr>&& _blocks, const Data::MentionMap& _mentions, LinksVisible _showLinks, ProcessLineFeeds _processLineFeeds, EmojiSizeType _emojiSizeType);

            void setText(const QString& _text, const QColor& _color = QColor());//set or replace existing text; also init blocks and evaluate height by desired width
            void setMentions(const Data::MentionMap& _mentions);//set mentions, find they in text and replace by clickable links
            const Data::MentionMap& getMentions() const;//get mentions

            void setTextAndMentions(const QString& _text, const Data::MentionMap& _mentions);//setText() + setMentions()

            void elide(int _width, ELideType _type = ELideType::ACCURATE);//elide text by specified width;
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
            int getHeight(int width, CallType _calltype = CallType::USUAL); //get height of the text for specified width

            void evaluateDesiredSize(); //calc height by text's desired width

            void init(const QFont& _font, const QColor& _color, const QColor& _linkColor = QColor(), const QColor& _selectionColor = QColor(), const QColor& _highlightColor = QColor(), HorAligment _align = HorAligment::LEFT, int _maxLinesCount = -1, LineBreakType _lineBreak = LineBreakType::PREFER_WIDTH, EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR, const LinksStyle _linksStyle = LinksStyle::PLAIN);
            //set initial parameters
            //_linkColor - color of links
            //_selectionColor - color of selection
            //_hightLightColor - color of highlighted text; currlently disabled
            //_align - horizontal alignment of the text
            //_maxLinesCount - maximum count of lines, last line will be elided by "..."
            //_lineBreak - type of line's break, see LineBreakType for details

            void setLastLineWidth(int _width); //only for units with _maxLinesCount != -1

            int getLineCount() const; // get actual line count

            void select(const QPoint& _from, const QPoint& _to);//select text between _from and _to

            void selectAll();//select all text

            void fixSelection();//temporarily fix the selection; selected text can't be unselected until releaseSelection() is called;

            void clearSelection();//clear selected text

            void releaseSelection();//release fixed selection

            [[nodiscard]] bool isSelected() const;//check text is selected

            void clicked(const QPoint& _p);//call clicked action for a word under cursor specified by _p; open links, mentions etc.

            void doubleClicked(const QPoint& _p, bool _fixSelection, std::function<void(bool)> _callback = std::function<void(bool)>());//call double clicked action for a word under cursor specified by _p

            [[nodiscard]] bool isOverLink(const QPoint& _p) const;//check cursor is over link; cursor specified by _p

            [[nodiscard]] QString getLink(const QPoint& _p) const;//get link under cursor, returns QString() by default

            [[nodiscard]] QString getSelectedText(TextType _type = TextType::VISIBLE) const;//get selected text, visible or source (depends on _type)

            [[nodiscard]] QString getText() const;//get visible text

            [[nodiscard]] QString getSourceText() const;//get source text

            [[nodiscard]] bool isAllSelected() const;//check all text is selected

            [[nodiscard]] int desiredWidth() const;//get text's desired width

            void applyLinks(const std::map<QString, QString>& _links);//set links; like <a href=second>first</a> (not literally)
            void applyFontToLinks(const QFont& _font); // set custom font to links

            [[nodiscard]] bool contains(const QPoint& _p) const;//check TextUnit's rect contains _p

            [[nodiscard]] QSize cachedSize() const;// get cached size

            void setColor(const QColor& _color);// set text color
            void setColor(const Styling::StyleVariable _var, const QString& _aimid = QString());
            void setSelectionColor(const Styling::StyleVariable _var, const QString& _aimid = QString());
            void setSelectionColor(const QColor _color);

            [[nodiscard]] QColor getColor() const;// get text color

            [[nodiscard]] HorAligment getAlign() const; // get text alignment

            void setColorForAppended(const QColor& _color);//set text color for appended blocks

            [[nodiscard]] const QFont& getFont() const;

            void append(std::unique_ptr<TextUnit>&& _other);//append words from _other to TextUnit

            void appendBlocks(std::unique_ptr<TextUnit>&& _other);//append blocks from _other to TextUnit (usually block is one line of a text separated by QChar::LineFeed)

            void setLineSpacing(int _spacing);//add _spacing to line's height; experemental function, don't use it

            void markdown(const QFont& _font, const QColor& _color, ProcessLineFeeds _linefeeds = ProcessLineFeeds::KEEP_LINE_FEEDS);// 'markdown' text with specified parameters;

            [[nodiscard]] bool sourceModified() const;//check source text is modified; f.e. markdown() modifies source text

            void setHighlighted(const bool _isHighlighted); // set all word highlighted attribute

            void setUnderline(const bool _enabled); //set underline

            int getLastLineWidth() const; //get width of last line; getHeight() or evaluateDesiredSize() must be called before

            int getMaxLineWidth() const; //get max line width; getHeight() or evaluateDesiredSize() must be called before

            void setEmojiSizeType(const EmojiSizeType _emojiSizeType);

            bool needsEmojiMargin() const;

            bool isEmpty() const;

        private:
            QString originText_;
            std::vector<BaseDrawingBlockPtr> blocks_;
            int horOffset_;
            int verOffset_;

            Data::MentionMap mentions_;
            LinksVisible showLinks_;
            ProcessLineFeeds processLineFeeds_;
            EmojiSizeType emojiSizeType_;

            QFont font_;
            QColor color_;
            QColor linkColor_;
            QColor selectionColor_;
            QColor highlightColor_;
            QSize cachedSize_;
            HorAligment align_;
            int maxLinesCount_;
            int appended_;
            LineBreakType lineBreak_;
            int lineSpacing_;
            bool sourceModified_;
            bool needsEmojiMargin_;
        };

        using TextUnitPtr = std::unique_ptr<TextUnit>;

        TextUnitPtr MakeTextUnit(
            const QString& _text,//text
            const Data::MentionMap& _mentions = Data::MentionMap(),//mentions
            LinksVisible _showLinks = LinksVisible::SHOW_LINKS,//show links ot not
            ProcessLineFeeds _processLineFeeds = ProcessLineFeeds::KEEP_LINE_FEEDS,//keep line feeds or cut
            EmojiSizeType _emojiSizeType = EmojiSizeType::REGULAR);//emoji size

        //some stuff for replacing blocks in debug mode
        bool InsertOrUpdateDebugMsgIdBlockIntoUnit(TextUnitPtr& _textUnit, qint64 _id, size_t _atPosition = 0);
        bool InsertDebugMsgIdBlockIntoUnit(TextUnitPtr& _textUnit, qint64 _id, size_t _atPosition = 0);
        bool UpdateDebugMsgIdBlock(TextUnitPtr& _textUnit, qint64 _newId);
        bool RemoveDebugMsgIdBlocks(TextUnitPtr& _textUnit);
        //
    }
}
