import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Im.Styles 1.0
import Im.Fonts 1.0
import Im.Components 1.0

Button {
    id: root
    property bool selected: false
    padding: 0
    width: Utils.scaleValue(32)
    height: width
    Layout.alignment: Qt.AlignVCenter
    contentItem: RowLayout {
        spacing: 0
        SvgIcon {
            id: burgerIcon
            iconPath: ":/expanded_gallery/burger"
            width: Utils.scaleValue(20)
            height: width
            color: Style.getColor(root.selected ? StyleVariable.TEXT_PRIMARY : StyleVariable.BASE_SECONDARY)
            Layout.alignment: Qt.AlignVCenter
        }
        SvgIcon {
            iconPath: ":/expanded_gallery/arrow_down"
            width: Utils.scaleValue(12)
            height: width
            color: burgerIcon.color
            rotation: root.selected ? 180 : 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

    CursorHandler {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    background: Item {}
}
