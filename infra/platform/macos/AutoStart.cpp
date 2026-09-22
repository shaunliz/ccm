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
//  infra/platform/macos/AutoStart.cpp
//
//  macOS 자리. 아직 비어 있다. 지금은 "지원하지 않는다" 고만 답한다.
//
//  채울 때 쓸 방법
//   ~/Library/LaunchAgents/kr.co.innogrid.ClaudeCodeMonitor.plist 를 만들고
//   launchctl bootstrap gui/$UID <plist> 로 올린다. plist 의 RunAtLoad 를 켜면
//   로그인할 때 뜬다. 지울 때는 bootout 하고 파일을 지운다.
//
//   Windows 와 다른 점이 둘 있다.
//    - 등록 대상이 실행 파일이 아니라 .app 묶음이다. 경로가
//      ".../ClaudeCodeMonitor.app/Contents/MacOS/ClaudeCodeMonitor" 가 된다.
//    - 사용자가 [시스템 설정 > 일반 > 로그인 항목] 에서 직접 끌 수 있다.
//      그러면 plist 는 남아 있는데 실제로는 뜨지 않으므로, isEnabled() 가
//      파일 유무만 보아서는 화면과 어긋난다. SMAppService 를 함께 봐야 한다.
// ============================================================================
#include "infra/AutoStart.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QDir>

namespace ccm::infra {
namespace {

/// 트레이로만 올라가게 하는 인자. Windows 구현과 같은 값을 쓴다.
constexpr auto kTrayArgument = "--tray";

QString unsupportedMessage()
{
    return QCoreApplication::translate(
        "ccm", "이 운영체제에서는 로그인 시 자동 실행을 아직 지원하지 않습니다.");
}

bool fail(QString *error)
{
    if (error) {
        *error = unsupportedMessage();
    }
    qCWarning(ccmInfra).noquote() << unsupportedMessage();
    return false;
}

} // namespace

bool AutoStart::isSupported()
{
    return false;
}

QString AutoStart::commandForCurrentExecutable()
{
    // 등록은 못 해도 "무엇이 등록될 것인가" 는 화면에 보여 줄 수 있다.
    const QString executable =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return QStringLiteral("\"%1\" %2").arg(executable, QLatin1String(kTrayArgument));
}

QString AutoStart::registeredCommand()
{
    return {};
}

bool AutoStart::isEnabled()
{
    return false;
}

bool AutoStart::enable(QString *error)
{
    return fail(error);
}

bool AutoStart::disable(QString *error)
{
    return fail(error);
}

bool AutoStart::setEnabled(bool enabled, QString *error)
{
    return enabled ? enable(error) : disable(error);
}

} // namespace ccm::infra
