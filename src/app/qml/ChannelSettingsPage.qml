import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    // Header và các dòng dùng cùng component, cùng chiều rộng vùng cuộn.
    component ChannelRow: Rectangle {
        id: row
        property bool heading: false
        property string symbol: ""
        property string unit: ""
        property string sourceName: ""
        property string rangeText: ""
        property string thresholdText: ""
        implicitHeight: heading ? 38 : 52
        radius: 5
        color: heading ? AppColors.surfaceContainer : AppColors.surfaceContainerLow
        border.width: heading ? 0 : 1
        border.color: AppColors.elevatedBorder

        Row {
            id: cells
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            Repeater {
                model: [row.symbol, row.unit, row.sourceName, row.rangeText, row.thresholdText]
                delegate: Text {
                    required property int index
                    required property var modelData
                    objectName: "channelCell_" + index
                    width: cells.width * ([0.16, 0.14, 0.17, 0.27, 0.26][index])
                    height: cells.height
                    text: modelData
                    color: row.heading ? AppColors.textMuted : AppColors.primaryText
                    font.family: "Inter"
                    font.pixelSize: row.heading ? 12 : 14
                    font.bold: row.heading || index === 0
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        Text {
            text: "Ngưỡng và dải đo (cố định)"
            color: AppColors.primaryText
            font.family: "Inter"
            font.pixelSize: 26
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            text: "Dải đo và ngưỡng do nhà máy cung cấp; ngưỡng được so trên giá trị chưa làm tròn."
            color: AppColors.textMuted
            font.family: "Inter"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                width: scroll.availableWidth
                spacing: 6
                ChannelRow {
                    objectName: "channelHeader"
                    width: parent.width
                    heading: true
                    symbol: "Thông số"
                    unit: "Đơn vị"
                    sourceName: "Nguồn"
                    rangeText: "Dải đo"
                    thresholdText: "Ngưỡng"
                }
                Repeater {
                    model: app.channelSettings
                    delegate: ChannelRow {
                        required property var modelData
                        objectName: "channelRow_" + modelData.id
                        width: scroll.availableWidth
                        symbol: modelData.symbol
                        unit: modelData.unit
                        sourceName: modelData.sourceName
                        rangeText: modelData.rangeText
                        thresholdText: (modelData.thresholdLow || "—") + " – "
                                       + (modelData.thresholdHigh || "—")
                    }
                }
            }
        }
    }
}
