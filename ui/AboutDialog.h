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
//  ui/AboutDialog.h
//
//  프로그램 정보 창. 이름, 버전, 만든 곳, 라이선스 고지를 담는다.
//
//  왜 필요한가
//   첫째로 버전을 볼 자리가 필요하다. 지금까지는 탐색기의 파일 속성이나
//   `ccm_probe --version` 밖에 없어서, 쓰는 사람이 "지금 이게 몇 번인가" 를
//   프로그램 안에서 알 길이 없었다.
//
//   둘째로 GPLv3 가 요구한다. 제5조(d)는 대화형 화면을 가진 프로그램에
//   "Appropriate Legal Notices" 를 띄우라고 하고, 제0조가 그것을 넷으로
//   정의한다. 저작권 고지, 보증이 없다는 사실, 조건을 지키면 재배포할 수
//   있다는 사실, 그리고 라이선스 사본을 보는 방법이다.
//
//  전문을 본문에 넣지 않는 이유
//   요구되는 것은 "사본을 보는 방법" 이지 전문 그 자체가 아니다. 35KB 를
//   본문에 쏟아 넣으면 정작 읽어야 할 네 줄이 묻힌다. 그래서 짧은 고지를
//   본문에 두고, 전문은 [라이선스 전문] 단추 뒤의 스크롤 창에 둔다.
//
//  전문을 실행 파일에 넣어 두는 이유
//   설치 폴더의 LICENSE 파일을 읽는 방법도 있지만, 개발 빌드나 파일을 지운
//   경우에 못 읽는다. "사본을 볼 수 있다" 는 약속이 파일의 유무에 걸리면
//   안 되므로 리소스로 싣는다. (:/legal/LICENSE)
// ============================================================================
#ifndef CCM_UI_ABOUTDIALOG_H
#define CCM_UI_ABOUTDIALOG_H

#include <QDialog>

namespace ccm::ui {

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    /// 라이선스 전문을 스크롤 창에 띄운다.
    void showLicense();
};

} // namespace ccm::ui

#endif // CCM_UI_ABOUTDIALOG_H
