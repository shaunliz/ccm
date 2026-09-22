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
//  infra/AutoStart.h
//
//  로그인 시 자동 실행 등록.
//
//  선언만 여기 있고 구현은 운영체제마다 다르다.
//   Windows : platform/windows/AutoStart.cpp  HKCU 의 Run 키
//   macOS   : platform/macos/AutoStart.cpp    (미구현) LaunchAgent plist
//   Linux   : platform/linux/AutoStart.cpp    (미구현) XDG autostart .desktop
//  어느 하나만 빌드에 들어간다. (CMakeLists.txt 의 CCM_PLATFORM 참조)
//
//  아래는 Windows 구현을 그렇게 정한 이유다.
//
//  왜 Run 키인가
//   세 가지 후보가 있다.
//    - 시작 폴더 바로가기 : 파일이라 사용자가 지우면 흔적 없이 사라진다.
//    - 작업 스케줄러      : 권한 상승이 필요하고 설정이 무겁다.
//    - HKCU Run 키        : 관리자 권한이 필요 없고, 작업 관리자의 "시작 앱"
//                           목록에 그대로 보이며 사용자가 끌 수 있다.
//   사용자 단위 프로그램이므로 마지막이 맞다. HKLM 은 쓰지 않는다. 권한 상승이
//   필요하고 다른 사용자에게까지 등록되기 때문이다.
//
//  경로에 공백이 있으므로 값은 반드시 따옴표로 감싼다. 감싸지 않으면 OS 가
//  "C:\Program" 을 실행하려 한다.
// ============================================================================
#ifndef CCM_INFRA_AUTOSTART_H
#define CCM_INFRA_AUTOSTART_H

#include <QString>

namespace ccm::infra {

class AutoStart
{
public:
    AutoStart() = delete;

    /// 이 운영체제에 구현이 있는가.
    ///
    /// 거짓이면 아래 함수들은 아무것도 하지 않고 실패를 돌려준다. 부르는 쪽은
    /// 이 값을 보고 화면의 확인란을 비활성으로 두어야 한다. 눌렀는데 아무 일도
    /// 일어나지 않는 것이 제일 나쁜 결과다.
    static bool isSupported();

    /// 등록되어 있는지. 다른 경로로 등록된 경우도 true 다.
    static bool isEnabled();

    /// 등록된 명령 문자열. 없으면 빈 문자열.
    static QString registeredCommand();

    /// 현재 실행 파일을 등록한다. 이미 같은 명령이면 아무것도 하지 않는다.
    /// 트레이로만 올라가도록 --tray 를 붙인다.
    static bool enable(QString *error = nullptr);

    static bool disable(QString *error = nullptr);

    /// enable/disable 을 한 번에. 결과 상태를 돌려준다.
    static bool setEnabled(bool enabled, QString *error = nullptr);

    /// 등록에 쓰는 명령 문자열. 화면에 보여 주기 위한 것이다.
    static QString commandForCurrentExecutable();
};

} // namespace ccm::infra

#endif // CCM_INFRA_AUTOSTART_H
