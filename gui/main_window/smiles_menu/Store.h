#pragma once
#include "../../controls/TextUnit.h"
#include "controls/ClickWidget.h"
#include "cache/stickers/StickerData.h"

namespace Ui
{
    class ScrollAreaWithTrScrollBar;
    class TextEmojiWidget;
    class LineEditEx;
    class CustomButton;
    class LottiePlayer;

    enum class FrameCountMode;

    namespace TextRendering
    {
        class TextUnit;
    }

    namespace Stickers
    {
        struct PackInfo
        {
            int32_t id_ = -1;

            QString name_;
            QString subtitle_;
            QString storeId_;
            StickerData iconData_;
            bool purchased_ = false;
            bool iconRequested_ = false;

            PackInfo() = default;
            PackInfo(
                const int32_t _id,
                const QString& _name,
                const QString& _subtitle,
                const QString& _storeId,
                bool _purchased)
                : id_(_id)
                , name_(_name)
                , subtitle_(_subtitle)
                , storeId_(_storeId)
                , purchased_(_purchased)
            {
            }
        };

        //////////////////////////////////////////////////////////////////////////
        class PackInfoObject : public QObject, public PackInfo
        {
            Q_OBJECT

        Q_SIGNALS:
            void iconChanged(QPrivateSignal) const;

        public:
            PackInfoObject(QObject* parent = nullptr) : QObject(parent) { }
            PackInfoObject(
                QObject* parent,
                const int32_t _id,
                const QString& _name,
                const QString& _subtitle,
                const QString& _storeId,
                bool _purchased)
                : QObject(parent), PackInfo(_id, _name, _subtitle, _storeId, _purchased)
            {
            }

            void setIconData(StickerData _data)
            {
                iconData_ = std::move(_data);
                Q_EMIT iconChanged(QPrivateSignal());
            }
        };

        //////////////////////////////////////////////////////////////////////////
        class PackItemBase : public ClickableWidget
        {
            Q_OBJECT

        public:
            PackItemBase(QWidget* _parent, std::unique_ptr<PackInfoObject> _info);
            int getId() const noexcept { return info_->id_; }
            PackInfoObject* getInfo() const noexcept { return info_.get(); }

            void onVisibilityChanged(bool _visible);

        protected:
            void onIconChanged();
            void setLottie(const QString& _path);
            virtual QRect getIconRect() const = 0;

        protected:
            std::unique_ptr<PackInfoObject> info_;
            TextRendering::TextUnitPtr name_;
            TextRendering::TextUnitPtr desc_;

            QPixmap icon_;
            LottiePlayer* lottie_ = nullptr;
        };

        //////////////////////////////////////////////////////////////////////////
        class PacksViewBase
        {
        public:
            enum class InitFrom
            {
                local,
                server,
            };

            virtual void addPack(std::unique_ptr<PackInfoObject> _pack) = 0;
            virtual void onPacksAdded() = 0;
            virtual void clear() = 0;

            virtual ~PacksViewBase() = default;
        };

        //////////////////////////////////////////////////////////////////////////
        class TopPackItem : public PackItemBase
        {
            Q_OBJECT

        public:
            TopPackItem(QWidget* _parent, std::unique_ptr<PackInfoObject> _info);

        protected:
            void paintEvent(QPaintEvent*) override;

        private:
            QRect getIconRect() const override;
        };

        //////////////////////////////////////////////////////////////////////////
        class TopPacksView : public QWidget, public PacksViewBase
        {
            Q_OBJECT

        private:
            ScrollAreaWithTrScrollBar* scrollArea_;
            QHBoxLayout* layout_;

            QPropertyAnimation* animScroll_;

            enum class direction
            {
                left = 0,
                right = 1
            };
            void scrollStep(direction _direction);
            void updateItemsVisibility();

        protected:
            void wheelEvent(QWheelEvent* _e) override;

        public:
            TopPacksView(QWidget* _parent);

            void addPack(std::unique_ptr<PackInfoObject> _pack) override;
            void updateSize();
            void onPacksAdded() override;
            void onSetIcon(const int32_t _setId);
            void clear() override;
            void connectNativeScrollbar(QScrollBar* _sb);
        };

        //////////////////////////////////////////////////////////////////////////
        class TopPacksWidget : public QWidget
        {
            Q_OBJECT

            TopPacksView* packs_;

