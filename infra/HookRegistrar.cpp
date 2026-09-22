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

#include "infra/HookRegistrar.h"

#include "infra/ClaudePaths.h"
#include "infra/Logger.h"
#include "infra/PlatformTraits.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>
#include <QSaveFile>
#include <QStringList>

namespace ccm::infra {
namespace {

constexpr auto kKeyStatusLine = "statusLine";
constexpr auto kKeyType = "type";
constexpr auto kKeyCommand = "command";
constexpr auto kTypeCommand = "command";

/// 실행 검증에 허용할 시간. 정상이면 수십 ms 안에 끝난다.
constexpr int kProbeCheckTimeoutMs = 10000;

// 훅 실행 파일의 이름과 경로 비교 규칙은 운영체제마다 다르다.
// 그 차이는 infra/PlatformTraits.h 한곳에 모아 두었다.
using ccm::infra::platform::pathCaseSensitivity;
using ccm::infra::platform::probeExecutableName;

QString backupSuffix()
{
    return QStringLiteral(".bak.")
           + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
}

/// settings.json 을 백업한다. 만든 경로를 돌려주고, 실패하면 빈 문자열이다.
///
/// 이름표가 초 단위라 한 초 안에 두 번 고치면 같은 이름이 나온다. QFile::copy 는
/// 이미 있는 파일을 덮지 않으므로 그때 백업이 실패하고, 백업이 없으면 고치는
/// 일 자체를 그만둔다. 등록을 지우고 곧바로 다시 등록하는 것은 사람이 하기에도
/// 어렵지 않고, 설정 창에서 [훅 해제] 다음 [훅 등록] 을 누르면 바로 걸린다.
/// 그래서 같은 이름이 있으면 꼬리번호를 붙인다.
QString makeBackup(const QString &settingsPath)
{
    const QString base = settingsPath + backupSuffix();
    if (!QFile::exists(base) && QFile::copy(settingsPath, base)) {
        return base;
    }
    for (int n = 2; n <= 99; ++n) {
        const QString candidate = base + QStringLiteral("-") + QString::number(n);
        if (QFile::exists(candidate)) {
            continue;
        }
        if (QFile::copy(settingsPath, candidate)) {
            return candidate;
        }
    }
    return {};
}

/// settings.json 에 적을 명령 문자열.
///
/// Claude Code 는 statusline 명령을 셸에 넘긴다. 윈도우에서도 셸은 bash 이므로
/// 역슬래시 경로는 이스케이프로 먹혀 `C:wsqt...` 가 되고, 명령을 찾지 못한 채
/// 조용히 실패한다. (상태줄이 그냥 비어 보이고 훅은 한 번도 실행되지 않는다.)
/// 그래서 구분자는 슬래시로 쓰고, 공백을 대비해 따옴표로 감싼다.
/// 윈도우 API 는 슬래시 경로를 그대로 받으므로 실행에는 영향이 없다.
///
/// 인자로 --color 를 붙인다. Claude Code 는 하위 프로세스에 NO_COLOR 를 물려줄
/// 때가 있고, 그것이 있으면 훅은 약속에 따라 색을 뺀다. 그 환경 변수는 도구
/// 출력물을 깨끗하게 받으려는 설정이지 상태줄을 위한 것이 아니므로, 등록 명령에
/// 켜 달라고 적어 둔다. 그러면 환경이 어떻든 결과가 같다.
/// 색이 싫으면 이 자리를 --no-color 로 바꾸면 된다.
QString shellCommandFor(const QString &executablePath)
{
    return QLatin1Char('"') + QDir::fromNativeSeparators(executablePath) + QLatin1Char('"')
           + QLatin1String(" --color");
}

/// 명령에서 실행 파일 부분만 떼어 낸다. 따옴표로 감싼 첫 덩이가 그것이다.
QString extractExecutable(const QString &command)
{
    const QString value = command.trimmed();
    if (value.startsWith(QLatin1Char('"'))) {
        const int closing = value.indexOf(QLatin1Char('"'), 1);
        if (closing > 0) {
            return QDir::fromNativeSeparators(value.mid(1, closing - 1));
        }
    }
    // 따옴표가 없으면 첫 공백까지. (손으로 적은 설정을 위한 대비)
    const int space = value.indexOf(QLatin1Char(' '));
    return QDir::fromNativeSeparators(space > 0 ? value.left(space) : value);
}

/// 따옴표와 구분자 표기만 다른 같은 명령을 다른 설정으로 오인하지 않도록 맞춘다.
QString normalizedCommand(const QString &command)
{
    QString value = command.trimmed();
    if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
        value = value.mid(1, value.size() - 2);
    return QDir::fromNativeSeparators(value);
}

bool equalPaths(const QString &lhs, const QString &rhs)
{
    return lhs.compare(rhs, pathCaseSensitivity()) == 0;
}

/// 인자까지 똑같은가. 같으면 손댈 것이 없다.
bool sameCommand(const QString &lhs, const QString &rhs)
{
    return equalPaths(normalizedCommand(lhs), normalizedCommand(rhs));
}

/// 이름이 ccm_probe 인가. 자리는 보지 않는다.
///
/// 예전 빌드나 다른 설치본이 등록해 둔 것을 가려내기 위한 것이다. 우리 것이긴
/// 하지만 지금 이 실행 파일은 아니므로, 지울 때 사용자에게 한 번 묻는다.
bool looksLikeOurProbe(const QString &command)
{
    const QString name = extractExecutable(command).section(QLatin1Char('/'), -1);
    return equalPaths(name, probeExecutableName());
}

/// 실행 파일이 같은가. 인자는 보지 않는다.
///
/// 인자가 달라진 경우(예전에 --color 없이 등록한 것)를 "남의 설정" 으로 보면,
/// 자기가 등록해 둔 것을 두고 덮어쓸지 묻게 된다. 그것은 충돌이 아니라 갱신이다.
bool sameExecutable(const QString &lhs, const QString &rhs)
{
    return equalPaths(extractExecutable(lhs), extractExecutable(rhs));
}

} // namespace

QString HookRegistrar::probePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(probeExecutableName());
}

