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
//  infra/platform/windows/ConsoleEncoding.cpp
//
//  Windows 구현. 한국어 윈도우의 콘솔 기본 코드 페이지는 CP949 라 UTF-8 로
//  나가는 한글이 깨진다. 들어오는 쪽(CP)과 나가는 쪽(OutputCP) 을 모두 바꾼다.
// ============================================================================
#include "infra/ConsoleEncoding.h"

#include <windows.h>

namespace ccm::infra {

void ConsoleEncoding::configure()
{
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
}

} // namespace ccm::infra
