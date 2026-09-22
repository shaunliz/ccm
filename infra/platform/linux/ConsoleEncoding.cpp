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
//  infra/platform/linux/ConsoleEncoding.cpp
//
//  Linux 에는 여기서 할 일이 없다. 빈 구현이 정답이다.
//
//  왜 빈가
//   로캘이 UTF-8 이면 그대로 동작한다. C 로캘처럼 UTF-8 이 아닌 환경에서는
//   깨질 수 있지만, 그것은 프로그램이 바꿀 일이 아니라 환경 변수가 정할 일이다.
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
