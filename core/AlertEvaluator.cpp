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

#include "core/AlertEvaluator.h"

#include "infra/Logger.h"

namespace ccm::core {

EvaluationResult AlertEvaluator::evaluate(const UsageReport &report,
                                          const ThresholdConfig &config,
                                          const FiredMarkMap &previous)
{
    EvaluationResult result;

    const QDateTime now = report.fetchedAt.isValid() ? report.fetchedAt
                                                     : QDateTime::currentDateTimeUtc();

    for (const UsageWindow &window : report.windows) {
        const ThresholdRules rules = config.rulesFor(window);
        const QString token = resetToken(window.resetsAt);

        // 지금 수준은 게이지와 같은 함수로 구한다. 여기서 따로 세면 화면과
        // 알림이 다른 답을 내놓게 된다.
        const AlertLevel windowLevel = levelAt(rules, window.usedPercent);
        if (windowLevel > result.highestLevel) {
            result.highestLevel = windowLevel;
        }

        for (const ThresholdRule &rule : rules) {
            if (!rule.enabled) {
                // 꺼 둔 규칙이다. 표식도 남기지 않고 수준에도 세지 않는다.
                // 차트에는 여전히 그릴 수 있지만 그것은 그리기 쪽 일이다.
                continue;
            }

            const QString markKey = firedMarkKey(window.key, rule.percent);

            if (window.usedPercent < rule.percent) {
                // 조건이 풀렸다. 표식을 남기지 않아 다음에 다시 넘으면 알린다.
                continue;
            }

            const QString before = previous.value(markKey);
            result.marks.insert(markKey, token);

            if (before == token) {
                // 이 구간에서 이미 알렸다.
                continue;
            }

            AlertEvent event;
            event.id = AlertEvent::makeId();
            event.at = now;
            event.windowKey = window.key;
            event.windowLabel = window.label;
            event.kind = window.kind;
            event.level = rule.level;
            event.usedPercent = window.usedPercent;
            event.thresholdPercent = rule.percent;
            result.events.append(event);

            qCInfo(ccmCore).noquote()
                << QStringLiteral("임계치 통과: %1 %2% >= %3% (%4)")
                       .arg(window.label)
                       .arg(window.usedPercent, 0, 'f', 1)
                       .arg(rule.percent, 0, 'f', 0)
                       .arg(alertLevelName(rule.level));
        }
    }

    return result;
}

} // namespace ccm::core
