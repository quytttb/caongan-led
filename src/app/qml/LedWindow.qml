import QtQuick
import QtQuick.Window
import CaonganLed.App

Window {
    id: win
    objectName: "ledWindow"
    title: "Cao Ngạn LED"
    color: "black"
    flags: app.ledFullScreen ? (Qt.FramelessWindowHint | Qt.Window) : Qt.Window
    visibility: app.ledWindowVisible && targetReady
                ? (app.ledFullScreen ? Window.FullScreen : Window.Windowed)
                : Window.Hidden
    width: app.ledFullScreen ? 1920 : 1280
    height: app.ledFullScreen ? 1200 : 800
    x: 40
    y: 40

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
            width: 1920
            height: 1200
            anchors.centerIn: parent
            scale: Math.min(parent.width / 1920, parent.height / 1200)
        }
    }
}
