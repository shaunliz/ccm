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

#include "ui/StandardButtons.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QList>
#include <QMessageBox>
#include <QPushButton>

namespace ccm::ui {
namespace {

/// 번역 컨텍스트. core/infra 와 같은 자리를 쓴다. 버튼 글자는 특정 창의 것이
/// 아니라 프로그램 전체가 함께 쓰는 낱말이다.
QString text(const char *source)
{
    return QCoreApplication::translate("ccm", source);
}

/// 표준 버튼 하나와 그 자리에 넣을 글자.
struct Label {
    QDialogButtonBox::StandardButton button;
    const char *source;
};

const QList<Label> &labels()
{
    // 번역된 글자를 담아 두지 않는다. 여기서 캐시하면 번역기가 나중에 설치될 때
    // 옛 글자가 남는다. (core/TimeFormat.cpp 에 같은 사정을 적어 두었다)
    static const QList<Label> table{
        {QDialogButtonBox::Ok, "확인"},
        {QDialogButtonBox::Save, "저장"},
        {QDialogButtonBox::Cancel, "취소"},
        {QDialogButtonBox::Close, "닫기"},
        {QDialogButtonBox::Yes, "예"},
        {QDialogButtonBox::No, "아니요"},
        {QDialogButtonBox::Apply, "적용"},
        {QDialogButtonBox::Reset, "되돌리기"},
        {QDialogButtonBox::RestoreDefaults, "기본값으로"},
    };
    return table;
}

} // namespace

void localizeButtons(QDialogButtonBox *box)
{
    if (!box) {
        return;
    }
    for (const Label &label : labels()) {
        if (QPushButton *button = box->button(label.button)) {
            button->setText(text(label.source));
        }
    }
}

bool askYesNo(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setIcon(QMessageBox::Question);
    box.setText(message);

    QPushButton *yes = box.addButton(text("예"), QMessageBox::YesRole);
    QPushButton *no = box.addButton(text("아니요"), QMessageBox::NoRole);

    // 확인을 묻는 자리는 대개 되돌릴 수 없는 일이다. Enter 와 Esc 모두
    // 아무 일도 일어나지 않는 쪽으로 걸어 둔다.
    box.setDefaultButton(no);
    box.setEscapeButton(no);

    box.exec();
    return box.clickedButton() == yes;
}

void showInfo(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setIcon(QMessageBox::Information);
    box.setText(message);
    box.addButton(text("확인"), QMessageBox::AcceptRole);
    box.exec();
}

void showWarning(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setIcon(QMessageBox::Warning);
    box.setText(message);
    box.addButton(text("확인"), QMessageBox::AcceptRole);
    box.exec();
}

} // namespace ccm::ui
