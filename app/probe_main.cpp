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
//  app/probe_main.cpp
//
//  Claude Code statusline 훅 진입점. 이 실행 파일의 동작은 하나뿐이다.
//
//   1. 표준 입력의 statusline JSON 을 읽는다.
//   2. 스냅샷 파일로 원자적으로 기록한다. (위젯이 읽을 원본)
//   3. 한도 사용률 한 줄을 표준 출력으로 낸다. (Claude Code 가 상태줄로 읽음)
//
//  출력 규약
//   stdout : 상태줄 문자열만
//   stderr : 로그
//   Claude Code 가 stdout 을 상태줄로 읽기 때문에 이 분리는 필수다.
// ============================================================================

#include "core/SnapshotParser.h"
#include "core/StatusLineFormatter.h"
#include "core/Version.h"
#include "infra/ClaudePaths.h"
#include "infra/Logger.h"
#include "infra/ConsoleEncoding.h"
#include "infra/HookRegistrar.h"
#include "infra/SnapshotStore.h"
#include "infra/StatusLineCache.h"
#include "infra/Translations.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

#include <string>

namespace {

constexpr auto kApplicationName = "ClaudeCodeMonitor";

/// 종료 코드. 설치 관리자와 스크립트가 판정에 사용한다.
constexpr int kExitSuccess = 0;
constexpr int kExitInvalidInput = 2;
constexpr int kExitWriteFailed = 3;

/// --unregister 전용. 지우지 않고 둔 경우와 고치지 못한 경우를 나눈다.
/// 제거 관리자가 이 값으로 무슨 일이 있었는지 알린다.
constexpr int kExitHookKept = 5;
constexpr int kExitHookFailed = 6;

/// --print-hook-state 전용. 종료 코드가 곧 상태다.
///
/// 파일로도 같은 내용을 내주지만(--out), 파일을 읽지 못하는 경우에도 제거
/// 관리자가 최소한 무엇이 걸려 있는지는 알 수 있어야 한다.
constexpr int kExitStateNone = 0;       ///< 등록된 statusLine 설정이 없다.
constexpr int kExitStateThisApp = 10;   ///< 이 설치본이 등록한 것이다.
constexpr int kExitStateOtherCopy = 11; ///< 이름은 같은데 다른 자리의 복사본이다.
constexpr int kExitStateForeign = 12;   ///< 남의 명령이다.

/// stdin 을 전부 읽는다.
QByteArray readAllStandardInput()
{
    QFile input;
    if (!input.open(stdin, QIODevice::ReadOnly)) {
        qCCritical(ccmApp) << "표준 입력을 열지 못했습니다.";
        return {};
    }
    const QByteArray data = input.readAll();
    input.close();
    return data;
}

/// UTF-16LE(BOM 포함)로 쓴다.
///
/// 읽는 쪽이 NSIS 의 ReadINIStr, 즉 GetPrivateProfileStringW 다. 그 API 는 BOM 이
/// 있는 UTF-16LE 파일만 유니코드로 읽고, 그 밖에는 시스템 ANSI 코드페이지로
/// 해석한다. 경로에 한글이 섞이면 UTF-8 로 써서는 깨진다.
bool writeUtf16Ini(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(ccmApp) << "상태 파일을 쓰지 못했습니다:" << path << file.errorString();
        return false;
    }
    const char16_t bom = 0xFEFF;
    file.write(reinterpret_cast<const char *>(&bom), sizeof(bom));
    const std::u16string body = text.toStdU16String();
    file.write(reinterpret_cast<const char *>(body.data()),
               qint64(body.size() * sizeof(char16_t)));
    file.close();
    if (file.error() != QFile::NoError) {
        qCWarning(ccmApp) << "상태 파일 기록 중 오류:" << file.errorString();
        return false;
    }
    return true;
}

