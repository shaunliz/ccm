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
//  ui/platform/macos/DebugConsole.cpp
//
//  macOS 자리. 아직 비어 있다. isAvailable() 이 거짓이므로 [도구] 메뉴의
//  디버그 콘솔 항목은 비활성으로 보인다.
//
//  채울 때 알아 둘 것
//   AllocConsole 에 해당하는 것이 없다. .app 묶음으로 실행하면 붙일 터미널
//   자체가 없기 때문이다. 창을 하나 띄워 로그를 그리는 편이 현실적이다.
//   (QPlainTextEdit 에 Logger 의 출력을 잇는 방식)
//
//   그때까지도 로그는 파일에 그대로 쌓인다. (infra/Logger, monitor.log)
//   콘솔은 "눈앞에서 흐르게 하는" 편의일 뿐 유일한 수단이 아니다.
// ============================================================================
#include "ui/DebugConsole.h"

#include <QCoreApplication>

namespace ccm::ui {
namespace {

QString translate(const char *source)
{
    return QCoreApplication::translate("ccm", source);
}

} // namespace

bool DebugConsole::isAvailable()
{
    return false;
}

bool DebugConsole::isOpen()
{
    return false;
}

bool DebugConsole::open(QString *error)
{
    if (error) {
        *error = translate("이 운영체제에서는 디버그 콘솔을 쓸 수 없습니다.");
    }
    return false;
}

void DebugConsole::close()
{
}

} // namespace ccm::ui
