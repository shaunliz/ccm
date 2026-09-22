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

#include "infra/ClaudeCli.h"

#include "core/TimeFormat.h"
#include "infra/Logger.h"
#include "infra/PlatformTraits.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace ccm::infra {
namespace {

/// 조회 1회에 claude 세션이 하나 뜬다. 시작이 느린 환경도 있으므로 넉넉하게 둔다.
constexpr int kDefaultTimeoutMs = 30000;

} // namespace

QString ClaudeCli::findExecutable()
{
    const QString onPath = QStandardPaths::findExecutable(QStringLiteral("claude"));
    if (!onPath.isEmpty()) {
        return QDir::cleanPath(onPath);
    }

    // GUI 앱의 PATH 에는 사용자가 셸에서 더한 경로가 없을 수 있다. 네이티브
    // 설치본의 기본 위치를 직접 본다. 그 자리는 운영체제마다 다르므로 목록은
    // infra/PlatformTraits.h 가 가지고 있다.
    const QString home = QDir::homePath();
    const QStringList candidates = platform::claudeExecutableCandidates(home);
    for (const QString &candidate : candidates) {
        if (QFileInfo(candidate).isFile()) {
            qCDebug(ccmInfra) << "PATH 에 없어 기본 위치를 씁니다:" << candidate;
            return candidate;
        }
    }
    return {};
}

QStringList ClaudeCli::sdkArguments()
{
    //  -p                     비대화 모드. 이게 없으면 TUI 가 뜬다.
    //  --input-format         control request 를 표준 입력으로 넣기 위한 형식.
    //  --output-format        control_response 를 표준 출력으로 받기 위한 형식.
    //  --verbose              -p 와 stream-json 을 함께 쓰면 claude 가 요구한다.
    //  --strict-mcp-config    사용자의 MCP 서버를 띄우지 않는다. 조회에 불필요하고
    //                         프로세스만 늘어난다.
    return QStringList{
        QStringLiteral("-p"),
        QStringLiteral("--input-format"), QStringLiteral("stream-json"),
        QStringLiteral("--output-format"), QStringLiteral("stream-json"),
        QStringLiteral("--verbose"),
        QStringLiteral("--strict-mcp-config"),
    };
}

QByteArray ClaudeCli::usageRequestLine()
{
    // skip_behaviors 는 로컬 전사 기록 스캔을 건너뛰라는 요청이다. 우리는 한도
    // 창만 쓰므로 켜 둔다.
    QJsonObject request;
    request.insert(QStringLiteral("subtype"), QStringLiteral("get_usage"));
    request.insert(QStringLiteral("skip_behaviors"), true);

    QJsonObject envelope;
    envelope.insert(QStringLiteral("type"), QStringLiteral("control_request"));
    envelope.insert(QStringLiteral("request_id"), QStringLiteral("ccm_usage_1"));
    envelope.insert(QStringLiteral("request"), request);

    return QJsonDocument(envelope).toJson(QJsonDocument::Compact) + '\n';
}

QByteArray ClaudeCli::responseMarker()
{
    return QByteArrayLiteral("\"control_response\"");
}

int ClaudeCli::defaultTimeoutMs()
{
    return kDefaultTimeoutMs;
}

QString ClaudeCli::describe(const ccm::core::UsageReport &report)
{
    QStringList lines;

    if (!report.valid) {
        return QCoreApplication::translate("ccm", "아직 조회한 값이 없습니다.");
    }

    if (report.windows.isEmpty()) {
        return report.rateLimitsAvailable
                   ? QCoreApplication::translate("ccm", "표시할 한도 창이 없습니다.")
                   : QCoreApplication::translate("ccm", "이 계정에는 플랜 한도가 적용되지 않습니다.");
    }

    // 라벨 폭은 맞추지 않는다. 한글은 문자 수와 표시 폭이 다르고, 표시하는
    // 쪽이 비고정폭 폰트이므로 패딩이 정렬로 이어지지 않는다.
    for (const ccm::core::UsageWindow &window : report.windows) {
        QString line = QStringLiteral("%1 : %2%")
                           .arg(window.label)
                           .arg(window.usedPercent, 0, 'f', 1);
        if (window.resetsAt.isValid()) {
            line += QCoreApplication::translate("ccm", "  (재설정 %1)")
                        .arg(ccm::core::formatDateTime(window.resetsAt));
        }
        lines << line;
    }
    return lines.join(QChar::LineFeed);
}

} // namespace ccm::infra
