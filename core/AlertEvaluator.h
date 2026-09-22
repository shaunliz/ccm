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
//  core/AlertEvaluator.h
//
//  보고와 임계치를 받아 "아직 알리지 않은 임계치 통과" 만 골라낸다.
//
//  왜 표식을 주고받는가
//   사용률은 한 번 올라가면 재설정 때까지 내려오지 않는다. 통과 사실만 보고
//   알리면 같은 알림이 재설정 시각까지 폴링마다 반복된다. 그래서 "무엇에 대해
//   이미 알렸는지" 를 (창, 임계치, 그 창의 재설정 시각) 으로 받아, 같은 조합이면
//   건너뛴다.
//
//  표식을 내부에 두지 않는 이유
//   프로그램을 다시 켜도 유지되어야 하므로 저장 대상이다. 순수 함수로 두고
//   보관은 호출하는 쪽(infra/Settings)에 맡긴다.
//
//  표식이 지워지는 경우 두 가지
//   1. 창의 재설정 시각이 바뀌었다 -> 새 구간이므로 다시 알린다.
//   2. 사용률이 그 임계치 아래로 내려갔다 -> 조건이 풀렸으므로 다시 넘으면 알린다.
//   사라진 창의 표식도 함께 버린다. 모델이 빠지면 그 설정이 남을 이유가 없다.
// ============================================================================
#ifndef CCM_CORE_ALERTEVALUATOR_H
#define CCM_CORE_ALERTEVALUATOR_H

#include "core/AlertTypes.h"
#include "core/UsageTypes.h"

#include <QList>

namespace ccm::core {

struct EvaluationResult {
    QList<AlertEvent> events;   ///< 이번에 새로 알릴 것. 없으면 빈 목록.
    FiredMarkMap marks;         ///< 다음 호출에 그대로 넘길 표식.
    AlertLevel highestLevel = AlertLevel::Normal;   ///< 현재 넘고 있는 수준 중 최고.
};

class AlertEvaluator
{
public:
    AlertEvaluator() = delete;

    /// \param report    이번에 받은 보고.
    /// \param config    창별 임계치.
    /// \param previous  지난번 호출이 돌려준 marks. 처음이면 빈 맵.
    ///
    /// 처음 호출(previous 가 빈 맵)일 때는 이미 넘고 있던 임계치도 알린다.
    /// 프로그램을 켠 직후 이미 위험한 상태라면 알리는 편이 옳다.
    static EvaluationResult evaluate(const UsageReport &report,
                                     const ThresholdConfig &config,
                                     const FiredMarkMap &previous);
};

} // namespace ccm::core

#endif // CCM_CORE_ALERTEVALUATOR_H
