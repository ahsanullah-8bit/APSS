import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQml
import APSS

Page {
	clip: true

	GridView {
		id: eventsView

		//  3 cells horizontally
		cellWidth: parent.width / 3
		cellHeight: cellWidth * 0.667

		anchors.fill: parent

		ScrollBar.vertical: ScrollBar {}

		model: Constants.isPreviewMode ? 10 : eventsModel
		delegate: Item {
			id: myDelegate
			height: eventsView.cellHeight
			width: eventsView.cellWidth

			Rectangle {
				color: "black"

				anchors.fill: parent
				anchors.margins: 10
				radius: 10

				Image {
					id: thumbnailView
					source: model.thumbnail
					sourceSize {
						width: parent.width
						height: parent.height
					}
					fillMode: Image.PreserveAspectFit

					anchors.fill: parent
				}

				VideoOutput {
					id: videoOutput
					visible: false
					anchors.fill: parent
				}

				Text {
					id: startTime
					text: Qt.formatDateTime(model.starttime,
											"ddd d, hh:mm:ss AP")
					color: "white"
					font.pixelSize: 20

					anchors {
						bottom: parent.bottom
						right: parent.right
						margins: 5
					}
				}

				MouseArea {
					hoverEnabled: true
					anchors.fill: parent

					onEntered: function () {
						if (containsMouse && model.reviewpath) {
							unifiedPlayer.videoOutput = videoOutput
							unifiedPlayer.source = model.reviewpath
							unifiedPlayer.play()

							videoOutput.visible = true
							thumbnailView.visible = false
						}
					}

					onExited: function () {
						if (!containsMouse) {
							videoOutput.visible = false
							thumbnailView.visible = true

							unifiedPlayer.stop()
						}
					}
				}
			}
		}
	}

	MediaPlayer {
		id: unifiedPlayer

		autoPlay: true
	}
}
