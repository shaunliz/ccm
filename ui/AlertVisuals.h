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
//  ui/AlertVisuals.h
//
//  알림 수준의 색과 아이콘. 게이지, 차트, 목록, 트레이 알림이 함께 쓴다.
//
//  왜 한곳에 모으는가
//   같은 수준이 화면 네 곳에 나온다. 색이나 모양이 곳마다 다르면 사용자가
//   같은 것을 다른 것으로 읽는다. 정의를 한 파일에 두고 모두 여기서 가져간다.
//
//  모양으로도 구분한다
//   색만으로 구분하면 색약인 사용자와 흑백 화면에서 세 수준이 같아진다.
//   그래서 수준마다 색과 모양을 함께 바꾼다.
//
//     기본  파랑         (임계치를 넘지 않은 상태)
//     알림  초록 느낌표
//     경고  주황 세모
//     위험  빨간 엑스표
//
//   색은 levelColor 하나만 본다. 아이콘 색을 따로 정하면 막대는 초록인데
//   아이콘은 파란 상황이 생긴다.
//
//   아이콘은 파일로 두지 않고 그때그때 그린다. 리소스 파일과 배포 목록이
//   늘지 않고, 화면 배율이 달라도 또렷하다.
// ============================================================================
#ifndef CCM_UI_ALERTVISUALS_H
#define CCM_UI_ALERTVISUALS_H

#include "core/AlertTypes.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>

namespace ccm::ui {

/// 수준별 색. 게이지 막대, 차트 선, 트레이 아이콘 배경이 쓴다.
QColor levelColor(ccm::core::AlertLevel level);

/// 수준별 아이콘. Normal 은 그릴 것이 없어 빈 아이콘을 돌려준다.
/// \param size 한 변의 픽셀 수.
QIcon levelIcon(ccm::core::AlertLevel level, int size = 16);

/// 아이콘을 그린 원본. 트레이 알림처럼 큰 크기가 필요할 때 쓴다.
QPixmap levelPixmap(ccm::core::AlertLevel level, int size);

} // namespace ccm::ui

#endif // CCM_UI_ALERTVISUALS_H
