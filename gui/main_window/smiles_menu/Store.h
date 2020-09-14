#pragma once
#include "../../controls/TextUnit.h"

namespace Ui
{
    class ScrollAreaWithTrScrollBar;
    class TextEmojiWidget;
    class LineEditEx;
    class CustomButton;
    class ClickableWidget;

    enum class FrameCountMode;

    namespace TextRendering
    {
        class TextUnit;
    }

    struct PackInfo
    {
        int32_t id_;

        QString name_;
        QString subtitle_;
        QString storeId_;
        QPixmap icon_;
        bool purchased_;
        bool iconRequested_;

        PackInfo() : id_(-1), purchased_(false), iconRequested_(false) {}
        PackInfo(
            const int32_t _id,
            const QString& _name,
            const QString& _subtitle,
            const QString& _storeId,
            const QPixmap& _icon,
            bool _purchased,
            bool _iconRequested)
            : id_(_id)
            , name_(_name)
            , subtitle_(_subtitle)
            , storeId_(_storeId)
            , icon_(_icon)
            , purchased_(_purchased)
            , iconRequested_(_iconRequested)
        {
        }
    };


    namespace Stickers
    {

        class PackInfoObject : public QObject, public PackInfo
        {
            Q_OBJECT

        public:
            PackInfoObject(QObject* parent = nullptr) : QObject(parent) { }
            PackInfoObject(
                QObject* parent,
                const int32_t _id,
                const QString& _name,
                const QString& _subtitle,
                const QString& _storeId,
                const QPixmap& _icon,
                bool _purchased,
                bool _iconRequested)
                : QObject(parent), PackInfo(_id, _name, _subtitle, _storeId, _icon, _purchased, _iconRequested)
            {
            }
        };

        class AccessibleStickerPackButton : public QAccessibleObject
        {
        public:
            AccessibleStickerPackButton(PackInfoObject* object) : QAccessibleObject(object) { }

            bool isValid() const override { return true; }
            QAccessibleInterface* parent() const override;
            int childCount() const override { return 0; }
            QAccessibleInterface* child(int index) const override { return nullptr; }
            int indexOfChild(const QAccessibleInterface* child) const override { return -1; }
            QRect rect() const override;
            QString text(QAccessible::Text t) const override { return QAccessible::Text::Name == t ? qsl("AS Stickers PackButton") : QString(); }
            QAccessible::Role role() const override { return QAccessible::Role::Button; }
            QAccessible::State state() const override { return {}; }
        };

        //////////////////////////////////////////////////////////////////////////
        class TopPacksView : public QWidget
        {
            Q_OBJECT

            ScrollAreaWithTrScrollBar* parent_;

            std::vector<PackInfoObject*> packs_;
            std::unordered_map<int, TextRendering::TextUnitPtr> nameUnits_;
            std::unordered_map<int, TextRendering::TextUnitPtr> descUnits_;


            int hoveredPack_;

            QPropertyAnimation* animScroll_;

        private:

            enum direction
            {
                left = 0,
                right = 1
            };

            static QRect getStickerRect(const int _pos);
            void scrollStep(direction _direction);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void wheelEvent(QWheelEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;
            virtual void leaveEvent(QEvent* _e) override;
            virtual void mouseMoveEvent(QMouseEvent* _e) override;


        public:

            TopPacksView(ScrollAreaWithTrScrollBar* _parent);

            void addPack(PackInfoObject* _pack);

            void updateSize();

            void onSetIcon(const int32_t _setId);

            void clear();
        };



        //////////////////////////////////////////////////////////////////////////
        class PacksWidget : public QWidget
        {
            Q_OBJECT

            TopPacksView* packs_;

            ScrollAreaWithTrScrollBar* scrollArea_;

        public:

            PacksWidget(QWidget* _parent);

            void init(const bool _fromServer);
            void onSetIcon(const int32_t _setId);
        };

        //////////////////////////////////////////////////////////////////////////
        class PacksView : public QWidget
        {
            Q_OBJECT

        public:

            enum class Mode {my_packs, search};

            enum CursorAction { MoveUp, MoveDown, MovePageUp, MovePageDown};

            PacksView(Mode _mode, QWidget* _parent = 0);

            void addPack(PackInfoObject* _pack);

            void updateSize();

            void clear();

            void onSetIcon(const int32_t _setId);

            bool empty() const;

            int moveCursor(CursorAction cursorAction);

            int selectedPackId() const;

            bool isDragInProgress() const;

            void setScrollBar(QWidget* _scrollBar);

            void selectFirst();

        Q_SIGNALS:

            void startScrollUp();
            void startScrollDown();
            void stopScroll();

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;
            virtual void mouseReleaseEvent(QMouseEvent* _e) override;
            virtual void mouseMoveEvent(QMouseEvent* _e) override;
            virtual void leaveEvent(QEvent *_e) override;
            virtual void wheelEvent(QWheelEvent *_e) override;

            int getPackInfoIndex(PackInfoObject* _packInfo) const;
            QRect getStickerRect(const int _pos) const;
            QRect getDelButtonRect(const QRect& _stickerRect) const;
            QRect getAddButtonRect(const QRect& _stickerRect) const;
            QRect getDragButtonRect(const QRect& _stickerRect) const;

        private:

            void startDrag(const int _packNum, const QPoint& _mousePos);
            void drawStickerPack(QPainter& _p, const QRect& _stickerRect, PackInfo& _pack, bool _hovered = false, bool _selected = false);

            int getDragPackNum();

            int getPackUnderCursor() const;

            void clearSelection();

            void updateHover();

            void updateCursor();

            std::vector<PackInfoObject*> packs_;

