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
//  infra/ClaudePaths.h
//
//  본 프로그램과 Claude Code 가 사용하는 경로를 한곳에서 결정한다.
//  경로 문자열을 코드 전반에 흩뿌리지 않기 위한 단일 지점이다.
//
//  Claude Code 설정 디렉터리는 CLAUDE_CONFIG_DIR 환경변수로 재정의될 수 있으므로
//  홈 디렉터리를 하드코딩하지 않는다.
// ============================================================================
#ifndef CCM_INFRA_CLAUDEPATHS_H
#define CCM_INFRA_CLAUDEPATHS_H

#include <QString>

namespace ccm::infra {

class ClaudePaths
{
public:
    ClaudePaths() = delete;

    // --- 본 프로그램 측 경로 ---

    /// %LOCALAPPDATA%/ClaudeCodeMonitor (Windows 기준).
    static QString appDataDir();

    /// 스냅샷 파일의 기본 경로.
    static QString snapshotFilePath();

    /// 설정 파일. INI 로 둔다. 레지스트리보다 들여다보기 쉽다.
    static QString settingsFilePath();

    /// 사용률 이력. 차트가 읽는다.
    static QString historyFilePath();

    /// 위젯 앱이 훅에게 건네는 모델별 사용률. 상태줄에만 쓴다.
    /// (infra/StatusLineCache)
    static QString statusLineCacheFilePath();

    /// 알림 기록. 알림 창이 읽는다.
    static QString alertFilePath();

    /// 위젯 실행 로그. GUI 앱은 콘솔이 없어 파일이 유일한 추적 수단이다.
    static QString logFilePath();

    // --- Claude Code 측 경로 ---

    /// CLAUDE_CONFIG_DIR 이 있으면 그 값을, 없으면 홈 아래 .claude 를 돌려준다.
    static QString claudeHomeDir();

    /// 훅을 등록할 settings.json 경로.
    static QString claudeSettingsFilePath();
};

} // namespace ccm::infra

#endif // CCM_INFRA_CLAUDEPATHS_H
