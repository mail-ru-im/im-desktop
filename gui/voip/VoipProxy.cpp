#include "stdafx.h"
#include "VoipProxy.h"

#include "MaskManager.h"
#include "VoipTools.h"

#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../my_info.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../controls/TextEmojiWidget.h"

#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../main_window/sounds/SoundsManager.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../../core/Voip/VoipSerialization.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include <functional>

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

#define INCLUDE_USER_NAME         1
#define INCLUDE_USER_NAME_IN_CONF 1
#define INCLUDE_REMOTE_CAMERA_OFF 0
#define INCLUDE_AVATAR            1
#define INCLUDE_BLUR_BACKGROUND   0
#if (defined BIZ_VERSION || defined DIT_VERSION)
#define INCLUDE_WATERMARK         0
#else
#define INCLUDE_WATERMARK         1
#endif //(defined BIZ_VERSION || defined DIT_VERSION)
#define INCLUDE_CAMERA_OFF_STATUS 1

const QColor bkColor(40, 40, 40, 255);

#define AVATAR_VIDEO_SIZE          360

#if defined(__linux__) || defined(__APPLE__)
#define IMAGE_FORMAT QImage::Format_RGBA8888
#else
// TODO: do not make useless format conversions on macos & win32 in VoipManager.cpp->bitmapHandleFromRGBData
#define IMAGE_FORMAT QImage::Format_ARGB32
#endif

namespace
{
    inline const size_t __hash(const std::wstring& _str)
    {
        std::hash<std::wstring> h;
        return h(_str);
    }
    inline const size_t __hash(const std::string& _str)
    {
        std::hash<std::string> h;
        return h(_str);
    }
}

void addImageToCollection(Ui::gui_coll_helper& collection, const std::string& dir, const std::string& name, const std::string& filenamePrefix, const std::string& filenamePostfix, bool withPng = true, int outputWidth = 0, int outputHeight = 0)
{
    std::stringstream resourceStr;

    int currentScale = Utils::scale_bitmap_with_value(100);
    if (currentScale > 200)
        currentScale = 200;

    resourceStr << ":/" + (!dir.empty() ? dir + "/" : "") << filenamePrefix << currentScale << filenamePostfix << (withPng ? ".png" : "");
    QImage image(QString::fromStdString(resourceStr.str()));

    assert(!image.isNull());

    if (outputHeight > 0 && outputWidth > 0)
    {
        QImage bigImage(outputWidth, outputHeight, IMAGE_FORMAT);
        bigImage.fill(Qt::transparent);

        {
            QPainter painter(&bigImage);

            QSize position = (bigImage.size() - image.size()) / 2;
            painter.drawImage(QPoint(position.width(), position.height()), image);
        }

        image = bigImage.copy();
    }

    auto dataSize = image.byteCount();
    auto dataPtr = image.bits();
    auto sz = image.size();

    assert(dataSize && dataPtr);

    if (dataSize && dataPtr)
    {
        core::ifptr<core::istream> stream(collection->create_stream());
        stream->write(dataPtr, dataSize);

        collection.set_value_as_stream(name, stream.get());
        collection.set_value_as_int(name + "_height", sz.height());
        collection.set_value_as_int(name + "_width", sz.width());
    }
}

void addTextToCollection(Ui::gui_coll_helper& collection, const std::string& name, const std::string& textString, const QColor& penColor, const QFont& font)
{
    const QString text = QT_TRANSLATE_NOOP("voip_pages", textString.c_str());

    QFontMetrics fm(font);
    const QSize textSz = fm.size(Qt::TextSingleLine, text);

    int nickW = textSz.width();
    int nickH = textSz.height();

    // Made it even
    if (nickW % 2 == 1)
    {
        nickW ++;
    }
    if (nickH % 2 == 1)
    {
        nickH ++;
    }

    assert(nickW % 2 == 0 && nickH % 2 == 0);
    if (nickW && nickH)
    {
        QPainter painter;

        QImage pm(nickW, nickH, IMAGE_FORMAT);
        pm.fill(Qt::transparent);

        const QPen pen(penColor);

        painter.begin(&pm);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.setPen(pen);
        painter.setFont(font);

        painter.setCompositionMode(QPainter::CompositionMode_Plus);

        if (textSz.width() > nickW)
        {
            painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, text);
        }
        else
        {
            painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignCenter, text);
        }

#ifdef _DEBUG
        painter.drawRect(QRect(0, 0, nickW - 1, nickH - 1));
#endif

        painter.end();

        QImage image = pm;

        auto dataSize = image.byteCount();
        auto dataPtr = image.bits();
        auto sz = image.size();

        if (dataSize && dataPtr)
        {
            core::ifptr<core::istream> stream(collection->create_stream());
            stream->write(dataPtr, dataSize);

            collection.set_value_as_stream(name, stream.get());
            collection.set_value_as_int(name + "_height", sz.height());
            collection.set_value_as_int(name + "_width",  sz.width());
        }
    }
}

#define STRING_ID(x) __hash((x))

