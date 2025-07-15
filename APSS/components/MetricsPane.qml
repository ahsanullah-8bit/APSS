import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: metricsRoot

    property var metrics: [["dummy fps", 10], ["dummmy process fps", 40]]

    background: Rectangle {
        color: metricsRoot.palette.window
        opacity: 0.5
        radius: 5
    }

    ColumnLayout {
        Repeater {
            model: metricsRoot.metrics

            RowLayout {
                Text {
                    id: nameText
                    text: modelData[0]

                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.fillWidth: true
                }

                Text {
                    text: modelData[1]

                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
            }
        }
    }
}
