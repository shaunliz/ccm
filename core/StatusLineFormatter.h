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
//  core/StatusLineFormatter.h
//
//  UsageSnapshot 을 Claude Code 상태줄 한 줄로 만든다.
//  표현 계층과 파싱 계층을 분리하기 위해 순수 함수로 유지한다.
//
//  나오는 모습
//   ClaudeCodeMonitor - Session : 33% | Model : 44% | Fable : 22%
//
//   앞의 이름표는 이 줄이 어디서 온 것인지 밝히는 자리다. 상태줄은 여러
//   프로그램이 나눠 쓸 수 있는 자리가 아니라 한 명령이 통째로 만드는 줄이므로,
//   적어 두지 않으면 나중에 이 글자의 출처를 알 수 없다.
//
//  모델별 사용률은 밖에서 받는다
//   Claude Code 가 훅에 넘기는 payload 에는 five_hour 와 seven_day 둘뿐이다.
//   "Fable" 같은 모델별 값은 CLI 조회(get_usage) 응답에만 있고, 그것은 위젯
//   앱이 가지고 있다. 그래서 앱이 남긴 값을 호출하는 쪽이 읽어 넘겨준다.
//   (infra/StatusLineCache)
//
//  색
//   터미널이 ANSI 이스케이프를 그대로 해석하므로 색을 입힐 수 있다. 다만 색을
//   모르는 자리에 내보내면 `[38;2;...m` 같은 글자가 그대로 보인다. 그래서 끄는
//   길을 함께 둔다. (ccm_probe 의 --no-color, 그리고 NO_COLOR 환경 변수)
// ============================================================================
#ifndef CCM_CORE_STATUSLINEFORMATTER_H
#define CCM_CORE_STATUSLINEFORMATTER_H

#include "core/UsageTypes.h"

#include <QList>
#include <QString>

namespace ccm::core {

/// 상태줄 뒤에 덧붙일 값 하나. 모델별 사용률에 쓴다.
struct StatusLineExtra {
    QString label;          ///< 짧은 이름. ("Fable")
    double usedPercent = 0.0;
};

class StatusLineFormatter
{
public:
    StatusLineFormatter() = delete;

    /// 상태줄에 넣을 한 줄 요약. 표시할 값이 하나도 없으면 빈 문자열.
    ///
    /// 빈 문자열일 때 이름표만 남기지 않는 이유는, 숫자 없는 이름표가 상태줄을
    /// 차지하고 있으면 무엇이 잘못됐는지 알리지도 못하면서 자리만 먹기 때문이다.
    ///
    /// \param extras  모델별 사용률. 뒤에 순서대로 붙는다. 비어도 된다.
    /// \param colored 참이면 ANSI 색을 입힌다.
    static QString format(const UsageSnapshot &snapshot,
                          const QList<StatusLineExtra> &extras = {},
                          bool colored = false);
};

} // namespace ccm::core

#endif // CCM_CORE_STATUSLINEFORMATTER_H
