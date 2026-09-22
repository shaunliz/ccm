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
//  core/WindowLabel.h
//
//  지표 이름을 짧게 줄인다.
//
//  왜 따로 두는가
//   서버가 주는 이름은 "현재 세션", "모든 모델 (주간)", "Fable (주간)" 처럼
//   설명을 겸한다. 게이지나 요약처럼 한 줄을 통째로 쓰는 자리에서는 그 편이
//   낫지만, 차트 범례는 여러 이름이 가로로 늘어서는 자리다. 긴 이름 세 개에
//   임계치 선 세 개까지 붙으면 범례가 두 줄, 세 줄로 접히고 차트가 눌린다.
//
//   그래서 좁은 자리 전용의 짧은 이름을 따로 만든다. 세션 / 모델 / Fable.
//   원래 이름을 바꾸는 것이 아니므로 넓은 자리는 그대로 둔다.
// ============================================================================
#ifndef CCM_CORE_WINDOWLABEL_H
#define CCM_CORE_WINDOWLABEL_H

#include <QString>

namespace ccm::core {

/// 차트 범례처럼 자리가 좁은 곳에 쓸 짧은 지표 이름.
///
/// \param key       창 키. ("five_hour", "seven_day", "model:Fable")
/// \param fullLabel 표시용 이름. 모르는 키일 때만 쓴다. 없으면 키를 쓴다.
QString shortWindowLabel(const QString &key, const QString &fullLabel = QString());

} // namespace ccm::core

#endif // CCM_CORE_WINDOWLABEL_H
