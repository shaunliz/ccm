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
//  infra/Translations.h
//
//  번역 파일을 찾아 설치한다.
//
//  원문이 한국어인 이유
//   화면 글자를 한국어로 적고, 영어를 번역으로 얹는다. 원문을 영어로 두고
//   한국어를 번역으로 얹는 방법도 있지만, 이 프로그램은 한국어로 먼저 쓰였고
//   원문을 뒤집으면 기존 글자를 전부 다시 쓰게 된다.
//
//   그래서 한국어 환경에서는 번역 파일이 없어도 그대로 한국어가 나오고,
//   영어 환경에서는 ClaudeCodeMonitor_en.qm 이 얹힌다.
//
//  두 실행 파일이 함께 쓴다
//   위젯 앱과 statusline 훅 모두 화면/콘솔에 한국어를 낸다. 설치 코드를 양쪽에
//   복사해 두면 한쪽만 고치는 일이 생기므로 여기로 모았다.
// ============================================================================
#ifndef CCM_INFRA_TRANSLATIONS_H
#define CCM_INFRA_TRANSLATIONS_H

#include <QString>

namespace ccm::infra {

class Translations
{
public:
    Translations() = delete;

    /// 시스템 로케일에 맞는 번역을 설치한다.
    /// QCoreApplication 을 만든 뒤에 호출한다.
    ///
    /// \return 설치한 번역의 이름. 원문(한국어)을 그대로 쓰면 빈 문자열.
    static QString install();
};

} // namespace ccm::infra

#endif // CCM_INFRA_TRANSLATIONS_H
