#pragma once

#include "../../corelib/enumerations.h"
#include "../../gui.shared/qt_overload.h"
#include "../types/message.h"
#include "../types/filesharing_download_result.h"
#include "../controls/TextUnit.h"
#include "stdafx.h"

class QApplication;
class QScreen;

#ifdef _WIN32
QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
#endif //_WIN32

namespace Ui
{
    class GeneralDialog;
    class CheckBox;
    class MessagesScrollArea;
}

namespace Utils
{
    inline void ensureMainThread() { assert(QThread::currentThread() == qApp->thread()); }

    QString getAppTitle();
    QString getVersionLabel();
    QString getVersionPrintable();

    struct QStringHasher
    {
        std::size_t operator()(const QString& _k) const noexcept
        {
            return qHash(_k);
        }
    };

    struct QColorHasher
    {
        std::size_t operator()(const QColor& _k) const noexcept
        {
            return qHash(_k.rgba());
        }
    };

    template <typename T>
    class [[nodiscard]] ScopeExitT
    {
    public:
        ScopeExitT() = delete;
        ScopeExitT(ScopeExitT&&) = delete;
        ScopeExitT(const ScopeExitT&) = delete;
        ScopeExitT(T t) : f(std::move(t)) {}
        ~ScopeExitT()
        {
            f();
        }

    private:
        T f;
    };

    class ShadowWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ShadowWidgetEventFilter(int _shadowWidth);

    protected:
        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        void setGradientColor(QGradient& _gradient, bool _isActive);

    private:
        int ShadowWidth_;
    };

    class [[nodiscard]] PainterSaver
    {
    public:
        PainterSaver(QPainter& _painter)
            : painter_(_painter)
        {
            painter_.save();
        }

        ~PainterSaver() noexcept
        {
            painter_.restore();
        }

        PainterSaver(const PainterSaver&) = delete;
        PainterSaver& operator=(const PainterSaver&) = delete;
    private:
        QPainter& painter_;
    };

    bool isUin(const QString& _aimId);

    const std::vector<QString>& urlSchemes();
    bool isInternalScheme(const QString& scheme);

    QString getCountryNameByCode(const QString& _iso_code);
    QString getCountryCodeByName(const QString& _name);
    QMap<QString, QString> getCountryCodes();
    // Stolen from Formatter in history_control , move it here completely?
    QString formatFileSize(const int64_t size);

    QString ScaleStyle(const QString& _style, double _scale);

    void SetProxyStyle(QWidget* _widget, QStyle* _style);
    void ApplyStyle(QWidget* _widget, const QString& _style);
    void ApplyPropertyParameter(QWidget* _widget, const char* _property, QVariant _parameter);

    QString LoadStyle(const QString& _qssFile);

    QPixmap getDefaultAvatar(const QString& _uin, const QString& _displayName, const int _sizePx);
    QColor getNameColor(const QString& _uin);

    std::vector<std::vector<QString>> GetPossibleStrings(const QString& _text, unsigned& _count);

    QPixmap roundImage(const QPixmap& _img, bool _isDefault, bool _miniIcons);

    void addShadowToWidget(QWidget* _target);
    void addShadowToWindow(QWidget* _target, bool _enabled = true);

    void grabTouchWidget(QWidget* _target, bool _topWidget = false);

    void removeLineBreaks(QString& _source);

    bool isValidEmailAddress(const QString& _email);

    bool foregroundWndIsFullscreened();

    QString rgbaStringFromColor(const QColor& _color);

    double fscale_value(const double _px) noexcept;
    int scale_value(const int _px) noexcept;
    QSize scale_value(const QSize& _px) noexcept;
    QSizeF scale_value(const QSizeF& _px) noexcept;
    QRect scale_value(const QRect& _px) noexcept;
    QPoint scale_value(const QPoint& _px) noexcept;

    int unscale_value(int _px) noexcept;
    QSize unscale_value(const QSize& _px) noexcept;
    QRect unscale_value(const QRect& _px) noexcept;
    QPoint unscale_value(const QPoint& _px) noexcept;

    int scale_bitmap_ratio() noexcept;
    int scale_bitmap(const int _px) noexcept;
    double fscale_bitmap(const double _px) noexcept;
    QSize scale_bitmap(const QSize& _px) noexcept;
    QSizeF scale_bitmap(const QSizeF& _px) noexcept;
    QRect scale_bitmap(const QRect& _px) noexcept;

    int unscale_bitmap(const int _px) noexcept;
    QSize unscale_bitmap(const QSize& _px) noexcept;
    QRect unscale_bitmap(const QRect& _px) noexcept;

    int scale_bitmap_with_value(const int _px) noexcept;
    double fscale_bitmap_with_value(const double _px) noexcept;
    QSize scale_bitmap_with_value(const QSize& _px) noexcept;
    QSizeF scale_bitmap_with_value(const QSizeF& _px) noexcept;
    QRect scale_bitmap_with_value(const QRect& _px) noexcept;
    QRectF scale_bitmap_with_value(const QRectF& _px) noexcept;

    int getBottomPanelHeight();
    int getTopPanelHeight();

    constexpr int text_sy() noexcept // spike for macos drawtext
    {
        return platform::is_apple() ? 2 : 0;
    }

    template <typename _T>
    void check_pixel_ratio(_T& _image);

    template <typename _T>
    void check_pixel_ratio(const _T&&) = delete;

    QString parse_image_name(const QString& _imageName);

    enum class KeepRatio
    {
        no,
        yes
    };

    QPixmap renderSvg(const QString& _resourcePath, const QSize& _scaledSize = QSize(), const QColor& _tintColor = QColor(), const KeepRatio _keepRatio = KeepRatio::yes);
    inline QPixmap renderSvgScaled(const QString& _resourcePath, const QSize& _unscaledSize = QSize(), const QColor& _tintColor = QColor(), const KeepRatio _keepRatio = KeepRatio::yes)
    {
        return renderSvg(_resourcePath, Utils::scale_value(_unscaledSize), _tintColor, _keepRatio);
    }

    using TintedLayer = std::pair<QString, QColor>;
    using SvgLayers = std::vector<TintedLayer>;
    QPixmap renderSvgLayered(const QString& _filePath, const SvgLayers& _layers = SvgLayers(), const QSize& _scaledSize = QSize());

    bool is_mac_retina() noexcept;
    void set_mac_retina(bool _val) noexcept;
    double getScaleCoefficient() noexcept;
    void setScaleCoefficient(double _coefficient) noexcept;
    double getBasicScaleCoefficient() noexcept;
    void initBasicScaleCoefficient(double _coefficient) noexcept;

    void groupTaskbarIcon(bool _enabled);

    bool isStartOnStartup();
    void setStartOnStartup(bool _start);

