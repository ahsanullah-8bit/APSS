import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.TabButton {
    id: control

    property int contentAlignment: Qt.AlignLeft
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    icon.width: 24
    icon.height: 24
    icon.color: checked ? control.palette.windowText : control.palette.brightText

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.contentAlignment

        icon: control.icon
        text: control.text
        font: control.font
        color: control.checked ? control.palette.windowText : control.palette.brightText
    }

    background: Rectangle {
        implicitHeight: 40
        color: Color.blend(control.checked ? control.palette.window : control.palette.dark,
                                             control.palette.mid, control.down ? 0.5 : 0.0)
    }
}