voip_proxy::VoipController::VoipController(Ui::core_dispatcher& _dispatcher)
    : dispatcher_(_dispatcher)
    , callTimeTimer_(this)
    , callTimeElapsed_(0)
    , haveEstablishedConnection_(false)
    , iTunesWasPaused_(false)
    , maskEngineEnable_(false)
    , voipIsKilled_(false)
    , localCameraEnabled_(false)
    , localVideoEnabled_(false)
{
    cipherState_.state= voip_manager::CipherState::kCipherStateUnknown;

    connect(&callTimeTimer_, SIGNAL(timeout()), this, SLOT(_updateCallTime()), Qt::QueuedConnection);
    callTimeTimer_.setInterval(1000);

    const std::vector<unsigned> codePoints
    {
        0xF09F8EA9,
        0xF09F8FA0,
        0xF09F92A1,
        0xF09F9AB2,
        0xF09F8C8D,
        0xF09F8D8C,
        0xF09F8D8F,
        0xF09F909F,
        0xF09F90BC,
        0xF09F928E,
        0xF09F98BA,
        0xF09F8CB2,
        0xF09F8CB8,
        0xF09F8D84,
        0xF09F8D90,
        0xF09F8E88,
        0xF09F90B8,
        0xF09F9180,
        0xF09F9184,
        0xF09F9191,
        0xF09F9193,
        0xF09F9280,
        0xF09F9494,
        0xF09F94A5,
        0xF09F9A97,
        0xE2AD90,
        0xE28FB0,
        0xE29ABD,
        0xE29C8F,
        0xE29882,
        0xE29C82,
        0xE29DA4,
    };

    voipEmojiManager_.addMap(64, 64, ":/emoji/secure_emoji_64.png", codePoints, 64);
    voipEmojiManager_.addMap(80, 80, ":/emoji/secure_emoji_80.png", codePoints, 80);
    voipEmojiManager_.addMap(96, 96, ":/emoji/secure_emoji_96.png", codePoints, 96);
    voipEmojiManager_.addMap(128, 128, ":/emoji/secure_emoji_128.png", codePoints, 128);
    voipEmojiManager_.addMap(160, 160, ":/emoji/secure_emoji_160.png", codePoints, 160);
    voipEmojiManager_.addMap(192, 192, ":/emoji/secure_emoji_192.png", codePoints, 192);

    connect(this, SIGNAL(onVoipCallIncoming(const std::string&, const std::string&)),
        this, SLOT(updateUserAvatar(const std::string&, const std::string&)));

    connect(this, SIGNAL(onVoipCallConnected(const voip_manager::ContactEx&)),
        this, SLOT(updateUserAvatar(const voip_manager::ContactEx&)));

    connect(this, SIGNAL(onVoipMaskEngineEnable(bool)),
        this, SLOT(updateVoipMaskEngineEnable(bool)));

    connect(this, SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)),
        this, SLOT(onCallCreate(const voip_manager::ContactEx&)));

    resetMaskManager();
}

voip_proxy::VoipController::~VoipController()
{
    callTimeTimer_.stop();
}

void voip_proxy::VoipController::_updateCallTime()
{
    callTimeElapsed_ += 1;
    emit onVoipCallTimeChanged(callTimeElapsed_, true);
}

void voip_proxy::VoipController::setAvatars(int _size, const char*_contact, bool forPreview)
{
    auto sendAvatar = [this](int _size, const char*_contact, bool forPreview, int userBitmapParamBitSet, voip_manager::AvatarThemeType theme)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);

        _prepareUserBitmaps(collection, _contact, _size, userBitmapParamBitSet, theme);
        if (forPreview)
        {
            collection.set_value_as_string("contact", PREVIEW_RENDER_NAME);
        }
        dispatcher_.post_message_to_core("voip_call", collection.get());
    };

    sendAvatar(_size, _contact, forPreview, AVATAR_ROUND  | USER_NAME_ONE_IS_BIG, voip_manager::MAIN_ONE_BIG);
    sendAvatar(_size, _contact, forPreview, AVATAR_SQUARE_BACKGROUND | USER_NAME_ALL_THE_SAME, voip_manager::MAIN_ALL_THE_SAME);
    sendAvatar(_size, _contact, forPreview, AVATAR_SQUARE_BACKGROUND | USER_NAME_MINI, voip_manager::MINI_WINDOW);
    sendAvatar(_size, _contact, forPreview, AVATAR_SQUARE_WITH_LETTER, voip_manager::INCOME_WINDOW);
    sendAvatar(_size, _contact, forPreview, AVATAR_SQUARE_BACKGROUND | USER_NAME_ALL_THE_SAME | USER_NAME_ALL_THE_SAME_5_PEERS, voip_manager::MAIN_ALL_THE_SAME_5_PEERS);
    sendAvatar(_size, _contact, forPreview, AVATAR_SQUARE_BACKGROUND | USER_NAME_ALL_THE_SAME | USER_NAME_ALL_THE_SAME_LARGE, voip_manager::MAIN_ALL_THE_SAME_LARGE);
}

std::string voip_proxy::VoipController::formatCallName(const std::vector<std::string>& _names, const char* _clip)
{
    std::string name;
    for (unsigned ix = 0; ix < _names.size(); ix++)
    {
        const std::string& nt = _names[ix];
        assert(!nt.empty());
        if (nt.empty())
        {
            continue;
        }

        if (name.empty())
        {
            // ------------------------------
        } else if (ix == _names.size() - 1 && _clip)
        {
            name += ' ';
            name += _clip;
            name += ' ';
        }
        else
        {
            name += ", ";
        }
        name += nt;
    }

    assert(!name.empty() || _names.empty());
    return name;
}

void voip_proxy::VoipController::setRequestSettings()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "update");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}


