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
//  ui/HealthScanner.h
//
//  프로그램이 살아 있음을 보여 주는 띠. 빛 하나가 좌우로 오간다.
//
//  왜 필요한가
//   트레이에 상주하며 몇 분에 한 번 조회하는 프로그램이다. 화면을 열어도
//   숫자가 그대로면 "도는 중인데 값이 그대로인 것" 인지 "멈춘 것" 인지 알 수
//   없다. 움직이는 것이 하나 있으면 그 물음이 사라진다.
//
//  통로가 둘이고 서로 겹치지 않는다
//
//   색    = 사용량 수준 (정상 파랑 / 알림 초록 / 경고 주황 / 위험 빨강)
//   움직임 + 아이콘 = 프로그램 상태 (도는 중 / 조회 중 / 주의 / 오류)
//
//   한 통로에 두 가지 뜻을 실으면 색이 답을 주지 못한다. 예전에는 램프 색이
//   프로그램 상태였는데, 그러면 "훅 미등록" 의 주황과 "경고 도달" 의 주황이,
//   "조회 실패" 의 빨강과 "위험 도달" 의 빨강이 겹쳤다. 빨간 램프를 보고도
//   글자를 읽어야 무슨 일인지 알 수 있었다.
//
//   그래서 색은 사용량에만 준다. 게이지와 트레이 아이콘이 쓰는 색과 같은 뜻이
//   되므로, 화면 어디를 보든 빨강은 언제나 "사용량이 위험하다" 이다.
//
//   프로그램 상태는 움직임이 맡는다. 오류일 때 오가지 않고 띠 전체가 숨 쉬듯
//   깜박이는 것은, "움직이지 않는다" 는 것 자체가 멈췄다는 뜻으로 읽히기
//   때문이다. 주의와 오류는 왼쪽에 아이콘(세모 / 엑스)도 함께 둔다.
//
//  창이 보이지 않으면 멈춘다
//   트레이로 내려가 있는 동안 30ms 마다 다시 그릴 이유가 없다. 아무도 보지 않는
//   그림에 전원을 쓰지 않는다.
// ============================================================================
#ifndef CCM_UI_HEALTHSCANNER_H
#define CCM_UI_HEALTHSCANNER_H

#include "core/AlertTypes.h"

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QColor;
class QPixmap;
class QTimer;
QT_END_NAMESPACE

namespace ccm::ui {

/// 프로그램의 건강 상태.
enum class HealthState {
    Ok,        ///< 마지막 조회가 성공했고 걸리는 것이 없다.
    Busy,      ///< 지금 조회 중이다.
    Warning,   ///< 돌기는 하는데 손볼 것이 있다. (예: statusline 훅 미등록)
    Error      ///< 마지막 조회가 실패했다.
};

class HealthScanner : public QWidget
{
    Q_OBJECT

public:
    explicit HealthScanner(QWidget *parent = nullptr);

    /// 프로그램 상태를 바꾼다. 움직임과 아이콘이 이것을 따른다.
    /// \param message 띠 오른쪽에 적을 짧은 말. ("정상", "조회 실패" 등)
    /// \param detail  도구 설명에 적을 자세한 사정. 비우면 message 를 쓴다.
    void setStatus(HealthState state, const QString &message,
                   const QString &detail = QString());

    /// 사용량 수준을 바꾼다. 색이 이것을 따른다.
    void setUsageLevel(ccm::core::AlertLevel level);

    HealthState state() const { return m_state; }
    ccm::core::AlertLevel usageLevel() const { return m_usageLevel; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void step();
    void applyPace();

    /// 오른쪽 글자의 색. 그 글자가 무엇을 말하느냐에 따라 달라진다.
    QColor messageColor() const;

    /// 프로그램 상태를 알리는 아이콘. 알릴 것이 없으면 빈 그림.
    QPixmap stateIcon(int size) const;

    HealthState m_state = HealthState::Ok;
    ccm::core::AlertLevel m_usageLevel = ccm::core::AlertLevel::Normal;
    QString m_message;

    QTimer *m_timer = nullptr;

    /// 0 에서 2π 를 도는 위상. 빛의 자리와 깜박임 모두 이것으로 정한다.
    double m_phase = 0.0;

    /// 한 걸음에 나아갈 위상. 상태에 따라 달라진다.
    double m_stepSize = 0.0;
};

} // namespace ccm::ui

#endif // CCM_UI_HEALTHSCANNER_H
