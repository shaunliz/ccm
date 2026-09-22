// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-FileCopyrightText: 2026 Innogrid Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-only

#include "ui/CloseChoice.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace ccm::ui {

CloseChoice askCloseChoice(QWidget *parent, bool *rememberChoice)
{
    if (rememberChoice) {
        *rememberChoice = false;
    }

    QMessageBox box(parent);
    box.setWindowTitle(QCoreApplication::translate("ccm", "종료"));
    box.setIcon(QMessageBox::Question);
    box.setText(QCoreApplication::translate("ccm", "창을 닫습니다. 어떻게 할까요?"));
    box.setInformativeText(QCoreApplication::translate(
        "ccm",
        "백그라운드에서 계속하면 트레이 아이콘으로 남아 사용량을 계속 조회하고 "
        "임계치를 넘으면 알립니다.\n"
        "완전 종료하면 조회도 알림도 멈춥니다."));

    // 버튼 역할을 제대로 주어야 플랫폼이 정한 자리에 놓이고, Esc 와 기본 선택이
    // 알맞게 걸린다.
    QPushButton *trayButton = box.addButton(
        QCoreApplication::translate("ccm", "백그라운드에서 계속"), QMessageBox::AcceptRole);
    QPushButton *quitButton = box.addButton(
        QCoreApplication::translate("ccm", "완전 종료"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = box.addButton(
        QCoreApplication::translate("ccm", "취소"), QMessageBox::RejectRole);

    box.setDefaultButton(trayButton);
    box.setEscapeButton(cancelButton);

    // QMessageBox 가 소유권을 가져간다.
    auto *remember = new QCheckBox(
        QCoreApplication::translate("ccm", "다시 묻지 않음"), &box);
    box.setCheckBox(remember);

    box.exec();

    const QAbstractButton *clicked = box.clickedButton();
    if (clicked == quitButton) {
        if (rememberChoice) {
            *rememberChoice = remember->isChecked();
        }
        return CloseChoice::Quit;
    }
    if (clicked == trayButton) {
        if (rememberChoice) {
            *rememberChoice = remember->isChecked();
        }
        return CloseChoice::KeepInTray;
    }

    // 취소이거나 창을 그냥 닫은 경우. 체크는 뜻이 없으므로 false 로 둔다.
    return CloseChoice::Cancel;
}

} // namespace ccm::ui
