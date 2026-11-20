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

    function stateColor(key, alarmOnExceeds) {
        if (key === "exceedsLimit")
            return alarmOnExceeds ? "#FF5252" : "#00E5FF"
        switch (key) {
        case "measuring":
        case "withinLimit":
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
        border.color: root.stateColor(root.stateKey, false)
    }

    Text {
        id: symbolText
        objectName: "parameterSymbol"
        x: 14
        y: 2
        text: root.symbol
        color: root.stateColor(root.stateKey, false)
        font.family: "Inter"
        font.pixelSize: 36
        font.bold: true
        renderType: Text.QtRendering
    }

    Text {
        id: unitText
        objectName: "parameterUnit"
        anchors.right: parent.right
        anchors.rightMargin: 14
        y: 6
        width: 160
        text: root.unit
        color: root.stateColor(root.stateKey, false)
        font.family: "Inter"
        font.pixelSize: 29
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
        renderType: Text.QtRendering
    }

    Text {
        id: valueText
        anchors.horizontalCenter: parent.horizontalCenter
        objectName: "readingValue"
        y: 48
        width: parent.width - 24
        height: 72
        text: root.valueText
        color: root.stateColor(root.stateKey, true)
        font.family: "Inter"
        font.pixelSize: 82
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 32
        renderType: Text.QtRendering
    }

    Text {
        objectName: "measurementRange"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 120
        width: parent.width - 24
        height: 32
        text: root.rangeText.length ? "(" + root.rangeText + ")" : ""
        color: root.stateColor(root.stateKey, false)
        font.family: "Inter"
        font.pixelSize: 29
        minimumPixelSize: 14
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.QtRendering
    }

    LedStatusLabel {
        objectName: "tileStatus"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        height: 36
        accentColor: root.stateColor(root.stateKey, true)
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
        fontSize: 26
        maximumWidth: parent.width - 24
    }
}
