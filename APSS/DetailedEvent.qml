import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: rootPopup

    property url thumbnail
    property string label
    property string camera
    property string time
    property string topScore
    property string licensePlateText
    property string licensePlateImageSource

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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
                text: "Type: " + rootPopup.label
            }

            Text {
                id: camera_
                text: "Camera: " + rootPopup.camera
            }

            Text {
                id: time_
                text: "Time: " + rootPopup.time
            }

            Text {
                id: topScore_
                text: "Top Score: " + rootPopup.topScore + "%"
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
                text: "License Plate: " + rootPopup.licensePlateText
            }

            Image {
                id: licensePlate_
                Layout.fillWidth: true
                Layout.fillHeight: true

                fillMode: Image.PreserveAspectFit
                source: rootPopup.licensePlateImageSource
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
            source: rootPopup.thumbnail
            sourceSize {
                width: img.width
                height: img.height
            }
        }
    }
}
