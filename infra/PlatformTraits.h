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
//  infra/PlatformTraits.h
//
//  운영체제마다 다르지만 "파일을 나눌 만큼은 아닌" 것들을 모아 둔 자리.
//
//  왜 한곳에 모으나
//   상수 하나, 줄 하나가 다른 것까지 platform/<os>/ 로 쪼개면 같은 로직이 세 벌이
//   되어 반드시 어긋난다. 반대로 쓰는 자리마다 #ifdef 를 두면 분기가 코드 전체에
//   흩어져 "이 프로그램이 OS 에 기대는 것이 무엇인가" 를 한눈에 볼 수 없다.
//   그래서 그런 것들은 전부 이 파일에 둔다. 새 운영체제를 올릴 때 여기부터 본다.
//
//  파일을 통째로 나눈 것들은 여기 없다. 그쪽은 각 헤더의 주석을 보라.
//   infra/AutoStart.h  infra/AppIdentity.h  infra/ConsoleEncoding.h
//   ui/DebugConsole.h
// ============================================================================
#ifndef CCM_INFRA_PLATFORMTRAITS_H
#define CCM_INFRA_PLATFORMTRAITS_H

#include <QLatin1String>
#include <QStringList>
#include <QtGlobal>

namespace ccm::infra::platform {

/// statusline 훅 실행 파일의 이름.
///
/// settings.json 에 적히는 명령이 이 이름으로 끝나는지 보고 "우리가 등록한
/// 것인가" 를 가린다. (infra/HookRegistrar.cpp)
inline QLatin1String probeExecutableName()
{
#ifdef Q_OS_WIN
    return QLatin1String("ccm_probe.exe");
#else
    return QLatin1String("ccm_probe");
#endif
}

/// 경로 두 개를 견줄 때 대소문자를 따질 것인가.
///
/// Windows 와 macOS 는 따지지 않는다. macOS 는 파일 시스템을 대소문자를 가리게
/// 만들 수도 있지만 기본값이 가리지 않는 쪽이고, 여기서 비교하는 것은 우리가
/// 적은 경로와 사용자가 가진 경로가 "같은 파일인가" 이므로 기본값에 맞춘다.
/// 틀리는 쪽으로 기울 때, 같은 것을 다르다고 보는 쪽이 피해가 크다.
inline Qt::CaseSensitivity pathCaseSensitivity()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

/// claude 실행 파일을 PATH 에서 찾지 못했을 때 들여다볼 자리들.
///
/// GUI 앱이 물려받는 PATH 에는 사용자가 셸에서 더한 경로가 없을 수 있다.
/// 그래서 네이티브 설치본이 흔히 놓이는 자리를 직접 본다.
/// \param home 사용자의 홈 디렉터리. (QDir::homePath())
inline QStringList claudeExecutableCandidates(const QString &home)
{
#if defined(Q_OS_WIN)
    return QStringList{
        home + QStringLiteral("/.local/bin/claude.exe"),
        home + QStringLiteral("/.local/bin/claude"),
    };
#elif defined(Q_OS_MACOS)
    return QStringList{
        home + QStringLiteral("/.local/bin/claude"),
        // Homebrew. 애플 실리콘은 /opt/homebrew, 인텔은 /usr/local 이다.
        QStringLiteral("/opt/homebrew/bin/claude"),
        QStringLiteral("/usr/local/bin/claude"),
    };
#else
    return QStringList{
        home + QStringLiteral("/.local/bin/claude"),
        QStringLiteral("/usr/local/bin/claude"),
        QStringLiteral("/usr/bin/claude"),
    };
#endif
}

} // namespace ccm::infra::platform

#endif // CCM_INFRA_PLATFORMTRAITS_H
