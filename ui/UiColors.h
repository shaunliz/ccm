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
//  ui/UiColors.h
//
//  본문보다 여린 보조 설명 글자색.
//
//  왜 별도로 두는가
//   처음에는 QPalette::Disabled 의 WindowText 를 그대로 썼다. 그 색은 "쓸 수
//   없는 위젯" 을 나타내려고 만든 것이라 읽히기를 포기한 밝기다. 설명 문구는
//   읽어야 하는 글이므로 그만큼 여려서는 안 된다.
//
//   그래서 본문색과 비활성색을 섞어 한 단계만 진하게 만든다. 본문색을 그대로
//   쓰면 설명과 본문이 구분되지 않으므로 완전히 진하게 가지는 않는다.
//
//   섞는 비율을 한곳에 두는 이유는 이 색을 쓰는 자리가 다섯 군데라서다.
//   (게이지의 재설정 시각, 차트 안내, 팝업 상태줄, 설정 설명, 요약의 갱신 시각)
//   자리마다 따로 계산하면 화면마다 밝기가 달라진다.
// ============================================================================
#ifndef CCM_UI_UICOLORS_H
#define CCM_UI_UICOLORS_H

#include <QColor>

QT_BEGIN_NAMESPACE
class QPalette;
class QWidget;
QT_END_NAMESPACE

namespace ccm::ui {

/// 보조 설명에 쓸 글자색. 본문보다 여리고 비활성색보다 진하다.
QColor hintTextColor(const QPalette &palette);

/// 위젯의 WindowText 를 위 색으로 바꾼다. 라벨에 쓴다.
void applyHintTextColor(QWidget *widget);

} // namespace ccm::ui

#endif // CCM_UI_UICOLORS_H
