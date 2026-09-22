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
//  core/AlertTypes.h
//
//  임계치와 알림 모델.
//
//  임계치는 지표마다 여러 개(N개) 둘 수 있다
//   "현재 세션 70% 에 한 번, 85% 에 한 번, 95% 에 한 번" 처럼 단계를 쌓는 것이
//   실제로 원하는 동작이다. 그래서 지표 하나에 규칙 목록이 붙는다.
//
//  규칙을 끄는 스위치가 있다
//   처음에는 두지 않았다. 쓰지 않을 규칙은 지우면 된다고 보았기 때문이다.
//   그런데 규칙은 차트에 점선으로 그릴 대상이기도 하다. "알리지는 말고 선만
//   보고 싶다" 와 "알리되 선은 긋지 말라" 가 둘 다 있을 수 있으므로, 끄고
//   켜는 일과 그리고 말고가 서로 다른 스위치여야 한다.
//
//  규칙은 창 키에 붙고, 없으면 종류 기본값을 쓴다
//   화면에는 "Fable (주간)" 처럼 실제 창 이름이 보여야 한다. 그래서 설정은 창
//   키(`model:Fable`)에 붙인다. 다만 모델별 창은 계정 사정에 따라 새로 생길 수
//   있으므로, 설정이 없는 창은 종류(세션 / 모든 모델 / 모델별) 기본값으로
//   동작한다. 모델이 하나 늘었을 때 알림이 조용히 빠지는 것을 막기 위함이다.
//
//  한 번 알린 것은 다시 알리지 않는다
//   사용률은 한 번 올라가면 재설정 때까지 내려오지 않는다. 그래서 "무엇에 대해
//   알렸는지" 를 (창, 임계치, 그 창의 재설정 시각) 으로 기억한다. 같은 조합이면
//   다시 알리지 않고, 재설정되어 새 구간이 시작되면 다시 알린다.
// ============================================================================
#ifndef CCM_CORE_ALERTTYPES_H
#define CCM_CORE_ALERTTYPES_H

#include "core/UsageTypes.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

namespace ccm::core {

/// 알림 수준. 값이 클수록 심각하다. 비교에 쓰므로 순서를 바꾸지 않는다.
///
/// Normal 은 규칙에 쓸 수 없다. "임계치를 하나도 넘지 않은 상태" 를 나타내는
/// 내부 값이며, 게이지 색과 트레이 아이콘 색에만 쓰인다. 사용자가 고르는 수준은
/// Notice / Warning / Critical 셋이다.
enum class AlertLevel {
    Normal = 0,
    Notice = 1,     ///< 알림. 파란 느낌표.
    Warning = 2,    ///< 경고. 노란 세모.
    Critical = 3    ///< 위험. 빨간 엑스표.
};

/// 사용자가 규칙에 지정할 수 있는 수준. 화면의 선택 목록이 이 순서를 쓴다.
QList<AlertLevel> selectableAlertLevels();

QString alertLevelName(AlertLevel level);

/// 저장과 복원에 쓰는 짧은 이름. 설정 파일을 사람이 읽을 수 있게 한다.
QString alertLevelToken(AlertLevel level);
bool alertLevelFromToken(const QString &token, AlertLevel *out);

// ----------------------------------------------------------------------------
//  임계치
// ----------------------------------------------------------------------------

/// 임계치 한 줄. "사용률이 percent 이상이면 level 로 알린다".
struct ThresholdRule {
    double percent = 70.0;
    AlertLevel level = AlertLevel::Warning;

    /// 끄면 알리지 않는다. 차트에 점선으로 그리는 것과는 별개다.
    bool enabled = true;