/// --print-hook-state. 지금 무엇이 등록되어 있는지 알려 주고 끝낸다.
///
/// 제거 관리자가 제거 화면을 그리기 전에 부른다. 화면에 "statusline 훅 등록도
/// 함께 지우기" 라고만 적어 두면 사용자는 무엇을 지우는지 모르고 누르게 된다.
/// 등록 쪽 확인 창은 경로를 보여 주는데 제거 쪽만 보여 주지 않았다.
///
/// 파일을 고치지 않는다. 읽기만 한다.
int runPrintHookState(const QString &outPath)
{
    using State = ccm::infra::HookRegistrar::State;

    QString command;
    const State current = ccm::infra::HookRegistrar::state(&command);

    QString name;
    int code = kExitStateNone;
    switch (current) {
    case State::NotRegistered: name = QStringLiteral("none");       code = kExitStateNone;      break;
    case State::ThisApp:       name = QStringLiteral("this-app");   code = kExitStateThisApp;   break;
    case State::OtherCopy:     name = QStringLiteral("other-copy"); code = kExitStateOtherCopy; break;
    case State::Other:         name = QStringLiteral("foreign");    code = kExitStateForeign;   break;
    }

    // 등록된 실행 파일이 아직 있는가. 없으면 지워도 잃을 것이 없다는 뜻이고,
    // 제거 화면이 그 사정을 그대로 적을 수 있다.
    const QString executable = ccm::infra::HookRegistrar::executableOf(command);
    const bool exists = !executable.isEmpty() && QFileInfo(executable).isFile();

    QTextStream out(stdout);
    out << name << Qt::endl;

    if (!outPath.isEmpty()) {
        // INI 값은 줄바꿈을 담을 수 없다. 명령에는 들어갈 일이 없지만 막아 둔다.
        QString safeCommand = command;
        safeCommand.replace(QLatin1Char('\r'), QLatin1Char(' '));
        safeCommand.replace(QLatin1Char('\n'), QLatin1Char(' '));

        const QString body =
            QStringLiteral("[hook]\r\nstate=%1\r\ncommand=%2\r\nexecutable=%3\r\n"
                           "exists=%4\r\nsettings=%5\r\n")
                .arg(name, safeCommand, QDir::toNativeSeparators(executable),
                     QString::number(exists ? 1 : 0),
                     QDir::toNativeSeparators(ccm::infra::ClaudePaths::claudeSettingsFilePath()));
        writeUtf16Ini(outPath, body);
    }

    qCInfo(ccmApp).noquote() << "훅 상태:" << name << "명령:" << command
                             << "실행 파일 있음:" << exists;
    return code;
}

/// --unregister. 우리가 등록한 statusline 훅만 지운다.
///
/// 표준 입력을 읽지 않는다. 훅으로 불려 온 것이 아니라 사람이나 제거 관리자가
/// 부른 것이기 때문이다.
///
/// allowOtherCopy 는 제거 관리자가 쓴다. 제거 화면이 등록된 경로를 보여 주고
/// 사용자가 그것을 보고 확인란을 켰을 때만 켜진다. "모르는 사이에 없어지면
/// 안 된다" 는 조건이 화면에서 이미 채워진 상태다.
int runUnregister(bool allowOtherCopy)
{
    const ccm::infra::HookRemovalResult result =
        ccm::infra::HookRegistrar::unregisterHook(allowOtherCopy);

    QTextStream out(stdout);
    out << result.message << Qt::endl;

    switch (result.status) {
    case ccm::infra::HookRemovalResult::Status::Removed:
    case ccm::infra::HookRemovalResult::Status::NotRegistered:
        return kExitSuccess;
    case ccm::infra::HookRemovalResult::Status::Foreign:
    case ccm::infra::HookRemovalResult::Status::NeedsConsent:
        // 지울 수 있었는데 두었다. 실패가 아니라 판단이다.
        return kExitHookKept;
    case ccm::infra::HookRemovalResult::Status::Failed:
        break;
    }
    return kExitHookFailed;
}

