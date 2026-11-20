import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    function syncScreenSelection() {
        autoCheck.checked = app.autoDetectExternal
        useLedCheck.checked = !app.autoDetectExternal && app.targetScreenName.length > 0
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

    Connections {
        target: app
        function onScreensChanged() {
            if (page.visible)
                page.syncScreenSelection()
        }
        function onConfigChanged() {
            if (page.visible)
                page.syncScreenSelection()
        }
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
                text: "Ứng dụng tự chọn màn hình ngoài cho LED khi khởi động và mở giao diện cấu hình phóng to trên màn còn lại. Cửa sổ vận hành không bao giờ tự dời sang màn hình công khai khi output biến mất."
                color: AppColors.textMuted
                font.family: "Inter"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }

        InlineNotice {
            id: notice
            Layout.fillWidth: true
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
                            id: autoCheck
                            text: "Tự động dùng màn hình ngoài cho LED"
                            font.family: "Inter"
                            onToggled: {
                                if (checked) {
                                    useLedCheck.checked = false
                                } else if (screenCombo.currentIndex < 0 && app.screens.length > 0) {
                                    let suggested = 0
                                    for (let i = 0; i < app.screens.length; ++i) {
                                        if (app.screens[i].external === true) {
                                            suggested = i
                                            break
                                        }
                                    }
                                    screenCombo.currentIndex = suggested
                                }
                            }
                        }

                        CheckBox {
                            id: useLedCheck
                            visible: !autoCheck.checked
                            text: "Bật LED công khai trên màn hình chọn thủ công"
                            font.family: "Inter"
                        }

                        ComboBox {
                            id: screenCombo
                            Layout.fillWidth: true
                            visible: !autoCheck.checked
                            enabled: useLedCheck.checked
                            model: app.screens
                            textRole: "label"
                            valueRole: "name"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: app.autoDetectExternal
                                  ? (app.ledScreenName.length > 0
                                     ? ("Tự động: LED toàn màn hình trên " + app.ledScreenName +
                                        ". Cấu hình mở phóng to trên " + app.operatorScreenName + ".")
                                     : "Chưa phát hiện màn hình ngoài — LED công khai đang ẩn; cấu hình mở trên màn chính.")
                                  : (app.targetScreenName.length > 0
                                     ? ("LED toàn màn hình, không viền. Đang chọn: " + app.targetScreenName)
                                     : "LED công khai đang tắt.")
                            color: AppColors.textMuted
                            font.family: "Inter"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Màn ngoài là màn không phải màn chính (đặt màn cấu hình làm màn chính trong Windows). Nếu có nhiều màn ngoài, ứng dụng chọn màn có diện tích lớn nhất. Dùng Extend; không dùng Duplicate cho vận hành thật."
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
                                    const auto = autoCheck.checked
                                    if (!auto && useLedCheck.checked && (screenCombo.currentIndex < 0 || !screenCombo.currentValue)) {
                                        notice.showError("Chưa chọn màn hình LED.")
                                        return
                                    }
                                    const chosen = auto ? "" : (useLedCheck.checked ? (screenCombo.currentValue || "") : "")
                                    const summary = auto
                                        ? "Tự động chọn màn hình ngoài cho LED"
                                        : (chosen.length ? ("Màn hình LED: " + chosen) : "LED công khai sẽ tắt")
                                    dialog.ask(
                                        "Lưu cấu hình hiển thị",
                                        summary +
                                        "\nChế độ: toàn màn hình, không viền" +
                                        "\nCửa sổ cấu hình: phóng to trên màn còn lại" +
                                        "\n\nThay đổi áp dụng ngay, không đổi cấu hình hiển thị của Windows.",
                                        function() {
                                            const error = app.updateDisplayMode(auto, chosen)
                                            notice.showResult(error, "Đã lưu cấu hình hiển thị.")
                                            if (error.length === 0)
                                                syncScreenSelection()
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