HookRegistrar::State HookRegistrar::state(QString *registeredCommand)
{
    if (registeredCommand) {
        registeredCommand->clear();
    }

    QFile file(ClaudePaths::claudeSettingsFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        // 파일이 없는 것은 흔한 일이다. Claude Code 를 아직 한 번도 쓰지 않았거나
        // 설정을 건드린 적이 없으면 만들어지지 않는다. 오류로 다루지 않는다.
        return State::NotRegistered;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!document.isObject()) {
        return State::NotRegistered;
    }

    const QJsonValue node = document.object().value(QLatin1String(kKeyStatusLine));
    if (!node.isObject()) {
        return State::NotRegistered;
    }

    const QString command = node.toObject().value(QLatin1String(kKeyCommand)).toString();
    if (command.trimmed().isEmpty()) {
        return State::NotRegistered;
    }
    if (registeredCommand) {
        *registeredCommand = command;
    }

    // 인자가 달라도(예전 등록) 실행 파일이 우리 것이면 우리 것이다.
    if (sameExecutable(command, shellCommandFor(probePath()))) {
        return State::ThisApp;
    }
    // 이름은 같은데 자리가 다르다. 우리 프로그램의 다른 복사본이다.
    if (looksLikeOurProbe(command)) {
        return State::OtherCopy;
    }
    return State::Other;
}

QString HookRegistrar::registrationCommand()
{
    return shellCommandFor(probePath());
}

QString HookRegistrar::executableOf(const QString &command)
{
    return extractExecutable(command);
}

