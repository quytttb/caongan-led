import QtQuick

// Nhãn trạng thái đặc thù LED: chữ lớn, tương phản cao, không phải chip 24 px
// của UI kit desktop. Màu chỉ là lớp phụ; nhãn chữ luôn hiện rõ.
Rectangle {
    id: root
    property string stateKey: "noData"
    property string text: "CHƯA CÓ DỮ LIỆU"
    property int fontSize: 30
    property int maximumWidth: 424

    implicitWidth: Math.min(label.implicitWidth + 30, maximumWidth)
    implicitHeight: 42
    radius: 4
    color: "#000000"
    border.width: 1
    border.color: foregroundFor(stateKey)

    function foregroundFor(key) {
        switch (key) {
        case "withinLimit":
        case "exceedsLimit":
            return "#00E5FF"
        case "deviceFault":
        case "conflict":
        case "calibrating":
        case "maintenance":
        case "unitMismatch":
            return "#FF5252"
        case "stale":
        case "futureTimestamp":
        case "invalid":
        case "unknownStatus":
            return "#D0D0D0"
        default:
            return "#FFFFFF"
        }
    }

    Text {
        id: label
        width: Math.min(implicitWidth, root.maximumWidth - 24)
        anchors.centerIn: parent
        text: root.text
        color: root.foregroundFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: root.fontSize
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 16
        renderType: Text.NativeRendering
    }
}
