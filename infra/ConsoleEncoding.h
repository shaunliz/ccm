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
//  infra/ConsoleEncoding.h
//
//  콘솔이 UTF-8 을 그대로 읽고 쓰게 맞춘다.
//
//  이 프로그램의 로그와 상태줄 문자열은 전부 UTF-8 이다. 콘솔이 다른 코드
//  페이지를 쓰면 한글이 깨져 나온다.
//
//  선언만 여기 있고 구현은 운영체제마다 다르다.
//   Windows : platform/windows/ConsoleEncoding.cpp  코드 페이지를 바꾼다.
//   macOS   : platform/macos/ConsoleEncoding.cpp    할 일이 없다.
//   Linux   : platform/linux/ConsoleEncoding.cpp    할 일이 없다.
// ============================================================================
#ifndef CCM_INFRA_CONSOLEENCODING_H
#define CCM_INFRA_CONSOLEENCODING_H

namespace ccm::infra {

class ConsoleEncoding
{
public:
    ConsoleEncoding() = delete;

    /// 콘솔을 쓰는 진입점에서 맨 먼저 한 번 부른다.
    static void configure();
};

} // namespace ccm::infra

#endif // CCM_INFRA_CONSOLEENCODING_H
