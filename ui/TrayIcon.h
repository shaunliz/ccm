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
//  ui/TrayIcon.h
//
//  트레이 상주. 아이콘, 도구 설명, 윈도우 알림, 우클릭 메뉴를 맡는다.
//
//  아이콘에 숫자를 그린다
//   사용률을 보려고 마우스를 올리거나 누르는 것은 한 단계 더 드는 일이다.
//   가장 심각한 창의 사용률을 아이콘에 직접 그려 두면 작업 표시줄만 봐도
//   값이 보인다. 색은 알림 수준을 따른다.
//
//  마우스를 올릴 때와 누를 때
//   올릴 때(도구 설명)는 마지막 조회값을 즉시 보여 준다. 조회가 1~2초 걸리므로
//   올릴 때마다 claude 를 띄우면 쓸 수 없다. 누를 때는 팝업을 열고 그때 새로
//   조회한다.
// ============================================================================
#ifndef CCM_UI_TRAYICON_H
#define CCM_UI_TRAYICON_H

#include "core/AlertTypes.h"

#include <QList>
#include <QObject>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace ccm::ui {

class UsagePopup;
class UsageService;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(UsageService *service, QObject *parent = nullptr);
    ~TrayIcon() override;

    /// 트레이를 쓸 수 있는 환경인지. 쓸 수 없으면 창만으로 동작해야 한다.
    static bool isAvailable();

    void show();

signals:
    void mainWindowRequested();
    void settingsRequested();
    void alertLogRequested();
    void quitRequested();

private:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onAlertsRaised(const QList<ccm::core::AlertEvent> &events);
    void updateIconAndTooltip();
    void updateAutoStartAction();

    UsageService *m_service = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    UsagePopup *m_popup = nullptr;
    QAction *m_autoStartAction = nullptr;
    QAction *m_refreshAction = nullptr;
};

} // namespace ccm::ui

#endif // CCM_UI_TRAYICON_H