void voip_proxy::VoipController::_prepareUserBitmaps(Ui::gui_coll_helper& _collection, const std::string& _contact,
    int _size, int userBitmapParamBitSet, voip_manager::AvatarThemeType theme)
{
    assert(!_contact.empty() && _size);
    if (_contact.empty() || !_size)
    {
        return;
    }

    _collection.set_value_as_string("type", "converted_avatar");
    _collection.set_value_as_string("contact", _contact);
    _collection.set_value_as_int("size", _size);
    _collection.set_value_as_int("theme", theme);

    bool needAvatar = (userBitmapParamBitSet & (AVATAR_ROUND | AVATAR_SQUARE_BACKGROUND | AVATAR_SQUARE_WITH_LETTER));

    QString friendContact;

    const auto contact = QString::fromStdString(_contact);

    if (Ui::MyInfo()->aimId() == contact)
    {
        friendContact = Ui::MyInfo()->friendly();
    }
    else
    {
        friendContact = Logic::GetFriendlyContainer()->getFriendly(contact);
    }
    if (friendContact.isEmpty())
    {
        friendContact = contact;
    }

    Logic::QPixmapSCptr realAvatar;
    bool defaultAvatar = false;
    if (needAvatar)
    {
        realAvatar = Logic::GetAvatarStorage()->Get(
            contact,
            friendContact,
            _size,
            defaultAvatar,
            false
        );
    }

    auto packAvatar = [&_collection](const QImage& image, const std::string& prefix)
    {
        const auto dataSize = image.byteCount();
        const auto dataPtr = image.bits();

        assert(dataSize && dataPtr);
        if (dataSize && dataPtr)
        {
            core::ifptr<core::istream> avatar_stream(_collection->create_stream());
            avatar_stream->write(dataPtr, dataSize);
            _collection.set_value_as_stream(prefix + "avatar", avatar_stream.get());
            _collection.set_value_as_int(prefix + "avatar_height", image.height());
            _collection.set_value_as_int(prefix + "avatar_width", image.width());
        }
    };

    if (realAvatar)
    {
        const auto rawAvatar = realAvatar->toImage();

        if (needAvatar)
        {
            enum AvatarType {AT_AVATAR_SQUARE_BACKGROUND = 0, AT_AVATAR_SQUARE_WITH_LETTER, AT_AVATAR_ROUND};

            auto makeAvatar = [_size, &defaultAvatar, &rawAvatar, packAvatar](AvatarType avatarType)
            {
                bool squareBackground = (avatarType == AT_AVATAR_SQUARE_BACKGROUND);
                bool squareLetter     = (avatarType == AT_AVATAR_SQUARE_WITH_LETTER);
                bool roundAvatar      = (avatarType == AT_AVATAR_ROUND);

                bool squareAvatar = (squareBackground || squareLetter);
                const int avatarSize = (roundAvatar) ? Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize)
                    : _size;
                QImage image(QSize(avatarSize, avatarSize), IMAGE_FORMAT);
                image.fill(Qt::transparent);

                QPainter painter(&image);

                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::TextAntialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::NoBrush);

                if (roundAvatar)
                {
                    QPainterPath path(QPointF(0, 0));
                    path.addEllipse(0, 0, avatarSize, avatarSize);
                    painter.setClipPath(path);
                }

                if (!defaultAvatar || roundAvatar || squareLetter)
                {
                    painter.drawImage(QRect(0, 0, avatarSize, avatarSize), rawAvatar);
                }
                else
                {
                    // Small hack get background color of default avatar.
                    painter.fillRect(QRect(0, 0, avatarSize, avatarSize), rawAvatar.pixelColor(0, 0));
                }

                if (squareAvatar)
                {
                    painter.fillRect(image.rect(), QColor(0, 0, 0, 128));
                }

                packAvatar(image, squareAvatar ? "background_" : "");
                if (!squareAvatar)
                {
                    packAvatar(image, "rem_camera_offline_");
                }
            };

            if (userBitmapParamBitSet & AVATAR_SQUARE_BACKGROUND)
            {
                makeAvatar(AT_AVATAR_SQUARE_BACKGROUND);
            }
            if (userBitmapParamBitSet & AVATAR_ROUND)
            {
                makeAvatar(AT_AVATAR_ROUND);
                makeAvatar(AT_AVATAR_SQUARE_BACKGROUND);
            }
            if (userBitmapParamBitSet & AVATAR_SQUARE_WITH_LETTER)
            {
                makeAvatar(AT_AVATAR_SQUARE_WITH_LETTER);
            }
        }
    }

    if ((USER_NAME_ONE_IS_BIG & userBitmapParamBitSet) ||
        (USER_NAME_ALL_THE_SAME & userBitmapParamBitSet) || (USER_NAME_MINI & userBitmapParamBitSet))
    {
        QPainter painter;

        const auto fontSize = 16;
        QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(fontSize), Fonts::FontWeight::SemiBold);
        font.setStyleStrategy(QFont::PreferAntialias);

        QFontMetrics fm(font);
        bool miniWindow = (USER_NAME_MINI & userBitmapParamBitSet);
        bool allTheSame = (USER_NAME_ALL_THE_SAME & userBitmapParamBitSet);
        bool allTheSame5Peers = (USER_NAME_ALL_THE_SAME_5_PEERS & userBitmapParamBitSet);
        bool allTheSameLarge  = (USER_NAME_ALL_THE_SAME_LARGE & userBitmapParamBitSet);

        const int nickW = Utils::scale_bitmap_with_value(
            miniWindow ? voip_manager::kNickTextMiniWW :
                         (allTheSame ? (allTheSame5Peers ? voip_manager::kNickTextAllSameW5peers :
                                                           (allTheSameLarge ? voip_manager::kNickTextAllSameWLarge :
                                                                              voip_manager::kNickTextAllSameW)) :
                                        voip_manager::kNickTextW));

        const int nickH = Utils::scale_bitmap_with_value(miniWindow ?
            voip_manager::kNickTextMiniWH : (allTheSame ? voip_manager::kNickTextAllSameH : voip_manager::kNickTextH));

        QImage pm(nickW, nickH, IMAGE_FORMAT);
        pm.fill(Qt::transparent);

        const QPen pen(QColor(ql1s("#ffffff")));

        painter.begin(&pm);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        //auto oldCompostion = painter.compositionMode();
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(pm.rect(), Qt::black);
        painter.setCompositionMode(QPainter::CompositionMode_Plus);

        painter.setPen(pen);
        painter.setFont(font);

        Ui::CompiledText compiledText;
        Ui::CompiledText::compileText(friendContact, compiledText, false, true);

        // Align to center.
        int xOffset = std::max((nickW - compiledText.width(fm)) / 2, 0);
        int yOffset = std::max(nickH - Utils::scale_bitmap_with_value(fontSize) / 2, 0);
        compiledText.draw(painter, xOffset, yOffset, nickW);

