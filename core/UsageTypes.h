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
//  core/UsageTypes.h
//
//  사용량 모델. 출처가 둘이므로 모델도 둘이다.
//
//   UsageSnapshot : statusline 훅이 남긴 스냅샷. 창이 두 개로 고정이다.
//                   (five_hour, seven_day)
//   UsageReport   : claude CLI 의 get_usage 응답. 창의 개수와 종류가
//                   플랜과 계정에 따라 달라지므로 목록으로 담는다.
//
//  주의: 여기에 없는 필드(모델명, 컨텍스트, 비용 등)가 버려지는 것은 아니다.
//        SnapshotStore 가 statusline 원본 payload 를 그대로 보존하므로,
//        필요해지면 이 구조체와 파서에 추가하면 된다.
// ============================================================================
#ifndef CCM_CORE_USAGETYPES_H
#define CCM_CORE_USAGETYPES_H

#include <QDateTime>
#include <QList>
#include <QString>

namespace ccm::core {

/// 단일 한도 창의 사용률.
struct RateLimitWindow {
    bool present = false;        ///< 스냅샷에 해당 필드가 존재했는지 여부.
    double usedPercent = 0.0;    ///< 0 ~ 100.
};

/// 한 시점의 사용량.
struct UsageSnapshot {
    bool valid = false;
    QDateTime capturedAt;        ///< UTC. 스냅샷이 기록된 시각.
    RateLimitWindow fiveHour;
    RateLimitWindow sevenDay;
};

// ----------------------------------------------------------------------------
//  get_usage 응답 모델
// ----------------------------------------------------------------------------

/// 한도 창의 종류.
///
/// 화면과 임계치는 "현재 세션 / 모든 모델 / 모델별" 세 갈래로만 구분하면 된다.
/// 모델별 창은 개수가 서버 사정에 따라 달라지므로 종류로 묶어 다룬다.
enum class UsageWindowKind {
    Session,      ///< five_hour. 현재 세션 (5시간)
    WeeklyAll,    ///< seven_day. 주간 전체 (모든 모델)
    WeeklyModel   ///< model_scoped[] 및 seven_day_sonnet / seven_day_opus
};

/// get_usage 가 돌려주는 한도 창 하나.
///
/// RateLimitWindow 와 달리 이름과 재설정 시각을 함께 가진다. 모델별 주간 창은
/// 서버가 이름("Fable")까지 내려주므로, 클라이언트가 모델 목록을 알 필요가 없다.
struct UsageWindow {
    QString key;                 ///< 이력과 임계치가 쓰는 안정된 식별자.
                                 ///  ("five_hour", "seven_day", "model:Fable")
    QString label;               ///< 표시용 이름. 모델별 창은 서버가 준 display_name.
    UsageWindowKind kind = UsageWindowKind::WeeklyModel;
    double usedPercent = 0.0;    ///< 0 ~ 100.
    QDateTime resetsAt;          ///< UTC. 서버가 주지 않으면 invalid.
};

/// get_usage 응답 한 건.
struct UsageReport {
    bool valid = false;
    bool rateLimitsAvailable = false;  ///< false 면 플랜 한도가 적용되지 않는 계정이다.
                                       ///  (API 키, Bedrock, Vertex 등)
    QString subscriptionType;          ///< "pro", "max", "team", "enterprise". 없으면 빈 문자열.
    QDateTime fetchedAt;               ///< UTC. 이 보고를 받은 시각.
    QList<UsageWindow> windows;        ///< 세션 창, 주간 전체 창, 모델별 주간 창 순서.

    /// key 로 창을 찾는다. 없으면 nullptr.
    const UsageWindow *find(const QString &windowKey) const
    {
        for (const UsageWindow &window : windows) {
            if (window.key == windowKey) {
                return &window;
            }
        }
        return nullptr;
    }
};

} // namespace ccm::core

#endif // CCM_CORE_USAGETYPES_H
