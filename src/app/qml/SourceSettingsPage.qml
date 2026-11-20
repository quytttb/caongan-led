import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page

    property var drafts: ({})

    function draftFor(source) {
        const existing = drafts[source.id]
        if (existing && existing.dirty === true)
            return existing
        return {
            folder: source.folder || "",
            stationCode: source.stationCode || "",
            staleSeconds: source.staleSeconds || 900,
            dirty: false
        }
    }

    function setDraftField(source, key, value) {
        const base = draftFor(source)
        const next = Object.assign({}, drafts)
        next[source.id] = {
            folder: key === "folder" ? value : base.folder,
            stationCode: key === "stationCode" ? value : base.stationCode,
            staleSeconds: key === "staleSeconds" ? value : base.staleSeconds,
            dirty: true
        }
        drafts = next
    }

    function clearDraft(sourceId) {
        const next = Object.assign({}, drafts)
        delete next[sourceId]
        drafts = next
    }

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
                    notice.showInfo("Đã gửi yêu cầu quét lại. Kết quả sẽ xuất hiện trong tab Chẩn đoán.")
                }
            }
        }

        InlineNotice {
            id: notice
            Layout.fillWidth: true
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
                        required property var modelData
                        objectName: "sourceCard_" + modelData.id
                        Layout.fillWidth: true
                        implicitHeight: cardColumn.implicitHeight + 44
                        radius: 8
                        color: AppColors.surfaceContainerLow
                        border.width: 1
                        border.color: AppColors.elevatedBorder

                        property var live: app.sourceStatuses[modelData.id] || ({})
                        property bool online: live.available === true

                        Component.onCompleted: {
                            const draft = page.draftFor(modelData)
                            folderField.text = draft.folder
                            stationField.text = draft.stationCode
                            staleSpin.value = draft.staleSeconds
                        }

                        FolderDialog {
                            id: folderDialog
                            title: "Chọn thư mục nguồn " + sourceCard.modelData.name
                            onAccepted: {
                                const path = app.urlToLocalPath(selectedFolder)
                                if (path.length > 0) {
                                    folderField.text = path
                                    page.setDraftField(sourceCard.modelData, "folder", path)
                                }
                            }
                        }

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
                                    text: sourceCard.modelData.name
                                    color: AppColors.primaryText
                                    font.family: "Inter"
                                    font.pixelSize: 20
                                    font.bold: true
                                }
                                Text {
                                    text: "• " + sourceCard.modelData.id
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
                                text: (sourceCard.live.message || "Chưa có trạng thái từ bộ đọc thư mục.")
                                      + (sourceCard.live.lastFileText ? (" • File gần nhất: " + sourceCard.live.lastFileText) : "")
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
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.columnSpan: 3
                                    spacing: 8

                                    TextField {
                                        id: folderField
                                        objectName: "sourceFolderField_" + sourceCard.modelData.id
                                        Layout.fillWidth: true
                                        placeholderText: "Ví dụ D:\\CEMS\\KHITO1 (để trống nếu chưa cấu hình)"
                                        selectByMouse: true
                                        font.family: "Inter"
                                        font.pixelSize: 14
                                        onTextEdited: page.setDraftField(sourceCard.modelData, "folder", text)
                                    }
                                    Button {
                                        objectName: "browseFolderButton_" + sourceCard.modelData.id
                                        text: "Chọn…"
                                        onClicked: folderDialog.open()
                                    }
                                }

                                Text {
                                    text: "Mã trạm"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                TextField {
                                    id: stationField
                                    objectName: "sourceStationField_" + sourceCard.modelData.id
                                    Layout.preferredWidth: 240
                                    selectByMouse: true
                                    font.family: "Inter"
                                    font.pixelSize: 14
                                    onTextEdited: page.setDraftField(sourceCard.modelData, "stationCode", text)
                                }
                                Text {
                                    text: "Thời gian coi dữ liệu cũ"
                                    color: AppColors.textMuted
                                    font.family: "Inter"
                                    font.pixelSize: 13
                                }
                                RowLayout {
                                    spacing: 8
                                    SpinBox {
                                        id: staleSpin
                                        objectName: "sourceStaleSpin_" + sourceCard.modelData.id
                                        Layout.preferredWidth: 180
                                        from: 10
                                        to: 86400
                                        stepSize: 60
                                        editable: true
                                        onValueModified: page.setDraftField(sourceCard.modelData, "staleSeconds", value)
                                    }
                                    Text {
                                        objectName: "sourceStaleUnit_" + sourceCard.modelData.id
                                        text: "giây"
                                        color: AppColors.textMuted
                                        font.family: "Inter"
                                        font.pixelSize: 13
                                    }
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
                                    objectName: "saveSourceButton_" + sourceCard.modelData.id
                                    text: "Lưu nguồn"
                                    highlighted: true
                                    onClicked: {
                                        if (staleSpin.contentItem && staleSpin.contentItem.text !== undefined) {
                                            const rawStale = staleSpin.contentItem.text.trim()
                                            if (/^-?\d+$/.test(rawStale)) {
                                                const parsed = parseInt(rawStale, 10)
                                                staleSpin.value = Math.max(staleSpin.from, Math.min(staleSpin.to, parsed))
                                                page.setDraftField(sourceCard.modelData, "staleSeconds", staleSpin.value)
                                            }
                                        }
                                        const id = sourceCard.modelData.id
                                        const name = sourceCard.modelData.name
                                        const folder = folderField.text
                                        const station = stationField.text
                                        const stale = staleSpin.value
                                        dialog.ask(
                                            "Lưu cấu hình nguồn " + name,
                                            "Thư mục: " + (folder.length ? folder : "(chưa cấu hình)") +
                                            "\nMã trạm: " + station + "\nThời gian coi dữ liệu cũ: " + stale + " giây" +
                                            "\n\nThay đổi này áp dụng cho toàn bộ kênh thuộc nguồn.",
                                            function() {
                                                const error = app.updateSource(id, folder, station, stale)
                                                if (error.length === 0)
                                                    page.clearDraft(id)
                                                notice.showResult(error, "Đã lưu cấu hình nguồn " + name + ".")
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