#ifdef _DEBUG
        painter.drawRect(QRect(0, 0, nickW - 1, nickH - 1));
#endif
        painter.end();

        packAvatar(pm, "sign_normal_");
    }
}


void voip_proxy::VoipController::handlePacket(core::coll_helper& _collParams)
{
    const std::string sigType = _collParams.get_value_as_string("sig_type");
    if (sigType == "video_window_show")
    {
        const bool enable = _collParams.get_value_as_bool("param");

        emit onVoipShowVideoWindow(enable);
    }
    else if (sigType == "device_vol_changed")
    {
        int volumePercent = _collParams.get_value_as_int("volume_percent");
        const std::string deviceType = _collParams.get_value_as_string("device_type");

        volumePercent = std::min(100, std::max(volumePercent, 0));
        emit onVoipVolumeChanged(deviceType, volumePercent);
    }
    else if (sigType == "voip_reset_complete")
    {
        voipIsKilled_ = true;
        emit onVoipResetComplete();
    }
    else if (sigType == "device_muted")
    {
        const bool muted = _collParams.get_value_as_bool("muted");
        const std::string deviceType = _collParams.get_value_as_string("device_type");

        emit onVoipMuteChanged(deviceType, muted);
    }
    else if (sigType == "call_invite")
    {
        const std::string account = _collParams.get_value_as_string("account");
        const std::string contact = _collParams.get_value_as_string("contact");

        emit onVoipCallIncoming(account, contact);
    }
    else if (sigType == "call_peer_list_changed")
    {
        voip_manager::ContactsList contacts;
        contacts << _collParams;

        if (contacts.isActive)
        {
            activePeerList_ = contacts;
        }

        // Updated avatars for conference.
        for (auto contact : contacts.contacts)
        {
            setAvatars(AVATAR_VIDEO_SIZE, contact.contact.c_str(), false);
        }

        emit onVoipCallNameChanged(contacts);
    }
    else if (sigType == "voip_window_add_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;

        emit onVoipWindowAddComplete(hwnd);
    }
    else if (sigType == "voip_window_remove_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;

        emit onVoipWindowRemoveComplete(hwnd);
    }
    else if (sigType == "call_in_accepted")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        emit onVoipCallIncomingAccepted(contactEx);
    }
    else if (sigType == "device_list_changed")
    {
        const int deviceCount = _collParams.get_value_as_int("count");
        const voip2::DeviceType deviceType = (voip2::DeviceType)_collParams.get_value_as_int("type");

        auto& devices = devices_[deviceType];

        devices.clear();
        devices.reserve(deviceCount);

        if (deviceCount > 0)
        {
            for (int ix = 0; ix < deviceCount; ix++)
            {
                std::stringstream ss;
                ss << "device_" << ix;

                auto device = _collParams.get_value_as_collection(ss.str().c_str());
                assert(device);
                if (device)
                {
                    core::coll_helper device_helper(device, false);

                    device_desc desc;
                    desc.name = device_helper.get_value_as_string("name");
                    desc.uid  = device_helper.get_value_as_string("uid");
                    desc.isActive = device_helper.get_value_as_bool("is_active");
                    desc.video_dev_type = (EvoipVideoDevTypes)device_helper.get_value_as_int("video_capture_type");

                    const std::string type = device_helper.get_value_as_string("device_type");
                    desc.dev_type = type == "audio_playback" ? kvoipDevTypeAudioPlayback :
                        type == "video_capture"  ? kvoipDevTypeVideoCapture :
                        type == "audio_capture"  ? kvoipDevTypeAudioCapture :
                        kvoipDevTypeUndefined;

                    assert(desc.dev_type != kvoipDevTypeUndefined);
                    if (desc.dev_type != kvoipDevTypeUndefined)
                    {
                        devices.push_back(desc);
                    }
                }
            }
        }

        assert((deviceCount > 0 && !devices.empty()) || (!deviceCount && devices.empty()));

        if (deviceType == voip2::VideoCapturing)
        {
            updateScreenList(devices);
        }
        emit onVoipDeviceListUpdated((EvoipDevTypes)(deviceType + 1), devices);
    }
    else if (sigType == "media_loc_a_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("param");

        emit onVoipMediaLocalAudio(enabled);
    }
    else if (sigType == "media_loc_v_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("param");
        localCameraEnabled_ = currentSelectedVideoDevice_.video_dev_type == kvoipDeviceCamera ? enabled : localCameraEnabled_;
        localVideoEnabled_  = enabled;

        emit onVoipMediaLocalVideo(enabled);
    }
    else if (sigType == "mouse_tap")
    {
        const quintptr hwnd = _collParams.get_value_as_int64("hwnd");
        const std::string tapType(_collParams.get_value_as_string("mouse_tap_type"));
        const std::string contact(_collParams.get_value_as_string("contact"));

        emit onVoipMouseTapped(hwnd, tapType, contact);
    }
    else if (sigType == "button_tap")
    {
        voip_manager::ButtonTap button;
        button << _collParams;

        if (button.type == voip2::ButtonType_Close)
        {
            setDecline(button.contact.c_str(), false);
        }
        else if (button.type == voip2::ButtonType_GoToChat)
        {
            openChat(QString::fromStdString(button.contact));
        }

        emit onVoipButtonTapped(button);
    }
    else if (sigType == "call_created")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        // We use sound_enabled settings for voip ring.
        // Set sound to 1.0.
        if (!haveEstablishedConnection_)
        {
            emit Utils::InterConnector::instance().stopPttRecord();
            onStartCall(!contactEx.incoming);
        }

        haveEstablishedConnection_ = true;

        emit onVoipCallCreated(contactEx);
        Ui::GetSoundsManager()->callInProgress(true);
    }
    else if (sigType == "call_destroyed")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        if (contactEx.call_count <= 1)
        {

            callTimeTimer_.stop();
            auto elapsed = callTimeElapsed_;

            callTimeElapsed_ = 0;
            haveEstablishedConnection_ = false;
            onEndCall();

            emit onVoipCallTimeChanged(callTimeElapsed_, false);
            Ui::GetSoundsManager()->callInProgress(false);
            Ui::GetDispatcher()->getVoipController().setHangup();
            if (!lastSelectedCamera_.uid.empty())
            {
                setActiveDevice(lastSelectedCamera_);
            }
            else
            {
                // Reset current device to none.
                _setNoneVideoCaptureDevice();
            }

            voip_manager::CallEndStat stat(contactEx.call_count,
                                           elapsed,
                                           contactEx.contact.contact);
            emit onVoipCallEndedStat(stat);
        }

        connectedPeerList_.remove(contactEx.contact);
        emit onVoipCallDestroyed(contactEx);
    }
    else if (sigType == "call_connected")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        // TODO: When we will have video conference, we need to update this code.
        // On reconnect we will not reset timer.
        auto it = std::find (connectedPeerList_.begin(), connectedPeerList_.end(), contactEx.contact);
        if (it == connectedPeerList_.end())
        {
            callTimeElapsed_ = 0;
        }
        emit onVoipCallTimeChanged(callTimeElapsed_, true);

        if (!callTimeTimer_.isActive())
        {
            // Disable mute, if it was set for ring.
            setAPlaybackMute(false);
            callTimeTimer_.start();
        }

        connectedPeerList_.push_back(contactEx.contact);
        emit onVoipCallConnected(contactEx);
    }
    else if (sigType == "frame_size_changed")
    {
        voip_manager::FrameSize fs;
        fs << _collParams;

        emit onVoipFrameSizeChanged(fs);
    }
    else if (sigType == "media_rem_v_changed")
    {
        voip_manager::VideoEnable videoEnable;
        videoEnable << _collParams;
        emit onVoipMediaRemoteVideo(videoEnable);
    }
    else if (sigType == "voip_cipher_state_changed")
    {
        cipherState_ << _collParams;
        emit onVoipUpdateCipherState(cipherState_);
    }
    else if (sigType == "call_out_accepted")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        emit onVoipCallOutAccepted(contactEx);
    }
    else if (sigType == "voip_minimal_bandwidth_state_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("enable");
        emit onVoipMinimalBandwidthChanged(enabled);
    }
    else if (sigType == "voip_mask_engine_enable")
    {
        const bool enabled = _collParams.get_value_as_bool("enable");
        emit onVoipMaskEngineEnable(enabled);
    }
    else if (sigType == "voip_load_mask")
    {
        const bool result       = _collParams.get_value_as_bool("result");
        const std::string name = _collParams.get_value_as_string("name");
        emit onVoipLoadMaskResult(name, result);
    }
    else if (sigType == "layout_changed")
    {
        bool bTray = _collParams.get_value_as_bool("tray");
        std::string layout = _collParams.get_value_as_string("layout_type");
        intptr_t hwnd = _collParams.get_value_as_int64("hwnd");

        emit onVoipChangeWindowLayout(hwnd, bTray, layout);
    }
    else if (sigType == "voip_main_video_layout_changed")
    {
        voip_manager::MainVideoLayout mainLayout;
        mainLayout << _collParams;

        if (mainLayout.type == voip_manager::MVL_OUTGOING)
        {
            callTimeTimer_.stop();
            callTimeElapsed_ = 0;
            emit onVoipCallTimeChanged(callTimeElapsed_, false);
        }

        emit onVoipMainVideoLayoutChanged(mainLayout);
    }
    else if (sigType == "media_rem_v_device_changed")
    {
        voip_manager::device_description device;
        device << _collParams;

        device_desc deviceProxy;
        deviceProxy.uid = device.uid;

        deviceProxy.dev_type = device.type == voip2::AudioPlayback ? kvoipDevTypeAudioPlayback :
            device.type == voip2::VideoCapturing ? kvoipDevTypeVideoCapture :
            device.type == voip2::AudioRecording ? kvoipDevTypeAudioCapture :
            kvoipDevTypeUndefined;

        deviceProxy.video_dev_type = (EvoipVideoDevTypes)device.videoCaptureType;

        if (deviceProxy.dev_type == kvoipDevTypeVideoCapture)
        {
            currentSelectedVideoDevice_ = deviceProxy;
            // Save last camera.
            if (deviceProxy.video_dev_type == kvoipDeviceCamera)
            {
                lastSelectedCamera_ = deviceProxy;
            }
            emit onVoipVideoDeviceSelected(deviceProxy);
        }
    }
}

