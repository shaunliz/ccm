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
//  ui/HookSetup.h
//
//  statusline 훅 등록을 사용자와 주고받으며 수행한다.
//
//  왜 창 코드에서 떼어 냈는가
//   같은 일을 메인 창의 도구 메뉴와 설정 창 둘이 한다. 등록은 남의 설정 파일
//   (Claude Code 의 settings.json)을 고치는 일이라 확인 절차가 붙는다. 두 벌로
//   두면 한쪽만 고쳐질 수 있고, 그때 한쪽은 백업 없이 덮어쓰게 된다.
// ============================================================================
#ifndef CCM_UI_HOOKSETUP_H
#define CCM_UI_HOOKSETUP_H

#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace ccm::ui {

/// ccm_probe 를 statusline 훅으로 등록한다.
///
/// 다른 statusLine 설정이 이미 있으면 덮기 전에 물어본다. 끝나면 결과를 창으로
/// 알린다. (실패도 알린다. 조용히 실패하면 나중에 스냅샷이 없는 이유를 찾느라
/// 헤매게 된다)
///
/// \param summary 상태줄에 쓸 한 줄 요약을 담아 준다. 필요 없으면 nullptr.
/// \return 등록되어 있는가. 이미 같은 명령으로 등록되어 있던 경우도 참이다.
bool runHookRegistration(QWidget *parent, QString *summary = nullptr);

/// 등록된 statusline 훅을 지운다.
///
/// 지우기 전에 무엇을 지우는지 보여 주고 확인받는다. 남의 상태줄 설정이면
/// 지우지 않고 그 사실만 알린다. 우리 프로그램의 다른 복사본이면 그것까지
/// 밝히고 묻는다.
///
/// \param summary 상태줄에 쓸 한 줄 요약을 담아 준다. 필요 없으면 nullptr.
/// eturn 지웠는가. 지울 것이 없었거나 사용자가 취소하면 거짓이다.
bool runHookRemoval(QWidget *parent, QString *summary = nullptr);

} // namespace ccm::ui

#endif // CCM_UI_HOOKSETUP_H
