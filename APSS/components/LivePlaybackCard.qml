import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

Pane {
    id: cameraCard

    enum CameraState {
        Active,
        Disabled,
        Error
    }

    property string name: "uknown"
    property int cameraState: LivePlaybackCard.Disabled
    property alias videoOutput: videoOutput
    property alias metricsList: metricsPane.metrics
    property bool showMetrics: false

    width: 400
    height: 300

    // Live Camera Feed Area
    Rectangle {
        anchors.fill: parent
        color: "black" // A slightly lighter grey for the video area background
        radius: 10 // Rounded corners for the video area
        clip: true // Ensure content inside doesn't overflow rounded corners

        VideoOutput {
            id: videoOutput
            // source: camera // Link the video output to the camera object
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit // Adjust to fit, preserving aspect ratio
        }

        ColumnLayout {
            anchors {
                left: parent.left
                top: parent.top
                margins: 15
            }

            // little info about the camera
            RowLayout {
                spacing: 10

                Text {
                    text: cameraCard.name
                    color: "white"
                    opacity: 0.7
                }

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: cameraCard.cameraState === LivePlaybackCard.Active ? "lightgreen" : (cameraCard.cameraState === LivePlaybackCard.Disabled ? "gray" : "red")
                    border.width: 1
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }
            }

            MetricsPane {
                id: metricsPane

                visible: cameraCard.showMetrics
            }
        }

        Row {
            anchors {
                right: parent.right
                top: parent.top
                margins: 15
            }

            RoundButton {
                width: 36
                height: 36
                radius: 10
                checkable: true

                icon.source: "icons/wave.png"
                display: RoundButton.IconOnly

                onCheckedChanged: function () {
                    cameraCard.showMetrics = checked
                }
            }
        }
    }
}