int runStatuslineHook(const ccm::infra::SnapshotStore &store, bool colored)
{
    const QByteArray payload = readAllStandardInput();
    if (payload.trimmed().isEmpty()) {
        qCWarning(ccmApp) << "표준 입력이 비어 있어 기록을 건너뜁니다.";
        return kExitInvalidInput;
    }

    QString error;
    if (!store.write(payload, &error)) {
        // 상태줄 장애가 Claude Code 동작을 방해하면 안 되므로 stdout 은 비워 둔다.
        qCCritical(ccmApp).noquote() << QStringLiteral("스냅샷 기록 실패: %1").arg(error);
        return kExitWriteFailed;
    }

    // 방금 받은 payload 를 그대로 해석한다. 파일을 다시 읽을 이유가 없다.
    const ccm::core::ParseResult parsed = ccm::core::SnapshotParser::parse(payload);
    if (!parsed.ok) {
        // 기록은 이미 성공했으므로 실패로 보지 않는다. 상태줄만 비운다.
        qCCritical(ccmApp).noquote()
            << QStringLiteral("사용률 해석 실패: %1").arg(parsed.error);
        return kExitSuccess;
    }

    // 모델별 사용률은 payload 에 없다. 위젯 앱이 남겨 둔 것이 있으면 빌려 쓴다.
    // 앱이 꺼져 있어 낡았으면 빈 목록이 오고, 상태줄은 Session/Model 둘로 줄어든다.
    const ccm::infra::StatusLineCache cache;
    const QList<ccm::core::StatusLineExtra> extras =
        cache.read(ccm::infra::StatusLineCache::defaultMaxAgeMinutes());

    const QString statusLine =
        ccm::core::StatusLineFormatter::format(parsed.snapshot, extras, colored);
    if (!statusLine.isEmpty()) {
        QTextStream out(stdout);
        out << statusLine << Qt::endl;
    }
    return kExitSuccess;
}

} // namespace

