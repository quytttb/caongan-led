import QtQuick
import QtQuick.Controls
import LoggerKit.Theme

Button {
    id: button

    // Các biểu tượng được vẽ bằng hình cơ bản để không phụ thuộc font hệ điều hành.
    property string kind: "minimize"
    property bool maximized: false
    property string description: ""

    implicitWidth: 40
    implicitHeight: 36
    flat: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: description
    ToolTip.text: description
    ToolTip.visible: hovered
    ToolTip.delay: 500

    background: Rectangle {
        radius: 6
        color: button.hovered || button.down
               ? (button.kind === "close" ? "#C42B1C" : AppColors.hoverFill)
               : "transparent"
        border.width: button.visualFocus ? 1 : 0
        border.color: AppColors.primaryColor
    }

    contentItem: Item {
        id: icon
        property color iconColor: button.kind === "close" && (button.hovered || button.down)
                                  ? "white" : AppColors.primaryText

        Rectangle {
            visible: button.kind === "minimize"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 4
            width: 14
            height: 1
            color: icon.iconColor
        }

        Rectangle {
            visible: button.kind === "maximize" && button.maximized
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: 2
            anchors.verticalCenterOffset: -2
            width: 11
            height: 9
            color: "transparent"
            border.width: 1
            border.color: icon.iconColor
        }

        Rectangle {
            visible: button.kind === "maximize"
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: button.maximized ? -2 : 0
            anchors.verticalCenterOffset: button.maximized ? 2 : 0
            width: button.maximized ? 11 : 14
            height: button.maximized ? 9 : 11
            color: button.maximized ? AppColors.surfaceContainerLow : "transparent"
            border.width: 1
            border.color: icon.iconColor
        }

        Repeater {
            model: button.kind === "close" ? 2 : 0
            Rectangle {
                anchors.centerIn: parent
                width: 16
                height: 1
                rotation: index === 0 ? 45 : -45
                color: icon.iconColor
            }
        }
    }
}
