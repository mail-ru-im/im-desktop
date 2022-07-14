#pragma once

#include "../corelib/enumerations.h"
#include "controls/FlowLayout.h"
#include "../history_control/complex_message/FileSharingUtils.h"

namespace Emoji
{
    struct EmojiRecord;
    using EmojiRecordVec = std::vector<EmojiRecord>;
    class EmojiCode;
}

namespace Ui
{
    class LottiePlayer;

    namespace Stickers
    {
        class Set;
        class Sticker;
        using stickerSptr = std::shared_ptr<Sticker>;
        typedef std::vector<std::shared_ptr<Stickers::Set>> setsArray;
        typedef std::shared_ptr<Set> setSptr;
        using stickersArray = std::vector<Utils::FileSharingId>;
    }

    namespace Smiles
    {
        class TabButton;
        class Toolbar;
        class StickerPreview;


        //////////////////////////////////////////////////////////////////////////
        // emoji_category
        //////////////////////////////////////////////////////////////////////////
        struct emoji_category
        {
            QLatin1String name_;
            const Emoji::EmojiRecordVec& emojis_;

            emoji_category(QLatin1String _name, const Emoji::EmojiRecordVec& _emojis)
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
            virtual bool isKeyboardActive() = 0;

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

            std::vector<emoji_category> emojiCategories_;

        public:
            EmojiViewItemModel(QWidget* _parent, bool _singleLine = false);

            QVariant data(const QModelIndex& _idx, int _role) const override;

            int addCategory(QLatin1String _category);
            int addCategory(const emoji_category& _category);
            bool resize(const QSize& _size, bool _force = false);
            int getEmojisCount() const;
            int getNeedHeight() const;
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            void onEmojiAdded();

            QModelIndex index(int _row, int _column, const QModelIndex& _parent = QModelIndex()) const override;

            const Emoji::EmojiRecord& getEmoji(int _col, int _row) const;

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

            int addCategory(QLatin1String _category);
            int addCategory(const emoji_category& _category);
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            const Emoji::EmojiRecord& getEmoji(int _col, int _row) const;
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

            bool isKeyboardActive() override;
        };




        //////////////////////////////////////////////////////////////////////////
        // EmojisWidget
        //////////////////////////////////////////////////////////////////////////
        class EmojisWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:
            void emojiSelected(const Emoji::EmojiRecord& _emoji, const QPoint _pos, QPrivateSignal) const;
            void scrollToGroup(const int _pos, QPrivateSignal) const;
            void emojiMouseMoved(QPrivateSignal) const;

        public:
            EmojisWidget(QWidget* _parent);

            void onViewportChanged(const QRect& _rect, bool _blockToolbarSwitch);
            void selectFirstButton();
            EmojiTableView* getView() const noexcept;

            bool sendSelectedEmoji();
            bool toolbarAtBottom() const;
            void updateToolbarSelection();

            bool isKeyboardActive() const;

            void updateTheme();

        protected:
            void resizeEvent(QResizeEvent* _event) override;

        private:
            void updateToolbar(const QRect& _viewRect);
            void placeToolbar(const QRect& _viewRect);

            void sendEmoji(const QModelIndex& _index, const EmojiSendSource _src) const;

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
            void emojiSelected(const Emoji::EmojiRecord& _emoji, const QPoint _pos);
            void stickerSelected(const Utils::FileSharingId& _stickerId);
            void stickerHovered(qint32 _setId, const Utils::FileSharingId& _stickerId);
            void stickerPreview(qint32 _setId, const Utils::FileSharingId& _stickerId);
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
            Emoji::EmojiRecordVec emojis_;

            QVBoxLayout* vLayout_;
            EmojiTableView* emojiView_;
            RecentsStickersTable* stickersView_;
            bool previewActive_;
            bool mouseIntoStickers_;

            void init();
            void storeStickers();

            void onStickerMigration(const std::vector<Ui::Stickers::stickerSptr>& _stickers);

            void saveEmojiToSettings();

            bool selectionInStickers() const;

            bool sendSelectedEmoji();
            void sendEmoji(const QModelIndex& _index, const EmojiSendSource _src);
            void addStickers(const std::vector<Utils::FileSharingId>& _stickers);

        public:
            RecentsWidget(QWidget* _parent);
            ~RecentsWidget();

