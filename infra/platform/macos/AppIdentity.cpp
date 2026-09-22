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
//  infra/platform/macos/AppIdentity.cpp
//
//  macOS 에는 여기서 할 일이 없다. 빈 구현이 정답이다.
//
//  왜 빈가
//   알림의 주인은 .app 묶음의 Info.plist 에 적힌 CFBundleIdentifier 로 정해진다.
//   코드가 실행 중에 밝히는 것이 아니라 묶음이 이미 가지고 있는 값이다.
//   그래서 채워야 할 것은 이 파일이 아니라 빌드 쪽이다.
//   (CMakeLists.txt 의 MACOSX_BUNDLE_GUI_IDENTIFIER)
//
//  함수 자체를 없애지 않는 이유
//   부르는 쪽(main.cpp)이 운영체제를 따지지 않게 하려는 것이다. 빈 함수 하나가
//   그쪽의 #ifdef 를 없앤다.
// ============================================================================
#include "infra/AppIdentity.h"

namespace ccm::infra {

void AppIdentity::declare()
{
}

} // namespace ccm::infra
