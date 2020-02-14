/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.9
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3

Item {
    property var appIcon: icon
    property var appName: name
    property var appId: id
    property var serviceDescription: description

    signal quit(int code)

    Component {
        id: dialog
        Dialog {
            id: dialogue
            Column {
                spacing: units.gu(0.5)
                UbuntuShape {
                    anchors.horizontalCenter: parent.horizontalCenter
                    id: iconShape
                    radius: "medium"
                    aspect: UbuntuShape.DropShadow
                    anchors.margins: units.gu(1)
                    sourceFillMode: UbuntuShape.PreserveAspectCrop
                    source: Image {
                        id: icon
                        sourceSize.width: iconShape.width
                        sourceSize.height: iconShape.height
                        source: appIcon
                    }
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appName
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appId
                    color: theme.palette.normal.overlaySecondaryText
                    fontSize: "small"
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                    maximumLineCount: 1
                }
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: serviceDescription
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                wrapMode: Text.Wrap
            }
            Button {
                text: i18n.tr("Allow")
                color: theme.palette.normal.positive
                onClicked: quit(0)
            }
            Button {
                text: i18n.tr("Don’t Allow")
                onClicked: quit(1)
            }
        }
    }
    Component.onCompleted: PopupUtils.open(dialog)
}
