import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import LoggerKit.Theme

ApplicationWindow {
    id: win
    objectName: "operatorWindow"
    title: "Cao Ngạn LED — Vận hành"
    width: 1440
    height: 920
    minimumWidth: 1120
    minimumHeight: 760
    visible: true
    color: AppColors.surface

    Material.theme: AppTheme.materialTheme
    Material.primary: AppTheme.primary
    Material.accent: AppTheme.accent
    Material.background: AppColors.surface
    Material.foreground: AppColors.primaryText

    property int pageIndex: 0
    property string clockText: Qt.formatDateTime(new Date(), "HH:mm:ss")
    property string dateText: Qt.formatDateTime(new Date(), "dd/MM/yyyy")

    onClosing: app.quit()

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            const now = new Date()
            win.clockText = Qt.formatDateTime(now, "HH:mm:ss")
            win.dateText = Qt.formatDateTime(now, "dd/MM/yyyy")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // -----------------------------------------------------------------
        // Header
        // -----------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            color: AppColors.surfaceContainerLow
            border.width: 0

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: AppColors.elevatedBorder
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    radius: 8
                    color: AppColors.accentContainer
                    Text {
                        anchors.centerIn: parent
                        text: "CN"
                        color: AppColors.accentContainerFg
                        font.family: "Inter"
                        font.pixelSize: 17
                        font.bold: true
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: "CAO NGẠN LED"
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 19
                        font.bold: true
                    }
                    Text {
                        text: "Bảng quan trắc khí thải và nước thải"
                        color: AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 12
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 34
                    color: AppColors.elevatedBorder
                }

                Rectangle {
                    Layout.preferredWidth: summaryRow.implicitWidth + 22
                    Layout.preferredHeight: 30
                    radius: 15
                    color: AppColors.withAlpha(AppColors.primaryColor, 0.12)
                    border.width: 1
                    border.color: AppColors.withAlpha(AppColors.primaryColor, 0.55)
                    Row {
                        id: summaryRow
                        anchors.centerIn: parent
                        spacing: 7
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: app.simulationActive ? AppColors.warning : AppColors.primaryColor
                        }
                        Text {
                            text: app.statusSummary
                            color: app.simulationActive ? AppColors.warning : AppColors.primaryColor
                            font.family: "Inter"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }

                Rectangle {
                    visible: app.previewMode
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 30
                    radius: 15
                    color: AppColors.withAlpha(AppColors.info, 0.12)
                    border.width: 1
                    border.color: AppColors.info
                    Text {
                        anchors.centerIn: parent
                        text: "PREVIEW"
                        color: AppColors.info
                        font.family: "Inter"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Rectangle {
                    visible: app.simulationActive
                    Layout.preferredWidth: simulationHeaderText.implicitWidth + 22
                    Layout.preferredHeight: 30
                    radius: 15
                    color: AppColors.withAlpha(AppColors.warning, 0.14)
                    border.width: 1
                    border.color: AppColors.warning
                    Text {
                        id: simulationHeaderText
                        anchors.centerIn: parent
                        text: "MÔ PHỎNG • " + app.simulationScenario
                        color: AppColors.warning
                        font.family: "Inter"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                ColumnLayout {
                    spacing: 0
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: win.clockText
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: "GIỜ HIỆN TẠI • " + win.dateText
                        color: AppColors.textMuted
                        font.family: "Inter"
                        font.pixelSize: 10
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Global warnings (chỉ trong cửa sổ vận hành)
        // -----------------------------------------------------------------
        InlineNotice {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.topMargin: app.configError.length > 0 ? 10 : 0
            visible: app.configError.length > 0
            text: app.configError
            isError: true
        }

        InlineNotice {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.topMargin: app.screenWarning.length > 0 ? 10 : 0
            visible: app.screenWarning.length > 0
            text: app.screenWarning
            isWarning: true
        }

        // -----------------------------------------------------------------
        // Navigation + pages
        // -----------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 224
                color: AppColors.navRail

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: AppColors.elevatedBorder
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 4

                    Text {
                        Layout.leftMargin: 10
                        Layout.topMargin: 4
                        text: "VẬN HÀNH"
                        color: AppColors.textFaint
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    Repeater {
                        model: [
                            "Nguồn dữ liệu",
                            "Ngưỡng & hiển thị",
                            "Màn hình & thương hiệu",
                            "Chẩn đoán"
                        ]

                        delegate: Button {
                            id: navButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            flat: true
                            property bool active: win.pageIndex === index
                            text: modelData
                            leftPadding: 12
                            rightPadding: 12

                            background: Rectangle {
                                radius: 7
                                color: navButton.active
                                       ? AppColors.withAlpha(AppColors.primaryColor, 0.16)
                                       : (navButton.hovered
                                          ? AppColors.hoverFill
                                          : "transparent")
                                Rectangle {
                                    visible: navButton.active
                                    width: 3
                                    height: parent.height - 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    radius: 2
                                    color: AppColors.primaryColor
                                }
                            }

                            contentItem: Text {
                                text: navButton.text
                                color: navButton.active ? AppColors.primaryText : AppColors.textSoft
                                font.family: "Inter"
                                font.pixelSize: 14
                                font.bold: navButton.active
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 10
                            }

                            onClicked: win.pageIndex = index
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: AppColors.elevatedBorder
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.margins: 8
                        text: "Nguồn file chỉ đọc.\nKhông kết nối trực tiếp cảm biến/Modbus.\nKhông tự đổi giờ Windows."
                        color: AppColors.textFaint
                        font.family: "Inter"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        lineHeight: 1.3
                    }
                }
            }

            StackLayout {
                id: stack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.pageIndex

                SourceSettingsPage {}
                ChannelSettingsPage {}
                DisplaySettingsPage {}
                DiagnosticsPage {}
            }
        }

        // -----------------------------------------------------------------
        // Footer
        // -----------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: AppColors.surfaceContainerLow
            border.width: 0

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: AppColors.elevatedBorder
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 12

                Text {
                    text: "Cấu hình: " + app.configPath
                    color: AppColors.textFaint
                    font.family: "Inter"
                    font.pixelSize: 10
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                Text {
                    text: app.targetScreenName.length > 0 ? ("LED: " + app.targetScreenName) : "LED: chưa chọn output"
                    color: app.targetScreenName.length > 0 ? AppColors.textMuted : AppColors.warning
                    font.family: "Inter"
                    font.pixelSize: 10
                }
                Text {
                    text: "v" + app.appVersion
                    color: AppColors.textFaint
                    font.family: "Inter"
                    font.pixelSize: 10
                }
            }
        }
    }
}