        public:
            TopPacksWidget(QWidget* _parent);
            void init();
            void onSetIcon(const int32_t _setId);
            void connectNativeScrollbar(QScrollBar* _sb);
        };

        //////////////////////////////////////////////////////////////////////////
        enum class PackViewMode
        {
            myPacks,
            search,
        };

        //////////////////////////////////////////////////////////////////////////
        class PackItem : public PackItemBase
        {
            Q_OBJECT

        Q_SIGNALS:
            void dragStarted(QPrivateSignal) const;
            void dragEnded(QPrivateSignal) const;

        public:
            PackItem(QWidget* _parent, std::unique_ptr<PackInfoObject> _info);

            void setDragging(bool _isDragging);
            bool isDragging() const noexcept { return dragging_; }

            void setHoverable(bool _isHoverable);
            bool isHoverable() const noexcept { return hoverable_; }

            void setMode(PackViewMode _mode);

        protected:
            void paintEvent(QPaintEvent*) override;
            void resizeEvent(QResizeEvent* _e) override;
            void mouseMoveEvent(QMouseEvent* _e) override;

        private:
            QRect getIconRect() const override;
            QRect getDelButtonRect() const;
            QRect getAddButtonRect() const;
            QRect getDragButtonRect() const;
            void onClicked();
            void onPressed();
            void onReleased();

            bool canDrag() const noexcept;
            void setDragCursor(bool _dragging);

        private:
            bool dragging_ = false;
            bool hoverable_ = true;
            PackViewMode mode_ = PackViewMode::myPacks;
        };

        //////////////////////////////////////////////////////////////////////////
        class PacksView : public QWidget, public PacksViewBase
        {
            Q_OBJECT

        public:
            enum CursorAction { MoveUp, MoveDown, MovePageUp, MovePageDown};

            PacksView(PackViewMode _mode, QWidget* _parent = nullptr);

            void addPack(std::unique_ptr<PackInfoObject> _pack) override;

            void updateSize();

            void clear() override;

            void onSetIcon(const int32_t _setId);

            bool empty() const;

            int moveCursor(CursorAction cursorAction);

            int selectedPackId() const;

            bool isDragInProgress() const;

            void setScrollBar(QWidget* _scrollBar);
            void connectNativeScrollbar(QScrollBar* _sb);

            void selectFirst();

            void onPacksAdded() override;

        Q_SIGNALS:
            void startScrollUp();
            void startScrollDown();
            void stopScroll();

        protected:
            void mouseReleaseEvent(QMouseEvent* _e) override;
            void mouseMoveEvent(QMouseEvent* _e) override;
            void wheelEvent(QWheelEvent* _e) override;
            void resizeEvent(QResizeEvent* _e) override;
            void leaveEvent(QEvent* _e) override;

            QRect getStickerRect(const int _pos) const;

        private:
            void startDrag();
            void stopDrag();

            int getDragPackNum();

            int getPackUnderCursor() const;

            void clearSelection();

            void updateHover();
            void updateItemsVisibility();
            void arrangePacks();

            std::vector<PackItem*> items_;
            PackViewMode mode_ = PackViewMode::myPacks;

            QPoint startDragPos_;

            int dragPack_ = -1;
            int dragOffset_ = 0;
            int selectedIdx_ = -1;
            bool orderChanged_ = false;

            QPointer<QWidget> scrollBar_;

            void postStickersOrder() const;
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

            void init(PacksViewBase::InitFrom _init);

            void setScrollBarToView(QWidget* _scrollBar);
            void connectNativeScrollbar(QScrollBar* _scrollBar);

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
            void init(PacksViewBase::InitFrom _init);

            int moveCursor(PacksView::CursorAction cursorAction);
            int selectedPackId();
            void setScrollBarToView(QWidget* _scrollBar);
            void connectNativeScrollbar(QScrollBar* _scrollBar);
            void touchSearchRequested(); // for statistics
        };


        class StickersPageHeader;

        //////////////////////////////////////////////////////////////////////////
        class Store : public QWidget
        {
            Q_OBJECT

            ScrollAreaWithTrScrollBar* rootScrollArea_;

            TopPacksWidget* packsView_;
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

            void initSearchView(PacksViewBase::InitFrom _init);

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

            const QString& currentTerm() const noexcept { return lastTerm_; }
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
