import QtQuick

Item {
    id: root
    width: 456
    height: 238

    property string symbol: ""
    property string unit: ""
    property string valueText: "—"
    property string statusText: "CHƯA CÓ DỮ LIỆU"
    property string stateKey: "noData"
    property string timeText: "—"
    property string detailText: ""

    function accentFor(key) {
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
            return "#FFFFFF"
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        radius: 6
        border.width: 2
        border.color: root.accentFor(root.stateKey)
    }

    Text {
        id: symbolText
        x: 20
        y: 8
        text: root.symbol
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 44
        font.bold: true
        renderType: Text.NativeRendering
    }

    Text {
        id: unitText
        anchors.right: parent.right
        anchors.rightMargin: 20
        y: 20
        width: 180
        text: root.unit
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 30
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight
        renderType: Text.NativeRendering
    }

    Text {
        id: valueText
        anchors.horizontalCenter: parent.horizontalCenter
        y: 62
        width: parent.width - 32
        height: 110
        text: root.valueText
        color: root.accentFor(root.stateKey)
        font.family: "Inter"
        font.pixelSize: 112
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 80
        renderType: Text.NativeRendering
    }

    LedStatusLabel {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - 52
        stateKey: root.stateKey === "withinLimit" || root.stateKey === "exceedsLimit" ? "measuring" : root.stateKey
        text: root.stateKey === "withinLimit" || root.stateKey === "exceedsLimit" ? "ĐANG ĐO" : root.statusText
        fontSize: 30
        maximumWidth: parent.width - 24
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 175
        width: parent.width - 24
        text: "VƯỢT NGƯỠNG"
        visible: root.stateKey === "exceedsLimit"
        color: "#FF7070"
        font.family: "Inter"
        font.pixelSize: 23
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        renderType: Text.NativeRendering
    }
}