void voip_proxy::VoipController::updateActivePeerList()
{
    emit onVoipCallNameChanged(activePeerList_);
}

void voip_proxy::VoipController::setHangup()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_stop");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchAPlaybackMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute_switch");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setVolumeAPlayback(int volume)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_volume_change");
    collection.set_value_as_int("volume", std::min(100, std::max(0, volume)));
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchACaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", false);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_setSwitchVCaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", true);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchVCaptureMute()
{
    if (currentSelectedVideoDevice_.video_dev_type == kvoipDeviceDesktop)
    {
        if (!lastSelectedCamera_.uid.empty())
        {
            setActiveDevice(lastSelectedCamera_);
        }
        else
        {
            if (localVideoEnabled_)
            {
                _setSwitchVCaptureMute();
            }
            _setNoneVideoCaptureDevice();
        }
    }
    else
    {
        _setSwitchVCaptureMute();
    }
}

void voip_proxy::VoipController::setMuteSounds(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_sounds_mute");
    collection.set_value_as_bool("mute", _mute);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setActiveDevice(const voip_proxy::device_desc& _description)
{
    assert(!_description.uid.empty());
    if (_description.uid.empty())
    {
        return;
    }

    std::string devType;
    switch (_description.dev_type)
    {
    case kvoipDevTypeAudioPlayback:
    {
        devType = "audio_playback";
        break;
    }
    case kvoipDevTypeAudioCapture:
    {
        devType = "audio_capture";
        break;
    }
    case kvoipDevTypeVideoCapture:
    {
        devType = "video_capture";
        break;
    }
    case kvoipDevTypeUndefined:

    default:
        assert(!"unexpected device type");
        return;
    };

    activeDevices_[_description.dev_type] = _description;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", devType);
    collection.set_value_as_string("uid", _description.uid);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptA(const char* _contact)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_accept");
    collection.set_value_as_string("mode", "audio");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptV(const char* _contact)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_accept");
    collection.set_value_as_string("mode", "video");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::voipReset()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_reset");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setDecline(const char* _contact, bool _busy)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_decline");
    collection.set_value_as_string("mode", _busy ? "busy" : "not_busy");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setStartA(const char* _contact, bool _attach, const char* _where)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    setAvatars(AVATAR_VIDEO_SIZE, _contact, false);

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");
    collection.set_value_as_qstring("contact", QString::fromUtf8(_contact));
    collection.set_value_as_string("call_type", "audio");
    collection.set_value_as_string("mode", _attach ? "attach" : "not attach");

    dispatcher_.post_message_to_core("voip_call", collection.get());

    // Send statistic
    if (!_attach && _where)
    {
        core::stats::event_props_type props;

        props.emplace_back("Where", _where);

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::voip_calls_users_caller, props);
    }
}

