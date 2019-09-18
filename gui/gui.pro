#-------------------------------------------------
#
# Project created by QtCreator 2015-12-27T23:44:05
#
#-------------------------------------------------

QT += multimedia network widgets

TARGET = icq
TEMPLATE = app

CONFIG += precompile_header

CONFIG += 64

CONFIG += res_internal

CORELIB_PATH = ../corelib

PRECOMPILED_HEADER = stdafx.h

SOURCES += \
    app_config.cpp \
    core_dispatcher.cpp \
    gui_settings.cpp \
    main.cpp \
    my_info.cpp \
    remote_proc.cpp \
    stdafx.cpp \
    cache/countries.cpp \
    cache/avatars/AvatarStorage.cpp \
    cache/emoji/Emoji.cpp \
    cache/emoji/EmojiDb.cpp \
    cache/emoji/EmojiIndexData.cpp \
    cache/stickers/stickers.cpp \
    controls/BackButton.cpp \
    controls/ContactAvatarWidget.cpp \
    controls/ContextMenu.cpp \
    controls/CountrySearchCombobox.cpp \
    controls/CustomButton.cpp \
    controls/DpiAwareImage.cpp \
    controls/FlatMenu.cpp \
    controls/FlowLayout.cpp \
    controls/GeneralDialog.cpp \
    controls/LabelEx.cpp \
    controls/LineEditEx.cpp \
    controls/PictureWidget.cpp \
    controls/SemitransparentWindow.cpp \
    controls/TextEditEx.cpp \
    controls/TextEmojiWidget.cpp \
    controls/WidgetsNavigator.cpp \
    main_window/ContactDialog.cpp \
    main_window/LoginPage.cpp \
    main_window/MainPage.cpp \
    main_window/MainWindow.cpp \
    main_window/contact_list/AbstractSearchModel.cpp \
    main_window/contact_list/ChatMembersModel.cpp \
    main_window/contact_list/Common.cpp \
    main_window/contact_list/ContactItem.cpp \
    main_window/contact_list/ContactList.cpp \
    main_window/contact_list/ContactListItemDelegate.cpp \
    main_window/contact_list/ContactListItemRenderer.cpp \
    main_window/contact_list/ContactListModel.cpp \
    main_window/contact_list/contact_profile.cpp \
    main_window/contact_list/RecentItemDelegate.cpp \
    main_window/contact_list/RecentsItemRenderer.cpp \
    main_window/contact_list/RecentsModel.cpp \
    main_window/contact_list/SearchMembersModel.cpp \
    main_window/contact_list/SearchModel.cpp \
    main_window/contact_list/SearchWidget.cpp \
    main_window/contact_list/SelectionContactsForGroupChat.cpp \
    main_window/contact_list/SettingsTab.cpp \
    main_window/contact_list/SettingsWidget.cpp \
    main_window/history_control/ChatEventInfo.cpp \
    main_window/history_control/ChatEventItem.cpp \
    main_window/history_control/FileSharingInfo.cpp \
    main_window/history_control/FileSizeFormatter.cpp \
    main_window/history_control/HistoryControl.cpp \
    main_window/history_control/HistoryControlPage.cpp \
    main_window/history_control/HistoryControlPageItem.cpp \
    main_window/history_control/KnownFileTypes.cpp \
    main_window/history_control/MessageItem.cpp \
    main_window/history_control/MessagesModel.cpp \
    main_window/history_control/NewMessagesPlate.cpp \
    main_window/history_control/ServiceMessageItem.cpp \
    main_window/history_control/StickerInfo.cpp \
    main_window/history_control/VoipEventInfo.cpp \
    main_window/history_control/VoipEventItem.cpp \
    main_window/history_control/auth_widget/AuthWidget.cpp \
    main_window/input_widget/InputWidget.cpp \
    main_window/search_contacts/ComboButton.cpp \
    main_window/search_contacts/SearchContactsWidget.cpp \
    main_window/search_contacts/SearchFilters.cpp \
    main_window/search_contacts/search_params.cpp \
    main_window/search_contacts/results/ContactWidget.cpp \
    main_window/search_contacts/results/FoundContacts.cpp \
    main_window/search_contacts/results/NoResultsWidget.cpp \
    main_window/search_contacts/results/SearchResults.cpp \
    main_window/settings/GeneralSettingsWidget.cpp \
    main_window/settings/ProfileSettingsWidget.cpp \
    main_window/smiles_menu/SmilesMenu.cpp \
    main_window/smiles_menu/toolbar.cpp \
    main_window/sounds/SoundsManager.cpp \
    main_window/tray/MessageAlertWidget.cpp \
    main_window/tray/RecentMessagesAlert.cpp \
    main_window/tray/TrayIcon.cpp \
    previewer/Previewer.cpp \
    themes/ThemePixmap.cpp \
    themes/Themes.cpp \
    types/chat.cpp \
    types/contact.cpp \
    types/message.cpp \
    utils/application.cpp \
    utils/gui_coll_helper.cpp \
    utils/InterConnector.cpp \
    utils/launch.cpp \
    utils/local_peer.cpp \
    utils/PainterPath.cpp \
    utils/SChar.cpp \
    utils/Text2DocConverter.cpp \
    utils/translator.cpp \
    utils/uid.cpp \
    utils/utils.cpp \
    voip/AvatarContainerWidget.cpp \
    voip/CallPanelMain.cpp \
    voip/DetachedVideoWnd.cpp \
    voip/IncomingCallWindow.cpp \
    voip/NameAndStatusWidget.cpp \
    voip/VideoPanel.cpp \
    voip/VideoPanelHeader.cpp \
    voip/VideoSettings.cpp \
    voip/VideoWindow.cpp \
    voip/VoipProxy.cpp \
    voip/VoipSysPanelHeader.cpp \
    ../gui.shared/translator_base.cpp \
    utils/log/log.cpp \
    utils/profiling/auto_stop_watch.cpp \
    controls/BackgroundWidget.cpp \
    ../common.shared/common_defs.cpp \
    main_window/history_control/MessageItemLayout.cpp \
    main_window/history_control/MessagesScrollArea.cpp \
    main_window/history_control/MessagesScrollAreaLayout.cpp \
    main_window/history_control/MessagesScrollbar.cpp \
    main_window/history_control/ResizePixmapTask.cpp \
    main_window/sounds/MpegLoader.cpp \
    utils/Text.cpp \
    main_window/history_control/MessageStatusWidget.cpp \
    main_window/history_control/MessageStyle.cpp \
    theme_settings.cpp \
    cache/themes/themes_cache.cpp \
    main_window/settings/themes/ThemesModel.cpp \
    main_window/settings/themes/ThemesSettingsWidget.cpp \
    main_window/settings/themes/ThemesWidget.cpp \
    main_window/settings/themes/ThemeWidget.cpp \
    main_window/settings/ContactUs.cpp \
    main_window/contact_list/CustomAbstractListModel.cpp \
    main_window/history_control/HistoryControlPageThemePanel.cpp \
    main_window/settings/AttachPhone.cpp \
    main_window/settings/AttachUin.cpp \
    controls/ConnectionSettingsWidget.cpp \
    main_window/IntroduceYourself.cpp \
    main_window/livechats/LiveChatMembersControl.cpp \
    main_window/livechats/LiveChatProfile.cpp \
    types/typing.cpp \
    controls/GeneralCreator.cpp \
    main_window/GroupChatOperations.cpp \
    utils/LoadPixmapFromDataTask.cpp \
    utils/LoadPixmapFromFile.cpp \
    main_window/history_control/ContentWidgets/FileSharingWidget.cpp \
    main_window/history_control/ContentWidgets/ImagePreviewWidget.cpp \
    main_window/history_control/ContentWidgets/MessageContentWidget.cpp \
    main_window/history_control/ContentWidgets/PreviewContentWidget.cpp \
    main_window/history_control/ContentWidgets/PttAudioWidget.cpp \
    controls/ImageCropper.cpp \
    main_window/livechats/LiveChatItemDelegate.cpp \
    main_window/livechats/LiveChatsHome.cpp \
    main_window/livechats/LiveChatsModel.cpp \
    main_window/sidebar/MenuPage.cpp \
    main_window/sidebar/ProfilePage.cpp \
    main_window/sidebar/Sidebar.cpp \
    main_window/sidebar/SidebarUtils.cpp \
    controls/AvatarPreview.cpp \
    main_window/PromoPage.cpp \
    main_window/history_control/MessageItemBase.cpp \
    main_window/history_control/DeletedMessageItem.cpp \
    voip/PushButton_t.cpp \
    voip/secureCallWnd.cpp \
    types/link_metadata.cpp \
    types/common_phone.cpp \
    controls/CommonStyle.cpp \
    main_window/history_control/complex_message/ComplexMessageItem.cpp \
    main_window/history_control/complex_message/ComplexMessageItemBuilder.cpp \
    main_window/history_control/complex_message/ComplexMessageItemLayout.cpp \
    main_window/history_control/complex_message/ComplexMessageUtils.cpp \
    main_window/history_control/complex_message/GenericBlock.cpp \
    main_window/history_control/complex_message/GenericBlockLayout.cpp \
    main_window/history_control/complex_message/IItemBlock.cpp \
    main_window/history_control/complex_message/IItemBlockLayout.cpp \
    main_window/history_control/complex_message/ILinkPreviewBlockLayout.cpp \
    main_window/history_control/complex_message/ImagePreviewBlock.cpp \
    main_window/history_control/complex_message/ImagePreviewBlockLayout.cpp \
    main_window/history_control/complex_message/LinkPreviewBlock.cpp \
    main_window/history_control/complex_message/LinkPreviewBlockBlankLayout.cpp \
    main_window/history_control/complex_message/LinkPreviewBlockLayout.cpp \
    main_window/history_control/complex_message/Style.cpp \
    main_window/history_control/complex_message/TextBlock.cpp \
    main_window/history_control/complex_message/TextBlockLayout.cpp \
    main_window/history_control/complex_message/YoutubeLinkPreviewBlockLayout.cpp \
    main_window/history_control/ActionButtonWidget.cpp \
    main_window/history_control/ActionButtonWidgetLayout.cpp \
    utils/LoadMovieFromFileTask.cpp \
    main_window/contact_list/UnknownItemDelegate.cpp \
    main_window/contact_list/UnknownsModel.cpp \
    previewer/DownloadWidget.cpp \
    previewer/GalleryFrame.cpp \
    previewer/GalleryWidget.cpp \
    previewer/ImageCache.cpp \
    previewer/ImageIterator.cpp \
    previewer/ImageLoader.cpp \
    previewer/ImageViewerWidget.cpp \
    main_window/history_control/complex_message/FileSharingBlock.cpp \
    main_window/history_control/complex_message/FileSharingBlockBase.cpp \
    main_window/history_control/complex_message/FileSharingImagePreviewBlockLayout.cpp \
    main_window/history_control/complex_message/FileSharingPlainBlockLayout.cpp \
    main_window/history_control/complex_message/FileSharingUtils.cpp \
    main_window/history_control/complex_message/IFileSharingBlockLayout.cpp \
    main_window/history_control/complex_message/PttBlock.cpp \
    main_window/history_control/complex_message/PttBlockLayout.cpp \
    controls/TransparentScrollBar.cpp \
    utils/exif.cpp \
    main_window/mplayer/FFMpegPlayer.cpp \
    main_window/mplayer/MultimediaViewer.cpp \
    main_window/mplayer/VideoPlayer.cpp \
    controls/ToolTipEx.cpp \
    main_window/settings/SettingsAboutUs.cpp \
    main_window/settings/SettingsGeneral.cpp \
    main_window/settings/SettingsNotifications.cpp \
    main_window/settings/SettingsVoip.cpp \
    types/images.cpp \
    utils/translit.cpp \
    main_window/selection/SelectionPanel.cpp \
    main_window/history_control/complex_message/QuoteBlock.cpp \
    main_window/history_control/complex_message/QuoteBlockLayout.cpp \
    ../gui.shared/TestingTools.cpp \
    voip/CommonUI.cpp \
    main_window/history_control/complex_message/StickerBlock.cpp \
    main_window/history_control/complex_message/StickerBlockLayout.cpp

