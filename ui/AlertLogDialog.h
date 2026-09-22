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

// ============================================================================
//  ui/AlertLogDialog.h
//
//  발생한 알림 목록을 관리한다.
//
//  윈도우 알림은 몇 초 뒤 사라지고, 알림 센터에서 지우면 그것으로 끝난다.
//  자리를 비운 동안 뜬 것을 나중에 보려면 프로그램이 따로 들고 있어야 한다.
//
//  확인과 삭제는 다른 동작이다
//   확인은 "봤다" 이고 목록에 남는다. 삭제는 목록에서 없앤다. 둘을 한 버튼으로
//   묶으면 이력을 남기려는 쪽과 치우려는 쪽 중 하나는 불편해진다.
//   미확인 줄은 굵게 표시해 한눈에 구분되게 한다.
// ============================================================================
#ifndef CCM_UI_ALERTLOGDIALOG_H
#define CCM_UI_ALERTLOGDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QTreeWidget;
QT_END_NAMESPACE

namespace ccm::ui {

class UsageService;

class AlertLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlertLogDialog(UsageService *service, QWidget *parent = nullptr);

    /// 목록을 다시 그린다. 알림이 추가되거나 바뀌면 서비스가 알려 준다.
    void reload();

private:
    QList<QString> selectedIds() const;
    void updateButtons();

    UsageService *m_service = nullptr;
    QTreeWidget *m_tree = nullptr;
    QLabel *m_summary = nullptr;
    QPushButton *m_acknowledgeButton = nullptr;
    QPushButton *m_acknowledgeAllButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_clearButton = nullptr;
};

} // namespace ccm::ui

#endif // CCM_UI_ALERTLOGDIALOG_H
