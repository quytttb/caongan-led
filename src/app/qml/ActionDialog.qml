import QtQuick
import QtQuick.Controls
import LoggerKit.Theme

// Hộp xác nhận dùng chung cho các thay đổi cấu hình. Hành động chỉ chạy sau
// khi người vận hành bấm OK; mọi thay đổi vẫn được controller validate/atomic.
Dialog {
    id: root
    property string detailText: ""
    property var pendingAction: null

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(640, parent ? parent.width - 48 : 640)
    modal: true
    focus: true
    title: "Xác nhận thay đổi"
    standardButtons: Dialog.Ok | Dialog.Cancel

    function ask(titleText, detail, action) {
        root.title = titleText
        root.detailText = detail
        root.pendingAction = action
        root.open()
    }

    onAccepted: {
        const action = root.pendingAction
        root.pendingAction = null
        if (action)
            action()
    }
    onRejected: root.pendingAction = null

    contentItem: Text {
        text: root.detailText
        color: AppColors.primaryText
        font.family: "Inter"
        font.pixelSize: 15
        wrapMode: Text.WordWrap
        padding: 4
    }
}
