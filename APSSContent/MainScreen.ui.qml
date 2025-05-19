

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import APSS

Rectangle {
    implicitWidth: Constants.width
    implicitHeight: Constants.height

    color: Constants.backgroundColor

    StackView {
        id: pages
        anchors.fill: parent

        initialItem: videoPlaybackPage
    }

    VideoPlaybackPage {
        id: videoPlaybackPage

        visible: false
    }

    Connections {
        target: videoPlaybackPage

        function onStopped() {
            pages.pop()
        }
    }
}
