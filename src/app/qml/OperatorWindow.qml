import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import LoggerKit.Theme

ApplicationWindow {
    id: win
    objectName: "operatorWindow"
    title: (app.branding.companyName || "Cao Ngạn") + " — Vận hành"
    width: Math.max(minimumWidth, Math.min(1440, screen ? screen.desktopAvailableWidth : 1440))
    height: Math.max(minimumHeight, Math.min(920, screen ? screen.desktopAvailableHeight : 920))
    minimumWidth: 1120
    minimumHeight: 760
    visible: true
    color: AppColors.surface
    // Trên Windows, native event filter chỉ bỏ non-client area của cửa sổ
    // này, giữ lại resize/snap của hệ điều hành. Các nền tảng khác dùng Qt.
    flags: Qt.Window | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint
           | Qt.WindowMaximizeButtonHint | Qt.WindowCloseButtonHint
           | (Qt.platform.os === "windows" ? 0 : Qt.FramelessWindowHint)

    Material.theme: AppTheme.materialTheme
    Material.primary: AppTheme.primary
    Material.accent: AppTheme.accent
    Material.background: AppColors.surface
    Material.foreground: AppColors.primaryText

    property int pageIndex: 0

    onClosing: app.quit()

    function toggleMaximize() {
        if (visibility === Window.Maximized)
            showNormal()
        else
            showMaximized()
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

            MouseArea {
                objectName: "operatorTitleBarDragArea"
                anchors.fill: parent
                onPressed: mouse => {
                    if (mouse.button === Qt.LeftButton)
                        win.startSystemMove()
                }
                onDoubleClicked: mouse => {
                    if (mouse.button === Qt.LeftButton)
                        win.toggleMaximize()
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 12
                spacing: 14

                Image {
                    objectName: "operatorLogo"
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    source: "qrc:/branding/factory-logo.jpg"
                    fillMode: Image.PreserveAspectFit
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 120
                    spacing: 1
                    Text {
                        Layout.fillWidth: true
                        text: app.branding.companyName || "Cao Ngạn"
                        color: AppColors.primaryText
                        font.family: "Inter"
                        font.pixelSize: 19
                        font.bold: true
                        elide: Text.ElideRight
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

                RowLayout {
                    spacing: 2

                    WindowChromeButton {
                        objectName: "operatorMinimizeButton"
                        kind: "minimize"
                        description: "Thu nhỏ"
                        onClicked: win.showMinimized()
                    }
                    WindowChromeButton {
                        objectName: "operatorMaximizeButton"
                        kind: "maximize"
                        maximized: win.visibility === Window.Maximized
                        description: maximized ? "Khôi phục kích thước" : "Phóng to"
                        onClicked: win.toggleMaximize()
                    }
                    WindowChromeButton {
                        objectName: "operatorCloseButton"
                        kind: "close"
                        description: "Đóng ứng dụng"
                        onClicked: win.close()
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
                            "Ngưỡng & dải đo",
                            "Màn hình LED",
                            "Chẩn đoán",
                            "Nhật ký"
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
                LogPage {}
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

    // Frameless trên X11/Wayland không có viền resize mặc định. Chỉ đặt
    // vùng bắt chuột ở mép; phần còn lại của UI và popup vẫn dùng bình thường.
    Item {
        anchors.fill: parent
        z: 1000
        visible: Qt.platform.os !== "windows" && win.visibility === Window.Windowed

        component ResizeHandle: MouseArea {
            property int edge: 0
            hoverEnabled: true
            cursorShape: {
                if (edge === Qt.TopEdge || edge === Qt.BottomEdge)
                    return Qt.SizeVerCursor
                if (edge === Qt.LeftEdge || edge === Qt.RightEdge)
                    return Qt.SizeHorCursor
                if (edge === (Qt.TopEdge | Qt.LeftEdge) || edge === (Qt.BottomEdge | Qt.RightEdge))
                    return Qt.SizeFDiagCursor
                return Qt.SizeBDiagCursor
            }
            onPressed: mouse => {
                if (mouse.button === Qt.LeftButton)
                    win.startSystemResize(edge)
            }
        }

        ResizeHandle { x: 8; y: 0; width: parent.width - 16; height: 6; edge: Qt.TopEdge }
        ResizeHandle { x: 8; y: parent.height - 6; width: parent.width - 16; height: 6; edge: Qt.BottomEdge }
        ResizeHandle { x: 0; y: 8; width: 6; height: parent.height - 16; edge: Qt.LeftEdge }
        ResizeHandle { x: parent.width - 6; y: 8; width: 6; height: parent.height - 16; edge: Qt.RightEdge }
        ResizeHandle { x: 0; y: 0; width: 8; height: 8; edge: Qt.TopEdge | Qt.LeftEdge }
        ResizeHandle { x: parent.width - 8; y: 0; width: 8; height: 8; edge: Qt.TopEdge | Qt.RightEdge }
        ResizeHandle { x: 0; y: parent.height - 8; width: 8; height: 8; edge: Qt.BottomEdge | Qt.LeftEdge }
        ResizeHandle { x: parent.width - 8; y: parent.height - 8; width: 8; height: 8; edge: Qt.BottomEdge | Qt.RightEdge }
    }
}
