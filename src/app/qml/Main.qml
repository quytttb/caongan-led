import QtQuick
import QtQuick.Window
import LoggerKit.Theme
import CaonganLed.App

// Root không hiển thị: hai cửa sổ độc lập (LED công khai + vận hành).
Item {
    id: root
    width: 0
    height: 0
    visible: false

    Component.onCompleted: {
        ThemeMode.mode = "dark"
        ThemeMode.touch = false
    }

    LedWindow {
        id: ledWindow
    }

    OperatorWindow {
        id: operatorWindow
        visible: true
    }
}