void voip_proxy::VoipController::setStartV(const char* _contact, bool _attach, const char* _where)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    setAvatars(AVATAR_VIDEO_SIZE, _contact, false);

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");
    collection.set_value_as_qstring("contact", QString::fromUtf8(_contact));
    collection.set_value_as_string("call_type", "video");
    collection.set_value_as_string("mode", _attach ? "attach" : "not attach");

    dispatcher_.post_message_to_core("voip_call", collection.get());

    // Send statistic
    if (!_attach && _where)
    {
        core::stats::event_props_type props;

        props.emplace_back("Where", _where);

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::voip_calls_users_caller, props);
    }
}

void voip_proxy::VoipController::setWindowOffsets(quintptr _hwnd, int _l, int _t, int _r, int _b)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_set_window_offsets");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_int("left", _l);
    collection.set_value_as_int("top", _t);
    collection.set_value_as_int("right", _r);
    collection.set_value_as_int("bottom", _b);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowAdd(quintptr _hwnd, bool _primaryWnd, bool _incomeingWnd, int _panelHeight)
{
    if (_hwnd)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
        {// window
            collection.set_value_as_string("type", "voip_add_window");
            collection.set_value_as_double("scale", Utils::getScaleCoefficient() * Utils::scale_bitmap_ratio());
            collection.set_value_as_bool("mode", _primaryWnd);
            collection.set_value_as_bool("incoming_mode", _incomeingWnd);
            collection.set_value_as_int64("handle", _hwnd);
        }

        if (INCLUDE_CAMERA_OFF_STATUS && _primaryWnd)
        {
            QColor penColor = QColor(255, 255, 255, (255 * 90) / 100);
            QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(9), Fonts::FontWeight::SemiBold);
            font.setStyleStrategy(QFont::PreferAntialias);
            addTextToCollection(collection, "camera_status", "VOICE", penColor, font);
            addTextToCollection(collection, "connecting_status", "CONNECTING", penColor, font);
            addTextToCollection(collection, "calling", "CALLING", penColor, font);
            addTextToCollection(collection, "closedByBusy", "BUSY", penColor, font);
        }

        if (INCLUDE_WATERMARK && _primaryWnd)
        {
            addImageToCollection(collection, "voip/", "watermark", "watermark_", "", false);
        }


        if (INCLUDE_CAMERA_OFF_STATUS && !_primaryWnd && !_incomeingWnd)
        {
            auto color = QColor(255, 255, 255, 255 * 70 / 100);
            QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(9), Fonts::FontWeight::SemiBold);
            font.setStyleStrategy(QFont::PreferAntialias);

            addTextToCollection(collection, "camera_status", "VOICE", color, font);
            addTextToCollection(collection, "connecting_status", "CONNECTING", color, font);
            addTextToCollection(collection, "calling", "CALLING", color, font);
        }

        dispatcher_.post_message_to_core("voip_call", collection.get());

        if (!_incomeingWnd)
        {
            setWindowOffsets(_hwnd, Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(_panelHeight) * Utils::scale_bitmap_ratio());
        }

        setWindowBackground(_hwnd, (char)bkColor.red(), (char)bkColor.green(), (char)bkColor.blue(), (char)bkColor.alpha());
    }
}

void voip_proxy::VoipController::setWindowRemove(quintptr _hwnd)
{
    if (_hwnd)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
        collection.set_value_as_string("type", "voip_remove_window");
        collection.set_value_as_int64("handle", _hwnd);
        dispatcher_.post_message_to_core("voip_call", collection.get());
    }
}

void voip_proxy::VoipController::getSecureCode(voip_manager::CipherState& _state) const
{
    _state.state      = cipherState_.state;
    _state.secureCode = cipherState_.secureCode;
}

