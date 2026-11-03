import QtQuick
import QtQuick.Window

// Bố cục 1600 × 960 theo tỉ lệ màn vật lý 1600 × 960 mm.
// Đây là tọa độ logic, không tự đặt độ phân giải tín hiệu của Windows.
Item {
    id: root
    objectName: "ledBoard"
    width: 1600
    height: 960
    implicitWidth: 1600
    implicitHeight: 960

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
        x: 20
        y: 16
        width: 1560
        height: 108
        color: "#000000"
        radius: 6
        border.width: 1
        border.color: "#26323D"

        Image {
            x: 12
            y: 9
            width: 90
            height: 90
            source: "qrc:/branding/factory-logo.jpg"
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: companyText
            x: 118
            y: 8
            width: 1000
            text: root.brand.companyName || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 34
            font.bold: true
            elide: Text.ElideRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 118
            y: 51
            width: 1000
            text: root.brand.branchName || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 21
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 118
            y: 79
            width: 1000
            text: root.brand.address || ""
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 18
            elide: Text.ElideRight
            visible: text.length > 0
            renderType: Text.NativeRendering
        }

        Text {
            x: 1120
            y: 6
            width: 420
            text: root.clockText
            color: "#FFFFFF"
            font.family: "monospace"
            font.pixelSize: 44
            font.bold: true
            font.letterSpacing: 1
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

        Text {
            x: 1120
            y: 62
            width: 420
            text: root.dateText
            color: "#FFFFFF"
            font.family: "Inter"
            font.pixelSize: 28
            horizontalAlignment: Text.AlignRight
            renderType: Text.NativeRendering
        }

    }

    // ---------------------------------------------------------------------
    // Group headings + captions
    // ---------------------------------------------------------------------
    Rectangle {
        x: 20
        y: 136
        width: 7
        height: 30
        color: "#FF3030"
    }

    Text {
        x: 40
        y: 130
        width: 360
        height: 36
        text: "KHÍ THẢI"
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 30
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    Text {
        x: 410
        y: 130
        width: 1170
        height: 36
        text: app.exhaustCaption
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 27
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideLeft
        renderType: Text.NativeRendering
    }

    Rectangle {
        x: 20
        y: 578
        width: 7
        height: 30
        color: "#00E5FF"
    }

    Text {
        x: 40
        y: 572
        width: 360
        height: 36
        text: "NƯỚC THẢI"
        color: "#FFFFFF"
        font.family: "Inter"
        font.pixelSize: 30
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    Text {
        x: 410
        y: 572
        width: 1170
        height: 36
        text: app.wastewaterCaption
        color: "#FFFFFF"
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
            objectName: "parameterTile_" + index
            x: 20 + (index % 4) * 393
            y: index < 4 ? 170 : (index < 8 ? 374 : 614)
            width: 381
            height: 195
            visible: index < 12

            symbol: model.symbol
            unit: model.unit
            rangeText: model.rangeText
            valueText: model.valueText
            statusText: model.statusText
            stateKey: model.stateKey
            timeText: model.timeText
            detailText: model.detailText
        }
    }

    component LegendBadge: Rectangle {
        property string message: ""
        property color accent: "white"
        width: 1560
        height: 48
        x: 20
        color: "#000000"
        radius: 5
        border.width: 2
        border.color: accent

        Text {
            anchors.fill: parent
            anchors.margins: 4
            text: parent.message
            color: parent.accent
            font.family: "Inter"
            font.bold: true
            font.pixelSize: 28
            minimumPixelSize: 20
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
        }
    }

    LegendBadge {
        objectName: "legendWithin"
        y: 821
        accent: "#00E5FF"
        message: "MÀU XANH: NẰM TRONG GIỚI HẠN CHO PHÉP"
    }

    LegendBadge {
        objectName: "legendExceeds"
        y: 876
        accent: "#FF5252"
        message: "MÀU ĐỎ: VƯỢT NGƯỠNG GIỚI HẠN CHO PHÉP"
    }

    Text {
        objectName: "simulationNotice"
        visible: app.simulationActive
        anchors.horizontalCenter: parent.horizontalCenter
        y: 929
        text: "DỮ LIỆU MÔ PHỎNG"
        color: "#FFD60A"
        font.family: "Inter"
        font.pixelSize: 20
        font.bold: true
        renderType: Text.NativeRendering
    }
}
