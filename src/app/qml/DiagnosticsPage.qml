import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    property string statusText: ""
    property bool statusIsError: false

    function stateColor(key) {
        switch (key) {
        case "measuring":
        case "withinLimit":
            return "#00E5FF"
        case "exceedsLimit":
        case "deviceFault":
        case "conflict":
            return AppColors.error
        case "calibrating":
        case "maintenance":
        case "unitMismatch":
            return AppColors.warning
        case "stale":
        case "futureTimestamp":
        case "invalid":
        case "unknownStatus":
            return AppColors.textMuted
        default:
            return AppColors.textFaint
        }
    }

    function severityColor(value) {
        switch (value) {
        case "error":
            return AppColors.error
        case "warning":
            return AppColors.warning
        case "info":
            return AppColors.info
        default:
            return AppColors.textMuted
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    text: "Chẩn đoán"
                    color: AppColors.primaryText
                    font.family: "Inter"
                    font.pixelSize: 26
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: "Theo dõi nguồn, tuổi dữ liệu từng kênh và lỗi nhập file. Không mở thêm API mạng cho chẩn đoán."
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }

            Button {
                text: "Quét lại"
                onClicked: {
                    app.requestRescan()
                    page.statusIsError = false
                    page.statusText = "Đã gửi yêu cầu quét lại."
                }
            }
            Button {
                text: "Tải lại cấu hình"
                onClicked: {
                    const message = app.reloadConfiguration()
                    page.statusIsError = message.length > 0
                    page.statusText = message.length > 0 ? message : "Đã tải lại cấu hình từ đĩa."
                }
            }
            Button {
                text: "Xóa sự kiện"
                onClicked: app.clearDiagnostics()
            }
        }

        InlineNotice {
            Layout.fillWidth: true
            text: page.statusText
            isError: page.statusIsError
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 640
                implicitHeight: sourceStatusColumn.implicitHeight + 32
                radius: 8
                color: AppColors.surfaceContainerLow
                border.width: 1
                border.color: AppColors.elevatedBorder

                ColumnLayout {
                    id: sourceStatusColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 8

                    Text {
                        text: "Nguồn dữ liệu"
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 17
                        font.bold: true
                    }

                    Repeater {
                        model: app.sources

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            property var live: app.sourceStatuses[modelData.id] || ({})

                            Rectangle {
                                Layout.preferredWidth: 9
                                Layout.preferredHeight: 9
                                radius: 5
                                color: parent.live.available === true ? AppColors.success : AppColors.warning
                            }
                            Text {
                                text: modelData.name
                                color: AppColors.primaryText
                                font.family: "Inter"
                                font.pixelSize: 14
                                font.bold: true
                                Layout.preferredWidth: 90
                            }
                            Text {
                                Layout.fillWidth: true
                                text: parent.live.message || "Chưa có trạng thái"
                                color: parent.live.available === true ? AppColors.textMuted : AppColors.warning
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 500
                implicitHeight: pathColumn.implicitHeight + 32
                radius: 8
                color: AppColors.surfaceContainerLow
                border.width: 1
                border.color: AppColors.elevatedBorder

                ColumnLayout {
                    id: pathColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 5

                    Text {
                        text: "Vị trí lưu trữ"
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 17
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Cấu hình: " + app.configPath
                        color: AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Log: " + app.logDir
                        color: AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Snapshot: " + app.databasePath
                        color: AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Mở log"
                            onClicked: app.openLogFolder()
                        }
                        Button {
                            text: "Mở dữ liệu"
                            onClicked: app.openDataFolder()
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "v" + app.appVersion
                            color: AppColors.textFaint
                            font.family: "Inter"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 240
            radius: 8
            color: AppColors.surfaceContainerLow
            border.width: 1
            border.color: AppColors.elevatedBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Trạng thái 12 kênh"
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 17
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: app.statusSummary
                        color: app.simulationActive ? AppColors.warning : AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 12
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: AppColors.surfaceContainer
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8
                        Text { Layout.preferredWidth: 100; text: "Kênh"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 110; text: "Giá trị"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 210; text: "Trạng thái"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 140; text: "Mốc đo"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.fillWidth: true; text: "Chi tiết"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                    }
                }

                ListView {
                    id: channelList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ledParameterModel
                    clip: true
                    ScrollBar.vertical: ScrollBar {}

                    delegate: Item {
                        width: channelList.width
                        height: 31

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8
                            Text {
                                Layout.preferredWidth: 100
                                text: model.symbol
                                color: AppColors.primaryText
                                font.family: "Inter"
                                font.pixelSize: 12
                                font.bold: true
                            }
                            Text {
                                Layout.preferredWidth: 110
                                text: model.valueText + " " + model.unit
                                color: page.stateColor(model.stateKey)
                                font.family: "Inter"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.preferredWidth: 210
                                text: model.statusText
                                color: page.stateColor(model.stateKey)
                                font.family: "Inter"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.preferredWidth: 140
                                text: model.timeText
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 11
                            }
                            Text {
                                Layout.fillWidth: true
                                text: model.detailText
                                color: AppColors.textFaint
                                font.family: "Inter"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: AppColors.surfaceContainerLow
            border.width: 1
            border.color: AppColors.elevatedBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                Text {
                    text: "Sự kiện gần đây"
                    color: AppColors.primaryText
                    font.family: "Inter"
                    font.pixelSize: 17
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: AppColors.surfaceContainer
                    radius: 4
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8
                        Text { Layout.preferredWidth: 125; text: "Thời gian"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 76; text: "Mức"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 95; text: "Nguồn"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 95; text: "Kênh"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.preferredWidth: 180; text: "Mã"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                        Text { Layout.fillWidth: true; text: "Nội dung"; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                    }
                }

                ListView {
                    id: eventList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: diagnosticsModel
                    clip: true
                    ScrollBar.vertical: ScrollBar {}

                    delegate: Item {
                        width: eventList.width
                        height: 30
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8
                            Text { Layout.preferredWidth: 125; text: model.timeText; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter" }
                            Text { Layout.preferredWidth: 76; text: model.severity; color: page.severityColor(model.severity); font.pixelSize: 11; font.family: "Inter"; font.bold: true }
                            Text { Layout.preferredWidth: 95; text: model.sourceId; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 95; text: model.channelId; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 180; text: model.code; color: AppColors.textMuted; font.pixelSize: 11; font.family: "Inter"; elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; text: model.message; color: AppColors.primaryText; font.pixelSize: 11; font.family: "Inter"; elide: Text.ElideRight }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: eventList.count === 0
                    text: "Chưa có sự kiện."
                    color: AppColors.textFaint
                    font.family: "Inter"
                    font.pixelSize: 12
                }
            }
        }
    }
}
