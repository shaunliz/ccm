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
//  ui/StandardButtons.h
//
//  Qt 표준 버튼의 글자를 우리 말로 바꾼다.
//
//  왜 필요한가
//   QDialogButtonBox::Save 나 QMessageBox::question 처럼 Qt 가 이름을 붙여 주는
//   버튼은 Qt 자신의 번역 파일(qtbase_ko.qm)로 번역된다. 그것을 싣지 않으면
//   "Save", "Cancel", "Close", "Yes", "No", "OK" 가 영어로 나온다.
//   화면의 나머지가 모두 한국어인데 버튼만 영어가 되는 것이 그 때문이다.
//
//  Qt 번역을 싣는 대신 직접 적는 이유
//   Qt 번역을 실으면 버튼 언어가 시스템 로케일을 따라간다. 그런데 이 프로그램의
//   글자는 로케일이 아니라 우리 번역 파일이 정한다. (원문 한국어, 영어는 번역)
//   그래서 영어 환경에서 우리 글자는 한국어인데 버튼만 영어가 되는, 섞인 화면이
//   여전히 남는다.
//
//   버튼 글자를 우리 번역 시스템에 함께 넣으면 화면 전체가 언제나 한 언어로
//   맞는다. Qt 번역 파일을 배포에 포함할 필요도 없다.
//
//  이 자리를 한곳에 모으는 이유
//   같은 버튼이 창마다 나온다. 창마다 따로 적으면 한 곳을 빠뜨려 "저장" 옆에
//   "Cancel" 이 놓이게 된다.
// ============================================================================
#ifndef CCM_UI_STANDARDBUTTONS_H
#define CCM_UI_STANDARDBUTTONS_H

#include <QString>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QWidget;
QT_END_NAMESPACE

namespace ccm::ui {

/// 버튼 상자에 들어 있는 표준 버튼의 글자를 우리 말로 바꾼다.
/// 없는 버튼은 그냥 넘어간다.
void localizeButtons(QDialogButtonBox *box);

/// 예 / 아니요 질문. 예를 고르면 true.
/// 기본 선택은 "아니요" 다. 확인을 묻는 자리는 대개 되돌릴 수 없는 일이다.
bool askYesNo(QWidget *parent, const QString &title, const QString &text);

/// 알림 창. 버튼은 "확인" 하나.
void showInfo(QWidget *parent, const QString &title, const QString &text);

/// 경고 창. 버튼은 "확인" 하나.
void showWarning(QWidget *parent, const QString &title, const QString &text);

} // namespace ccm::ui

#endif // CCM_UI_STANDARDBUTTONS_H
