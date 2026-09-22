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
//  ui/CloseChoice.h
//
//  창을 닫을 때 무엇을 할지 묻는 팝업.
//
//  왜 물어보는가
//   트레이 상주 프로그램의 닫기(X)는 뜻이 둘이다. "치워 두고 계속 돌려라" 와
//   "그만 써라". 어느 쪽인지 프로그램이 정해 버리면 반대를 원한 사용자는 매번
//   당황한다. 그래서 처음에는 묻고, 사용자가 "다시 묻지 않음" 을 켜면 그때 고른
//   동작을 기억해 그대로 쓴다.
//
//  취소를 함께 두는 이유
//   닫기는 실수로도 눌린다. 되돌릴 길이 없으면 조회 이력이 끊긴다.
//
//  "다시 묻지 않음" 과 취소가 겹칠 때
//   취소를 고르면 그 체크는 무시한다. "묻지 말고 앞으로 취소해라" 는 뜻이 성립
//   하지 않기 때문이다. 저장하는 쪽에서 그렇게 다룬다.
// ============================================================================
#ifndef CCM_UI_CLOSECHOICE_H
#define CCM_UI_CLOSECHOICE_H

#include <QtGlobal>   // QT_BEGIN_NAMESPACE

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace ccm::ui {

enum class CloseChoice {
    Cancel,       ///< 닫지 않는다.
    KeepInTray,   ///< 창만 숨기고 트레이에서 계속 돈다.
    Quit          ///< 프로그램을 끝낸다.
};

/// 종료 확인 팝업을 띄운다.
/// \param rememberChoice 사용자가 "다시 묻지 않음" 을 켰으면 true 로 채운다.
///                       취소를 골랐을 때의 값은 뜻이 없다.
CloseChoice askCloseChoice(QWidget *parent, bool *rememberChoice);

} // namespace ccm::ui

#endif // CCM_UI_CLOSECHOICE_H
