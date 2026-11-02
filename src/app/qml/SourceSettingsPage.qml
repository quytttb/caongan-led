import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    property string statusText: ""
    property bool statusIsError: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    text: "Nguồn dữ liệu"
                    color: AppColors.primaryText
                    font.family: "Inter"
                    font.pixelSize: 26
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: "Ứng dụng chỉ đọc file .txt trong các thư mục bên dưới. Không move, đổi tên, sửa hoặc xóa file nguồn."
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }
            }
            Button {
                text: "Quét lại ngay"
                onClicked: {
                    app.requestRescan()
                    page.statusIsError = false
                    page.statusText = "Đã gửi yêu cầu quét lại. Kết quả sẽ xuất hiện trong tab Chẩn đoán."
                }
            }
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

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 14

                Repeater {
                    model: app.sources

                    delegate: Rectangle {
                        id: sourceCard
                        Layout.fillWidth: true
                        implicitHeight: cardColumn.implicitHeight + 44
                        radius: 8
                        color: AppColors.surfaceContainerLow
                        border.width: 1
                        border.color: AppColors.elevatedBorder

                        property var live: app.sourceStatuses[modelData.id] || ({})
                        property bool online: live.available === true

                        ColumnLayout {
                            id: cardColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 22
                            spacing: 12

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Text {
                                    text: modelData.name
                                    color: AppColors.primaryText
                                    font.family: "Inter"
                                    font.pixelSize: 20
                                    font.bold: true
                                }
                                Text {
                                    text: "• " + modelData.id
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    Layout.preferredWidth: statusRow.implicitWidth + 20
                                    Layout.preferredHeight: 30
                                    radius: 15
                                    color: sourceCard.online
                                           ? AppColors.withAlpha(AppColors.success, 0.14)
                                           : AppColors.withAlpha(AppColors.warning, 0.12)
                                    border.width: 1
                                    border.color: sourceCard.online ? AppColors.success : AppColors.warning
                                    Row {
                                        id: statusRow
                                        anchors.centerIn: parent
                                        spacing: 7
                                        Rectangle {
                                            width: 9
                                            height: 9
                                            radius: 5
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: sourceCard.online ? AppColors.success : AppColors.warning
                                        }
                                        Text {
                                            text: sourceCard.online ? "Sẵn sàng" : "Chưa sẵn sàng"
                                            color: sourceCard.online ? AppColors.success : AppColors.warning
                                            font.family: "Inter"
                                            font.pixelSize: 13
                                            font.bold: true
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: sourceCard.live.message || "Chưa có trạng thái từ bộ đọc thư mục."
                                color: sourceCard.online ? AppColors.textMuted : AppColors.warning
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                columnSpacing: 12
                                rowSpacing: 7

                                Text {
                                    text: "Thư mục nhận file"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                TextField {
                                    id: folderField
                                    Layout.fillWidth: true
                                    Layout.columnSpan: 3
                                    text: modelData.folder
                                    placeholderText: "Ví dụ D:\\CEMS\\KHITO1 (để trống nếu chưa cấu hình)"
                                    selectByMouse: true
                                    font.family: "Inter"
                                    font.pixelSize: 14
                                }

                                Text {
                                    text: "Mã trạm"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                TextField {
                                    id: stationField
                                    Layout.preferredWidth: 240
                                    text: modelData.stationCode
                                    selectByMouse: true
                                    font.family: "Inter"
                                    font.pixelSize: 14
                                }
                                Text {
                                    text: "Stale timeout"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                SpinBox {
                                    id: staleSpin
                                    Layout.preferredWidth: 180
                                    from: 10
                                    to: 86400
                                    stepSize: 60
                                    editable: true
                                    value: modelData.staleSeconds
                                }
                                Text {
                                    text: "giây"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: "Mã trạm được dùng để chọn đúng file; ký hiệu thông số lấy theo cấu hình kênh."
                                    color: AppColors.textFaint
                                    font.family: "Inter"
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                }
                                Button {
                                    text: "Lưu nguồn"
                                    highlighted: true
                                    onClicked: {
                                        const id = modelData.id
                                        const folder = folderField.text
                                        const station = stationField.text
                                        const stale = staleSpin.value
                                        dialog.ask(
                                            "Lưu cấu hình nguồn " + modelData.name,
                                            "Thư mục: " + (folder.length ? folder : "(chưa cấu hình)") +
                                            "\nMã trạm: " + station + "\nStale timeout: " + stale + " giây" +
                                            "\n\nThay đổi này áp dụng cho toàn bộ kênh thuộc nguồn.",
                                            function() {
                                                const error = app.updateSource(id, folder, station, stale)
                                                if (error.length > 0) {
                                                    page.statusIsError = true
                                                    page.statusText = error
                                                } else {
                                                    page.statusIsError = false
                                                    page.statusText = "Đã lưu cấu hình nguồn " + modelData.name + "."
                                                }
                                            })
                                    }
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true; implicitHeight: 8 }
            }
        }
    }

    ActionDialog {
        id: dialog
    }
}
