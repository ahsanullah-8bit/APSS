pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Qt.labs.platform
import APSS

Page {
    id: playbackRoot

    signal stopped()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        // Video Playback Screen
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "black"

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
            }

            ListView {
                id: detections

                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                    margins: 10
                }
                height: 110
                spacing: 10
                // model: apssEngine.licenseplatePaths
                clip: true
                orientation: Qt.Horizontal
                delegate: Rectangle {
                    width: 160
                    height: detections.height
                    color: "#D9D9D9"
                }
            }
        }

        Row {
            spacing: 10
            Layout.alignment: Qt.AlignHCenter

            Button {
                id: selectButton

                text: "Select"
                onClicked: function () {
                    fileDialog.open()
                }
            }

            Button {
                id: selectRemoteCameraButton

                text: "Select a Camera"
                onClicked: function () {
                    fileDialog.open()
                }
            }

            // TODO: Force video file selection by formats
            FileDialog {
                id: fileDialog

                folder: StandardPaths.writableLocation(StandardPaths.MoviesLocation)
                onFileChanged: function () {
                    apssEngine.openAFootage(fileDialog.file, videoOutput.videoSink)
                }
            }
        }
    }
}
