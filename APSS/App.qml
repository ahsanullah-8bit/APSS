// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import APSS

Window {
    width: Constants.width
    height: Constants.height

    visible: true
    title: "APSS"

    MainScreen {
        id: mainScreen
        anchors.fill: parent
    }
}
