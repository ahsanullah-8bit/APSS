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

    contentItem: ColumnLayout {
        clip: true

        ComboBox {
            id: sourceCombo
            model: dialogRoot.sources

            onCurrentIndexChanged: sourceField.clear()

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        }

        RowLayout {
            TextField {
                id: sourceField
                placeholderText: dialogRoot.placeholderForSource[sourceCombo.currentIndex]

                // Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Button {
                id: browseFile
                text: "Browse"
                visible: sourceCombo.currentIndex === 0
                onClicked: function () {
                    fileDialog.open()
                }

                Layout.preferredHeight: sourceField.height
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
    }

    // TODO: Force video file selection by formats
    FileDialog {
        id: fileDialog

        currentFolder: StandardPaths.writableLocation(StandardPaths.MoviesLocation)
        onAccepted: {
            let sourceFile = sliceFilePrefix(fileDialog.selectedFile)

            console.warn(sourceFile)
            dialogRoot.source = sourceField.text = sourceFile
        }
    }

    function sliceFilePrefix(path: string): string {
        if (path.startsWith("file:///"))
            return path.slice(8)

        return path
    }
}
