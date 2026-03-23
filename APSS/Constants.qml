pragma Singleton
import QtQuick

QtObject {
    readonly property int width: 1080
    readonly property int height: 800
    // readonly property int width: 1920
    // readonly property int height: 1080

    property string relativeFontDirectory: "fonts"

    readonly property color backgroundColor: "#EAEAEA"

    // Use this property if you want one time read of arguments
    readonly property bool isPreviewMode: isPreview()
    // Call this function to determine mode on the spot.
    function isPreview() {
        var args = Application.arguments

        // return args.contains("--qml-runtime") // Valid, but list<string> doesn't have these methods.
        return args[1] === "--qml-runtime"
    }
}
