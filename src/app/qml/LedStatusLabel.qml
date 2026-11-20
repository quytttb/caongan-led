import QtQuick

// Nhãn trạng thái đặc thù LED: chữ lớn, tương phản cao, không phải chip 24 px
// của UI kit desktop. Màu chỉ là lớp phụ; nhãn chữ luôn hiện rõ.
Rectangle {
    id: root
    property color accentColor: "#FFFFFF"
    property string text: "CHƯA CÓ DỮ LIỆU"
    property int fontSize: 30
    property int maximumWidth: 424

    implicitWidth: Math.min(label.implicitWidth + 30, maximumWidth)
    implicitHeight: 42
    radius: 4
    color: "#000000"
    border.width: 1
    border.color: root.accentColor

    Text {
        id: label
        objectName: "statusLabelText"
        width: Math.min(implicitWidth, root.maximumWidth - 24)
        anchors.centerIn: parent
        text: root.text
        color: root.accentColor
        font.family: "Inter"
        font.pixelSize: root.fontSize
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 18
        renderType: Text.QtRendering
    }
}
