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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "Ngưỡng, đơn vị và độ chính xác"
                color: AppColors.primaryText
                font.family: "Inter"
                font.pixelSize: 26
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "Thứ tự 12 kênh cố định. Ngưỡng là khoảng cho phép: giá trị nhỏ hơn cận dưới hoặc lớn hơn cận trên bị coi là vượt ngưỡng. Chỉ bật ngưỡng khi đã có căn cứ được duyệt."
                color: AppColors.textMuted
                font.family: "Inter"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                text: "Nhập số thập phân bằng dấu chấm (ví dụ 7.5). Temp/Flow/P/O₂ là chỉ tiêu vận hành: không tự áp ngưỡng nếu chưa có yêu cầu."
                color: AppColors.warning
                font.family: "Inter"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
        }

        InlineNotice {
            Layout.fillWidth: true
            text: page.statusText
            isError: page.statusIsError
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: AppColors.surfaceContainer
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Text {
                    Layout.preferredWidth: 190
                    text: "Thông số"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 100
                    text: "Đơn vị"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 110
                    text: "Nguồn"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 100
                    text: "Thập phân"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 92
                    text: "Ngưỡng"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 118
                    text: "Cận dưới"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 118
                    text: "Cận trên"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
            }
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
                spacing: 8

                Repeater {
                    model: app.channelSettings

                    delegate: Rectangle {
                        id: channelCard
                        Layout.fillWidth: true
                        implicitHeight: 76
                        radius: 7
                        color: AppColors.surfaceContainerLow
                        border.width: 1
                        border.color: AppColors.elevatedBorder

                        property bool operational: ["o2", "temp_flue", "flow_flue", "pressure_flue"].indexOf(modelData.id) >= 0

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            ColumnLayout {
                                Layout.preferredWidth: 190
                                spacing: 0
                                Text {
                                    text: modelData.symbol
                                    color: AppColors.primaryText
                                    font.family: "Inter"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.displayName
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: channelCard.operational ? "Chỉ tiêu vận hành" : modelData.groupTitle
                                    color: channelCard.operational ? AppColors.warning : AppColors.textFaint
                                    font.family: "Inter"
                                    font.pixelSize: 11
                                }
                            }

                            Text {
                                Layout.preferredWidth: 100
                                text: modelData.unit
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                Layout.preferredWidth: 110
                                text: modelData.sourceName
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            SpinBox {
                                id: decimalsSpin
                                Layout.preferredWidth: 100
                                from: 0
                                to: 6
                                editable: true
                                value: modelData.decimals
                            }

                            Switch {
                                id: thresholdSwitch
                                Layout.preferredWidth: 92
                                text: checked ? "Bật" : "Tắt"
                                checked: modelData.thresholdEnabled
                                font.family: "Inter"
                                font.pixelSize: 12
                            }

                            TextField {
                                id: lowField
                                Layout.preferredWidth: 118
                                enabled: thresholdSwitch.checked
                                text: modelData.lowText
                                placeholderText: "—"
                                selectByMouse: true
                                font.family: "Inter"
                                font.pixelSize: 13
                            }

                            TextField {
                                id: highField
                                Layout.preferredWidth: 118
                                enabled: thresholdSwitch.checked
                                text: modelData.highText
                                placeholderText: "—"
                                selectByMouse: true
                                font.family: "Inter"
                                font.pixelSize: 13
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 74
                                text: "Lưu"
                                highlighted: true
                                onClicked: {
                                    const id = modelData.id
                                    const decimals = decimalsSpin.value
                                    const enabled = thresholdSwitch.checked
                                    const low = lowField.text
                                    const high = highField.text
                                    dialog.ask(
                                        "Lưu " + modelData.symbol,
                                        "Độ chính xác: " + decimals + " chữ số thập phân"
                                        + "\nNgưỡng: " + (enabled ? ("bật (cận dưới " + (low.length ? low : "—") + ", cận trên " + (high.length ? high : "—") + ")") : "tắt")
                                        + "\n\nThay đổi được áp dụng ngay cho dữ liệu đã nhập.",
                                        function() {
                                            const error = app.updateChannelSettings(id, decimals, enabled, low, high)
                                            if (error.length > 0) {
                                                page.statusIsError = true
                                                page.statusText = error
                                            } else {
                                                page.statusIsError = false
                                                page.statusText = "Đã lưu cấu hình cho " + modelData.symbol + "."
                                            }
                                        })
                                }
                            }
                        }
                    }
                }

                Item { implicitHeight: 8 }
            }
        }
    }

    ActionDialog {
        id: dialog
    }
}
