import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import LoggerKit.Theme

Item {
    id: page
    property string statusText: ""
    property bool statusIsError: false

    function report(error, success) {
        statusIsError = error.length > 0
        statusText = statusIsError ? error : success
    }

    Component.onCompleted: report(app.refreshLogFiles(), "")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text {
            text: "Nhật ký ứng dụng"
            color: AppColors.primaryText
            font.family: "Inter"
            font.pixelSize: 26
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            text: "Xem các tệp log đã lưu, sao chép nội dung đang xem hoặc xuất toàn bộ tệp."
            color: AppColors.textMuted
            font.family: "Inter"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                id: logChoice
                objectName: "logFileChoice"
                Layout.fillWidth: true
                model: app.logFiles
                currentIndex: app.logFiles.indexOf(app.selectedLog)
                onActivated: page.report(app.selectLog(currentText), "")
            }
            Button {
                objectName: "refreshLogsButton"
                text: "Làm mới"
                onClicked: page.report(app.refreshLogFiles(), "Đã cập nhật nhật ký.")
            }
            Button {
                objectName: "copyLogButton"
                text: "Sao chép"
                enabled: app.logContent.length > 0
                onClicked: page.report(app.copyLog(), "Đã sao chép nội dung đang xem.")
            }
            Button {
                objectName: "exportLogButton"
                text: "Xuất tệp…"
                enabled: app.selectedLog.length > 0
                onClicked: exportDialog.open()
            }
            Button {
                text: "Mở thư mục"
                onClicked: app.openLogFolder()
            }
        }

        InlineNotice {
            Layout.fillWidth: true
            text: page.statusText
            isError: page.statusIsError
        }
        Text {
            Layout.fillWidth: true
            text: app.logViewNote
            color: AppColors.textMuted
            font.family: "Inter"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                objectName: "logViewerText"
                text: app.logContent
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.NoWrap
                color: AppColors.primaryText
                font.family: "monospace"
                font.pixelSize: 12
                background: Rectangle {
                    color: AppColors.surfaceContainerLow
                    border.color: AppColors.elevatedBorder
                    border.width: 1
                    radius: 6
                }
            }
        }
    }

    FileDialog {
        id: exportDialog
        objectName: "exportLogDialog"
        title: "Xuất tệp nhật ký"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Tệp log (*.log)", "Tệp văn bản (*.txt)", "Tất cả tệp (*)"]
        defaultSuffix: "log"
        onAccepted: page.report(app.exportLog(selectedFile), "Đã xuất tệp nhật ký.")
    }
}
