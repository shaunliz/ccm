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
//  ui/UsagePopup.h
//
//  트레이 아이콘을 누르면 뜨는 작은 팝업. 게이지 세 개와 마지막 갱신 시각만
//  보여 준다.
//
//  창을 띄우는 것과 다른 점
//   메인 창은 차트와 이력까지 있어 무겁다. "지금 몇 퍼센트인가" 를 보려고
//   창을 띄우고 싶지는 않다. 팝업은 테두리 없는 위젯으로 트레이 옆에 뜨고,
//   초점을 잃으면 스스로 닫힌다.
//
//  표시 즉시 새로 조회한다. 다만 조회는 1~2초 걸리므로 먼저 마지막 값을
//  그려 두고, 응답이 오면 갈아 끼운다. 팝업이 빈 채로 떠 있지 않게 하기 위함이다.
// ============================================================================
#ifndef CCM_UI_USAGEPOPUP_H
#define CCM_UI_USAGEPOPUP_H

#include <QList>
#include <QPoint>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ccm::ui {

class UsageGauge;
class UsageService;

class UsagePopup : public QWidget
{
    Q_OBJECT

public:
    explicit UsagePopup(UsageService *service, QWidget *parent = nullptr);

    /// 화면 좌표 anchor 근처에 띄운다. 화면 밖으로 나가지 않게 맞춘다.
    void showNear(const QPoint &anchor);

signals:
    void mainWindowRequested();

private:
    void rebuild();
    void updateStatusLine();

    UsageService *m_service = nullptr;
    QVBoxLayout *m_gaugeLayout = nullptr;
    QList<UsageGauge *> m_gauges;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_openButton = nullptr;
};

} // namespace ccm::ui

#endif // CCM_UI_USAGEPOPUP_H
