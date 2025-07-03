

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import APSS
import APSS.Components

Rectangle {
    implicitWidth: Constants.width
    implicitHeight: Constants.height

    color: Constants.backgroundColor

    // Side-bar
    RowLayout {
        anchors.fill: parent

        Pane {
            Layout.fillHeight: true
            Layout.preferredWidth: 70

            ButtonGroup {
                buttons: sideBarCol.children
            }

            Column {
                id: sideBarCol

                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20
                padding: 10

                RoundButton {
                    text: "Home"
                    checkable: true

                    height: 50
                    width: 50
                    radius: 10
                }

                RoundButton {
                    text: "Preview"
                    checkable: true

                    height: 50
                    width: 50
                    radius: 10
                }

                RoundButton {
                    text: "Recordings"
                    checkable: true

                    height: 50
                    width: 50
                    radius: 10
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            rows: 2
            columns: 2

            Repeater {
                model: Constants.isPreviewMode ? 1 : apssEngine.cameraMetricsModel

                delegate: LivePlaybackCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    name: Constants.isPreviewMode ? "uknown" : model.name
                    visible: true

                    Component.onCompleted: {
                        // VideoSink must be set, so the engine can forward frames
                        model.videosink = videoOutput.videoSink
                    }
                }
            }

            // LivePlaybackCard {
            //     Layout.fillWidth: true
            //     Layout.fillHeight: true

            //     visible: true
            // }

            // LivePlaybackCard {
            //     Layout.fillWidth: true
            //     Layout.fillHeight: true

            //     visible: true
            // }

            // LivePlaybackCard {
            //     Layout.fillWidth: true
            //     Layout.fillHeight: true

            //     visible: true
            // }
        }
    }
}
