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
//  ui/platform/linux/DebugConsole.cpp
//
//  Linux 자리. 아직 비어 있다. isAvailable() 이 거짓이므로 [도구] 메뉴의
//  디버그 콘솔 항목은 비활성으로 보인다.
//
//  채울 때 알아 둘 것
//   터미널에서 실행했다면 stderr 가 이미 그 터미널로 간다. 데스크톱
//   아이콘으로 띄운 경우에만 보낼 곳이 없다. x-terminal-emulator 를 띄우는
//   방법도 있으나 환경마다 달라, macOS 와 같이 창을 띄우는 편이 낫다.
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
