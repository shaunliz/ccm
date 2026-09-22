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
//  infra/platform/macos/ConsoleEncoding.cpp
//
//  macOS 에는 여기서 할 일이 없다. 빈 구현이 정답이다.
//
//  왜 빈가
//   터미널이 처음부터 UTF-8 로 동작한다. 코드 페이지라는 개념 자체가 없다.
//
//  함수 자체를 없애지 않는 이유
//   부르는 쪽(app/probe_main.cpp)이 운영체제를 따지지 않게 하려는 것이다.
//   빈 함수 하나가 그쪽의 #ifdef 를 없앤다.
// ============================================================================
#include "infra/ConsoleEncoding.h"

namespace ccm::infra {

void ConsoleEncoding::configure()
{
}

} // namespace ccm::infra