            std::unordered_map<int, TextRendering::TextUnitPtr> nameUnits_;
            std::unordered_map<int, TextRendering::TextUnitPtr> descUnits_;

            Mode mode_;

            QPoint startDragPos_;

            int dragPack_;

            int dragOffset_;

            int selectedIdx_;

            int hoveredIdx_;

            QPointer<QWidget> scrollBar_;

            void postStickersOrder() const;

            friend class AccessiblePacksView;
            friend class AccessibleStickerPackButton;
        };

        class AccessiblePacksView : public QAccessibleWidget
        {
        public:
            AccessiblePacksView(PacksView* widget = nullptr) : QAccessibleWidget(widget), packsView_(widget) { }

            int childCount() const override { return packsView_->packs_.size(); }

            QAccessibleInterface* child(int index) const override;

            int indexOfChild(const QAccessibleInterface* child) const override;

            QString	text(QAccessible::Text t) const override;

        protected:
            PacksView* packsView_ = nullptr;
        };

        //////////////////////////////////////////////////////////////////////////
        class MyPacksHeader : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:
            void addClicked(QPrivateSignal);

        public:
            MyPacksHeader(QWidget* _parent);

        protected:
            void paintEvent(QPaintEvent* _e) override;
            void resizeEvent(QResizeEvent* _e) override;

        private:
            TextRendering::TextUnitPtr createStickerPack_;
            TextRendering::TextUnitPtr myText_;
            ClickableWidget* buttonAdd_;
            const int marginText_;
            const int buttonWidth_;
        };


        //////////////////////////////////////////////////////////////////////////
        class MyPacksWidget : public QWidget
        {
            Q_OBJECT

            PacksView* packs_;

            bool syncedWithServer_;

            QVariantAnimation* scrollAnimationUp_;
            QVariantAnimation* scrollAnimationDown_;

        private Q_SLOTS:

            void createPackButtonClicked();

            void onScrollUp();
            void onScrollDown();
            void onStopScroll();

        Q_SIGNALS:
            void scrollStep(const int _step);

        public:

            void onSetIcon(const int32_t _setId);

            MyPacksWidget(QWidget* _parent);

            void init(const bool _fromServer);

            void setScrollBarToView(QWidget* _scrollBar);

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;
        };

        //////////////////////////////////////////////////////////////////////////
        class SearchPacksWidget : public QWidget
        {
            Q_OBJECT

            PacksView* packs_;
            bool syncedWithServer_;
            QWidget* noResultsWidget_;
            bool searchRequested_;

        public:

            SearchPacksWidget(QWidget* _parent);
            void onSetIcon(const int32_t _setId);
            void init(const bool _fromServer);

            int moveCursor(PacksView::CursorAction cursorAction);
            int selectedPackId();
            void setScrollBarToView(QWidget* _scrollBar);
            void touchSearchRequested(); // for statistics
        };


        class StickersPageHeader;

        //////////////////////////////////////////////////////////////////////////
        class Store : public QWidget
        {
            Q_OBJECT

            ScrollAreaWithTrScrollBar* rootScrollArea_;

            PacksWidget* packsView_;
            MyPacksWidget* myPacks_;
            SearchPacksWidget* searchPacks_;
            StickersPageHeader* headerWidget_;

        private Q_SLOTS:

            void touchScrollStateChanged(QScroller::State _state);
            void onSetBigIcon(const qint32 _setId);
            void onStickers();
            void onStore();
            void onSearchStore();
            void onSearchRequested(const QString &term);
            void onSearchCancelled();
            void onScrollStep(const int _step);

        Q_SIGNALS:
            void back();

        protected:

            virtual void resizeEvent(QResizeEvent * event) override;
            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void keyPressEvent(QKeyEvent* _e) override;

        public:

            Store(QWidget* _parent);
            void hideSearch();

            void setBadgeText(const QString& _text);
            void setFrameCountMode(FrameCountMode _mode);
        };

        class BackToChatsHeader : public QWidget
        {
            Q_OBJECT;

        public:
            explicit BackToChatsHeader(QWidget* _parent = nullptr);
            ~BackToChatsHeader();

            void setText(const QString& _text);
            void setBadgeText(const QString& _text);
            void setBadgeVisible(bool _value);

        Q_SIGNALS:
            void backClicked(QPrivateSignal) const;
            void searchClicked(QPrivateSignal) const;

        protected:
            void paintEvent(QPaintEvent* _event) override;

        private:
            std::unique_ptr<Ui::TextRendering::TextUnit> textUnit_;
            std::unique_ptr<Ui::TextRendering::TextUnit> textBadgeUnit_;
            CustomButton* backButton_;
            CustomButton* searchButton_;
            bool badgeVisible_;
        };

        //////////////////////////////////////////////////////////////////////////
        class StickersPageHeader : public QFrame
        {
            Q_OBJECT

        public:
            explicit StickersPageHeader(QWidget *_parent = nullptr);

            QString currentTerm() const;
            void cancelSearch();

            void setBadgeText(const QString& _text);
            void setFrameCountMode(FrameCountMode _mode);

        Q_SIGNALS:
            void backClicked();
            void searchCancelled();
            void searchRequested(const QString& _searchTerm);

        private Q_SLOTS:
            void onSearch();
            void onSearchCancelled();
            void onSearchActivated();
            void onSearchTextChanged(const QString& _text);


        protected:
            bool eventFilter(QObject* _o, QEvent* _e) override;

        private:
            BackToChatsHeader* backToChatsHeader_;
            QWidget* searchHeader_;
            LineEditEx* searchInput_;
            QPushButton* searchButton_;
            QTimer searchTimer_;
            QString lastTerm_;
        };
    }
}
