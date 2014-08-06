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

import QtQuick 2.0
import Ubuntu.Components 1.1
import Ubuntu.Components.Popups 1.0

Rectangle {
    width: units.gu(40)
    height: units.gu(71)
    color: "transparent"

    property var dialogTitle: title
    property var dialogDescription: description

    signal quit(int code)

    Component.onCompleted: dialog.show();

    Dialog {
        id: dialog

        title: dialogTitle
        text: dialogDescription
        fadingAnimation: PropertyAnimation { duration: 0 }

        Button {
            objectName: "deny"
            text: i18n.tr("Deny")
            color: UbuntuColors.red
            onClicked: quit(1)
        }

        Button {
            objectName: "allow"
            text: i18n.tr("Allow")
            color: UbuntuColors.green
            onClicked: quit(0)
        }
    }
}