HookRemovalResult HookRegistrar::unregisterHook(bool allowOtherCopy)
{
    using Status = HookRemovalResult::Status;

    HookRemovalResult result;
    const QString settingsPath = ClaudePaths::claudeSettingsFilePath();
    result.settingsPath = QDir::toNativeSeparators(settingsPath);

    const auto fail = [&result](Status status, const QString &message) {
        result.status = status;
        result.message = message;
        qCWarning(ccmInfra).noquote() << message;
        return result;
    };

    QString registered;
    const State current = state(&registered);
    result.command = registered;

    if (current == State::NotRegistered) {
        result.status = Status::NotRegistered;
        result.message =
            QCoreApplication::translate("ccm", "등록된 statusLine 설정이 없습니다.");
        qCInfo(ccmInfra).noquote() << result.message;
        return result;
    }

    // 남의 것은 파일을 열지도 않는다.
    if (current == State::Other) {
        result.status = Status::Foreign;
        result.message =
            QCoreApplication::translate("ccm", "다른 프로그램의 statusLine 설정이라 지우지 않았습니다: %1")
                .arg(registered);
        qCWarning(ccmInfra).noquote() << result.message;
        return result;
    }

    // 다른 자리의 복사본이라도 그 파일이 이미 없으면 묻지 않고 지운다.
    //
    // 없는 파일을 가리키는 statusLine 은 누구에게도 쓸모가 없다. Claude Code 는
    // 그 명령을 실행하려다 조용히 실패하고, 상태줄은 그냥 빈칸이 된다. 로그도
    // 남지 않아 "스냅샷이 없습니다" 와 증상이 구별되지 않는다.
    //
    // 묻지 않아도 되는 근거는 실행 파일 이름이 ccm_probe 라는 것이다. 우리가
    // 남긴 것이 분명하고, 되살릴 방법도 없다. 파일이 살아 있는 복사본은 지금도
    // 동작 중인 설정이므로 그대로 둔다. 남의 명령(Other)은 위에서 이미 걸렀다.
    const bool danglingCopy =
        current == State::OtherCopy && !QFileInfo(extractExecutable(registered)).isFile();

    if (current == State::OtherCopy && !allowOtherCopy && !danglingCopy) {
        result.status = Status::NeedsConsent;
        result.message =
            QCoreApplication::translate("ccm", "이 프로그램의 다른 복사본이 등록되어 있습니다: %1")
                .arg(registered);
        qCInfo(ccmInfra).noquote() << result.message;
        return result;
    }

    // --- 읽기 ---
    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 을 열지 못했습니다: %1")
                        .arg(file.errorString()));
    }
    const QByteArray original = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(original, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 을 해석하지 못했습니다: %1")
                        .arg(parseError.errorString()));
    }
    QJsonObject root = document.object();

    // --- 백업 ---
    // 등록할 때와 같다. Qt 가 다시 쓰면 키 차례와 들여쓰기가 바뀌므로,
    // 되돌릴 수단을 먼저 만든다.
    const QString backupPath = makeBackup(settingsPath);
    if (backupPath.isEmpty()) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "백업을 만들지 못해 중단합니다: %1")
                        .arg(QDir::toNativeSeparators(settingsPath)));
    }
    result.backupPath = QDir::toNativeSeparators(backupPath);

    // --- 지우기 ---
    // 항목을 통째로 없앤다. 빈 객체로 두면 Claude Code 가 명령 없는 상태줄
    // 설정을 들고 있게 되어, 나중에 무엇이 남은 것인지 알기 어렵다.
    root.remove(QLatin1String(kKeyStatusLine));

    QSaveFile out(settingsPath);
    if (!out.open(QIODevice::WriteOnly)) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 을 쓰기 모드로 열지 못했습니다: %1")
                        .arg(out.errorString()));
    }
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(encoded) != encoded.size()) {
        out.cancelWriting();
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 기록 중 오류가 발생했습니다: %1")
                        .arg(out.errorString()));
    }
    if (!out.commit()) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 교체에 실패했습니다: %1")
                        .arg(out.errorString()));
    }

    result.status = Status::Removed;
    result.message =
        danglingCopy
            ? QCoreApplication::translate(
                  "ccm", "없는 파일을 가리키던 statusline 훅 등록을 지웠습니다.")
            : QCoreApplication::translate("ccm", "statusline 훅 등록을 지웠습니다.");
    qCInfo(ccmInfra).noquote() << result.message << registered;
    return result;
}

