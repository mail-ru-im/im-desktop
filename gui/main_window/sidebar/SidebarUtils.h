#pragma once

#include "../../types/message.h"
#include "../../controls/TextUnit.h"
#include "../contact_list/Common.h"
#include "../containers/StatusContainer.h"
#include "GalleryList.h"

namespace Logic
{
    class ChatMembersModel;
    class ContactListItemDelegate;
}

namespace Ui
{
    class ContextMenu;
    class SwitcherCheckbox;
    class ImageVideoItem;
    class TextEditEx;
    class ContactAvatarWidget;
    class TextWidget;

    enum class Fading
    {
        On, Off
    };

    enum class ButtonState
    {
        NORMAL = 0,
        HOVERED = 1,
        PRESSED = 2,
        HIDDEN = 3,
    };

    class SidebarButton : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void clicked(QPrivateSignal);

    public:
        SidebarButton(QWidget* _parent);

        virtual void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }

        void setTextOffset(int _textOffset);

        void setIcon(const QPixmap& _icon);

        void initText(const QFont& _font, const QColor& _textColor, const QColor& _disabledColor);
        void setText(const QString& _text);

        void initCounter(const QFont& _font, const QColor& _textColor);
        void setCounter(int _count, bool _autoHide = true);
        void setColors(const QColor& _hover, const QColor& _active_ = QColor());

        virtual void setEnabled(bool _isEnabled);

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void resizeEvent(QResizeEvent* _event) override;

        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

        virtual void enterEvent(QEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;

        virtual bool isHovered() const;
        virtual bool isActive() const;

        void elideText(int _width);

    private:
        QString label_;
        int textOffset_;

        QPixmap icon_;

        Ui::TextRendering::TextUnitPtr text_;
        Ui::TextRendering::TextUnitPtr counter_;

        QColor hover_;
        QColor active_;

        QColor textColor_;
        QColor disabledColor_;

        QPoint clickedPoint_;

        bool isHovered_;
        bool isActive_;

    protected:
        QMargins margins_;
        bool isEnabled_;
    };

    class SidebarCheckboxButton : public SidebarButton
    {
        Q_OBJECT
    Q_SIGNALS:
        void checked(bool, QPrivateSignal);

    public:
        SidebarCheckboxButton(QWidget* _parent);

        void setChecked(bool _checked);
        bool isChecked() const;

        virtual void setEnabled(bool _isEnabled) override;

    protected:
        virtual void resizeEvent(QResizeEvent* _event) override;

        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        QPoint clickedPoint_;
        SwitcherCheckbox* checkbox_;
    };

    class StatusPlate;

    class AvatarNameInfo : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void avatarClicked(QPrivateSignal);
        void badgeClicked(QPrivateSignal);
        void clicked(QPrivateSignal);

    public:
        AvatarNameInfo(QWidget* _parent);

        void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }

        void setTextOffset(int _textOffset);

        void initName(const QFont& _font, const QColor& _color);
        void initInfo(const QFont& _font, const QColor& _color);

        void setAimIdAndSize(const QString& _aimid, int _size, const QString& _friendly = QString());
        void setFrienlyAndSize(const QString& _friendly, int _size);
        void setInfo(const QString& _info, const QColor& _color = QColor());

        const QString& getFriendly() const;
        void makeClickable();
        void nameOnly();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void timerEvent(QTimerEvent* _event) override;
        void changeEvent(QEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private Q_SLOTS:
        void avatarChanged(const QString& aimId);
        void friendlyChanged(const QString& _aimId, const QString& _friendlyName);
        void statusChanged(const QString& _aimid);
        void stateChanged();
    private:
        void loadAvatar();
        void setBadgeRect();
        void updateSize();
        QPoint statusPos() const;
        void paintEnabled(QPainter& painter);
        void paintDisabled(QPainter& painter);
        QPoint avatarTopLeft() const;

    private:
        QBasicTimer timer_;
        QMargins margins_;
        int textOffset_;
        int animStep_;

        QString aimId_;
        QString friendlyName_;
        QPixmap avatar_;
        int avatarSize_;

        Ui::TextRendering::TextUnitPtr name_;
        Ui::TextRendering::TextUnitPtr info_;

        QPoint clicked_;
        bool defaultAvatar_;
        bool clickable_;
        bool hovered_;
        bool nameOnly_;

        QRect badgeRect_;
        StatusPlate* statusPlate_;
    };

    class StatusPlate : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void update(QPrivateSignal);

    public:
        StatusPlate(QObject* _parent);
        ~StatusPlate();
        void setContactId(const QString& _id);
        void draw(QPainter& _p);
        int height() const;
        QRect rect() const;
        bool isEmpty() const;

        void onMouseMove(const QPoint& _pos);
        void onMousePress(const QPoint& _pos);
        void onMouseRelease(const QPoint& _pos);
        void onMouseLeave();
        void updateGeometry(const QPoint& _topLeft, int _availableWidth);

    private Q_SLOTS:
        void onStatusChanged(const QString& _contactId);

    private:
        int availableForText(int _availableWidth) const;
        void updateTextGeometry(int _availableWidth);

        Ui::TextRendering::TextUnitPtr text_;
        int cachedAvailableWidth_ = 0;
        Statuses::Status status_;
        QString contactId_;
        bool hoverEnabled_ = false;
        bool hovered_ = false;
        bool pressed_ = false;
        QRect rect_;
    };

    class BigEmojiWidget;

    class StatusPopup : public QWidget
    {
        Q_OBJECT
    public:
        StatusPopup(const Statuses::Status& _status, QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QWidget* content_ = nullptr;
        Statuses::Status status_;
        BigEmojiWidget* emoji_ = nullptr;
        TextWidget* text_ = nullptr;
        TextWidget* duration_ = nullptr;
        QScrollArea* scrollArea_ = nullptr;
    };

    class TextLabel : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void textClicked(QPrivateSignal);
        void copyClicked(const QString&, QPrivateSignal);
        void shareClicked(QPrivateSignal);
        void selectionChanged(QPrivateSignal);

        void menuAction(QAction*);

    public:
        TextLabel(QWidget* _parent, int _maxLinesCount = -1);

        void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }

        QMargins getMargins() const noexcept { return margins_; }

        void init(const QFont& _font, const QColor& _color, const QColor& _linkColor = QColor());
        void setText(const QString& _text, const QColor& _color = QColor());
        void setCursorForText();
        void unsetCursorForText();
        void setColor(const QColor& _color);
        void setLinkColor(const QColor& _color);

        void clearMenuActions();
        void addMenuAction(const QString& _iconPath, const QString& _name, const QVariant& _data);
        void makeCopyable();
        void makeTextField();
        void showButtons();
        void allowOnlyCopy();
        QString getText() const;

        void setTextAlign(TextRendering::HorAligment _align) { textAlign_ = _align; }
        void setBackgroundColor(QColor _color) { bgColor_ = _color; }
        void setIconColors(QColor _normal, QColor _hover, QColor _pressed);
        QString getSelectedText() const;
        void tryClearSelection(const QPoint& _pos);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        void updateSize(bool _forceCollapsed = false);
        void selectText(const QPoint& _from, const QPoint& _to);
        void clearSelection();

    private:
        QMargins margins_;
        TextRendering::TextUnitPtr text_;
        TextRendering::TextUnitPtr collapsedText_;
        TextRendering::TextUnitPtr readMore_;
        TextRendering::HorAligment textAlign_ = TextRendering::HorAligment::LEFT;
        QPoint selectFrom_;
        QPoint selectTo_;
        int maxLinesCount_;
        bool collapsed_;
        bool copyable_;
        bool buttonsVisible_;
        bool onlyCopyButton_;
        bool cursorForText_;
        bool isTextField_;
        ContextMenu* menu_;
        QColor bgColor_;
        QColor iconNormalColor_;
        QColor iconHoverColor_;
        QColor iconPressedColor_;
        QTimer* TripleClickTimer_ = nullptr;
    };

    class InfoBlock
    {
    public:
        void hide();
        void show();
        void setVisible(bool _value);
        void setHeaderText(const QString& _text, const QColor& _color = QColor());
        void setText(const QString& _text, const QColor& _color = QColor());
        void setTextLinkColor(const QColor& _color);
        void setHeaderLinkColor(const QColor& _color);
        bool isVisible() const;
        QString getSelectedText() const;

        TextLabel* header_ = nullptr;
        TextLabel* text_ = nullptr;
    };

    class GalleryPreviewItem : public QWidget
    {
        Q_OBJECT
    public:
        GalleryPreviewItem(QWidget* _parent, const QString& _link, qint64 _msg, qint64 _seq, const QString& _aimId, const QString& _sender, bool _outgoing, time_t _time, int _previewSize);
        qint64 msg();
        qint64 seq();

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void enterEvent(QEvent*) override;
        virtual void leaveEvent(QEvent*) override;

    private Q_SLOTS:
        void updateWidget(const QRect& _rect);
        void onMenuAction(QAction* _action);

    private:
        QString aimId_;
        ImageVideoItem* item_;
        QPoint pos_;
    };

    class GalleryPreviewWidget : public QWidget
    {
        Q_OBJECT
    public:
        GalleryPreviewWidget(QWidget* _parent, int _previewSize, int _previewCount);
        void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }
        void setSpacing(int _spacing);
        void setAimId(const QString& _aimid);

    private Q_SLOTS:
        void dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted);
        void dialogGalleryUpdate(const QString& _aimId, const QVector<Data::DialogGalleryEntry>& _entries);
        void dialogGalleryInit(const QString& _aimId);
        void dialogGalleryErased(const QString& _aimId);
        void dialogGalleryHolesDownloading(const QString& _aimId);

    private:
        void clear();
        void requestGallery();

    private:
        QString aimId_;
        int previewSize_;
        int previewCount_;
        QHBoxLayout* layout_;
        qint64 reqId_;
    };

    class MembersPlate : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void searchClicked(QPrivateSignal);

    public:
        MembersPlate(QWidget* _parent);

        void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }
        void setMembersCount(int _count, bool _isChannel);

        void initMembersLabel(const QFont& _font, const QColor& _color);
        void initSearchLabel(const QFont& _font, const QColor& _color);

    protected:

        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        Ui::TextRendering::TextUnitPtr members_;
        Ui::TextRendering::TextUnitPtr search_;

        QMargins margins_;
        QPoint clicked_;
    };

    class MembersWidget : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void selected(const QString&, QPrivateSignal);
        void removeClicked(const QString&, QPrivateSignal);
        void moreClicked(const QString&, QPrivateSignal);

    public:
        MembersWidget(QWidget* _parent, Logic::ChatMembersModel* _model, Logic::ContactListItemDelegate* _delegate, int _maxMembersCount);
        void clearCache();

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void resizeEvent(QResizeEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;

    private Q_SLOTS:
        void dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&);
        void lastseenChanged(const QString&);

    private:
        void updateSize();

    private:
        Logic::ChatMembersModel* model_;
        Logic::ContactListItemDelegate* delegate_;
        Ui::ContactListParams params_;
        int maxMembersCount_;
        int memberCount_;
        int hovered_;
        QPoint clicked_;
    };

    class ColoredButton : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void clicked(QPrivateSignal);
    public:
        ColoredButton(QWidget* _parent);

        void setMargins(const QMargins& _margins);
        inline void setMargins(int _left, int _top, int _right, int _bottom)
        {
            setMargins(QMargins(_left, _top, _right, _bottom));
        }
        void setTextOffset(int _offset);
        void updateTextOffset();

        void setHeight(int _height);
        void initColors(const QColor& _base, const QColor& _hover, const QColor& _active);
        void setIcon(const QPixmap& _icon);
        void initText(const QFont& _font, const QColor& _color);
        void setText(const QString& _text);

        void makeRounded();
        QMargins getMargins() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void enterEvent(QEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;

    private:
        Ui::TextRendering::TextUnitPtr text_;
        QPixmap icon_;

        QColor base_;
        QColor active_;
        QColor hover_;

        bool isHovered_;
        bool isActive_;

        int height_;
        int textOffset_;
        QMargins margins_;
        QPoint clicked_;
    };

    class EditWidget : public QWidget
    {
        Q_OBJECT
    public:
        EditWidget(QWidget* _parent);

        void init(const QString& _aimid, const QString& _name, const QString& _description, const QString& _rules);

        QString name() const;
        QString desription() const;
        QString rules() const;
        bool avatarChanged() const;
        QPixmap getAvatar() const;

        void nameOnly(bool _nameOnly);

    protected:
        virtual void showEvent(QShowEvent *event) override;

    private:
        ContactAvatarWidget* avatar_;
        QWidget* avatarSpacer_;
        TextEditEx* name_;
        QWidget* nameSpacer_;
        TextEditEx* description_;
        QWidget* descriptionSpacer_;
        TextEditEx* rules_;
        QWidget* rulesSpacer_;
        bool avatarChanged_;
        bool nameOnly_;
    };

    class GalleryPopup : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void galleryPhotoClicked(QPrivateSignal);
        void galleryVideoClicked(QPrivateSignal);
        void galleryFilesClicked(QPrivateSignal);
        void galleryLinksClicked(QPrivateSignal);
        void galleryPttClicked(QPrivateSignal);

    public:
        GalleryPopup();

        void setCounters(int _photo, int _video, int _files, int _links, int _ptt);

    private:
        SidebarButton* galleryPhoto_;
        SidebarButton* galleryVideo_;
        SidebarButton* galleryFiles_;
        SidebarButton* galleryLinks_;
        SidebarButton* galleryPtt_;
    };

    QPixmap editGroup(const QString& _aimid, QString& _name, QString& _description, QString& _rules);
    QString editName(const QString& _name);

    QByteArray avatarToByteArray(const QPixmap &_avatar);
    int getIconSize();

    QString formatTimeStr(const QDateTime& _dt);

    AvatarNameInfo* addAvatarInfo(QWidget* _parent, QLayout* _layout);
    TextLabel* addLabel(const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset = 0, TextRendering::HorAligment _align = TextRendering::HorAligment::LEFT, Fading _faded = Fading::Off);
    TextLabel* addText(const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset = 0, int _maxLineNumbers = 0, Fading _faded = Fading::Off);
    std::unique_ptr<InfoBlock> addInfoBlock(const QString& _header, const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset = 0, Fading _faded = Fading::Off);
    QWidget* addSpacer(QWidget* _parent, QLayout* _layout, int height = -1);
    SidebarButton* addButton(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, const QString& _accName = QString());
    SidebarCheckboxButton* addCheckbox(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, Fading _faded = Fading::Off);
    GalleryPreviewWidget* addGalleryPrevieWidget(QWidget* _parent, QLayout* _layout);
    MembersPlate* addMembersPlate(QWidget* _parent, QLayout* _layout);
    MembersWidget* addMembersWidget(Logic::ChatMembersModel* _model, Logic::ContactListItemDelegate* _delegate, int _membersCount, QWidget* _parent, QLayout* _layout);
    ColoredButton* addColoredButton(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, const QSize& _iconSize = QSize(), Fading _faded = Fading::Off);

    class BlockAndDeleteWidget : public QWidget
    {
        Q_OBJECT
    public:
        BlockAndDeleteWidget(QWidget* _parent, const QString& _friendly, const QString& _chatAimid);
        bool needToRemoveMessages() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        Ui::CheckBox* checkbox_;
        std::unique_ptr<Ui::TextRendering::TextUnit> label_;
        bool removeMessages_;
    };
}