    /// 같은 규칙인가.
    ///
    /// enabled 는 견주지 않는다. 알린 것을 기억하는 표식이 (창, 사용률) 로만
    /// 잡히므로, 사용률과 수준이 같은 두 줄은 켜짐과 무관하게 한 줄로 다뤄야
    /// 앞뒤가 맞는다. (설정 저장 때 같은 줄을 걸러 내는 데 쓰인다)
    bool operator==(const ThresholdRule &other) const
    {
        return qFuzzyCompare(percent, other.percent) && level == other.level;
    }
};

/// 지표 하나의 임계치 목록. 개수에 제한을 두지 않는다.
using ThresholdRules = QList<ThresholdRule>;

/// 창 종류별 기본 규칙. 설정이 없는 창이 이것으로 동작한다.
ThresholdRules defaultThresholdRules(UsageWindowKind kind);

/// 규칙을 사용률 오름차순으로 정렬한다. 화면과 저장 모두 이 순서를 쓴다.
void sortThresholdRules(ThresholdRules *rules);

/// 지금 사용률이 넘고 있는 규칙 중 가장 심각한 수준. 넘는 것이 없으면 Normal.
///
/// **꺼 둔 규칙(enabled == false)은 세지 않는다.**
///
/// 이 계산을 한 곳에 모아 두는 이유가 있다. 예전에는 알림을 판정하는 쪽과
/// 게이지 색을 정하는 쪽이 같은 계산을 따로 적고 있었고, 규칙을 끄는 스위치를
/// 더할 때 한쪽만 고쳐서 "알림은 멈췄는데 막대는 빨간 채로 남는" 일이
/// 생겼다. 판정이 하나면 그런 어긋남이 생길 자리가 없다.
///
/// 표식(이미 알렸는지)과는 무관하다. 이것은 "지금 상태" 다.
AlertLevel levelAt(const ThresholdRules &rules, double usedPercent);

/// 창 키 -> 규칙 목록. 비어 있는 창은 종류 기본값으로 동작한다.
class ThresholdConfig
{
public:
    /// 이 창에 적용할 규칙. 설정된 것이 없으면 종류 기본값.
    ThresholdRules rulesFor(const UsageWindow &window) const;

    /// 창 키에 직접 설정된 규칙만. 없으면 빈 목록.
    ThresholdRules configuredRules(const QString &windowKey) const;

    /// 규칙을 설정한다. 빈 목록을 넣으면 "이 창은 알리지 않음" 이 된다.
    void setRules(const QString &windowKey, const ThresholdRules &rules);

    /// 설정이 있는 창 키 전부.
    QList<QString> configuredKeys() const;

    bool isEmpty() const { return m_byKey.isEmpty(); }

private:
    QHash<QString, ThresholdRules> m_byKey;
};

// ----------------------------------------------------------------------------
//  알림
// ----------------------------------------------------------------------------

/// 임계치를 넘은 사건 하나. 알림 목록에 그대로 쌓인다.
struct AlertEvent {
    QString id;                  ///< 목록에서 하나씩 지우기 위한 고유 식별자.
    QDateTime at;                ///< UTC.
    QString windowKey;
    QString windowLabel;
    UsageWindowKind kind = UsageWindowKind::Session;
    AlertLevel level = AlertLevel::Warning;
    double usedPercent = 0.0;
    double thresholdPercent = 0.0;
    bool acknowledged = false;   ///< 사용자가 확인했는지.

    /// 알림 문구 한 줄.
    QString message() const;

    /// 새 사건에 붙일 식별자를 만든다.
    static QString makeId();
};

/// "무엇에 대해 이미 알렸는지" 를 기억하는 표식.
///
/// 키   : 창 키와 임계치를 합친 것.  (firedMarkKey 참조)
/// 값   : 그 알림을 낼 때 창의 재설정 시각을 나타내는 토큰.
///        재설정 시각이 바뀌면 새 구간이므로 다시 알린다.
///        토큰은 분 단위다. 초 단위로 두면 서버 값이 1초씩 흔들릴 때마다
///        새 구간으로 읽혀 같은 알림이 반복된다. (resetToken 참조)
using FiredMarkMap = QHash<QString, QString>;

QString firedMarkKey(const QString &windowKey, double thresholdPercent);

/// 창의 재설정 시각을 표식 값으로 쓸 문자열로. 시각이 없으면 고정 토큰.
///
/// 분 단위로 반올림해서 내놓는다("2026-09-15T14:00Z"). 서버가 같은 경계를
/// 13:59:59 과 14:00:00 사이로 오가며 주기 때문에, 초를 살려 두면 그 흔들림이
/// 그대로 중복 판정에 들어온다.
QString resetToken(const QDateTime &resetsAt);

} // namespace ccm::core

#endif // CCM_CORE_ALERTTYPES_H
