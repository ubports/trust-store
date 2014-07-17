import QtQuick 2.0
import Ubuntu.Components 0.1
import QtQuick.Layouts 1.1

Rectangle {
    anchors.fill: parent

    signal quit(int code)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: units.gu(2)
        spacing: units.gu(2)

        Label {
            anchors.horizontalCenter: parent.horizontalCenter

            text: title
            font.family: "Ubuntu"
            fontSize: "large"

            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {

            text: description
            font.family: "Ubuntu"
            fontSize: "medium"
            Layout.maximumWidth: parent.width
            Layout.fillHeight: true

            wrapMode: Text.WordWrap
        }

        RowLayout {
            spacing: units.gu(1)
            height: units.gu(3)
            anchors.right: parent.right

            Button {
                text: "Deny"
                onClicked: quit(1)
            }
            Button {
                text: "Grant"
                onClicked: quit(0)
            }
        }
    }
}

