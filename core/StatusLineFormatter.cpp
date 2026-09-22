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

#include "core/StatusLineFormatter.h"

#include <QLatin1String>
#include <QStringList>

namespace ccm::core {
namespace {

/// 상태줄 맨 앞에 붙는 이름표.
///
/// 번역하지 않는다. 상태줄은 터미널 한 줄이고, 그 터미널의 나머지 글자(모델
/// 이름, 경로)가 영문이다. 여기만 한글이면 오히려 튄다.
constexpr auto kPrefix = "ClaudeCodeMonitor";

/// 이름표와 값 사이, 값과 값 사이의 구분자.
constexpr auto kPrefixSeparator = " - ";
constexpr auto kValueSeparator = " | ";

// ----------------------------------------------------------------------------
//  색
//
//  검은 바탕을 기준으로 고른다. 터미널 기본 16색의 파랑(#0000AA 계열)은 검은
//  바탕에서 거의 읽히지 않으므로 쓰지 않고, 24비트 색으로 명도를 올려 잡는다.
//  채도는 낮추는 쪽이다. 완전히 채도를 올린 원색은 검은 바탕에서 번져 보이고
//  오래 보기 힘들다.
//
//  구분자만 회색으로 낮춘다. 셋 다 또렷하면 눈이 어디를 먼저 볼지 정하지
//  못한다. 값이 앞에 서고 구분자는 뒤로 물러나야 한 줄이 세 덩이로 읽힌다.
// ----------------------------------------------------------------------------

/// 파랑 #58A6FF. 이름표와 Session 이 함께 쓴다.
constexpr auto kColorBlue = "\033[38;2;88;166;255m";

/// 보라 #BC8CFF. Model.
constexpr auto kColorPurple = "\033[38;2;188;140;255m";

/// 초록 #4ADE80. 모델별 값(Fable 등).
constexpr auto kColorGreen = "\033[38;2;74;222;128m";

/// 회색 #6E7681. 구분자.
constexpr auto kColorDim = "\033[38;2;110;118;129m";

constexpr auto kReset = "\033[0m";

QString paint(const QString &text, const char *color, bool colored)
{
    if (!colored) {
        return text;
    }
    return QLatin1String(color) + text + QLatin1String(kReset);
}

QString entry(const QString &label, double percent, const char *color, bool colored)
{
    return paint(QStringLiteral("%1 : %2%").arg(label).arg(percent, 0, 'f', 0),
                 color, colored);
}

} // namespace

QString StatusLineFormatter::format(const UsageSnapshot &snapshot,
                                    const QList<StatusLineExtra> &extras,
                                    bool colored)
{
    if (!snapshot.valid) {
        return QString();
    }

    QStringList parts;
    if (snapshot.fiveHour.present) {
        parts << entry(QStringLiteral("Session"), snapshot.fiveHour.usedPercent,
                       kColorBlue, colored);
    }
    if (snapshot.sevenDay.present) {
        // seven_day 는 "모든 모델 주간" 이다. 뒤에 붙는 모델별 값과 나란히 놓고
        // 보면 Model 이 곧 그 묶음이라는 것이 읽힌다.
        parts << entry(QStringLiteral("Model"), snapshot.sevenDay.usedPercent,
                       kColorPurple, colored);
    }
    for (const StatusLineExtra &extra : extras) {
        parts << entry(extra.label, extra.usedPercent, kColorGreen, colored);
    }

    if (parts.isEmpty()) {
        return QString();
    }

    return paint(QLatin1String(kPrefix), kColorBlue, colored)
           + paint(QLatin1String(kPrefixSeparator), kColorDim, colored)
           + parts.join(paint(QLatin1String(kValueSeparator), kColorDim, colored));
}

} // namespace ccm::core
