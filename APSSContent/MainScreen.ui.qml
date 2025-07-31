

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

    RowLayout {
        anchors.fill: parent

        // Side-bar
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
                    checked: true
                    display: RoundButton.IconOnly
                    icon.source: "icons/home-solid.svg"

                    height: 50
                    width: 50
                    radius: 10

                    onCheckedChanged: function () {
                        if (checked) {
                            mainSwipeView.setCurrentIndex(0)
                        }
                    }
                }

                RoundButton {
                    text: "Review"
                    checkable: true
                    display: RoundButton.IconOnly
                    icon.source: "icons/camera-solid.svg"

                    height: 50
                    width: 50
                    radius: 10

                    onCheckedChanged: function () {
                        if (checked) {
                            mainSwipeView.setCurrentIndex(1)
                        }
                    }
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

        SwipeView {
            id: mainSwipeView
            Layout.fillWidth: true
            Layout.fillHeight: true

            interactive: false
            orientation: Qt.Vertical

            HomePage {}
            ReviewPage {}
        }

        PageIndicator {
            id: indicator

            count: mainSwipeView.count
            currentIndex: mainSwipeView.currentIndex
        }
    }
}
