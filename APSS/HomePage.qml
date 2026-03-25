import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import APSS
import APSS.Components

Page {

    LoggingCategory {
        id: logger
        name: "apss.ui.home"
        defaultLogLevel: LoggingCategory.Debug
    }

    GridLayout {
        anchors.fill: parent

        rows: 2
        columns: 2

        Repeater {
            model: Constants.isPreviewMode ? 1 : apssEngine.cameraMetricsModel

            delegate: LivePlaybackCard {
                id: liveplaycard
                Layout.fillWidth: true
                Layout.fillHeight: true

                metricsList: Constants.isPreviewMode
                             || !showMetrics ? [] : [["Process FPS", model.processfps], ["Detection FPS", model.detectionfps]]
                name: Constants.isPreviewMode ? "uknown" : model.name
                visible: true

                Component.onCompleted: function () {
                    // VideoSink must be set, so the engine can forward frames
                    if (!Constants.isPreviewMode) {
                        model.videosink = videoOutput.videoSink
                    }
                }

                onShowMetricsChanged: function () {
                    if (liveplaycard.showMetrics)
                        fpsTimer.start()
                    else
                        fpsTimer.stop()
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

    Timer {
        id: fpsTimer

        interval: 1000 // 1sec
        repeat: true
    }
}
