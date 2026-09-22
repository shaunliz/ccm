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
//  infra/platform/windows/AutoStart.cpp
//
//  Windows 구현. HKCU 의 Run 키에 적는다.
//
//  다른 운영체제의 구현은 platform/macos, platform/linux 에 같은 이름으로 있다.
//  어느 하나만 빌드에 들어간다. (CMakeLists.txt 의 CCM_PLATFORM 참조)
// ============================================================================
#include "infra/AutoStart.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace ccm::infra {
namespace {

constexpr auto kRunKeyPath =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto kValueName = "ClaudeCodeMonitor";

/// 트레이로만 올라가게 하는 인자. 로그인 직후 창이 튀어나오지 않게 한다.
constexpr auto kTrayArgument = "--tray";

} // namespace

bool AutoStart::isSupported()
{
    return true;
}

QString AutoStart::commandForCurrentExecutable()
{
    const QString executable =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    // 경로에 공백이 있으면 따옴표 없이는 실행되지 않는다.
    return QStringLiteral("\"%1\" %2").arg(executable, QLatin1String(kTrayArgument));
}

QString AutoStart::registeredCommand()
{
    QSettings run(QLatin1String(kRunKeyPath), QSettings::NativeFormat);
    return run.value(QLatin1String(kValueName)).toString();
}

bool AutoStart::isEnabled()
{
    return !registeredCommand().trimmed().isEmpty();
}

bool AutoStart::enable(QString *error)
{
    const QString command = commandForCurrentExecutable();

    QSettings run(QLatin1String(kRunKeyPath), QSettings::NativeFormat);
    if (run.value(QLatin1String(kValueName)).toString() == command) {
        qCDebug(ccmInfra) << "시작프로그램이 이미 같은 명령으로 등록되어 있습니다.";
        return true;
    }

    run.setValue(QLatin1String(kValueName), command);
    run.sync();

    if (run.status() != QSettings::NoError) {
        const QString message =
            QCoreApplication::translate("ccm", "시작프로그램 등록에 실패했습니다. (레지스트리 오류 %1)")
                .arg(static_cast<int>(run.status()));
        if (error) {
            *error = message;
        }
        qCWarning(ccmInfra).noquote() << message;
        return false;
    }

    qCInfo(ccmInfra).noquote()
        << QStringLiteral("시작프로그램에 등록했습니다: %1").arg(command);
    return true;
}

bool AutoStart::disable(QString *error)
{
    QSettings run(QLatin1String(kRunKeyPath), QSettings::NativeFormat);
    run.remove(QLatin1String(kValueName));
    run.sync();

    if (run.status() != QSettings::NoError) {
        const QString message =
            QCoreApplication::translate("ccm", "시작프로그램 해제에 실패했습니다. (레지스트리 오류 %1)")
                .arg(static_cast<int>(run.status()));
        if (error) {
            *error = message;
        }
        qCWarning(ccmInfra).noquote() << message;
        return false;
    }

    qCInfo(ccmInfra) << "시작프로그램 등록을 해제했습니다.";
    return true;
}

bool AutoStart::setEnabled(bool enabled, QString *error)
{
    return enabled ? enable(error) : disable(error);
}

} // namespace ccm::infra
