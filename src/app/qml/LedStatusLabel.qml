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
    color: backgroundFor(stateKey)
    border.width: 1
    border.color: foregroundFor(stateKey)

    function foregroundFor(key) {
        switch (key) {
        case "measuring":
        case "withinLimit":
            return "#00E5FF"
        case "exceedsLimit":
        case "deviceFault":
        case "conflict":
            return "#FF5252"
        case "calibrating":
        case "unitMismatch":
            return "#FFD60A"
        case "stale":
        case "futureTimestamp":
        case "invalid":
        case "unknownStatus":
            return "#D0D0D0"
        default:
            return "#9AA4AE"
        }
    }

    function backgroundFor(key) {
        switch (key) {
        case "measuring":
        case "withinLimit":
            return "#062A33"
        case "exceedsLimit":
        case "deviceFault":
        case "conflict":
            return "#3A0808"
        case "calibrating":
        case "unitMismatch":
            return "#332700"
        default:
            return "#1A1D21"
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
        minimumPixelSize: 22
        renderType: Text.NativeRendering
    }
}
