import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: rootDialog

    property url thumbnail
    property string label
    property string camera
    property string time
    property string topScore
    property string licensePlateText
    property string licensePlateImageSource

    standardButtons: Dialog.Ok
    modal: true

    RowLayout {
        anchors.fill: parent

        // details
        ColumnLayout {
            Layout.alignment: Qt.AlignTop

            Text {
                id: details_

                text: "Details"
                font {
                    pointSize: 14
                }
            }

            Text {
                id: label_
                text: "Type: " + rootDialog.label
            }

            Text {
                id: camera_
                text: "Camera: " + rootDialog.camera
            }

            Text {
                id: time_
                text: "Time: " + rootDialog.time
            }

            Text {
                id: topScore_
                text: "Top Score: " + rootDialog.topScore + "%"
            }

            // additional
            Text {
                id: additional_

                text: "Additional Data"
                font {
                    pointSize: 14
                }
            }

            Text {
                id: licensePlateText_
                text: "License Plate: " + rootDialog.licensePlateText
            }

            Image {
                id: licensePlate_
                Layout.fillWidth: true
                Layout.fillHeight: true

                fillMode: Image.PreserveAspectFit
                source: rootDialog.licensePlateImageSource
                sourceSize {
                    width: licensePlate_.width
                    height: licensePlate_.height
                }
            }
        }

        // image
        Image {
            id: img
            Layout.fillWidth: true
            Layout.fillHeight: true

            fillMode: Image.PreserveAspectFit
            source: rootDialog.thumbnail
            sourceSize {
                width: img.width
                height: img.height
            }
        }
    }
}
