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
//  infra/AppIdentity.h
//
//  운영체제의 알림 센터에 "이 알림은 어느 앱의 것인가" 를 밝힌다.
//
//  밝히지 않으면 트레이 알림이 실행 파일 경로에 따라 다른 앱으로 취급되어,
//  설치 위치가 바뀌면 이전 알림과 묶이지 않는다.
//
//  선언만 여기 있고 구현은 운영체제마다 다르다.
//   Windows : platform/windows/AppIdentity.cpp  AppUserModelID 를 설정한다.
//   macOS   : platform/macos/AppIdentity.cpp    할 일이 없다. (아래 참조)
//   Linux   : platform/linux/AppIdentity.cpp    할 일이 없다. (아래 참조)
//
//  실패해도 프로그램은 그대로 돈다. 알림이 묶이지 않을 뿐이다. 그래서 결과를
//  돌려주지 않고 로그만 남긴다.
// ============================================================================
#ifndef CCM_INFRA_APPIDENTITY_H
#define CCM_INFRA_APPIDENTITY_H

namespace ccm::infra {

class AppIdentity
{
public:
    AppIdentity() = delete;

    /// 프로그램이 시작할 때 한 번 부른다. QApplication 을 만든 뒤여야 한다.
    static void declare();
};

} // namespace ccm::infra

#endif // CCM_INFRA_APPIDENTITY_H
