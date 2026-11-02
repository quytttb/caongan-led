import QtQuick
import QtQuick.Window
import CaonganLed.App

// Khung thiết kế cố định 1920 × 1200. Cửa sổ LED co giãn bằng scale ở
// LedWindow.qml; bố cục bên trong không đổi theo DPI/màn hình.
Item {
    id: root
    width: 1920
    height: 1200
    implicitWidth: 1920
    implicitHeight: 1200

    property string clockText: Qt.formatDateTime(new Date(), "HH:mm:ss")
    property string dateText: Qt.formatDateTime(new Date(), "dd/MM/yyyy")
    property var brand: app.branding

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            const now = new Date()
            root.clockText = Qt.formatDateTime(now, "HH:mm:ss")
            root.dateText = Qt.formatDateTime(now, "dd/MM/yyyy")
        }
    }

    // ---------------------------------------------------------------------
    // Header
    // ---------------------------------------------------------------------
    Rectangle {
        x: 24
        y: 24
        width: 1872
        height: 132
        color: "#000000"
        radius: 6
        border.width: 1
        border.color: "#26323D"

        Image {
            x: 20
            y: 12
            width: 105
            height: 105
            source: "qrc:/branding/factory-logo.jpg"
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: companyText
            x: 140
            y: 16
            width: 1020
            text: root.brand.companyName || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 42
            font.bold: true
            elide: Text.ElideRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 140
            y: 66
            width: 1020
            text: root.brand.branchName || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 25
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 140
            y: 98
            width: 1020
            text: root.brand.address || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 21
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 1340
            y: 10
            width: 506
            text: root.clockText
            color: "#FFFFFF"
            font.family: "monospace"
            font.pixelSize: 52
            font.bold: true
            font.letterSpacing: 1
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 1340
            y: 76
            width: 506
            text: root.dateText
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 32
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

    }

    // ---------------------------------------------------------------------
    // Group headings + captions
    // ---------------------------------------------------------------------
    Rectangle {
        x: 24
        y: 192
        width: 8
        height: 40
        color: "#FF3030"
    }

    Text {
        x: 48
        y: 176
        width: 420
        height: 48
        text: "KHÍ THẢI"
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 38
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    Text {
        x: 480
        y: 176
        width: 1416
        height: 48
        text: app.exhaustCaption
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 34
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideLeft
        renderType: Text.NativeRendering
    }

    Rectangle {
        x: 24
        y: 840
        width: 8
        height: 40
        color: "#00E5FF"
    }

    Text {
        x: 48
        y: 824
        width: 420
        height: 48
        text: "NƯỚC THẢI"
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 38
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    Text {
        x: 480
        y: 824
        width: 1416
        height: 48
        text: app.wastewaterCaption
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 34
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideLeft
        renderType: Text.NativeRendering
    }

    // ---------------------------------------------------------------------
    // 12 ô cố định: 4 cột × 3 hàng
    // ---------------------------------------------------------------------
    Repeater {
        model: ledParameterModel

        delegate: ParameterTile {
            x: 24 + (index % 4) * 472
            y: index < 4 ? 232 : (index < 8 ? 524 : 880)
            width: 456
            height: 280
            visible: index < 12

            symbol: model.symbol
            unit: model.unit
            valueText: model.valueText
            statusText: model.statusText
            stateKey: model.stateKey
            timeText: model.timeText
            detailText: model.detailText
        }
    }

}
