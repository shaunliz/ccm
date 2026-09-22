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
//  ui/AppIcon.h
//
//  프로그램 아이콘. 그림 파일이 아니라 그리기 코드가 원본이다.
//
//  무엇을 그리는가
//   네 다리로 선 작은 로봇이 모니터 앞에 앉아 꺾은선 그래프를 지켜보는 모습.
//   "사용량을 지켜보는 도우미" 라는 이 프로그램의 일을 한 장면으로 담는다.
//   남의 상표를 베끼지 않도록 형태는 직접 지었다.
//
//  왜 코드로 그리는가
//   화면 배율이 100% 가 아닌 환경이 흔하고, 트레이(16px)부터 설치 관리자
//   (256px)까지 크기 폭이 넓다. 한 장을 늘려 쓰면 작은 크기에서 뭉개지므로
//   크기마다 선 두께와 생략 정도를 달리 그린다.
//
//   창 아이콘과 작업 표시줄은 이 코드가 바로 그린다. 탐색기와 설치 관리자가
//   읽는 .ico 는 이 코드로 한 번 만들어 넣은 산출물이다. (res/ 참조)
// ============================================================================
#ifndef CCM_UI_APPICON_H
#define CCM_UI_APPICON_H

#include <QIcon>
#include <QPixmap>

namespace ccm::ui {

/// 한 변이 size 인 아이콘 한 장.
QPixmap appIconPixmap(int size);

/// 여러 크기를 담은 아이콘. 창과 작업 표시줄이 알맞은 것을 골라 쓴다.
QIcon appIcon();

} // namespace ccm::ui

#endif // CCM_UI_APPICON_H
