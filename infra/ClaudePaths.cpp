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

#include "infra/ClaudePaths.h"

#include "infra/Logger.h"

#include <QDir>
#include <QStandardPaths>

namespace ccm::infra {
namespace {

constexpr auto kAppDirName = "ClaudeCodeMonitor";
constexpr auto kSnapshotFileName = "snapshot.json";
constexpr auto kSettingsFileName = "settings.ini";
constexpr auto kHistoryFileName = "history.json";
constexpr auto kStatusLineCacheFileName = "statusline.json";
constexpr auto kAlertFileName = "alerts.json";
constexpr auto kLogFileName = "monitor.log";

constexpr auto kClaudeConfigDirEnv = "CLAUDE_CONFIG_DIR";
constexpr auto kClaudeDirName = ".claude";
constexpr auto kClaudeSettingsFileName = "settings.json";

QString joinPath(const QString &base, const char *child)
{
    return QDir(base).filePath(QLatin1String(child));
}

} // namespace

QString ClaudePaths::appDataDir()
{
    // GenericDataLocation 은 운영체제마다 알맞은 자리를 준다.
    //   Windows : %LOCALAPPDATA%
    //   macOS   : ~/Library/Application Support
    //   Linux   : $XDG_DATA_HOME (없으면 ~/.local/share)
    // 그래서 이 함수에는 운영체제 분기가 필요 없다.
    //
    // AppLocalDataLocation 을 쓰지 않는 이유는 그쪽이 조직명·앱 이름을 끼워
    // 넣어 설치 관리자가 쓰는 경로와 어긋나기 때문이다. 우리가 폴더 이름을
    // 직접 붙인다.
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath();
        qCWarning(ccmInfra) << "표준 데이터 경로를 찾지 못해 홈 디렉터리를 사용합니다.";
    }
    return QDir::cleanPath(joinPath(base, kAppDirName));
}

QString ClaudePaths::snapshotFilePath()
{
    return joinPath(appDataDir(), kSnapshotFileName);
}

QString ClaudePaths::settingsFilePath()
{
    return joinPath(appDataDir(), kSettingsFileName);
}

QString ClaudePaths::historyFilePath()
{
    return joinPath(appDataDir(), kHistoryFileName);
}

QString ClaudePaths::statusLineCacheFilePath()
{
    return joinPath(appDataDir(), kStatusLineCacheFileName);
}

QString ClaudePaths::alertFilePath()
{
    return joinPath(appDataDir(), kAlertFileName);
}

QString ClaudePaths::logFilePath()
{
    return joinPath(appDataDir(), kLogFileName);
}

QString ClaudePaths::claudeHomeDir()
{
    const QString configDir = qEnvironmentVariable(kClaudeConfigDirEnv).trimmed();
    if (!configDir.isEmpty()) {
        qCDebug(ccmInfra) << "CLAUDE_CONFIG_DIR 로 설정 경로가 재정의되었습니다:" << configDir;
        return QDir::cleanPath(configDir);
    }
    return QDir::cleanPath(joinPath(QDir::homePath(), kClaudeDirName));
}

QString ClaudePaths::claudeSettingsFilePath()
{
    return joinPath(claudeHomeDir(), kClaudeSettingsFileName);
}

} // namespace ccm::infra
