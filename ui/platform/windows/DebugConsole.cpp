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
//  ui/platform/windows/DebugConsole.cpp
//
//  Windows 구현. AllocConsole 로 콘솔을 붙이고 stderr 를 그리로 돌린다.
//
//  왜 이 파일에는 #ifdef 가 없나
//   예전에는 한 파일 안에서 #ifdef Q_OS_WIN 으로 갈랐다. 그런데 그 방식은
//   윈도우가 아닌 쪽을 아무도 컴파일해 보지 않아 조용히 깨져 있었다.
//   (윈도우 전용 구역 안에 있던 translate() 를 #else 쪽에서 부르고 있었다)
//   파일을 나누면 각 구현이 제 힘으로 서야 하므로 그런 일이 생기지 않는다.
//
//  다른 운영체제의 구현은 platform/macos, platform/linux 에 있다.
// ============================================================================
#include "ui/DebugConsole.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include <cstdio>

#include <windows.h>

namespace ccm::ui {
namespace {

/// 콘솔이 붙어 있는 동안 참.
bool g_open = false;

/// 콘솔을 열기 전의 로그 수준. 닫을 때 되돌린다.
ccm::infra::LogLevel g_previousLevel = ccm::infra::LogLevel::Info;

/// 되돌아 볼 수 있는 줄 수. 기본값은 화면 몇 배 정도라 조회 몇 번이면 넘친다.
constexpr SHORT kBufferLines = 9999;
constexpr SHORT kBufferColumns = 200;

QString translate(const char *source)
{
    return QCoreApplication::translate("ccm", source);
}

/// 콘솔에 한 줄 적는다. 로그가 아니라 사람에게 하는 말이다.
void say(const QString &text)
{
    std::fputs(text.toUtf8().constData(), stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

/// 닫기 단추를 없앤다.
///
/// 콘솔을 닫으면 프로세스가 함께 끝나므로, 실수로 누르는 길을 막아 둔다.
/// 윈도우 터미널이 기본 터미널이면 GetConsoleWindow 가 주는 창 핸들이 실제 창이
/// 아니라 아무 일도 일어나지 않는다. 실패해도 진행한다.
void disableCloseButton()
{
    HWND window = ::GetConsoleWindow();
    if (window == nullptr) {
        return;
    }
    if (HMENU menu = ::GetSystemMenu(window, FALSE)) {
        ::DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        ::DrawMenuBar(window);
    }
}

} // namespace

bool DebugConsole::isAvailable()
{
    return true;
}

bool DebugConsole::isOpen()
{
    return g_open;
}

bool DebugConsole::open(QString *error)
{
    if (g_open) {
        return true;
    }

    if (::AllocConsole() == FALSE) {
        // 이미 콘솔을 가진 채 시작한 경우다. (cmd 에서 띄웠다면 그럴 수 있다)
        // 그때는 붙일 것이 없으므로 지금 것을 그대로 쓴다.
        if (::GetConsoleWindow() == nullptr) {
            if (error) {
                *error = translate("콘솔을 만들지 못했습니다. (오류 %1)")
                             .arg(static_cast<uint>(::GetLastError()));
            }
            return false;
        }
    }

    // 콘솔 기본 코드페이지는 한국어 윈도우에서 CP949 다. 로그는 UTF-8 로
    // 나가므로 맞춰 주지 않으면 한글이 깨진다.
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);

    // stderr 를 새 콘솔로 돌린다. 로거가 stderr 에만 쓰므로 이 한 줄로 충분하다.
    FILE *stream = nullptr;
    if (::freopen_s(&stream, "CONOUT$", "w", stderr) != 0) {
        if (error) {
            *error = translate("콘솔로 로그를 보내지 못했습니다.");
        }
        ::FreeConsole();
        return false;
    }

    const COORD size{kBufferColumns, kBufferLines};
    ::SetConsoleScreenBufferSize(::GetStdHandle(STD_ERROR_HANDLE), size);
    ::SetConsoleTitleW(L"ClaudeCodeMonitor - 디버그 콘솔");
    disableCloseButton();

    g_open = true;

    // 수준을 올리기 전에 기억해 둔다.
    g_previousLevel = ccm::infra::Logger::minimumLevel();
    ccm::infra::Logger::setMinimumLevel(ccm::infra::LogLevel::Debug);

    say(QString());
    say(translate("=== ClaudeCodeMonitor 디버그 콘솔 ==="));
    say(translate("이 창을 닫으면 프로그램도 함께 종료됩니다. "
                  "[도구 > 디버그 콘솔] 로 닫으십시오."));
    // 수준 이름이 영문이라 조사를 붙이면 어색하다. 괄호로 떼어 적는다.
    say(translate("로그 수준을 디버그로 올렸습니다. 이전 수준: %1 "
                  "(콘솔을 닫으면 되돌립니다)")
            .arg(ccm::infra::Logger::levelName(g_previousLevel)));
    const QString file = ccm::infra::Logger::logFilePath();
    if (!file.isEmpty()) {
        say(translate("같은 내용이 파일에도 쌓입니다: %1")
                .arg(QDir::toNativeSeparators(file)));
    }
    say(QString());

    qCInfo(ccmApp) << "디버그 콘솔을 열었습니다.";
    return true;
}

void DebugConsole::close()
{
    if (!g_open) {
        return;
    }

    ccm::infra::Logger::setMinimumLevel(g_previousLevel);
    qCInfo(ccmApp) << "디버그 콘솔을 닫았습니다.";

    // 콘솔을 떼기 전에 stderr 를 빈 곳으로 돌린다. FreeConsole 이 CONOUT$ 핸들을
    // 무효로 만들기 때문이다. 그대로 두면 닫힌 핸들에 계속 쓰게 되고, 그 뒤의
    // 로그가 어디로도 가지 않는다.
    FILE *stream = nullptr;
    ::freopen_s(&stream, "NUL", "w", stderr);

    ::FreeConsole();
    g_open = false;
}

} // namespace ccm::ui
