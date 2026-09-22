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
//  infra/platform/linux/AppIdentity.cpp
//
//  Linux 에는 여기서 할 일이 없다. 빈 구현이 정답이다.
//
//  왜 빈가
//   알림의 주인은 데스크톱 항목 파일(ClaudeCodeMonitor.desktop)과 그 이름을
//   가리키는 QGuiApplication::desktopFileName() 으로 정해진다. 실행 중에
//   밝히는 API 가 따로 있는 것이 아니다. 그래서 채워야 할 것은 이 파일이 아니라
//   .desktop 파일과 그 설치 규칙이다.
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