void voip_proxy::VoipController::setAPlaybackMute(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute");
    collection.set_value_as_string("mute", _mute ? "on" : "off");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::loadPlaybackVolumeFromSettings()
{
    bool soundEnabled = Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true);
    setAPlaybackMute(!soundEnabled);
}

void voip_proxy::VoipController::_checkIgnoreContact(const QString& _contact)
{
    std::string contactName = _contact.toStdString();

    if (
        std::any_of(
            activePeerList_.contacts.cbegin(), activePeerList_.contacts.cend(), [&contactName](const voip_manager::Contact& _contact)
            {
                return _contact.contact == contactName;
            }
            )
        )
    {
        setHangup();
    }
}

void voip_proxy::VoipController::onStartCall(bool _bOutgoing)
{
    setVolumeAPlayback(100);

    // Change volume from application settings only for incomming calls.
    if (!_bOutgoing)
    {
        loadPlaybackVolumeFromSettings();
    }
#ifdef __APPLE__
        // Pause iTunes if we have call.
    iTunesWasPaused_ = iTunesWasPaused_ || platform_macos::pauseiTunes();
#endif

    connect(Logic::getContactListModel(), &Logic::ContactListModel::ignore_contact, this, &voip_proxy::VoipController::_checkIgnoreContact);
    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &voip_proxy::VoipController::avatarChanged);

}

void voip_proxy::VoipController::onEndCall()
{
#ifdef __APPLE__
    // Resume iTunes if we paused it.
    if (iTunesWasPaused_)
    {
        platform_macos::playiTunes();
        iTunesWasPaused_ = false;
    }
#endif

    disconnect(Logic::getContactListModel(), &Logic::ContactListModel::ignore_contact, this, &voip_proxy::VoipController::_checkIgnoreContact);
    disconnect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &voip_proxy::VoipController::avatarChanged);
}

void voip_proxy::VoipController::switchMinimalBandwithMode()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_minimal_bandwidth_switch");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowBackground(quintptr _hwnd, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);

    collection.set_value_as_string("type", "backgroung_update");

    core::ifptr<core::istream> background_stream(collection->create_stream());
    const unsigned  char pixel[] = {r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a};
    background_stream->write((const uint8_t*)pixel, 4 * 4);

    collection.set_value_as_stream("background", background_stream.get());
    collection.set_value_as_int("background_height", 2);
    collection.set_value_as_int("background_width", 2);
    collection.set_value_as_int64("window_handle", _hwnd);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::updateUserAvatar(const voip_manager::ContactEx& _contactEx)
{
    setAvatars(AVATAR_VIDEO_SIZE, _contactEx.contact.contact.c_str(), false);
}

void voip_proxy::VoipController::updateUserAvatar(const std::string& _account, const std::string& _contact)
{
    setAvatars(AVATAR_VIDEO_SIZE, _contact.c_str(), false);
}

void voip_proxy::VoipController::loadMask(const std::string& maskPath)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_load_mask");
    collection.set_value_as_string("path", maskPath);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowSetPrimary(quintptr _hwnd, const char* _contact)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "window_set_primary");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_string("contact", _contact);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

voip_masks::MaskManager* voip_proxy::VoipController::getMaskManager() const
{
    return maskManager_.get();
}

void voip_proxy::VoipController::updateVoipMaskEngineEnable(bool _enable)
{
    maskEngineEnable_ = _enable;
}

bool voip_proxy::VoipController::isMaskEngineEnabled() const
{
    return maskEngineEnable_;
}

void voip_proxy::VoipController::initMaskEngine() const
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_init_mask_engine");

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::resetMaskManager()
{
    maskManager_.reset(new voip_masks::MaskManager(dispatcher_, this));
}

const std::vector<voip_proxy::device_desc>& voip_proxy::VoipController::deviceList(EvoipDevTypes type) const
{
    auto arrayIndex = type - 1;
    if (arrayIndex >= 0 && arrayIndex < sizeof(devices_) / sizeof(devices_[0]))
    {
        return devices_[arrayIndex];
    }

    assert(false && "Wrong device type");

    static const std::vector<voip_proxy::device_desc> emptyList;

    return emptyList;
}

std::optional<voip_proxy::device_desc> voip_proxy::VoipController::activeDevice(EvoipDevTypes type) const
{
    if (const auto it = activeDevices_.find(type); it != activeDevices_.end())
        return it->second;
    return std::nullopt;
}

void voip_proxy::VoipController::setWindowVideoLayout(quintptr _hwnd, voip_manager::VideoLayout& layout)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_window_set_conference_layout");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_int("layout", layout);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::onCallCreate(const voip_manager::ContactEx& _contactEx)
{
    setAvatars(360, _contactEx.contact.account.c_str(), true);
}

void voip_proxy::VoipController::avatarChanged(const QString& aimId)
{
    auto currentContact = aimId.toStdString();
    auto foundContact = std::find_if(activePeerList_.contacts.begin(), activePeerList_.contacts.end(), [&currentContact](const voip_manager::Contact& contact) {
        return currentContact == contact.contact;
    });

    // Update only own avatar.
    //bool hasActiveCall = (foundContact != activePeerList_.contacts.end() && (activePeerList_.contacts.size() > 1 || !connectedPeerList_.empty()));

    setAvatars(360, currentContact.c_str(), false);

    if (aimId == Ui::MyInfo()->aimId())
    {
        setAvatars(360, currentContact.c_str(), true);
    }
}

const std::vector<voip_manager::Contact>& voip_proxy::VoipController::currentCallContacts() const
{
    return activePeerList_.contacts;
}

bool voip_proxy::VoipController::isVoipKilled() const
{
    return voipIsKilled_;
}

bool voip_proxy::VoipController::isConnectionEstablished() const
{
    return haveEstablishedConnection_;
}

