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
        fullScreenCheck.checked = app.fullScreenEnabled
        let found = -1
        for (let i = 0; i < app.screens.length; ++i) {
            if (app.screens[i].name === app.targetScreenName) {
                found = i
                break
            }
        }
        screenCombo.currentIndex = found
    }

    function syncSimulationSelection() {
        const index = app.simulationScenarios.indexOf(app.simulationScenario)
        if (index >= 0)
            simulationCombo.currentIndex = index
    }

    onVisibleChanged: {
        if (visible) {
            syncScreenSelection()
            syncSimulationSelection()
        }
    }

    Component.onCompleted: {
        syncScreenSelection()
        syncSimulationSelection()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: "Màn hình và thương hiệu"
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
                            text: "Hiển thị cửa sổ LED công khai trên màn hình đã chọn"
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

                        CheckBox {
                            id: fullScreenCheck
                            text: "Toàn màn hình trên output LED (khuyến nghị khi vận hành)"
                            font.family: "Inter"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: app.previewMode
                                  ? "Cửa sổ LED đang hiển thị trên màn hình hiện tại."
                                  : (app.targetScreenName.length > 0
                                     ? ("Đang chọn: " + app.targetScreenName)
                                     : "Chưa chọn output — cửa sổ LED công khai đang ẩn.")
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
                                        (chosen.length ? ("Màn hình LED: " + chosen) : "Cửa sổ LED công khai sẽ ẩn") +
                                        "\nChế độ: " + (fullScreenCheck.checked ? "toàn màn hình" : "cửa sổ") +
                                        "\n\nThay đổi áp dụng ngay, không đổi cấu hình hiển thị của Windows.",
                                        function() {
                                            const error = app.updateDisplay(chosen, fullScreenCheck.checked)
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

                // ---------------------------------------------------------
                // Branding
                // ---------------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: brandingColumn.implicitHeight + 40
                    radius: 8
                    color: AppColors.surfaceContainerLow
                    border.width: 1
                    border.color: AppColors.elevatedBorder

                    ColumnLayout {
                        id: brandingColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 10

                        Text {
                            text: "Thương hiệu và chú thích"
                            color: AppColors.primaryText
                            font.family: "Inter"
                            font.pixelSize: 19
                            font.bold: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

                            Text {
                                text: "Tên công ty"
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                            }
                            TextField {
                                id: companyField
                                Layout.fillWidth: true
                                text: app.branding.companyName || ""
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                text: "Chi nhánh"
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                            }
                            TextField {
                                id: branchField
                                Layout.fillWidth: true
                                text: app.branding.branchName || ""
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                text: "Địa chỉ"
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                            }
                            TextField {
                                id: addressField
                                Layout.fillWidth: true
                                text: app.branding.address || ""
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                text: "Quy chuẩn / căn cứ"
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                            }
                            TextField {
                                id: regulationField
                                Layout.fillWidth: true
                                text: app.branding.regulationText || ""
                                placeholderText: "Chỉ nhập nội dung đã được nhà máy duyệt"
                                font.family: "Inter"
                                font.pixelSize: 14
                            }

                            Text {
                                text: "Ghi chú footer"
                                color: AppColors.textMuted
                                font.family: "Inter"
                                font.pixelSize: 13
                            }
                            TextField {
                                id: footerField
                                Layout.fillWidth: true
                                text: app.branding.footerNote || ""
                                placeholderText: "Ví dụ: dữ liệu quan trắc tự động"
                                font.family: "Inter"
                                font.pixelSize: 14
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Không sao chép ngưỡng/hệ số từ PDF hoặc database cũ thành nội dung được duyệt. Footer sẽ để trống nếu chưa cấu hình."
                            color: AppColors.textFaint
                            font.family: "Inter"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Lưu thương hiệu"
                                highlighted: true
                                onClicked: {
                                    dialog.ask(
                                        "Lưu thương hiệu",
                                        "Thay đổi nội dung header/footer trên màn LED công khai.",
                                        function() {
                                            const error = app.updateBranding(companyField.text,
                                                                             branchField.text,
                                                                             addressField.text,
                                                                             regulationField.text,
                                                                             footerField.text)
                                            if (error.length > 0) {
                                                page.statusIsError = true
                                                page.statusText = error
                                            } else {
                                                page.statusIsError = false
                                                page.statusText = "Đã lưu thương hiệu và chú thích."
                                            }
                                        })
                                }
                            }
                        }
                    }
                }

                // ---------------------------------------------------------
                // Simulation
                // ---------------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: simulationColumn.implicitHeight + 40
                    radius: 8
                    color: app.simulationActive
                           ? AppColors.withAlpha(AppColors.warning, 0.08)
                           : AppColors.surfaceContainerLow
                    border.width: 1
                    border.color: app.simulationActive ? AppColors.warning : AppColors.elevatedBorder

                    ColumnLayout {
                        id: simulationColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Kịch bản hiển thị"
                                color: AppColors.primaryText
                                font.family: "Inter"
                                font.pixelSize: 19
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Chọn kịch bản hiển thị. Nguồn dữ liệu tạm dừng khi chạy kịch bản; cấu hình vận hành không thay đổi."
                            color: AppColors.textMuted
                            font.family: "Inter"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            ComboBox {
                                id: simulationCombo
                                Layout.fillWidth: true
                                model: app.simulationScenarios
                            }
                            Button {
                                text: app.simulationActive ? "Áp dụng cảnh" : "Bắt đầu kịch bản"
                                highlighted: true
                                onClicked: {
                                    const scenario = simulationCombo.currentText
                                    dialog.ask(
                                        "Bắt đầu kịch bản",
                                        "Cảnh: " + scenario +
                                        "\n\nNguồn dữ liệu thật sẽ tạm dừng cho đến khi kết thúc kịch bản.",
                                        function() {
                                            if (app.simulationActive)
                                                app.applySimulationScenario(scenario)
                                            else
                                                app.setSimulation(true, scenario)
                                            page.statusIsError = false
                                            page.statusText = "Đã áp dụng kịch bản " + scenario + "."
                                        })
                                }
                            }
                            Button {
                                visible: app.simulationActive
                                text: "Dừng kịch bản"
                                onClicked: {
                                    app.setSimulation(false, "")
                                    page.statusIsError = false
                                    page.statusText = "Đã dừng kịch bản; đang chờ dữ liệu từ nguồn."
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Kịch bản có sẵn: " + app.simulationScenarios.join(" · ")
                            color: AppColors.textFaint
                            font.family: "Inter"
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
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
