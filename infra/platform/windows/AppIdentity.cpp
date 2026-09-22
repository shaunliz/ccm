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
//  infra/platform/windows/AppIdentity.cpp
//
//  Windows 구현. AppUserModelID 를 밝힌다.
//
//  이 값은 설치 관리자가 만드는 시작 메뉴 바로가기와 짝이 되는 값이다. 둘이
//  다르면 알림 센터가 바로가기와 실행 중인 프로세스를 다른 앱으로 본다.
// ============================================================================
#include "infra/AppIdentity.h"

#include "infra/Logger.h"

#include <shobjidl.h>

namespace ccm::infra {
namespace {

constexpr auto kAppUserModelId = L"ClaudeCodeMonitor.UsageMonitor";

} // namespace

void AppIdentity::declare()
{
    const HRESULT result = ::SetCurrentProcessExplicitAppUserModelID(kAppUserModelId);
    if (FAILED(result)) {
        // 알림이 묶이지 않을 뿐 동작에는 지장이 없다. 사실만 남긴다.
        qCWarning(ccmInfra) << "AppUserModelID 를 설정하지 못했습니다. HRESULT="
                            << static_cast<int>(result);
    }
}

} // namespace ccm::infra
