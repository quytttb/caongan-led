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
        color: "#030506"
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
        color: "#080D12"
        radius: 6
        border.width: 1
        border.color: "#26323D"

        Text {
            id: companyText
            x: 26
            y: 16
            width: 1240
            text: root.brand.companyName || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 42
            font.bold: true
            elide: Text.ElideRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 26
            y: 66
            width: 1240
            text: root.brand.branchName || ""
            color: "#C6D0D9"
            font.family: "Inter"
            font.pixelSize: 25
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 26
            y: 98
            width: 1240
            text: root.brand.address || ""
            color: "#8593A0"
            font.family: "Inter"
            font.pixelSize: 21
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 1340
            y: 16
            width: 506
            text: "GIỜ HIỆN TẠI"
            color: "#8E9CA9"
            font.family: "Inter"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 1340
            y: 42
            width: 506
            text: root.clockText
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 52
            font.bold: true
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 1340
            y: 100
            width: 506
            text: root.dateText
            color: "#AEB8C2"
            font.family: "Inter"
            font.pixelSize: 23
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

        Rectangle {
            x: 1030
            y: 80
            width: 286
            height: 38
            radius: 4
            visible: app.simulationActive
            color: "#3A2C00"
            border.width: 1
            border.color: "#FFD60A"

            Text {
                anchors.centerIn: parent
                text: "MÔ PHỎNG • " + app.simulationScenario
                color: "#FFD60A"
                font.family: "Inter"
                font.pixelSize: 20
                font.bold: true
                renderType: Text.NativeRendering
            }
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
        color: "#00E5FF"
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
        color: "#96A3AF"
        font.family: "Inter"
        font.pixelSize: 27
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideLeft
        renderType: Text.NativeRendering
    }

    Rectangle {
        x: 24
        y: 756
        width: 8
        height: 40
        color: "#00E5FF"
    }

    Text {
        x: 48
        y: 740
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
        y: 740
        width: 1416
        height: 48
        text: app.wastewaterCaption
        color: "#96A3AF"
        font.family: "Inter"
        font.pixelSize: 27
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
            y: index < 4 ? 232 : (index < 8 ? 482 : 796)
            width: 456
            height: 238
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

    // ---------------------------------------------------------------------
    // Footer
    // ---------------------------------------------------------------------
    Rectangle {
        x: 24
        y: 1050
        width: 1872
        height: 126
        color: "#080D12"
        radius: 6
        border.width: 1
        border.color: "#26323D"

        Text {
            x: 24
            y: 14
            text: "CHÚ GIẢI TRẠNG THÁI"
            color: "#7D8B98"
            font.family: "Inter"
            font.pixelSize: 20
            font.bold: true
            renderType: Text.NativeRendering
        }

        Row {
            x: 24
            y: 52
            spacing: 30

            Repeater {
                model: [
                    { label: "ĐANG ĐO", color: "#00E5FF" },
                    { label: "TRONG GIỚI HẠN", color: "#00E5FF" },
                    { label: "VƯỢT NGƯỠNG", color: "#FF5252" },
                    { label: "HIỆU CHUẨN", color: "#FFD60A" },
                    { label: "LỖI / KHÔNG HỢP LỆ", color: "#D0D0D0" }
                ]

                Row {
                    spacing: 10
                    Rectangle {
                        width: 18
                        height: 18
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: modelData.color
                    }
                    Text {
                        text: modelData.label
                        color: "#D6DEE5"
                        font.family: "Inter"
                        font.pixelSize: 22
                        renderType: Text.NativeRendering
                    }
                }
            }
        }

        Column {
            x: 1060
            y: 16
            width: 785
            spacing: 6

            Text {
                width: parent.width
                text: root.brand.regulationText || ""
                color: "#D6DEE5"
                font.family: "Inter"
                font.pixelSize: 21
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                visible: text.length > 0
                renderType: Text.NativeRendering
            }

            Text {
                width: parent.width
                text: root.brand.footerNote || ""
                color: "#8E9CA9"
                font.family: "Inter"
                font.pixelSize: 19
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                visible: text.length > 0
                renderType: Text.NativeRendering
            }
        }
    }
}
