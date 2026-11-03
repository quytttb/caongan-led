import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "Ngưỡng và dải đo (cố định)"
                color: AppColors.primaryText
                font.family: "Inter"
                font.pixelSize: 26
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "Ngưỡng và dải đo đã cố định theo nhà máy duyệt, không chỉnh sửa trên màn hình này."
                color: AppColors.textMuted
                font.family: "Inter"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: AppColors.surfaceContainer
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12 + 8
                spacing: 10

                Text {
                    Layout.preferredWidth: 150
                    text: "Thông số"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 90
                    text: "Đơn vị"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 120
                    text: "Nguồn"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 70
                    text: "Thập phân"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 190
                    text: "Ngưỡng"
                    color: AppColors.textMuted
                    font.family: "Inter"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    Layout.preferredWidth: 190
                    text: "Dải đo"
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
                        Layout.fillWidth: true
                        implicitHeight: 52
                        radius: 7
                        color: AppColors.surfaceContainerLow
                        border.width: 1
                        border.color: AppColors.elevatedBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            ColumnLayout {
                                Layout.preferredWidth: 150
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
                            }

                            Text {
                                Layout.preferredWidth: 90
                                text: modelData.unit
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                Layout.preferredWidth: 120
                                text: modelData.sourceName
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.preferredWidth: 70
                                text: modelData.decimals
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Text {
                                Layout.preferredWidth: 190
                                text: (modelData.thresholdLow.length ? modelData.thresholdLow : "—")
                                      + " – " + (modelData.thresholdHigh.length ? modelData.thresholdHigh : "—")
                                color: AppColors.primaryText
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.preferredWidth: 190
                                text: modelData.rangeText
                                color: AppColors.textSoft
                                font.family: "Inter"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                Item { implicitHeight: 8 }
            }
        }
    }
}
