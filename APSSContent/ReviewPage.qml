import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import APSS

Page {
	clip: true

	GridView {
		id: eventsView

		cellWidth: 600
		cellHeight: 400

		anchors.fill: parent

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

				MouseArea {
					hoverEnabled: true
					anchors.fill: parent

					onEntered: function () {
						if (containsMouse) {
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
