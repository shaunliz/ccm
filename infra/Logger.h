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
//  infra/Logger.h
//
//  로깅 기반 구조.
//
//  설계 원칙
//   1. 모든 로그는 stderr 로 나간다. stdout 은 statusline 출력과 보고서 전용이다.
//      Claude Code 는 statusline 스크립트의 stdout 을 상태줄로 읽으므로,
//      로그가 stdout 으로 새면 상태줄이 오염된다.
//   2. QLoggingCategory 를 사용해 영역별로 필터링 가능하게 한다.
//   3. 파일 로깅은 선택 사항이며, 크기 초과 시 1세대 회전한다.
// ============================================================================
#ifndef CCM_INFRA_LOGGER_H
#define CCM_INFRA_LOGGER_H

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(ccmCore)
Q_DECLARE_LOGGING_CATEGORY(ccmInfra)
Q_DECLARE_LOGGING_CATEGORY(ccmApp)

namespace ccm::infra {

/// 로그 수준. 값이 클수록 심각하다.
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger
{
public:
    Logger() = delete;

    /// 메시지 핸들러를 설치한다. 프로그램 시작 직후 1회 호출한다.
    /// \param minimumLevel 이 수준 미만은 출력하지 않는다.
    /// \param logFilePath  비어 있지 않으면 파일에도 기록한다.
    static void install(LogLevel minimumLevel = LogLevel::Info,
                        const QString &logFilePath = QString());

    /// 이전 핸들러를 복원한다.
    static void shutdown();

    static void setMinimumLevel(LogLevel level);
    static LogLevel minimumLevel();

    /// "debug", "info", "warning", "error" 를 해석한다. 실패하면 false.
    static bool parseLevel(const QString &text, LogLevel *out);

    static QString levelName(LogLevel level);

    /// 현재 설정된 로그 파일 경로. 파일 로깅이 꺼져 있으면 빈 문자열.
    static QString logFilePath();
};

} // namespace ccm::infra

#endif // CCM_INFRA_LOGGER_H
