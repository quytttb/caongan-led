import QtQuick
import QtQuick.Window
import LoggerKit.Theme
import CaonganLed.App

// QtObject không có trạng thái visible/parent Item để vô tình ẩn hai cửa sổ
// top-level. Hai property giữ ownership suốt vòng đời QML engine.
QtObject {
    id: root

    Component.onCompleted: {
        ThemeMode.mode = "dark"
        ThemeMode.touch = false
    }

    property LedWindow ledWindow: LedWindow {
        id: ledWindow
    }

    property OperatorWindow operatorWindow: OperatorWindow {
        id: operatorWindow
        visible: true
    }
}