HookRegistrationResult HookRegistrar::registerHook(bool force)
{
    using Status = HookRegistrationResult::Status;

    HookRegistrationResult result;

    const QString probe = probePath();
    const QString settingsPath = ClaudePaths::claudeSettingsFilePath();

    const QString command = shellCommandFor(probe);

    result.probePath = command;
    result.settingsPath = QDir::toNativeSeparators(settingsPath);

    const auto fail = [&result](Status status, const QString &message) {
        result.status = status;
        result.message = message;
        qCWarning(ccmInfra).noquote() << message;
        return result;
    };

    // --- 1. 대상 실행 파일이 있는가 ---
    if (!QFileInfo(probe).isFile()) {
        return fail(Status::ProbeMissing,
                    QCoreApplication::translate("ccm", "ccm_probe 를 찾지 못했습니다: %1").arg(command));
    }

    // --- 2. 실제로 실행되는가 ---
    // --version 은 인자만 처리하고 즉시 끝나므로 부작용이 없다.
    QProcess check;
    check.start(probe, QStringList{QStringLiteral("--version")});
    if (!check.waitForStarted(kProbeCheckTimeoutMs)) {
        return fail(Status::ProbeNotRunnable,
                    QCoreApplication::translate("ccm", "ccm_probe 를 시작하지 못했습니다: %1").arg(check.errorString()));
    }
    if (!check.waitForFinished(kProbeCheckTimeoutMs)) {
        check.kill();
        check.waitForFinished();
        return fail(Status::ProbeNotRunnable,
                    QCoreApplication::translate("ccm", "ccm_probe 가 응답하지 않습니다: %1").arg(command));
    }
    if (check.exitStatus() != QProcess::NormalExit || check.exitCode() != 0) {
        return fail(Status::ProbeNotRunnable,
                    QStringLiteral("ccm_probe 실행 검증에 실패했습니다 (종료 코드 %1). "
                                   "Qt 런타임 DLL 이 실행 파일 옆에 있는지 확인하십시오: %2")
                        .arg(check.exitCode())
                        .arg(command));
    }
    qCDebug(ccmInfra) << "ccm_probe 실행 검증 통과:" << command;

    // --- 3. 기존 설정 읽기 ---
    QJsonObject root;
    const bool settingsExist = QFileInfo(settingsPath).isFile();
    if (settingsExist) {
        QFile file(settingsPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return fail(Status::Failed,
                        QCoreApplication::translate("ccm", "settings.json 을 열지 못했습니다: %1").arg(file.errorString()));
        }
        const QByteArray original = file.readAll();
        file.close();

        if (!original.trimmed().isEmpty()) {
            QJsonParseError parseError{};
            const QJsonDocument document = QJsonDocument::fromJson(original, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                return fail(Status::Failed,
                            QCoreApplication::translate("ccm", "settings.json 을 해석하지 못했습니다: %1")
                                .arg(parseError.errorString()));
            }
            root = document.object();
        }
    }

    // --- 4. 이미 등록되어 있는가 ---
    const QJsonValue existingNode = root.value(QLatin1String(kKeyStatusLine));
    if (existingNode.isObject()) {
        const QString existingCommand =
            existingNode.toObject().value(QLatin1String(kKeyCommand)).toString();
        result.existingCommand = existingCommand;

        if (sameCommand(existingCommand, command)) {
            result.status = Status::AlreadyRegistered;
            result.message = QCoreApplication::translate("ccm", "이미 같은 명령으로 등록되어 있습니다. 변경하지 않았습니다.");
            qCInfo(ccmInfra).noquote() << result.message;
            return result;
        }
        if (sameExecutable(existingCommand, command)) {
            // 우리 실행 파일인데 인자만 다르다. 묻지 않고 새 인자로 고친다.
            qCInfo(ccmInfra).noquote()
                << QStringLiteral("등록된 명령의 인자를 갱신합니다: %1 -> %2")
                       .arg(existingCommand, command);
        } else if (!force) {
            result.status = Status::Conflict;
            result.message =
                QCoreApplication::translate("ccm", "이미 다른 statusLine 설정이 있어 그대로 두었습니다: %1")
                    .arg(existingCommand);
            qCWarning(ccmInfra).noquote() << result.message;
            return result;
        }
    }

    // --- 5. 백업 ---
    // Qt 가 JSON 을 다시 쓰면 키 순서와 들여쓰기가 바뀐다. 되돌릴 수단을 먼저 만든다.
    if (settingsExist) {
        const QString backupPath = makeBackup(settingsPath);
        if (backupPath.isEmpty()) {
            return fail(Status::Failed,
                        QCoreApplication::translate("ccm", "백업을 만들지 못해 중단합니다: %1")
                            .arg(QDir::toNativeSeparators(settingsPath)));
        }
        result.backupPath = QDir::toNativeSeparators(backupPath);
        qCInfo(ccmInfra).noquote() << QStringLiteral("설정을 백업했습니다: %1").arg(result.backupPath);
    }

    // --- 6. 기록 ---
    QJsonObject hook;
    hook.insert(QLatin1String(kKeyType), QLatin1String(kTypeCommand));
    hook.insert(QLatin1String(kKeyCommand), command);
    root.insert(QLatin1String(kKeyStatusLine), hook);

    const QString directory = QFileInfo(settingsPath).absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "설정 디렉터리를 만들지 못했습니다: %1")
                        .arg(QDir::toNativeSeparators(directory)));
    }

    QSaveFile out(settingsPath);
    if (!out.open(QIODevice::WriteOnly)) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 을 쓰기 모드로 열지 못했습니다: %1")
                        .arg(out.errorString()));
    }
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(encoded) != encoded.size()) {
        out.cancelWriting();
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 기록 중 오류가 발생했습니다: %1")
                        .arg(out.errorString()));
    }
    if (!out.commit()) {
        return fail(Status::Failed,
                    QCoreApplication::translate("ccm", "settings.json 교체에 실패했습니다: %1").arg(out.errorString()));
    }

    result.status = Status::Registered;
    result.message = QCoreApplication::translate("ccm", "statusline 훅을 등록했습니다.");
    qCInfo(ccmInfra).noquote() << result.message << command;

    return result;
}

} // namespace ccm::infra
