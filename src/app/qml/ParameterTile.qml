import QtQuick

Item {
    id: root
    width: 381
    height: 195

    property string symbol: ""
    property string unit: ""
    property string rangeText: ""
    property string valueText: "—"
    property string statusText: "CHƯA CÓ DỮ LIỆU"
    property string stateKey: "noData"
    property string timeText: "—"
    property string detailText: ""

    function accentFor(key) {
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

    Rectangle {
        objectName: "tileFrame"
        anchors.fill: parent
        color: "#000000"
        radius: 6
        border.width: 2
        border.color: root.accentFor(root.stateKey)
    }

    Text {
        id: symbolText
        x: 14
        y: 4
        text: root.symbol
        color: root.accentFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: 34
        font.bold: true
        renderType: Text.NativeRendering
    }

    Text {
        id: unitText
        anchors.right: parent.right
        anchors.rightMargin: 14
        y: 10
        width: 160
        text: root.unit
        color: root.accentFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: 24
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
        renderType: Text.NativeRendering
    }

    Text {
        id: valueText
        anchors.horizontalCenter: parent.horizontalCenter
        objectName: "readingValue"
        y: 43
        width: parent.width - 24
        height: 72
        text: root.valueText
        color: root.stateKey === "exceedsLimit" ? "#FF5252" : root.accentFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: 82
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 32
        renderType: Text.NativeRendering
    }

    Text {
        objectName: "measurementRange"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 118
        width: parent.width - 24
        height: 27
        text: root.rangeText.length ? "(" + root.rangeText + ")" : ""
        color: root.accentFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: 24
        minimumPixelSize: 14
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    LedStatusLabel {
        objectName: "tileStatus"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 7
        height: 32
        stateKey: root.stateKey
        text: {
            switch (root.stateKey) {
            case "withinLimit":
            case "measuring": return "ĐANG ĐO"
            case "exceedsLimit": return "ĐANG ĐO · VƯỢT NGƯỠNG"
            case "deviceFault": return "LỖI THIẾT BỊ"
            case "futureTimestamp": return "SAI GIỜ ĐO"
            case "invalid": return "DỮ LIỆU LỖI"
            case "conflict": return "XUNG ĐỘT"
            case "unknownStatus": return "LỖI TRẠNG THÁI"
            default: return root.statusText
            }
        }
        fontSize: 22
        maximumWidth: parent.width - 24
    }
}
