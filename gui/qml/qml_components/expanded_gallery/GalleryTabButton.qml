import QtQuick 2.15
import QtQuick.Controls 2.15

import Im.Styles 1.0
import Im.Fonts 1.0
import Im.Components 1.0

Button {
    id: root
    property bool selected: false
    property alias name: contentText.text
    leftPadding: Utils.scaleValue(10)
    rightPadding: leftPadding

    readonly property color selectedColor: Style.getColor(StyleVariable.PRIMARY)
    readonly property color hoveredColor: Style.getColor(StyleVariable.BASE_BRIGHT_INVERSE)

    contentItem: Text {
        id: contentText
        color: root.selected ? Style.getColor(StyleVariable.TEXT_PRIMARY) : Style.getColor(StyleVariable.BASE_PRIMARY)
        font: Fonts.appFontScaled(15, FontWeight.Medium)
    }

    background: Rectangle {
        radius: Utils.scaleValue(16)
        color: root.selected ? root.selectedColor : root.hovered ? root.hoveredColor : "transparent"
        opacity: root.selected ? 0.1 : 1
        border.width: Utils.scaleValue(1)
        border.color: root.selected ? root.selectedColor : root.hoveredColor
    }

    CursorHandler {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
}