bool voip_proxy::VoipController::hasEstablishCall() const
{
    return !connectedPeerList_.empty();
}

int voip_proxy::VoipController::maxVideoConferenceMembers() const
{
    return 5;
}

void voip_proxy::VoipController::openChat(const QString& contact)
{
    if (Ui::MainPage* mainPage = Ui::MainPage::instance())
    {
        if (Ui::MainWindow* wnd = static_cast<Ui::MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
        }
    }

    Logic::getContactListModel()->setCurrent(contact, -1, true);
}


void voip_proxy::VoipController::switchShareScreen(voip_proxy::device_desc const * _description)
{
    // If user has not camera.
    if (lastSelectedCamera_.uid.empty())
    {
        if (currentSelectedVideoDevice_.video_dev_type == kvoipDeviceDesktop)
        {
            _setNoneVideoCaptureDevice();
        }
        else
        {
            setActiveDevice(*_description);
        }

        _setSwitchVCaptureMute();
    }
    else
    {
        if (_description && currentSelectedVideoDevice_.video_dev_type == kvoipDeviceCamera)
        {
            setActiveDevice(*_description);

            if (!localVideoEnabled_)
            {
                _setSwitchVCaptureMute();
            }
        }
        else
        {
            setActiveDevice(lastSelectedCamera_);

            if (localVideoEnabled_ && !localCameraEnabled_)
            {
                _setSwitchVCaptureMute();
            }
        }
    }
}

void voip_proxy::VoipController::updateScreenList(const std::vector<voip_proxy::device_desc>& _devices)
{
    screenList_.clear();

    for (voip_proxy::device_desc device : _devices)
    {
        if (device.dev_type == voip_proxy::kvoipDevTypeVideoCapture && device.video_dev_type == voip_proxy::kvoipDeviceDesktop)
        {
            screenList_.push_back(device);
        }
    }
}

void voip_proxy::VoipController::_setNoneVideoCaptureDevice()
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", "video_capture");
    collection.set_value_as_string("uid", "");

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

const QList<voip_proxy::device_desc>& voip_proxy::VoipController::screenList() const
{
    return screenList_;
}

void voip_proxy::VoipController::fillColorPlanes(QImage& image, QColor color)
{
    for (int y = 0; y < image.height(); y++)
    {
        char* line = (char*)image.scanLine(y);
        if (line)
        {
            for (int x = 0; x < image.width(); x++)
            {
                {
                    //line[0] = line[3];
                    //line[1] = line[3];
                    //line[2] = line[3];
                    //line[3] = (char)sqrt(line[3] * 255);
                }

                //if (line[3] > 0)
                //{
                //    line[0] = (char)color.red();
                //    line[1] = (char)color.green();
                //    line[2] = (char)color.blue();
                //}
                line += 4;
            }
        }
    }
}

void voip_proxy::VoipController::passWindowHover(quintptr _hwnd, bool hover)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "voip_window_set_hover");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_bool("hover", hover);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::updateLargeState(quintptr _hwnd, bool large)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "voip_window_large_state");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_bool("large", large);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}


voip_proxy::VoipEmojiManager::VoipEmojiManager()
: activeMapId_(0)
{

}

voip_proxy::VoipEmojiManager::~VoipEmojiManager()
{

}

bool voip_proxy::VoipEmojiManager::addMap(const unsigned _sw, const unsigned _sh, const std::string& _path, const std::vector<unsigned>& _codePoints, const size_t _size)
{
    assert(!_codePoints.empty());

    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    const size_t id = STRING_ID(_path);
    bool existsSameSize = false;
    bool exactlySame    = false;

    if (it != codeMaps_.end())
    {
        existsSameSize = (it->codePointSize == _size);
        exactlySame    = existsSameSize && (it->id == id);
    }

    if (exactlySame)
    {
        return true;
    }

    if (existsSameSize)
    {
        codeMaps_.erase(it);
    }

    CodeMap codeMap;
    codeMap.id            = id;
    codeMap.path          = _path;
    codeMap.codePointSize = _size;
    codeMap.sw            = _sw;
    codeMap.sh            = _sh;
    codeMap.codePoints    = _codePoints;

    codeMaps_.push_back(codeMap);
    std::sort(
        codeMaps_.begin(), codeMaps_.end(), [] (const CodeMap& l, const CodeMap& r)
        {
            return l.codePointSize < r.codePointSize;
        }
    );
    return true;
}

bool voip_proxy::VoipEmojiManager::getEmoji(const unsigned _codePoint, const size_t _size, QImage& _image)
{
    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    if (it == codeMaps_.end())
    {
        if (codeMaps_.empty())
        {
            return false;
        }
        it = codeMaps_.end();

        assert(false);
        return false;
    }

    const CodeMap& codeMap = *it;

    if (codeMap.id != activeMapId_)
    { // reload code map
        activeMap_.load(QString::fromStdString(codeMap.path));
        activeMapId_ = codeMap.id;
    }

    const size_t mapW = activeMap_.width();
    const size_t mapH = activeMap_.height();
    assert(mapW != 0 && mapH != 0);

    if (mapW == 0 || mapH == 0)
    {
        return false;
    }

    const size_t rows = mapH / codeMap.sh;
    const size_t cols = mapW / codeMap.sw;

    for (size_t ix = 0; ix < codeMap.codePoints.size(); ++ix)
    {
        const unsigned code = codeMap.codePoints[ix];
        if (code == _codePoint)
        {
            const size_t targetRow = ix / cols;
            const size_t targetCol = ix - (targetRow * cols);

            if (targetRow >= rows)
            {
                return false;
            }

            const QRect r(targetCol * codeMap.sw, targetRow * codeMap.sh, codeMap.sw, codeMap.sh);
            _image = activeMap_.copy(r);
            return true;
        }
    }

    assert(false);
    return false;
}

