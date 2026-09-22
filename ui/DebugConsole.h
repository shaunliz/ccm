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
//  ui/DebugConsole.h
//
//  실행 중에 콘솔 창을 붙여 로그를 눈앞에서 흐르게 한다.
//
//  왜 필요한가
//   GUI 앱이라 콘솔이 없다. 그래서 지금까지는 트레이에서 조용히 도는 동안의
//   문제를 monitor.log 를 열어 보는 방법으로만 쫓을 수 있었다. 설치본에서
//   "지금 무슨 일이 일어나는가" 를 보려면 창이 하나 필요하다.
//
//  릴리즈 빌드에서도 동작한다
//   AllocConsole 은 디버그 전용 장치가 아니라 평범한 Win32 API 다. 이 프로그램은
//   /SUBSYSTEM:WINDOWS 로 링크되어 시작할 때 콘솔이 없을 뿐, 실행 중에 붙이는
//   것은 막혀 있지 않다. qCDebug 도 릴리즈에서 살아 있다. Qt 가 로그를 컴파일
//   단계에서 지우는 것은 QT_NO_DEBUG_OUTPUT 이 정의될 때뿐이고, 릴리즈가 주는
//   QT_NO_DEBUG 는 Q_ASSERT 만 없앤다.
//
//  로깅 코드는 건드리지 않는다
//   infra/Logger 가 이미 모든 줄을 UTF-8 로 stderr 에 쓴다. 콘솔을 붙이고
//   stderr 를 그 콘솔로 다시 열어 주면 그대로 흐른다.
//
//  콘솔 창을 닫으면 프로그램이 함께 죽는다
//   콘솔을 닫으면 CTRL_CLOSE_EVENT 가 오고, 처리기가 무엇을 하든 윈도우가 곧
//   프로세스를 끝낸다. 막을 방법이 없다. 그래서 닫기 단추를 시스템 메뉴에서
//   빼고, 여는 순간 콘솔 첫 줄에 그 사실을 적어 둔다. (윈도우 터미널에서는
//   단추를 뺄 수 없어 적어 두는 것이 유일한 수단이다)
// ============================================================================
#ifndef CCM_UI_DEBUGCONSOLE_H
#define CCM_UI_DEBUGCONSOLE_H

#include <QString>

namespace ccm::ui {

class DebugConsole
{
public:
    DebugConsole() = delete;

    /// 이 운영체제에서 쓸 수 있는가. 윈도우가 아니면 거짓이다.
    static bool isAvailable();

    /// 지금 콘솔이 붙어 있는가.
    static bool isOpen();

    /// 콘솔을 붙이고 stderr 를 그리로 돌린다.
    ///
    /// 로그 수준을 디버그로 올리고 이전 수준을 기억해 둔다. 콘솔을 여는 목적이
    /// 대개 자세히 보려는 것인데, 기본 수준(정보)에서는 qCDebug 가 걸러져
    /// 빈 창만 보이기 때문이다. close() 가 되돌린다.
    ///
    /// \return 성공 여부. 이미 열려 있으면 참.
    static bool open(QString *error = nullptr);

    /// 콘솔을 떼고 로그 수준을 되돌린다.
    static void close();
};

} // namespace ccm::ui

#endif // CCM_UI_DEBUGCONSOLE_H
