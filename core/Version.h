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
//  core/Version.h
//
//  프로그램 버전 한 곳.
//
//  왜 한 곳이어야 하는가
//   예전에는 버전 문자열이 세 곳에 따로 박혀 있었다. 위젯 앱, 훅, NSIS 스크립트.
//   그러다 훅만 0.1.0 에 머무르고 나머지가 0.2.0 으로 올라가, `ccm_probe
//   --version` 이 설치된 것과 다른 번호를 말하는 일이 생겼다.
//
//   이제 CMake 의 project(VERSION ...) 이 유일한 원본이다. 여기로는 컴파일
//   정의(CCM_VERSION_STRING)로 들어오고, 실행 파일 리소스(VERSIONINFO)와
//   NSIS 스크립트에도 같은 값이 생성되어 들어간다.
// ============================================================================
#ifndef CCM_CORE_VERSION_H
#define CCM_CORE_VERSION_H

#include <QLatin1String>
#include <QString>

// 빌드 시스템이 주지 않는 경우를 대비한다. (IDE 가 파일 하나만 여는 경우 등)
// 실제 배포본에서는 언제나 CMake 가 준 값이 쓰인다.
#ifndef CCM_VERSION_STRING
#  define CCM_VERSION_STRING "0.0.0-dev"
#endif

namespace ccm::core {

inline QString applicationVersion()
{
    return QString(QLatin1String(CCM_VERSION_STRING));
}

} // namespace ccm::core

#endif // CCM_CORE_VERSION_H
