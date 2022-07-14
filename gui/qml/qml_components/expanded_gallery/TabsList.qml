import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Im.Styles 1.0
import Im.Fonts 1.0
import Im.Components 1.0

ListView {
    id: root
    readonly property int contentPadding: Utils.scaleValue(10)
    readonly property int gradientWidth: Utils.scaleValue(20)
    readonly property color gradientColor: Style.getColor(StyleVariable.BASE_GLOBALWHITE, 0.8)
    readonly property color gradientEndColor: "transparent"
    orientation: ListView.Horizontal
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    header: Item { width: contentPadding }
    footer: Item { width: contentPadding }

    clip: true

    spacing: contentPadding

    Rectangle {
        width: root.gradientWidth
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        visible: !parent.atXBeginning
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0
                color: root.gradientColor
            }
            GradientStop {
                position: 1
                color: root.gradientEndColor
            }
        }
    }

    Rectangle {
        width: root.gradientWidth
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        visible: !parent.atXEnd
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 1
                color: root.gradientColor
            }
            GradientStop {
                position: 0
                color: root.gradientEndColor
            }
        }
    }

    WheelInverter {
        anchors.fill: parent
    }

    function scrollToIndex(_index) {
        if (_index < 0 || _index >= root.count)
            return;
        let item = root.itemAtIndex(_index);
        if (item === null) { // item not created yet
            root.positionViewAtIndex(_index, ListView.Contain);
            item = root.itemAtIndex(_index);
        }
        if (item.x - root.contentX < root.gradientWidth)
            root.contentX = Math.max(-root.contentPadding, item.x - root.gradientWidth);
        else if (item.x + item.width - root.contentX > root.width - root.gradientWidth)
            root.contentX = Math.min(root.contentWidth - root.contentPadding - root.width, item.x + item.width - root.width + root.gradientWidth);
    }
}
