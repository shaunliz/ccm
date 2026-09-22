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

#include "ui/HookSetup.h"

#include "infra/ClaudePaths.h"
#include "infra/HookRegistrar.h"
#include "infra/Logger.h"
#include "ui/StandardButtons.h"

#include <QChar>
#include <QDir>
#include <QCoreApplication>
#include <QStringList>

namespace ccm::ui {
namespace {

/// 번역 컨텍스트. 창에 매이지 않은 글자이므로 프로그램 공용 자리를 쓴다.
QString text(const char *source)
{
    return QCoreApplication::translate("ccm", source);
}

/// 명령에서 실행 파일 부분만. 따옴표로 감싼 첫 덩이가 그것이다.
QString executableOf(const QString &command)
{
    const QString value = command.trimmed();
    if (value.startsWith(QLatin1Char('"'))) {
        const int closing = value.indexOf(QLatin1Char('"'), 1);
        if (closing > 0) {
            return value.mid(1, closing - 1);
        }
    }
    const int space = value.indexOf(QLatin1Char(' '));
    return space > 0 ? value.left(space) : value;
}

/// 실행 파일 뒤에 붙은 인자. 없으면 빈 문자열.
QString argumentsOf(const QString &command)
{
    const QString value = command.trimmed();
    const QString executable = executableOf(value);
    // 따옴표가 있었다면 두 글자만큼 더 건너뛴다.
    const int skip = value.startsWith(QLatin1Char('"')) ? executable.size() + 2
                                                        : executable.size();
    return value.mid(skip).trimmed();
}

} // namespace

bool runHookRegistration(QWidget *parent, QString *summary)
{
    using Status = ccm::infra::HookRegistrationResult::Status;

    const QString title = text("statusline 훅 등록");

    ccm::infra::HookRegistrationResult result = ccm::infra::HookRegistrar::registerHook();

    // 다른 statusLine 설정이 있으면 말없이 덮지 않고 확인받는다.
    if (result.status == Status::Conflict) {
        const QString question = QStringList{
            text("이미 다른 statusLine 설정이 있습니다."),
            QString(),
            text("현재 설정: %1").arg(result.existingCommand),
            QString(),
            text("백업한 뒤 교체할까요?")
        }.join(QChar::LineFeed);

        if (!askYesNo(parent, title, question)) {
            qCInfo(ccmApp) << "사용자가 훅 등록을 취소했습니다.";
            if (summary) {
                *summary = text("등록을 취소했습니다.");
            }
            return false;
        }
        result = ccm::infra::HookRegistrar::registerHook(true);
    }

    QStringList lines;
    lines << result.message;
    lines << QString();
    lines << text("명령      : %1").arg(result.probePath);
    lines << text("설정 파일 : %1").arg(result.settingsPath);
    if (!result.backupPath.isEmpty()) {
        lines << text("백업      : %1").arg(result.backupPath);
    }

    if (result.status == Status::Registered) {
        lines << QString();
        lines << text("Claude Code 가 상태줄을 갱신할 때 스냅샷이 만들어집니다.");
    }

    const QString body = lines.join(QChar::LineFeed);

    if (result.ok()) {
        qCInfo(ccmApp).noquote() << result.message;
        showInfo(parent, title, body);
    } else {
        qCWarning(ccmApp).noquote() << result.message;
        showWarning(parent, title, body);
    }

    if (summary) {
        *summary = result.message;
    }
    return result.ok();
}

namespace {

/// 확인 창에 보여 줄 내용.
///
/// 무엇을 지우는지 한눈에 보이게 한다. 경로만 적으면 "그래서 이걸 지우면
/// 무슨 일이 생기나" 를 알 수 없으므로, 대상과 결과를 함께 적는다.
QString removalQuestion(const ccm::infra::HookRegistrar::State state,
                        const QString &command)
{
    const QString executable = executableOf(command);
    const QString arguments = argumentsOf(command);

    QStringList lines;

    if (state == ccm::infra::HookRegistrar::State::OtherCopy) {
        lines << text("등록된 것은 이 프로그램의 다른 자리에 있는 복사본입니다.");
        lines << text("그 복사본을 쓰고 있다면 그쪽 상태줄이 사라집니다.");
        lines << QString();
    }

    lines << text("아래 statusline 훅 등록을 지웁니다.");
    lines << QString();
    lines << text("실행 파일 : %1").arg(executable.section(QLatin1Char('/'), -1));
    lines << text("경로      : %1").arg(executable);
    if (!arguments.isEmpty()) {
        lines << text("인자      : %1").arg(arguments);
    }
    lines << text("설정 파일 : %1")
                 .arg(QDir::toNativeSeparators(ccm::infra::ClaudePaths::claudeSettingsFilePath()));
    lines << QString();
    lines << text("지우면 Claude Code 터미널 아래의 사용률 표시가 사라집니다.");
    lines << text("사용량 조회와 알림은 그대로 동작합니다.");
    lines << text("고치기 전에 설정 파일을 백업합니다.");
    lines << QString();
    lines << text("계속할까요?");

    return lines.join(QChar::LineFeed);
}

} // namespace

bool runHookRemoval(QWidget *parent, QString *summary)
{
    using Registrar = ccm::infra::HookRegistrar;
    using Status = ccm::infra::HookRemovalResult::Status;

    const QString title = text("statusline 훅 해제");

    QString command;
    const Registrar::State state = Registrar::state(&command);

    // 지울 것이 없거나 남의 것이면 파일을 건드리기 전에 끝난다.
    if (state == Registrar::State::NotRegistered) {
        const QString message = text("등록된 statusline 훅이 없습니다.");
        showInfo(parent, title, message);
        if (summary) {
            *summary = message;
        }
        return false;
    }

    if (state == Registrar::State::Other) {
        const QString message =
            QStringList{
                text("다른 프로그램의 statusLine 설정입니다. 지우지 않았습니다."),
                QString(),
                text("등록된 명령: %1").arg(command),
                QString(),
                text("이 프로그램이 등록한 것만 지웁니다."),
            }.join(QChar::LineFeed);
        qCInfo(ccmApp) << "남의 훅이라 지우지 않았습니다.";
        showWarning(parent, title, message);
        if (summary) {
            *summary = text("다른 프로그램의 설정이라 지우지 않았습니다.");
        }
        return false;
    }

    if (!askYesNo(parent, title, removalQuestion(state, command))) {
        qCInfo(ccmApp) << "사용자가 훅 해제를 취소했습니다.";
        if (summary) {
            *summary = text("해제를 취소했습니다.");
        }
        return false;
    }

    // 다른 복사본이라도 방금 확인을 받았으므로 지운다.
    const ccm::infra::HookRemovalResult result =
        Registrar::unregisterHook(state == Registrar::State::OtherCopy);

    QStringList lines;
    lines << result.message;
    if (result.status == Status::Removed) {
        lines << QString();
        lines << text("설정 파일 : %1").arg(result.settingsPath);
        if (!result.backupPath.isEmpty()) {
            lines << text("백업      : %1").arg(result.backupPath);
        }
    }
    const QString body = lines.join(QChar::LineFeed);

    if (result.ok()) {
        qCInfo(ccmApp).noquote() << result.message;
        showInfo(parent, title, body);
    } else {
        qCWarning(ccmApp).noquote() << result.message;
        showWarning(parent, title, body);
    }

    if (summary) {
        *summary = result.message;
    }
    return result.status == Status::Removed;
}

} // namespace ccm::ui