HEADERS  += \
    app_config.h \
    collection_helper_ext.h \
    constants.h \
    core_dispatcher.h \
    gui_settings.h \
    my_info.h \
    stdafx.h \
    cache/countries.h \
    cache/avatars/AvatarStorage.h \
    cache/emoji/Emoji.h \
    cache/emoji/EmojiDb.h \
    cache/stickers/stickers.h \
    controls/Alert.h \
    controls/BackButton.h \
    controls/ContactAvatarWidget.h \
    controls/ContextMenu.h \
    controls/CountrySearchCombobox.h \
    controls/CustomButton.h \
    controls/DpiAwareImage.h \
    controls/FlatMenu.h \
    controls/FlowLayout.h \
    controls/GeneralDialog.h \
    controls/LabelEx.h \
    controls/LineEditEx.h \
    controls/PictureWidget.h \
    controls/SemitransparentWindow.h \
    controls/TextEditEx.h \
    controls/TextEmojiWidget.h \
    controls/WidgetsNavigator.h \
    main_window/ContactDialog.h \
    main_window/LoginPage.h \
    main_window/MainPage.h \
    main_window/MainWindow.h \
    main_window/contact_list/AbstractSearchModel.h \
    main_window/contact_list/ChatMembersModel.h \
    main_window/contact_list/Common.h \
    main_window/contact_list/ContactItem.h \
    main_window/contact_list/ContactList.h \
    main_window/contact_list/ContactListItemDelegate.h \
    main_window/contact_list/ContactListItemRenderer.h \
    main_window/contact_list/ContactListModel.h \
    main_window/contact_list/contact_profile.h \
    main_window/contact_list/RecentItemDelegate.h \
    main_window/contact_list/RecentsItemRenderer.h \
    main_window/contact_list/RecentsModel.h \
    main_window/contact_list/SearchMembersModel.h \
    main_window/contact_list/SearchModel.h \
    main_window/contact_list/SearchWidget.h \
    main_window/contact_list/SelectionContactsForGroupChat.h \
    main_window/contact_list/SettingsTab.h \
    main_window/contact_list/SettingsWidget.h \
    main_window/history_control/ChatEventInfo.h \
    main_window/history_control/ChatEventItem.h \
    main_window/history_control/FileSharingInfo.h \
    main_window/history_control/FileSharingWidget.h \
    main_window/history_control/FileSizeFormatter.h \
    main_window/history_control/HistoryControl.h \
    main_window/history_control/HistoryControlPage.h \
    main_window/history_control/HistoryControlPageItem.h \
    main_window/history_control/KnownFileTypes.h \
    main_window/history_control/MessageItem.h \
    main_window/history_control/MessagesModel.h \
    main_window/history_control/NewMessagesPlate.h \
    main_window/history_control/ServiceMessageItem.h \
    main_window/history_control/StickerInfo.h \
    main_window/history_control/TextWidget.h \
    main_window/history_control/VoipEventInfo.h \
    main_window/history_control/VoipEventItem.h \
    main_window/history_control/auth_widget/AuthWidget.h \
    main_window/input_widget/InputWidget.h \
    main_window/search_contacts/ComboButton.h \
    main_window/search_contacts/SearchContactsWidget.h \
    main_window/search_contacts/SearchFilters.h \
    main_window/search_contacts/search_params.h \
    main_window/search_contacts/results/ContactWidget.h \
    main_window/search_contacts/results/FoundContacts.h \
    main_window/search_contacts/results/NoResultsWidget.h \
    main_window/search_contacts/results/SearchResults.h \
    main_window/settings/GeneralSettingsWidget.h \
    main_window/settings/ProfileSettingsWidget.h \
    main_window/smiles_menu/SmilesMenu.h \
    main_window/smiles_menu/toolbar.h \
    main_window/sounds/SoundsManager.h \
    main_window/tray/MessageAlertWidget.h \
    main_window/tray/RecentMessagesAlert.h \
    main_window/tray/TrayIcon.h \
    previewer/Previewer.h \
    previewer/PreviewWidget.h \
    themes/IcqStyle.h \
    themes/ResourceIds.h \
    themes/ThemePixmap.h \
    themes/Themes.h \
    types/chat.h \
    types/contact.h \
    types/message.h \
    utils/application.h \
    utils/gui_coll_helper.h \
    utils/InterConnector.h \
    utils/launch.h \
    utils/local_peer.h \
    utils/mac_support.h \
    utils/PainterPath.h \
    utils/SChar.h \
    utils/Text2DocConverter.h \
    utils/translator.h \
    utils/uid.h \
    utils/utils.h \
    voip/AvatarContainerWidget.h \
    voip/CallPanelMain.h \
    voip/DetachedVideoWnd.h \
    voip/IncomingCallWindow.h \
    voip/NameAndStatusWidget.h \
    voip/VideoPanel.h \
    voip/VideoPanelHeader.h \
    voip/VideoSettings.h \
    voip/VideoWindow.h \
    voip/VoipProxy.h \
    voip/VoipSysPanelHeader.h \
    voip/VoipTools.h \
    ../gui.shared/constants.h \
    ../gui.shared/translator_base.h \
    utils/log/log.h \
    utils/profiling/auto_stop_watch.h \
    cp_afxres.h \
    controls/BackgroundWidget.h \
    ../common.shared/common_defs.h \
    main_window/history_control/MessageItemLayout.h \
    main_window/history_control/MessagesScrollArea.h \
    main_window/history_control/MessagesScrollAreaLayout.h \
    main_window/history_control/MessagesScrollbar.h \
    main_window/history_control/ResizePixmapTask.h \
    main_window/sounds/MpegLoader.h \
    utils/Text.h \
    LoadPixmapFromDataTask.h \
    resource.h \
    main_window/history_control/MessageStatusWidget.h \
    main_window/history_control/MessageStyle.h \
    theme_settings.h \
    cache/themes/themes.h \
    main_window/settings/themes/ThemesModel.h \
    main_window/settings/themes/ThemesSettingsWidget.h \
    main_window/settings/themes/ThemesWidget.h \
    main_window/settings/themes/ThemeWidget.h \
    main_window/contact_list/CustomAbstractListModel.h \
    main_window/history_control/HistoryControlPageThemePanel.h \
    controls/ConnectionSettingsWidget.h \
    main_window/IntroduceYourself.h \
    main_window/livechats/LiveChatMembersControl.h \
    main_window/livechats/LiveChatProfile.h \
    types/typing.h \
    controls/GeneralCreator.h \
    main_window/GroupChatOperations.h \
    utils/LoadPixmapFromDataTask.h \
    utils/LoadPixmapFromFileTask.h \
    main_window/history_control/ContentWidgets/FileSharingWidget.h \
    main_window/history_control/ContentWidgets/ImagePreviewWidget.h \
    main_window/history_control/ContentWidgets/MessageContentWidget.h \
    main_window/history_control/ContentWidgets/PreviewContentWidget.h \
    main_window/history_control/ContentWidgets/PttAudioWidget.h \
    controls/ImageCropper.h \
    main_window/livechats/LiveChatItemDelegate.h \
    main_window/livechats/LiveChatsHome.h \
    main_window/livechats/LiveChatsModel.h \
    main_window/sidebar/MenuPage.h \
    main_window/sidebar/ProfilePage.h \
    main_window/sidebar/Sidebar.h \
    main_window/sidebar/SidebarUtils.h \
    controls/AvatarPreview.h \
    main_window/PromoPage.h \
    main_window/history_control/MessageItemBase.h \
    main_window/history_control/DeletedMessageItem.h \
    voip/PushButton_t.h \
    voip/secureCallWnd.h \
    types/link_metadata.h \
    types/common_phone.h \
    controls/CommonStyle.h \
    main_window/history_control/complex_message/ComplexMessageItem.h \
    main_window/history_control/complex_message/ComplexMessageItemBuilder.h \
    main_window/history_control/complex_message/ComplexMessageItemLayout.h \
    main_window/history_control/complex_message/ComplexMessageUtils.h \
    main_window/history_control/complex_message/GenericBlock.h \
    main_window/history_control/complex_message/GenericBlockLayout.h \
    main_window/history_control/complex_message/IItemBlock.h \
    main_window/history_control/complex_message/IItemBlockLayout.h \
    main_window/history_control/complex_message/ILinkPreviewBlockLayout.h \
    main_window/history_control/complex_message/ImagePreviewBlock.h \
    main_window/history_control/complex_message/ImagePreviewBlockLayout.h \
    main_window/history_control/complex_message/LinkPreviewBlock.h \
    main_window/history_control/complex_message/LinkPreviewBlockBlankLayout.h \
    main_window/history_control/complex_message/LinkPreviewBlockLayout.h \
    main_window/history_control/complex_message/Selection.h \
    main_window/history_control/complex_message/Style.h \
    main_window/history_control/complex_message/TextBlock.h \
    main_window/history_control/complex_message/TextBlockLayout.h \
    main_window/history_control/complex_message/YoutubeLinkPreviewBlockLayout.h \
    main_window/history_control/ActionButtonWidget.h \
    main_window/history_control/ActionButtonWidgetLayout.h \
    utils/LoadMovieFromFileTask.h \
    main_window/contact_list/UnknownItemDelegate.h \
    main_window/contact_list/UnknownsModel.h \
    previewer/DownloadWidget.h \
    previewer/GalleryFrame.h \
    previewer/GalleryWidget.h \
    previewer/ImageCache.h \
    previewer/ImageIterator.h \
    previewer/ImageLoader.h \
    previewer/ImageViewerWidget.h \
    main_window/history_control/complex_message/FileSharingBlock.h \
    main_window/history_control/complex_message/FileSharingBlockBase.h \
    main_window/history_control/complex_message/FileSharingImagePreviewBlockLayout.h \
    main_window/history_control/complex_message/FileSharingPlainBlockLayout.h \
    main_window/history_control/complex_message/FileSharingUtils.h \
    main_window/history_control/complex_message/IFileSharingBlockLayout.h \
    main_window/history_control/complex_message/PttBlock.h \
    main_window/history_control/complex_message/PttBlockLayout.h \
    controls/TransparentScrollBar.h \
    utils/exif.h \
    main_window/mplayer/ffmpeg.h \
    main_window/mplayer/FFMpegPlayer.h \
    main_window/mplayer/MultimediaViewer.h \
    main_window/mplayer/VideoPlayer.h \
    controls/ToolTipEx.h \
    types/images.h \
    utils/translit.h \
    main_window/selection/SelectionPanel.h \
    main_window/history_control/complex_message/QuoteBlock.h \
    main_window/history_control/complex_message/QuoteBlockLayout.h \
    ../gui.shared/implayer.h \
    ../gui.shared/TestingTools.h \
    voip/CommonUI.h \
    main_window/history_control/complex_message/StickerBlock.h \
    main_window/history_control/complex_message/StickerBlockLayout.h

