#pragma once

#include "../corelib/enumerations.h"

class QPushButton;

namespace Emoji
{
    struct EmojiRecord;
    typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;
    typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;
    class EmojiCode;
}

namespace Utils
{
    class MediaLoader;
}

namespace Ui
{
    namespace Stickers
    {
        class Set;
        class Sticker;
        typedef std::vector<std::shared_ptr<Stickers::Set>> setsArray;
        typedef std::shared_ptr<Set> setSptr;
    }

    using stickersArray = std::vector<QString>;

    class smiles_Widget;

    class DialogPlayer;

    namespace Smiles
    {
        class TabButton;
        class Toolbar;


        //////////////////////////////////////////////////////////////////////////
        // emoji_category
        //////////////////////////////////////////////////////////////////////////
        struct emoji_category
        {
            QString name_;
            const Emoji::EmojiRecordSptrVec& emojis_;

            emoji_category(const QString& _name, const Emoji::EmojiRecordSptrVec& _emojis)
                : name_(_name), emojis_(_emojis)
            {
            }
        };

        enum class EmojiSendSource
        {
            click,
            keyboard,
        };

        class SmilesMenuTable
        {
        public:
            virtual bool selectUp() = 0;
            virtual bool selectDown() = 0;
            virtual bool selectLeft() = 0;
            virtual bool selectRight() = 0;

            virtual void selectFirst() = 0;
            virtual void selectLast() = 0;

            virtual bool hasSelection() const = 0;
            virtual int selectedItemColumn() const = 0;

            virtual void selectFirstRowAtColumn(const int _column) = 0;
            virtual void selectLastRowAtColumn(const int _column) = 0;

            virtual void clearSelection() = 0;

            virtual ~SmilesMenuTable() = default;
        };


        //////////////////////////////////////////////////////////////////////////
        // EmojiViewItemModel
        //////////////////////////////////////////////////////////////////////////
        class EmojiViewItemModel : public QStandardItemModel
        {
            QSize prevSize_;
            int emojisCount_;
            int needHeight_;
            bool singleLine_;
            int spacing_;

            std::vector<emoji_category> emojiCategories_;

        public:
            EmojiViewItemModel(QWidget* _parent, bool _singleLine = false);

            QVariant data(const QModelIndex& _idx, int _role) const override;

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            bool resize(const QSize& _size, bool _force = false);
            int getEmojisCount() const;
            int getNeedHeight() const;
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            void onEmojiAdded();
            int spacing() const;
            void setSpacing(int _spacing);

            QModelIndex index(int _row, int _column, const QModelIndex& _parent = QModelIndex()) const override;

            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;

        private:
            int getLinearIndex(const int _row, const int _col) const;
        };




        //////////////////////////////////////////////////////////////////////////
        // EmojiTableItemDelegate
        //////////////////////////////////////////////////////////////////////////
        class EmojiTableItemDelegate : public QItemDelegate
        {
            Q_OBJECT

            Q_PROPERTY(int prop READ getProp WRITE setProp);

        public:
            EmojiTableItemDelegate(QObject* parent);

            void animate(const QModelIndex& index, int start, int end, int duration);

            void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
            QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

            int getProp() const { return Prop_; }
            void setProp(int val);

            void setHoverEnabled(const bool _enabled);
            bool isHoverEnabled() const noexcept;

        private:
            int Prop_;
            QPropertyAnimation* Animation_;
            QModelIndex AnimateIndex_;
            bool hoverEnabled_;
        };

        class EmojiTableView : public QTableView, public SmilesMenuTable
        {
            Q_OBJECT

            EmojiViewItemModel* model_;
            EmojiTableItemDelegate* itemDelegate_;

        private:
            void selectIndex(const QModelIndex& _index);

        Q_SIGNALS:
            void mouseMoved(QPrivateSignal) const;

        protected:
            void mouseMoveEvent(QMouseEvent*) override;
            void resizeEvent(QResizeEvent * _e) override;
            void leaveEvent(QEvent* _event) override;

        public:
            EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model);

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;
            void onEmojiAdded();

            bool selectUp() override;
            bool selectDown() override;
            bool selectLeft() override;
            bool selectRight() override;

            void selectFirst() override;
            void selectLast() override;

