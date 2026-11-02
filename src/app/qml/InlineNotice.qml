import QtQuick
import LoggerKit.Theme

Rectangle {
    id: root
    property string text: ""
    property bool isError: false
    property bool isWarning: false
    visible: text.length > 0
    implicitHeight: label.implicitHeight + 20
    radius: 6
    color: isError
           ? AppColors.withAlpha(AppColors.error, 0.14)
           : (isWarning ? AppColors.withAlpha(AppColors.warning, 0.12)
                        : AppColors.withAlpha(AppColors.success, 0.10))
    border.width: 1
    border.color: isError
                  ? AppColors.error
                  : (isWarning ? AppColors.warning : AppColors.success)

    Text {
        id: label
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        text: root.text
        color: root.isError
               ? AppColors.error
               : (root.isWarning ? AppColors.warning : AppColors.success)
        font.family: "Inter"
        font.pixelSize: 14
        wrapMode: Text.WordWrap
    }
}