            void addSticker(const Utils::FileSharingId& _stickerId);
            void addEmoji(const Emoji::EmojiRecord& _emoji);
            void initEmojisFromSettings();
            void initStickersFromSettings();
            void onStickerUpdated(int32_t _setOd, const Utils::FileSharingId& _stickerId);
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

            void clearIfNotSelected(const Utils::FileSharingId& _stickerId);

            bool isKeyboardActive() override;

            void onVisibilityChanged();
        };

        //////////////////////////////////////////////////////////////////////////
        // StickerWidget
        //////////////////////////////////////////////////////////////////////////
        class StickerWidget : public QWidget
        {
            Q_OBJECT

        public:
            StickerWidget(QWidget* _parent, const Utils::FileSharingId& _stickerId, int _itemSize, int _stickerSize);
            void clearCache();
            const Utils::FileSharingId& getId() const noexcept { return stickerId_; }
            void onVisibilityChanged(bool _visible);

        protected:
            void paintEvent(QPaintEvent* _e) override;

        private:
            Utils::FileSharingId stickerId_;
            QPixmap cached_;
            int stickerSize_;

            LottiePlayer* lottie_ = nullptr;
        };

        //////////////////////////////////////////////////////////////////////////
        // StickersTable
        //////////////////////////////////////////////////////////////////////////
        class StickersTable : public QWidget, public SmilesMenuTable
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(const Utils::FileSharingId& _stickerId);
            void stickerPreview(const Utils::FileSharingId& _stickerId);
            void stickerHovered(qint32 _setId, const Utils::FileSharingId& _stickerId);

        private Q_SLOTS:
            void longtapTimeout();

        protected:

            FlowLayout* layout_;

            QTimer* longtapTimer_;

            int32_t stickersSetId_;

            int needHeight_;

            QSize prevSize_;

            int columnCount_;

            int rowCount_;

            int32_t stickerSize_;

            int32_t itemSize_;

            std::pair<int32_t, Utils::FileSharingId> hoveredSticker_;

            bool keyboardActive_;

            void resizeEvent(QResizeEvent * _e) override;
            void paintEvent(QPaintEvent* _e) override;

            virtual bool resize(const QSize& _size, bool _force = false);

            virtual std::pair<int32_t, Utils::FileSharingId> getStickerFromPos(const QPoint& _pos) const;

            virtual void redrawSticker(const int32_t _setId, const Utils::FileSharingId& _stickerId);
            void populateStickerWidgets();

            int getNeedHeight() const;

        public:

            QRect getStickerRect(const int _index) const;
            QRect getSelectedStickerRect() const;

            void mouseReleaseEventInternal(const QPoint& _pos);
            void mousePressEventInternal(const QPoint& _pos);
            void mouseMoveEventInternal(const QPoint& _pos);
            void leaveEventInternal();

            virtual void onStickerUpdated(int32_t _setId, const Utils::FileSharingId& _stickerId);
            void onStickerAdded();

            void onVisibilityChanged();

            virtual int32_t getStickerPosInSet(const Utils::FileSharingId& _stickerId) const;
            virtual const Ui::Stickers::stickersArray& getStickerIds() const;

            StickersTable(
                QWidget* _parent,
                const int32_t _stickersSetId,
                const qint32 _stickerSize,
                const qint32 _itemSize,
                const bool _trasparentForMouse = true);

            ~StickersTable();

            std::pair<int32_t, Utils::FileSharingId> getSelected() const;
            virtual void setSelected(const std::pair<int32_t, Utils::FileSharingId>& _sticker);

            bool sendSelected();
            void clearIfNotSelected(const Utils::FileSharingId& _stickerId);

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

            bool isKeyboardActive() override;

