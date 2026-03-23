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

				function getTypeIcon(type) {
					if (type === "person")
						return "icons/person.svg"
					else if (type === "car" || type === "truck"
							 || type === "bus" || type === "boat")
						return "icons/vehicle.svg"
					else if (type === "bicycle" || type === "motorcycle")
						return "icons/bike.svg"
				}

				// top layer
				Button {
					id: typeIcon

					anchors {
						top: parent.top
						left: parent.left
						margins: 5
					}

					width: 40
					height: 40
					icon.source: parent.getTypeIcon(model.label)
					display: Button.IconOnly
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

					onClicked: function () {
						detailedEvent.thumbnail = model.thumbnail
						detailedEvent.label = model.label
						detailedEvent.camera = model.camera
						detailedEvent.time = model.timeinterval
						detailedEvent.topScore = model.topscore
						var lppath = model.lppath
						if (lppath)
							detailedEvent.licensePlateImageSource = lppath
						detailedEvent.open()
					}
				}
			}
		}
	}

	MediaPlayer {
		id: unifiedPlayer

		autoPlay: true
	}

	DetailedEvent {
		id: detailedEvent

		width: parent.width / 2
		height: parent.height / 2
		anchors.centerIn: parent
		onAccepted: function () {
			detailedEvent.close()
		}
	}
}