DEFINES += STRIP_VOIP
QMAKE_CXXFLAGS += -std=c++0x
QMAKE_LIBS += -lopenal -lavformat -lavcodec -lswresample -lavfilter -lavutil -lswscale -lcorelib -lminizip -lboost_system -lboost_locale -lcurl_static -lssl_static -lcrypto_static -lboost_filesystem -lidn -lrtmp -lgcrypt -lgnutls -lgpg-error -ltasn1 -lz -lstdc++ -lrt -lxcb-util
CONFIG(32, 64|32) {
    QMAKE_LIBS += -lp11-kit
}

QMAKE_LIBS_THREAD = -lxcb-util -lffi -lpcre -lexpat -lXext -lXau -lXdmcp -lz -Wl,-Bdynamic -ldl -lpthread -lX11

QMAKE_LFLAGS += -Wl,-Bstatic -static-libgcc -static-libstdc++ -L$$CORELIB_PATH

CONFIG(64, 64|32) {
    QMAKE_LFLAGS += -L$${PWD}/../external/linux/x64 -L$${PWD}/../external/OpenAl/lib/linux/x64 -L$${PWD}/../external/ffmpeg/lib/linux/x64 -L/x64
} else {
    QMAKE_LFLAGS += -L$${PWD}/../external/linux -L$${PWD}/../external/OpenAl/lib/linux -L$${PWD}/../external/ffmpeg/lib/linux -L/x32
}
INCLUDEPATH += . $${PWD}/../external/OpenAl/include $${PWD}/../external/ffmpeg/include

DISTFILES += \
    themes/ThemePixmapDb.inc

CONFIG(res_internal, res_internal|res_external) {
    RESOURCES = resource.qrc
    DEFINES += INTERNAL_RESOURCES
} else {
    QMAKE_POST_LINK += $$quote(cp -r $${PWD}/qresource $${OUT_PWD})
}