            void cancelStickerRequests();
            void clearCache();
            void clearWidgets();
            void clear();
        };





        //////////////////////////////////////////////////////////////////////////
        // RecentsStickersTable
        //////////////////////////////////////////////////////////////////////////
        class RecentsStickersTable : public StickersTable
        {
            Ui::Stickers::stickersArray recentStickersArray_;

            int maxRowCount_;

            bool resize(const QSize& _size, bool _force = false) override;

        public:
            void clear();

            bool addSticker(const Utils::FileSharingId& _stickerId);

            void setMaxRowCount(int _val);

            RecentsStickersTable(QWidget* _parent, const qint32 _stickerSize, const qint32 _itemSize);

            int32_t getStickerPosInSet(const Utils::FileSharingId& _stickerId) const override;
            const Ui::Stickers::stickersArray& getStickerIds() const override;

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

            void stickerSelected(const Utils::FileSharingId& _stickerId);
            void stickerHovered(qint32 _setId, const Utils::FileSharingId& _stickerId);
            void stickerPreview(int32_t _setId, const Utils::FileSharingId& _stickerId);
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
            void onStickerUpdated(int32_t _setId, const Utils::FileSharingId& _stickerId);
            void resetPreview();

            void scrollToSticker(int32_t _setId, const Utils::FileSharingId& _stickerId);

            StickersWidget(QWidget* _parent, Toolbar* _toolbar);
        };


        //////////////////////////////////////////////////////////////////////////
        // SmilesMenu
        //////////////////////////////////////////////////////////////////////////
        class SmilesMenu : public QWidget
        {
            Q_OBJECT
            Q_PROPERTY(int currentHeight READ getCurrentHeight WRITE setCurrentHeight)

        Q_SIGNALS:
            void emojiSelected(const Emoji::EmojiCode&, const QPoint _pos);
            void stickerSelected(const Utils::FileSharingId& _stickerId);
            void visibilityChanged(const bool _isVisible, QPrivateSignal) const;
            void scrolled();

        private Q_SLOTS:
            void onSetIconChanged(int _setId);
            void touchScrollStateChanged(QScroller::State);
            void stickersMetaEvent();
            void stickerEvent(const qint32 _error, const qint32 _setId, const Utils::FileSharingId& _stickerId);
            void onScroll(int _value);
            void hideStickerPreview();
            void updateTheme();

        public:
            SmilesMenu(QWidget* _parent, const QString& _aimId = {});
            ~SmilesMenu();

            void showAnimated();
            void hideAnimated();
            void showHideAnimated(const bool _fromKeyboard = false);
            bool isHidden() const;

            void setCurrentHeight(int _val);
            int getCurrentHeight() const;
            void addStickerToRecents(const qint32 _setId, const Utils::FileSharingId& _stickerId);

            void clearSelection();
            bool hasSelection() const;

            void setToolBarVisible(bool _visible);
            void setRecentsVisible(bool _visible);
            void setStickersVisible(bool _visible);
            void setAddButtonVisible(bool _visible);
            void setHorizontalMargins(int _margin);
            void setDrawTopBorder(bool _draw);
            void setTopSpacing(int _spacing);

            void setPreviewAreaAdditions(QMargins _margins);

        private:
            Toolbar* topToolbar_ = nullptr;
            Toolbar* bottomToolbar_ = nullptr;
            TabButton* recentsButton_ = nullptr;

            QScrollArea* viewArea_ = nullptr;
            QBoxLayout* viewAreaLayout_ = nullptr;

            RecentsWidget* recentsView_ = nullptr;
            EmojisWidget* emojiView_ = nullptr;
            StickersWidget* stickersView_ = nullptr;
            StickerPreview* stickerPreview_ = nullptr;
            QVBoxLayout* rootVerticalLayout_ = nullptr;
            QPropertyAnimation* animHeight_ = nullptr;
            QPropertyAnimation* animScroll_ = nullptr;
            QSpacerItem* topSpacer_ = nullptr;

            bool isVisible_ = false;
            bool blockToolbarSwitch_ = false;
            int currentHeight_ = 0;
            bool lottieAllowed_ = false;
            bool drawTopBorder_ = true;
            QMargins previewMargins_;
            QString aimId_;

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

            void showStickerPreview(const int32_t _setId, const Utils::FileSharingId& _stickerId);
            void updateStickerPreview(const int32_t _setId, const Utils::FileSharingId& _stickerId);
            void onStickerHovered(const int32_t _setId, const Utils::FileSharingId& _stickerId);
            void onEmojiHovered();

            bool iskeyboardActive() const;

            void clearCache();
            void updateStickersVisibility();

            void onOmicronUpdate();

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void focusOutEvent(QFocusEvent* _event) override;
            void resizeEvent(QResizeEvent * _e) override;
            void keyPressEvent(QKeyEvent* _event) override;
            bool focusNextPrevChild(bool) override;
        };
    }
}