#ifdef _WIN32
    HWND createFakeParentWindow();
#endif //WIN32

    constexpr uint getInputMaximumChars() { return 10000; }

    int calcAge(const QDateTime& _birthdate);

    void drawText(QPainter& painter, const QPointF& point, int flags, const QString& text, QRectF* boundingRect = nullptr);

    const std::vector<QLatin1String>& getImageExtensions();
    const std::vector<QLatin1String>& getVideoExtensions();

    bool is_image_extension(const QString& _ext);
    bool is_image_extension_not_gif(const QString& _ext);
    bool is_video_extension(const QString& _ext);

    void copyFileToClipboard(const QString& _path);

    void saveAs(const QString& _inputFilename, std::function<void (const QString& _filename, const QString& _directory)> _callback, std::function<void ()> _cancel_callback = std::function<void ()>(), bool asSheet = true /* for OSX only */);

    using SendKeysIndex = std::vector<std::pair<QString, Ui::KeyToSendMessage>>;

    const SendKeysIndex& getSendKeysIndex();

    using ShortcutsCloseActionsList = std::vector<std::pair<QString, Ui::ShortcutsCloseAction>>;
    const ShortcutsCloseActionsList& getShortcutsCloseActionsList();
    QString getShortcutsCloseActionName(Ui::ShortcutsCloseAction _action);
    using ShortcutsSearchActionsList = std::vector<std::pair<QString, Ui::ShortcutsSearchAction>>;
    const ShortcutsSearchActionsList& getShortcutsSearchActionsList();

    using PrivacyAllowVariantsList = std::vector<std::pair<QString, core::privacy_access_right>>;
    const PrivacyAllowVariantsList& getPrivacyAllowVariants();

    void post_stats_with_settings();
    QRect GetMainRect();
    QPoint GetMainWindowCenter();
    QRect GetWindowRect(QWidget* window);

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields);

    QString getItemSafe(const std::vector<QString>& _values, size_t _selected, const QString& _default);

    Ui::GeneralDialog *NameEditorDialog(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool NameEditor(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool GetConfirmationWithTwoButtons(const QString& _buttonLeft, const QString& _buttonRight,
        const QString& _messageText, const QString& _labelText, QWidget* _parent, QWidget* _mainWindow = nullptr, bool _withSemiwindow = true);

    bool GetConfirmationWithOneButton(const QString& _buttonText, const QString& _messageText, const QString& _labelText, QWidget* _parent, QWidget* _mainWindow = nullptr, bool _withSemiwindow = true);

    bool GetErrorWithTwoButtons(const QString& _buttonLeftText, const QString& _buttonRightText,
        const QString& _messageText, const QString& _labelText, const QString& _errorText, QWidget* _parent);

    struct ProxySettings
    {
        const static int invalidPort = -1;

        core::proxy_type type_;
        core::proxy_auth authType_;
        QString username_;
        bool needAuth_;
        QString password_;
        QString proxyServer_;
        int port_;

        ProxySettings(core::proxy_type _type,
                      core::proxy_auth _authType,
                      QString _username,
                      QString _password,
                      QString _proxy,
                      int _port,
                      bool _needAuth);

        ProxySettings();

        void postToCore();

        static QString proxyTypeStr(core::proxy_type _type);

        static QString proxyAuthStr(core::proxy_auth _type);
    };

    ProxySettings* get_proxy_settings();

    QSize getMaxImageSize();

    QScreen* mainWindowScreen();

    QPixmap loadPixmap(const QString& _resource);

    bool loadPixmap(const QString& _path, Out QPixmap& _pixmap);

    bool loadPixmap(const QByteArray& _data, Out QPixmap& _pixmap);

    enum class PanoramicCheck
    {
        no,
        yes,
    };
    bool loadPixmapScaled(const QString& _path, const QSize& _maxSize, Out QPixmap& _pixmap, Out QSize& _originalSize, const PanoramicCheck _checkPanoramic = PanoramicCheck::yes);

    bool loadPixmapScaled(QByteArray& _data, const QSize& _maxSize, Out QPixmap& _pixmap, Out QSize& _originalSize);

    bool dragUrl(QObject* _parent, const QPixmap& _preview, const QString& _url);

    class StatsSender : public QObject
    {
        Q_OBJECT
    public :
        StatsSender();

    public Q_SLOTS:
        void recvGuiSettings() { guiSettingsReceived_ = true; trySendStats(); }
        void recvThemeSettings() { themeSettingsReceived_ = true; trySendStats(); }

    public:
        void trySendStats() const;

    private:
        bool guiSettingsReceived_;
        bool themeSettingsReceived_;
    };

    StatsSender* getStatsSender();

    bool haveText(const QMimeData *);

    QStringRef normalizeLink(const QStringRef& _link);

    std::string_view get_crossprocess_mutex_name();

    QHBoxLayout* emptyHLayout(QWidget* parent = nullptr);
    QVBoxLayout* emptyVLayout(QWidget* parent = nullptr);
    QGridLayout* emptyGridLayout(QWidget* parent = nullptr);

    void emptyContentsMargins(QWidget* w);
    void transparentBackgroundStylesheet(QWidget* w);

    QString getProductName();
    QString getInstallerName();

    void openMailBox(const QString& email, const QString& mrimKey, const QString& mailId);
    void openAttachUrl(const QString& email, const QString& mrimKey, bool canCancel);
    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey);

    QString getUnreadsBadgeStr(const int _unreads);

    QSize getUnreadsBadgeSize(const int _unreads, const int _height);

    QPixmap getUnreadsBadge(const int _unreads, const QColor _bgColor, const int _height);

    int drawUnreads(
        QPainter& _p,
        const QFont& _font,
        const QColor& _bgColor,
        const QColor& _textColor,
        const int _unreadsCount,
        const int _badgeHeight,
        const int _x,
        const int _y);

    int drawUnreads(
        QPainter *p, const QFont &font, QColor bgColor, QColor textColor, QColor borderColor,
        int unreads, int balloonSize, int x, int y, QPainter::CompositionMode borderMode = QPainter::CompositionMode_SourceOver);

    QPixmap pixmapWithEllipse(const QPixmap& _original, const QColor& _brushColor, int brushWidth);

    namespace Badge
    {
        enum class Color
        {
            Red,
            Green,
            Gray
        };
        int drawBadge(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _x, int _y, Color _color);
        int drawBadgeRight(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _x, int _y, Color _color);
    }

    QSize getUnreadsSize(const QFont& _font, const bool _withBorder, const int _unreads, const int _balloonSize);

    QImage iconWithCounter(int size, int count, QColor bg, QColor fg, QImage back = QImage());

    void openUrl(const QStringRef& _url);
    inline void openUrl(const QString& _url)
    {
        openUrl(QStringRef(&_url));
    }

    enum class OpenAt
    {
        Folder,
        Launcher,
    };

    void openFileOrFolder(const QStringRef& _path, const OpenAt _openAt);
    inline void openFileOrFolder(const QString& _path, const OpenAt _openAt)
    {
        openFileOrFolder(QStringRef(&_path), _openAt);
    }

    QString convertMentions(const QString& _source, const Data::MentionMap& _mentions);
    QString convertFilesPlaceholders(const QString& _source, const Data::FilesPlaceholderMap& _files);
    QString replaceFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files);
    QString setFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files);

    bool isNick(const QStringRef& _text);
    QString makeNick(const QString& _text);

    bool isMentionLink(const QStringRef& _url);
    inline bool isMentionLink(const QString& _url)
    {
        return isMentionLink(QStringRef(&_url));
    }

    bool isContainsMentionLink(const QStringRef& _url);
    inline bool isContainsMentionLink(const QString& _url)
    {
        return isContainsMentionLink(QStringRef(&_url));
    }

    bool isServiceLink(const QString& _url);
    void clearContentCache();
    void clearAvatarsCache();
    void removeOmicronFile();
    void checkForUpdates();

    enum class OpenDOPResult
    {
        dialog,
        profile,
        chat_popup
    };

    enum class OpenDOPParam
    {
        aimid,
        stamp,
    };

    OpenDOPResult openChatDialog(const QString& _stamp, const QString& _aimid, bool _joinApproval, bool _public);
    OpenDOPResult openDialogOrProfile(const QString& _contact, const OpenDOPParam _paramType = OpenDOPParam::aimid);
    void openDialogWithContact(const QString& _contact, qint64 _id = -1, bool _sel = true);

    bool clicked(const QPoint& _prev, const QPoint& _cur, int dragDistance = 0);

    void drawBubbleRect(QPainter& _p, const QRectF& _rect, const QColor& _color, int _bWidth, int _bRadious);

    int getShadowMargin();
    void drawBubbleShadow(QPainter& _p, const QPainterPath& _bubble, const int _clipLength = -1, const int _shadowMargin = -1, const QColor _shadowColor = QColor());

    QSize avatarWithBadgeSize(const int _avatarWidthScaled);
    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm, const bool _isOfficial, const bool _isMuted, const bool _isSelected, const bool _isOnline, const bool _small_online);
    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm, const QString& _aimid, const bool _officialOnly = false, const bool _isSelected = false, const bool _small_online = true);
    void drawAvatarWithoutBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm);

    template<typename T>
    QString replaceLine(T&& _s)
    {
        return std::forward<T>(_s).simplified();
    }

    enum MirrorDirection
    {
        vertical = 0x1,
        horizontal = 0x2,
        both = vertical | horizontal
    };

    QPixmap mirrorPixmap(const QPixmap& _pixmap, const MirrorDirection _direction);

    inline QPixmap mirrorPixmapHor(const QPixmap& _pixmap)
    {
        return mirrorPixmap(_pixmap, MirrorDirection::horizontal);
    }

    inline QPixmap mirrorPixmapVer(const QPixmap& _pixmap)
    {
        return mirrorPixmap(_pixmap, MirrorDirection::vertical);
    }

    void drawUpdatePoint(QPainter& _p, const QPoint &_center, int _radius, int _borderRadius);

    void restartApplication();

    struct CloseWindowInfo
    {
        enum class Initiator
        {
            MainWindow = 0,
            MacEventFilter = 1,
            Unknown = -1
        };

        enum class Reason
        {
            MW_Resizing = 0,
            Keep_Sidebar,
            MacEventFilter,
            ShowLoginPage,
            ShowGDPRPage,
        };

        Initiator initiator_ = Initiator::Unknown;
        Reason reason_ = Reason::MW_Resizing;
    };

    struct SidebarVisibilityParams
    {
        SidebarVisibilityParams(bool _show = false, bool _returnToMain = false)
            : show_(_show),
              returnMenuToMain_(_returnToMain)
        {
        }

        bool show_;
        bool returnMenuToMain_;
    };

    class PhoneValidator : public QValidator
    {
        Q_OBJECT
    public:
        PhoneValidator(QWidget* _parent, bool _isCode, bool _isParseFullNumber = false);
        virtual QValidator::State validate(QString& input, int&) const override;

    private:
        bool isCode_;
        bool isParseFullNumber_;
    };

    class DeleteMessagesWidget : public QWidget
    {
        Q_OBJECT
    public:
        DeleteMessagesWidget(QWidget* _parent, int _deleteForYou, int _deleteForAll);
        bool isDeleteForAll() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        Ui::CheckBox* checkbox_;
        std::unique_ptr<Ui::TextRendering::TextUnit> label_;
        bool deleteForAll_;
    };

    void logMessage(const QString& _message);

    void registerCustomScheme();

    void openFileLocation(const QString& _path);

    bool isPanoramic(const QSize& _size);
    bool isMimeDataWithImageDataURI(const QMimeData* _mimeData);
    bool isMimeDataWithImage(const QMimeData* _mimeData);
    QImage getImageFromMimeData(const QMimeData* _mimeData);

    void updateBgColor(QWidget* _widget, const QColor& _color);
    void setDefaultBackground(QWidget* _widget);
    void drawBackgroundWithBorders(QPainter& _p, const QRect& _rect, const QColor& _bg, const QColor& _border, const Qt::Alignment _borderSides, const int _borderWidth = Utils::scale_value(1));

    bool startsCyrillic(const QString& _str);
    bool startsNotLetter(const QString& _str);

    QPixmap tintImage(const QPixmap& _source, const QColor& _tint);
    bool isChat(const QString& _aimid);

    QString getDomainUrl();

    QString getStickerBotAimId();

    template<typename T, typename V>
    bool is_less_by_first(const std::pair<T, V>& x1, const std::pair<T, V>& x2)
    {
        static_assert(std::is_same_v<decltype(x1), decltype(x2)>);
        return static_cast<std::underlying_type_t<decltype(x1.first)>>(x1.first) < static_cast<std::underlying_type_t<decltype(x2.first)>>(x2.first);
    };

    QString msgIdLogStr(qint64 _msgId);
}

Q_DECLARE_METATYPE(Utils::CloseWindowInfo)
Q_DECLARE_METATYPE(Utils::SidebarVisibilityParams)

namespace Logic
{
    enum class Placeholder
    {
        Contacts,
        Recents
    };
    void updatePlaceholders(const std::vector<Placeholder> &_locations = {Placeholder::Contacts, Placeholder::Recents});
}

namespace MimeData
{
    QString getMentionMimeType();
    QString getRawMimeType();
    QString getFileMimeType();

    QByteArray convertMapToArray(const std::map<QString, QString, StringComparator>& _map);
    std::map<QString, QString, StringComparator> convertArrayToMap(const QByteArray& _array);

    void copyMimeData(const Ui::MessagesScrollArea& _area);

    const std::vector<QString>& getFilesPlaceholderList();
}

namespace FileSharing
{
    enum class FileType
    {
        archive,
        xls,
        html,
        keynote,
        numbers,
        pages,
        pdf,
        ppt,
        audio,
        txt,
        doc,
        apk,

        unknown,

        max_val,
    };

    FileType getFSType(const QString& _filename);

    QString getPlaceholderForType(FileType _fileType);
}