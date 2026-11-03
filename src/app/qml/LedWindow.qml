import QtQuick
import QtQuick.Window

Window {
    id: win
    objectName: "ledWindow"
    title: app.branding.companyName || "Cao Ngạn"
    color: "black"
    flags: Qt.FramelessWindowHint | Qt.Window
    visibility: app.ledWindowVisible && targetReady
                ? Window.FullScreen
                : Window.Hidden
    width: 1600
    height: 960

    property bool targetReady: false
    property string positionedName: ""

    function positionTarget() {
        // Ẩn trước khi chuyển màn; nếu output mất kết nối, không đưa LED
        // sang màn vận hành. C++ dùng QGuiApplication::screens().
        targetReady = false
        positionedName = app.targetScreenName
        targetReady = app.positionLedWindow(win)
    }

    Component.onCompleted: positionTarget()

    Connections {
        target: app
        function onConfigChanged() {
            if (win.positionedName !== app.targetScreenName)
                win.positionTarget()
        }
        function onScreensChanged() { win.positionTarget() }
    }

    Item {
        anchors.fill: parent
        clip: true

        LedBoard {
            anchors.centerIn: parent
            scale: Math.min(parent.width / width, parent.height / height)
        }
    }
}
