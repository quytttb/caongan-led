import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page
    property string statusText: ""
    property bool statusIsError: false

    // Header và các dòng dùng cùng component, cùng chiều rộng vùng cuộn.
    component ChannelRow: Rectangle {
        id: row
        property bool heading: false
        property string symbol: ""
        property string unit: ""
        property string sourceName: ""
        property string rangeText: ""
        property string thresholdText: ""
        property string channelId: ""
        signal editRequested()
        implicitHeight: heading ? 38 : 52
        radius: 5
        color: heading ? AppColors.surfaceContainer : AppColors.surfaceContainerLow
        border.width: heading ? 0 : 1
        border.color: AppColors.elevatedBorder

        Row {
            id: cells
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 94

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

        Text {
            visible: row.heading
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: "Thao tác"
            color: AppColors.textMuted
            font.family: "Inter"
            font.pixelSize: 12
            font.bold: true
        }

        Button {
            objectName: "editThreshold_" + row.channelId
            visible: !row.heading
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 76
            height: 38
            leftPadding: 8
            rightPadding: 8
            text: "Sửa"
            onClicked: row.editRequested()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        Text {
            text: "Ngưỡng cảnh báo và dải đo"
            color: AppColors.primaryText
            font.family: "Inter"
            font.pixelSize: 26
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            text: "Dải đo chỉ để tham khảo. Ngưỡng ban đầu lấy từ máy cũ và cần nhà máy xác nhận; so ngưỡng trên giá trị chưa làm tròn."
            color: AppColors.textMuted
            font.family: "Inter"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }
        InlineNotice {
            Layout.fillWidth: true
            text: page.statusText
            isError: page.statusIsError
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
                        channelId: modelData.id
                        thresholdText: (modelData.thresholdLow || "—") + " – "
                                       + (modelData.thresholdHigh || "—")
                        onEditRequested: thresholdEditor.edit(modelData)
                    }
                }
            }
        }
    }

    Dialog {
        id: thresholdEditor
        objectName: "thresholdEditor"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(560, parent ? parent.width - 48 : 560)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        title: "Sửa ngưỡng cảnh báo"
        background: Rectangle {
            color: AppColors.surfaceContainerLow
            radius: 8
            border.color: AppColors.elevatedBorder
        }
        Overlay.modal: Rectangle { color: "#99000000" }

        property string channelId: ""
        property string channelSymbol: ""
        property string channelUnit: ""
        property string previousLow: ""
        property string previousHigh: ""
        property string errorText: ""

        function edit(channel) {
            channelId = channel.id
            channelSymbol = channel.symbol
            channelUnit = channel.unit
            previousLow = channel.thresholdLow
            previousHigh = channel.thresholdHigh
            lowInput.text = previousLow
            highInput.text = previousHigh
            errorText = ""
            open()
        }

        contentItem: ColumnLayout {
            spacing: 12
            Text {
                Layout.fillWidth: true
                text: thresholdEditor.channelSymbol + " (" + thresholdEditor.channelUnit + ")"
                color: AppColors.primaryText
                font.family: "Inter"
                font.pixelSize: 17
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "Giá trị bằng ngưỡng vẫn trong giới hạn. Dùng dấu chấm cho phần thập phân."
                color: AppColors.textMuted
                font.family: "Inter"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 10
                Label { text: "Ngưỡng dưới" }
                TextField {
                    id: lowInput
                    objectName: "thresholdLowInput"
                    Layout.fillWidth: true
                    selectByMouse: true
                }
                Label { text: "Ngưỡng trên" }
                TextField {
                    id: highInput
                    objectName: "thresholdHighInput"
                    Layout.fillWidth: true
                    selectByMouse: true
                }
            }
            Text {
                objectName: "thresholdEditorError"
                Layout.fillWidth: true
                visible: text.length > 0
                text: thresholdEditor.errorText
                color: AppColors.error
                font.family: "Inter"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
        }

        footer: DialogButtonBox {
            Button {
                objectName: "cancelThresholdButton"
                text: "Hủy"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                objectName: "saveThresholdButton"
                text: "Lưu ngưỡng"
                highlighted: true
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: {
                    const id = thresholdEditor.channelId
                    const low = lowInput.text.trim()
                    const high = highInput.text.trim()
                    const error = app.validateThreshold(id, low, high)
                    thresholdEditor.errorText = error
                    if (error.length > 0)
                        return
                    confirmThreshold.ask(
                        "Xác nhận đổi ngưỡng",
                        thresholdEditor.channelSymbol + " (" + thresholdEditor.channelUnit + ")" +
                        "\nNgưỡng cũ: " + thresholdEditor.previousLow + " – " + thresholdEditor.previousHigh +
                        "\nNgưỡng mới: " + low + " – " + high +
                        "\n\nNgưỡng mới sẽ áp dụng cho giá trị đang hiển thị.",
                        function() {
                            const saveError = app.updateThreshold(id, low, high)
                            if (saveError.length > 0) {
                                thresholdEditor.errorText = saveError
                            } else {
                                thresholdEditor.close()
                                page.statusIsError = false
                                page.statusText = "Đã lưu và áp dụng ngưỡng cảnh báo."
                            }
                        })
                }
            }
        }
    }

    ActionDialog {
        id: confirmThreshold
        objectName: "confirmThreshold"
    }
}
