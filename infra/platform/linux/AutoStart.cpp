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
//  infra/platform/linux/AutoStart.cpp
//
//  Linux 자리. 아직 비어 있다. 지금은 "지원하지 않는다" 고만 답한다.
//
//  채울 때 쓸 방법
//   XDG 자동 시작 규격을 따른다. $XDG_CONFIG_HOME/autostart (없으면
//   ~/.config/autostart) 에 ClaudeCodeMonitor.desktop 을 만들고, Exec= 에
//   실행 파일 경로와 --tray 를 적는다. 지울 때는 그 파일을 지운다.
//
//   Windows 와 다른 점이 둘 있다.
//    - 데스크톱 환경이 이 규격을 지킬 때만 동작한다. GNOME, KDE, XFCE 는
//      지키지만 창 관리자만 쓰는 환경은 읽지 않는다. 등록에 성공해도 뜨지
//      않을 수 있으므로, 성공을 단정하는 문구는 피해야 한다.
//    - 트레이 아이콘 자체가 없는 환경이 있다. AutoStart 가 되더라도
//      TrayIcon::isAvailable() 이 거짓이면 창으로만 뜬다.
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
