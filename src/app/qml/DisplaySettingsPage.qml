import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    property string statusText: ""
    property bool statusIsError: false

    function syncScreenSelection() {
        useLedCheck.checked = app.targetScreenName.length > 0
        let found = -1
        for (let i = 0; i < app.screens.length; ++i) {
            if (app.screens[i].name === app.targetScreenName) {
                found = i
                break
            }
        }
        screenCombo.currentIndex = found
    }

    onVisibleChanged: {
        if (visible) {
            syncScreenSelection()
        }
    }

    Component.onCompleted: {
        syncScreenSelection()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: "Màn hình LED"
                color: AppColors.primaryText
                font.family: "Inter"
                font.pixelSize: 26
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "Chọn đúng output LED đã kết nối. Cửa sổ vận hành không bao giờ tự dời sang màn hình công khai khi output biến mất."
                color: AppColors.textMuted
                font.family: "Inter"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }

        InlineNotice {
            Layout.fillWidth: true
            text: page.statusText
            isError: page.statusIsError
        }

        InlineNotice {
            Layout.fillWidth: true
            visible: app.screenWarning.length > 0
            text: app.screenWarning
            isWarning: true
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

                // ---------------------------------------------------------
                // Màn hình đích
                // ---------------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: screenColumn.implicitHeight + 40
                    radius: 8
                    color: AppColors.surfaceContainerLow
                    border.width: 1
                    border.color: AppColors.elevatedBorder

                    ColumnLayout {
                        id: screenColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 10

                        Text {
                            text: "Output LED"
                            color: AppColors.primaryText
                            font.family: "Inter"
                            font.pixelSize: 19
                            font.bold: true
                        }

                        CheckBox {
                            id: useLedCheck
                            text: "Hiển thị LED công khai trên màn hình đã chọn"
                            font.family: "Inter"
                        }

                        ComboBox {
                            id: screenCombo
                            Layout.fillWidth: true
                            enabled: useLedCheck.checked
                            model: app.screens
                            textRole: "label"
                            valueRole: "name"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: app.previewMode
                                  ? "LED preview luôn toàn màn hình trên màn chính. Dùng Alt+Tab để trở lại cửa sổ vận hành."
                                  : (app.targetScreenName.length > 0
                                     ? ("LED toàn màn hình, không viền. Đang chọn: " + app.targetScreenName)
                                     : "Chưa chọn output — LED công khai đang ẩn.")
                            color: AppColors.textMuted
                            font.family: "Inter"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Nếu dùng Extend trên Windows, chọn màn LED theo tên trong danh sách. Không dùng Duplicate cho vận hành thật."
                            color: AppColors.textFaint
                            font.family: "Inter"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Lưu output"
                                highlighted: true
                                onClicked: {
                                    if (useLedCheck.checked && (screenCombo.currentIndex < 0 || !screenCombo.currentValue)) {
                                        page.statusIsError = true
                                        page.statusText = "Chưa chọn màn hình LED."
                                        return
                                    }
                                    const chosen = useLedCheck.checked ? (screenCombo.currentValue || "") : ""
                                    dialog.ask(
                                        "Lưu cấu hình hiển thị",
                                        (chosen.length ? ("Màn hình LED: " + chosen) : "LED công khai sẽ ẩn") +
                                        "\nChế độ: toàn màn hình, không viền" +
                                        "\n\nThay đổi áp dụng ngay, không đổi cấu hình hiển thị của Windows.",
                                        function() {
                                            const error = app.updateTargetScreen(chosen)
                                            if (error.length > 0) {
                                                page.statusIsError = true
                                                page.statusText = error
                                            } else {
                                                page.statusIsError = false
                                                page.statusText = "Đã lưu cấu hình hiển thị."
                                                syncScreenSelection()
                                            }
                                        })
                                }
                            }
                        }
                    }
                }

                Item { implicitHeight: 12 }
            }
        }
    }

    ActionDialog {
        id: dialog
    }
}
