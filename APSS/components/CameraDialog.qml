import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialogRoot

    property string source: ""

    implicitHeight: 150
    implicitWidth: 100

    title: "Select a Camera"
    standardButtons: Dialog.Ok | Dialog.Cancel

    readonly property list<string> sources: ["Local File", "Remote Camera"]
    readonly property list<string> placeholderForSource: ["Enter file path here", "Enter camera url here"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        clip: true

        ComboBox {
            id: sourceComboBox
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop

            model: dialogRoot.sources
        }

        RowLayout {
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop

            TextField {
                // Layout.fillHeight: true
                Layout.fillWidth: true
                placeholderText: dialogRoot.placeholderForSource[sourceComboBox.currentIndex]
            }

            Button {
                Layout.preferredHeight: 40

                visible: sourceComboBox.currentIndex === 0
                text: "Browse"
                onClicked: function () {
                    fileDialog.open()
                }
            }

            // TODO: Force video file selection by formats
            FileDialog {
                id: fileDialog

                currentFolder: StandardPaths.writableLocation(StandardPaths.MoviesLocation)
                onSelectedFileChanged: dialogRoot.source = fileDialog.selectedFile
            }
        }
    }
}