            bool hasSelection() const override;
            int selectedItemColumn() const override;
            void selectFirstRowAtColumn(const int _column) override;
            void selectLastRowAtColumn(const int _column) override;

            void clearSelection() override;
        };




        //////////////////////////////////////////////////////////////////////////
        // EmojisWidget
        //////////////////////////////////////////////////////////////////////////
        class EmojisWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:
            void emojiSelected(Emoji::EmojiRecordSptr _emoji, const QPoint _pos);
            void scrollToGroup(const int _pos);
            void emojiMouseMoved(QPrivateSignal) const;

        public:
            EmojisWidget(QWidget* _parent);

            void onViewportChanged(const QRect& _rect, bool _blockToolbarSwitch);
            void selectFirstButton();
            EmojiTableView* getView() const noexcept;

            bool sendSelectedEmoji();
            bool toolbarAtBottom() const;
            void updateToolbarSelection();

        private:
            void updateToolbar(const QRect& _viewRect);
            void placeToolbar(const QRect& _viewRect);

            void sendEmoji(const QModelIndex& _index, const EmojiSendSource _src);

        private:
            EmojiTableView* view_;

            Toolbar* toolbar_;
            std::vector<TabButton*> buttons_;
            bool toolbarAtBottom_;
        };




        class RecentsStickersTable;


        //////////////////////////////////////////////////////////////////////////
        // RecentsWidget
        //////////////////////////////////////////////////////////////////////////
        class RecentsWidget : public QWidget, public SmilesMenuTable
        {
            Q_OBJECT

        Q_SIGNALS:
            void emojiSelected(Emoji::EmojiRecordSptr _emoji, const QPoint _pos);
            void stickerSelected(qint32 _setId, const QString& _stickerId);
            void stickerHovered(qint32 _setId, const QString& _stickerId);
            void stickerPreview(qint32 _setId, const QString& _stickerId);
            void stickerPreviewClose();

        private Q_SLOTS:
            void stickers_event();
            void emojiMouseMoved();

        protected:
            void mouseReleaseEvent(QMouseEvent* _e) override;
            void mousePressEvent(QMouseEvent* _e) override;
            void mouseMoveEvent(QMouseEvent* _e) override;
            void leaveEvent(QEvent* _e) override;

        private:
            Emoji::EmojiRecordSptrVec emojis_;

            QVBoxLayout* vLayout_;
            EmojiTableView* emojiView_;
            RecentsStickersTable* stickersView_;
            bool previewActive_;
            bool mouseIntoStickers_;

            void init();
            void storeStickers();

            void onStickerMigration(const std::vector<Ui::Stickers::Sticker>& _stickers);

            void saveEmojiToSettings();

            bool selectionInStickers() const;

            bool sendSelectedEmoji();
            void sendEmoji(const QModelIndex& _index, const EmojiSendSource _src);

        public:
            RecentsWidget(QWidget* _parent);

            void addSticker(int32_t _set_id, const QString& _stickerOd);
            void addEmoji(Emoji::EmojiRecordSptr _emoji);
            void initEmojisFromSettings();
            void initStickersFromSettings();
            void onStickerUpdated(int32_t _setOd, const QString& _stickerOd);
            void resetPreview();

            bool sendSelected();

            bool selectUp() override;
            bool selectDown() override;
            bool selectLeft() override;
            bool selectRight() override;

            void selectFirst() override;
            void selectLast() override;

            bool hasSelection() const override;
            int selectedItemColumn() const override;
            void selectFirstRowAtColumn(const int _column) override;
            void selectLastRowAtColumn(const int _column) override;

            void clearSelection() override;

            void clearIfNotSelected(const QString& _stickerId);
        };



        //////////////////////////////////////////////////////////////////////////
        // StickersTable
        //////////////////////////////////////////////////////////////////////////
        class StickersTable : public QWidget, public SmilesMenuTable
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(qint32 _setId, const QString& _stickerId);
            void stickerPreview(qint32 _setId, const QString& _stickerId);
            void stickerHovered(qint32 _setId, const QString& _stickerId);

        private Q_SLOTS:
            void longtapTimeout();

        protected:

            QTimer* longtapTimer_;

            int32_t stickersSetId_;

            int needHeight_;

            QSize prevSize_;

            int columnCount_;

            int rowCount_;

            int32_t stickerSize_;

            int32_t itemSize_;

            std::pair<int32_t, QString> hoveredSticker_;

            bool keyboardActive_;

            void resizeEvent(QResizeEvent * _e) override;
            void paintEvent(QPaintEvent* _e) override;

            virtual bool resize(const QSize& _size, bool _force = false);

            virtual std::pair<int32_t, QString> getStickerFromPos(const QPoint& _pos) const;

            virtual void drawSticker(QPainter& _painter, const int32_t _setId, const QString& _stickerId, const QRect& _rect);

            virtual void redrawSticker(const int32_t _setId, const QString& _stickerId);

            int getNeedHeight() const;

        public:

            QRect getStickerRect(const int _index) const;
            QRect getSelectedStickerRect() const;

            void mouseReleaseEventInternal(const QPoint& _pos);
            void mousePressEventInternal(const QPoint& _pos);
            void mouseMoveEventInternal(const QPoint& _pos);
            void leaveEventInternal();

            virtual void onStickerUpdated(int32_t _setId, const QString& _stickerId);
            void onStickerAdded();

            virtual int32_t getStickerPosInSet(const QString& _stickerId) const;
            virtual const stickersArray& getStickerIds() const;

            StickersTable(
                QWidget* _parent,
                const int32_t _stickersSetId,
                const qint32 _stickerSize,
                const qint32 _itemSize,
                const bool _trasparentForMouse = true);

            virtual ~StickersTable();

            std::pair<int32_t, QString> getSelected() const;
            virtual void setSelected(const std::pair<int32_t, QString>& _sticker);

            bool sendSelected();
            void clearIfNotSelected(const QString& _stickerId);

            void clearSelection() override;

            bool selectUp() override;
            bool selectDown() override;
            bool selectLeft() override;
            bool selectRight() override;

            void selectFirst() override;
            void selectLast() override;

            bool hasSelection() const override;
            int selectedItemColumn() const override;
            void selectFirstRowAtColumn(const int _column) override;
            void selectLastRowAtColumn(const int _column) override;
        };





        //////////////////////////////////////////////////////////////////////////
        // RecentsStickersTable
        //////////////////////////////////////////////////////////////////////////
        class RecentsStickersTable : public StickersTable
        {
            stickersArray recentStickersArray_;

            int maxRowCount_;

            bool resize(const QSize& _size, bool _force = false) override;

        public:
            void clear();

            bool addSticker(int32_t _setId, const QString& _stickerId);

            void setMaxRowCount(int _val);

            RecentsStickersTable(QWidget* _parent, const qint32 _stickerSize, const qint32 _itemSize);

            int32_t getStickerPosInSet(const QString& _stickerId) const override;
            const stickersArray& getStickerIds() const override;

            void selectLast() override;
            bool selectRight() override;
            bool selectDown() override;
        };





        //////////////////////////////////////////////////////////////////////////
        // StickersWidget
        //////////////////////////////////////////////////////////////////////////
        class StickersWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(int32_t _setId, const QString& _stickerId);
            void stickerHovered(qint32 _setId, const QString& _stickerId);
            void stickerPreview(int32_t _setId, const QString& _stickerId);
            void stickerPreviewClose();
            void scrollTo(int _pos);

        private:
            QVBoxLayout* vLayout_;
            Toolbar* toolbar_;
            std::map<int32_t, StickersTable*> setTables_;
            bool previewActive_;

        private:
            void insertNextSet(int32_t _setId);
            void onMouseMoved(const QPoint& _pos);

        protected:
            void mouseReleaseEvent(QMouseEvent* _e) override;
            void mousePressEvent(QMouseEvent* _e) override;
            void mouseMoveEvent(QMouseEvent* _e) override;
            void leaveEvent(QEvent* _e) override;
            void wheelEvent(QWheelEvent* _e) override;

        public:
            void init();
            void clear();
            void onStickerUpdated(int32_t _setId, const QString& _stickerId);
            void resetPreview();

            void scrollToSticker(int32_t _setId, const QString& _stickerId);

            StickersWidget(QWidget* _parent, Toolbar* _toolbar);
        };







        //////////////////////////////////////////////////////////////////////////
        // StickerPreview
        //////////////////////////////////////////////////////////////////////////
        class StickerPreview : public QWidget
        {
            Q_OBJECT

        public:

            enum class Context
            {
                Picker,
                Popup
            };

            StickerPreview(
                QWidget* _parent,
                const int32_t _setId,
                const QString& _stickerId,
                Context _context);

            ~StickerPreview();

            void showSticker(
                const int32_t _setId,
                const QString& _stickerId);

            void hide();

        Q_SIGNALS:

            void needClose();

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mouseReleaseEvent(QMouseEvent* _e) override;

        private Q_SLOTS:
            void onGifLoaded(const QString& _path);
            void onActivationChanged(bool _active);

        private:

            void init(const int32_t _setId, const QString& _stickerId);

            void drawSticker(QPainter& _p);

            void drawEmoji(QPainter& _p);

            void updateEmoji();

            int getTopImageMargin() const;

            int getCommonImageMargin() const;

            int getBottomImageMargin() const;

            QRect getImageRect() const;
            QRect getAdjustedImageRect() const;

            void loadSticker();

            void scaleSticker();

            int32_t setId_;
            QString stickerId_;

            QPixmap sticker_;
            std::vector<QImage> emojis_;

            Context context_;
            std::unique_ptr<DialogPlayer> player_;
            std::unique_ptr<Utils::MediaLoader> loader_;

            bool hiding_ = false;
        };







        //////////////////////////////////////////////////////////////////////////
        // SmilesMenu
        //////////////////////////////////////////////////////////////////////////
        class SmilesMenu : public QFrame
        {
            Q_OBJECT
            Q_PROPERTY(int currentHeight READ getCurrentHeight WRITE setCurrentHeight)

        Q_SIGNALS:
            void emojiSelected(const Emoji::EmojiCode&, const QPoint _pos);
            void stickerSelected(int32_t _setId, const QString& _stickerId);
            void visibilityChanged(const bool _isVisible, QPrivateSignal) const;
            void scrolled();

        private Q_SLOTS:
            void im_created();
            void touchScrollStateChanged(QScroller::State);
            void stickersMetaEvent();
            void stickerEvent(const qint32 _error, const qint32 _setId, const QString& _stickerId);
            void onScroll(int _value);
            void hideStickerPreview();

        public:
            SmilesMenu(QWidget* _parent);
            ~SmilesMenu();

            void showAnimated();
            void hideAnimated();
            void showHideAnimated(const bool _fromKeyboard = false);
            bool isHidden() const;

            void setCurrentHeight(int _val);
            int getCurrentHeight() const;
            void addStickerToRecents(const qint32 _setId, const QString& _stickerId);

            void clearSelection();
            bool hasSelection() const;

        private:
            Toolbar* topToolbar_;
            Toolbar* bottomToolbar_;

            QScrollArea* viewArea_;

            RecentsWidget* recentsView_;
            EmojisWidget* emojiView_;
            StickersWidget* stickersView_;
            StickerPreview* stickerPreview_;
            QVBoxLayout* rootVerticalLayout_;
            QPropertyAnimation* animHeight_;
            QPropertyAnimation* animScroll_;

            bool isVisible_;
            bool blockToolbarSwitch_;
            bool stickerMetaRequested_;
            int currentHeight_;
            bool setFocusToButton_;

        private:
            void InitSelector();
            void InitStickers();
            void InitRecents();

            void selectItemUp();
            void selectItemDown();
            void selectItemLeft();
            void selectItemRight();
            void selectLastAtColumn(const int _column);
            void selectFirstAtColumn(const int _column);
            void selectFirst();
            void selectLast();

            void clearTablesSelection();

            bool sendSelectedItem();
            void ensureSelectedVisible();

            SmilesMenuTable* getCurrentMenuTable() const;
            QWidget* getCurrentView() const;

            void ScrollTo(const int _pos);
            void HookScroll();
            void showRecents();

            void showStickerPreview(const int32_t _setId, const QString& _stickerId);
            void updateStickerPreview(const int32_t _setId, const QString& _stickerId);
            void onStickerHovered(const int32_t _setId, const QString& _stickerId);
            void onEmojiHovered();

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void focusOutEvent(QFocusEvent* _event) override;
            void resizeEvent(QResizeEvent * _e) override;
            void keyPressEvent(QKeyEvent* _event) override;
            bool focusNextPrevChild(bool) override;
        };
    }
}
