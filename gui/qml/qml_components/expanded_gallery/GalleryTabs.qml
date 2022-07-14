import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Im.Styles 1.0
import Im.Fonts 1.0
import Im.Components 1.0

Control {
    id: root
    leftPadding: Utils.scaleValue(20)
    rightPadding: 0

    property color backgroundColor: Style.getColor(StyleVariable.BASE_GLOBALWHITE)

    contentItem: RowLayout {
        spacing: 0

        BurgerButton {
            id: burgerButton
            Accessible.name: Testing.accessibleName("AS ExpandedGallery burgerButton")
            selected: im.gallery.popupVisible
            property bool pressedWhenPopupHidden: false
            onPressed: {
                pressedWhenPopupHidden = !im.gallery.popupVisible;
            }
            onClicked: {
                if (pressedWhenPopupHidden) {
                    im.gallery.popupVisible = true;
                }
                pressedWhenPopupHidden = false;
            }
        }

        TabsList {
            id: tabsList
            model: im.gallery.tabs
            Layout.alignment: Qt.AlignVCenter
            height: Utils.scaleValue(32)
            Layout.fillWidth: true
            Accessible.name: Testing.accessibleName("AS TabsList")
            delegate: GalleryTabButton {
                name: tabName
                selected: tabSelected
                onClicked: {
                    im.gallery.tabs.selectedTab = contentType;
                }
                Accessible.name: Testing.accessibleName("AS GalleryTabButton " + tabName)
            }
        }
    }

    background: Rectangle {
        color: root.backgroundColor
    }

    Connections {
        target: im.gallery.tabs
        function onSelectedTabChanged(_tab) {
            root.scrollToTab(_tab)
        }
        function onTabClicked(_tab) {
            root.scrollToTab(_tab)
        }
    }

    function scrollToTab(_tab) {
        const tabIndex = im.gallery.tabs.tabIndex(_tab);
        tabsList.scrollToIndex(tabIndex);
    }
}
