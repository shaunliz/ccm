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

#include "core/AlertTypes.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QList>
#include <QtGlobal>
#include <QTimeZone>
#include <QUuid>

#include <algorithm>

namespace ccm::core {
namespace {

constexpr auto kTokenNotice = "notice";
constexpr auto kTokenWarning = "warn";
constexpr auto kTokenCritical = "critical";
constexpr auto kTokenNormal = "normal";

/// 재설정 시각이 없는 창에 쓸 표식. (nimbus_quill 처럼 resets_at 이 null 인 창)
constexpr auto kNoResetToken = "-";

/// 설정이 없는 지표에 적용할 기본 임계치.
constexpr double kDefaultNoticePercent = 70.0;

/// 재설정 시각을 표식으로 묶는 단위. (resetToken 참조)
constexpr qint64 kSecondsPerMinute = 60;

} // namespace

QList<AlertLevel> selectableAlertLevels()
{
    return QList<AlertLevel>{AlertLevel::Notice, AlertLevel::Warning, AlertLevel::Critical};
}

QString alertLevelName(AlertLevel level)
{
    switch (level) {
    case AlertLevel::Normal:
        return QCoreApplication::translate("ccm", "정상");
    case AlertLevel::Notice:
        return QCoreApplication::translate("ccm", "알림");
    case AlertLevel::Warning:
        return QCoreApplication::translate("ccm", "경고");
    case AlertLevel::Critical:
        return QCoreApplication::translate("ccm", "위험");
    }
    return QString();
}

QString alertLevelToken(AlertLevel level)
{
    switch (level) {
    case AlertLevel::Normal:
        return QLatin1String(kTokenNormal);
    case AlertLevel::Notice:
        return QLatin1String(kTokenNotice);
    case AlertLevel::Warning:
        return QLatin1String(kTokenWarning);
    case AlertLevel::Critical:
        return QLatin1String(kTokenCritical);
    }
    return QLatin1String(kTokenWarning);
}

bool alertLevelFromToken(const QString &token, AlertLevel *out)
{
    const QString value = token.trimmed().toLower();
    if (value == QLatin1String(kTokenNotice)) {
        *out = AlertLevel::Notice;
        return true;
    }
    if (value == QLatin1String(kTokenWarning)) {
        *out = AlertLevel::Warning;
        return true;
    }
    if (value == QLatin1String(kTokenCritical)) {
        *out = AlertLevel::Critical;
        return true;
    }
    if (value == QLatin1String(kTokenNormal)) {
        *out = AlertLevel::Normal;
        return true;
    }
    return false;
}

ThresholdRules defaultThresholdRules(UsageWindowKind kind)
{
    // 지표 종류와 무관하게 "70% 에 알림" 한 줄만 둔다.
    //
    // 처음부터 세 단계를 다 깔아 두면 켜는 순간 지표마다 알림이 여러 건 쌓여
    // 목록이 시끄럽다. 필요한 단계는 사용자가 설정 화면에서 더한다.
    //
    // 종류 인자는 남겨 둔다. 호출하는 쪽이 창의 종류로 기본값을 찾는 구조이고,
    // 나중에 종류마다 다시 달라질 수 있다.
    Q_UNUSED(kind);
    return ThresholdRules{ThresholdRule{kDefaultNoticePercent, AlertLevel::Notice}};
}

void sortThresholdRules(ThresholdRules *rules)
{
    std::sort(rules->begin(), rules->end(),
              [](const ThresholdRule &lhs, const ThresholdRule &rhs) {
                  if (!qFuzzyCompare(lhs.percent, rhs.percent)) {
                      return lhs.percent < rhs.percent;
                  }
                  return static_cast<int>(lhs.level) < static_cast<int>(rhs.level);
              });
}

AlertLevel levelAt(const ThresholdRules &rules, double usedPercent)
{
    AlertLevel level = AlertLevel::Normal;
    for (const ThresholdRule &rule : rules) {
        if (!rule.enabled) {
            continue;
        }
        if (usedPercent >= rule.percent && rule.level > level) {
            level = rule.level;
        }
    }
    return level;
}

ThresholdRules ThresholdConfig::rulesFor(const UsageWindow &window) const
{
    const auto it = m_byKey.constFind(window.key);
    if (it != m_byKey.constEnd()) {
        return it.value();
    }
    // 설정이 없는 창은 종류 기본값으로 동작한다. 모델이 새로 생겼을 때
    // 알림이 조용히 빠지지 않게 하기 위함이다.
    return defaultThresholdRules(window.kind);
}

ThresholdRules ThresholdConfig::configuredRules(const QString &windowKey) const
{
    return m_byKey.value(windowKey);
}

void ThresholdConfig::setRules(const QString &windowKey, const ThresholdRules &rules)
{
    ThresholdRules sorted = rules;
    sortThresholdRules(&sorted);
    m_byKey.insert(windowKey, sorted);
}

QList<QString> ThresholdConfig::configuredKeys() const
{
    QList<QString> keys = m_byKey.keys();
    keys.sort();
    return keys;
}

QString AlertEvent::message() const
{
    return QCoreApplication::translate("ccm", "%1 사용률 %2% (%3 임계치 %4%)")
        .arg(windowLabel)
        .arg(usedPercent, 0, 'f', 1)
        .arg(alertLevelName(level))
        .arg(thresholdPercent, 0, 'f', 0);
}

QString AlertEvent::makeId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString firedMarkKey(const QString &windowKey, double thresholdPercent)
{
    // 임계치는 소수 한 자리까지만 구분한다. 그보다 잘게 쪼갤 이유가 없고,
    // 부동소수 표기가 흔들리면 같은 규칙을 다른 것으로 보게 된다.
    return QStringLiteral("%1@%2").arg(windowKey).arg(thresholdPercent, 0, 'f', 1);
}

QString resetToken(const QDateTime &resetsAt)
{
    if (!resetsAt.isValid()) {
        return QLatin1String(kNoResetToken);
    }

    // 분 단위로 묶어서 쓴다.
    //
    // 초 단위까지 쓰면 구간을 더 정확히 가른다고 생각했지만 사실이 아니었다.
    // 서버는 같은 재설정 경계를 13:59:59 로도 14:00:00 으로도 준다. 그 1초
    // 차이가 "새 구간" 으로 읽혀, 사용률이 74% 로 그대로인데도 조회할 때마다
    // 같은 알림이 다시 나갔다. 창이 1분 안에 두 번 재설정되는 일은 없으므로
    // 분까지만으로 구간은 충분히 갈리고, 화면도 분까지만 보여 준다.
    const qint64 secs = resetsAt.toUTC().toSecsSinceEpoch();
    const qint64 rounded = ((secs + kSecondsPerMinute / 2) / kSecondsPerMinute) * kSecondsPerMinute;
    const QDateTime normalized = QDateTime::fromSecsSinceEpoch(rounded, QTimeZone::UTC);

    // 초를 뺀 형식이라 예전에 저장된 초 단위 표식과 섞이지 않는다.
    return normalized.toString(QStringLiteral("yyyy-MM-ddThh:mm")) + QLatin1Char('Z');
}

} // namespace ccm::core