int main(int argc, char *argv[])
{
    // 콘솔이 UTF-8 을 그대로 내보내게 맞춘다. 운영체제마다 할 일이 다르다.
    // (infra/ConsoleEncoding.h)
    ccm::infra::ConsoleEncoding::configure();

    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String(kApplicationName));
    QCoreApplication::setApplicationVersion(ccm::core::applicationVersion());

    // --help 와 오류 문구도 번역 대상이다. 인자를 해석하기 전에 설치한다.
    ccm::infra::Translations::install();

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate(
        "ccm",
        "Claude Code statusline 훅. 표준 입력의 JSON 을 스냅샷으로 기록하고 "
        "한도 사용률 한 줄을 출력합니다."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption logLevelOption(
        QStringList{QStringLiteral("log-level")},
        QCoreApplication::translate("ccm", "로그 수준 (debug, info, warning, error). 기본값 info."),
        QStringLiteral("level"),
        QStringLiteral("info"));
    const QCommandLineOption logFileOption(
        QStringList{QStringLiteral("log-file")},
        QCoreApplication::translate("ccm", "로그를 기록할 파일 경로. 생략하면 파일 로깅을 하지 않습니다."),
        QStringLiteral("path"));
    const QCommandLineOption snapshotOption(
        QStringList{QStringLiteral("snapshot")},
        QCoreApplication::translate("ccm", "스냅샷 파일 경로를 직접 지정합니다."),
        QStringLiteral("path"));
    const QCommandLineOption noColorOption(
        QStringList{QStringLiteral("no-color")},
        QCoreApplication::translate(
            "ccm", "상태줄에 색을 넣지 않습니다. 색을 모르는 터미널에서 쓰십시오."));
    const QCommandLineOption unregisterOption(
        QStringList{QStringLiteral("unregister")},
        QCoreApplication::translate(
            "ccm", "이 프로그램이 등록한 statusline 훅을 지우고 끝냅니다."));
    const QCommandLineOption allowOtherCopyOption(
        QStringList{QStringLiteral("allow-other-copy")},
        QCoreApplication::translate(
            "ccm",
            "--unregister 와 함께 씁니다. 다른 자리의 복사본이 등록되어 있어도 지웁니다."));
    const QCommandLineOption printHookStateOption(
        QStringList{QStringLiteral("print-hook-state")},
        QCoreApplication::translate(
            "ccm", "지금 등록된 statusline 설정을 알려 주고 끝냅니다. 고치지 않습니다."));
    const QCommandLineOption outOption(
        QStringList{QStringLiteral("out")},
        QCoreApplication::translate(
            "ccm", "--print-hook-state 의 결과를 적을 INI 파일 경로."),
        QStringLiteral("path"));
    const QCommandLineOption colorOption(
        QStringList{QStringLiteral("color")},
        QCoreApplication::translate(
            "ccm", "NO_COLOR 환경 변수가 있어도 상태줄에 색을 넣습니다."));

    parser.addOption(logLevelOption);
    parser.addOption(logFileOption);
    parser.addOption(snapshotOption);
    parser.addOption(noColorOption);
    parser.addOption(colorOption);
    parser.addOption(unregisterOption);
    parser.addOption(allowOtherCopyOption);
    parser.addOption(printHookStateOption);
    parser.addOption(outOption);
    parser.process(application);

    ccm::infra::LogLevel level = ccm::infra::LogLevel::Info;
    const QString levelText = parser.value(logLevelOption);
    const bool levelParsed = ccm::infra::Logger::parseLevel(levelText, &level);

    ccm::infra::Logger::install(level, parser.value(logFileOption));

    if (!levelParsed) {
        qCWarning(ccmApp) << "알 수 없는 로그 수준입니다. info 로 대체합니다:" << levelText;
    }

    qCInfo(ccmApp) << kApplicationName << ccm::core::applicationVersion() << "시작";
    qCDebug(ccmApp) << "로그 수준:" << ccm::infra::Logger::levelName(level);

    // 상태 확인과 해제는 표준 입력을 기다리지 않는다. 색을 정하기 전에 갈라진다.
    if (parser.isSet(printHookStateOption)) {
        const int code = runPrintHookState(parser.value(outOption));
        qCDebug(ccmApp) << "종료 코드:" << code;
        ccm::infra::Logger::shutdown();
        return code;
    }

    if (parser.isSet(unregisterOption)) {
        const int code = runUnregister(parser.isSet(allowOtherCopyOption));
        qCDebug(ccmApp) << "종료 코드:" << code;
        ccm::infra::Logger::shutdown();
        return code;
    }

    // 색을 넣을지 정하는 차례. 위가 이긴다.
    //
    //  --no-color        이 실행에서만 끈다.
    //  --color           NO_COLOR 가 있어도 켠다.
    //  NO_COLOR 환경 변수 여러 프로그램이 함께 지키는 약속이라 따른다.
    //                    (값은 보지 않는다. 있기만 하면 끈다는 것이 그 약속이다)
    //  그 밖             켠다.
    //
    // --color 가 NO_COLOR 를 이기는 것은 약속에 어긋나지 않는다. NO_COLOR 는
    // "따로 시키지 않았으면 색을 넣지 말라" 는 뜻이고, 등록 명령에 --color 를
    // 적는 것이 곧 따로 시키는 일이다. 훅 등록기가 그렇게 적어 준다.
    // (Claude Code 가 하위 프로세스에 NO_COLOR 를 주는 경우가 있는데, 그것은
    //  도구 출력물을 깨끗하게 받기 위한 것이지 상태줄을 위한 설정이 아니다)
    bool colored = true;
    if (parser.isSet(noColorOption)) {
        colored = false;
    } else if (!parser.isSet(colorOption) && qEnvironmentVariableIsSet("NO_COLOR")) {
        colored = false;
    }
    qCDebug(ccmApp) << "상태줄 색:" << (colored ? "켬" : "끔");

    const ccm::infra::SnapshotStore store(parser.value(snapshotOption));
    const int exitCode = runStatuslineHook(store, colored);

    qCDebug(ccmApp) << "종료 코드:" << exitCode;
    ccm::infra::Logger::shutdown();
    return exitCode;
}
