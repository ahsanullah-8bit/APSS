

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
                    id: liveplaycard
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    metricsList: Constants.isPreviewMode ? [] : [["Process FPS", model.processfps], ["Detection FPS", model.detectionfps]]
                    name: Constants.isPreviewMode ? "uknown" : model.name
                    visible: true

                    Component.onCompleted: {
                        // VideoSink must be set, so the engine can forward frames
                        if (!Constants.isPreviewMode) {
                            model.videosink = videoOutput.videoSink
                        }
                    }

                    Connections {
                        target: fpsTimer

                        onTriggered: function () {
                            var metrics = liveplaycard.metricsList

                            if (!Constants.isPreviewMode) {
                                // metrics[0][1] = model.camerafps
                                metrics[0][1] = model.processfps
                                metrics[1][1] = model.detectionfps
                            }

                            liveplaycard.metricsList = metrics
                        }
                    }
                }
            }
        }
    }

    Timer {
        id: fpsTimer

        interval: 1000 // 1sec
        repeat: true

        Component.onCompleted: function () {
            fpsTimer.start()
        }
    }
}
