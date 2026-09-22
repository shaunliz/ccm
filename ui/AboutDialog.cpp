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

#include "ui/AboutDialog.h"

#include "core/Version.h"
#include "ui/AppIcon.h"
#include "ui/StandardButtons.h"
#include "ui/UiColors.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTimer>
#include <QTextCursor>
#include <QPushButton>
#include <QVBoxLayout>

namespace ccm::ui {
namespace {

/// 실행 파일에 실려 있는 라이선스 전문.
constexpr auto kLicenseResource = ":/legal/LICENSE";

/// 문의 받을 자리.
constexpr auto kContactAddress = "ilkweon.ban@innogrid.com";

constexpr int kIconSize = 56;
constexpr int kLicenseWidth = 720;
constexpr int kLicenseHeight = 520;

} // namespace

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("ClaudeCodeMonitor 정보"));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(14);

    // --- 아이콘과 이름 ---
    auto *header = new QHBoxLayout();
    header->setSpacing(14);

    auto *icon = new QLabel(this);
    icon->setPixmap(appIcon().pixmap(kIconSize, kIconSize));
    icon->setFixedSize(kIconSize, kIconSize);
    header->addWidget(icon);

    auto *titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);
    titleBox->setAlignment(Qt::AlignVCenter);

    auto *name = new QLabel(tr("Claude Code 사용량 모니터"), this);
    QFont nameFont = name->font();
    nameFont.setPointSizeF(nameFont.pointSizeF() * 1.3);
    nameFont.setBold(true);
    name->setFont(nameFont);
    titleBox->addWidget(name);

    // 값은 숫자만 담는다. 'v' 는 보여 줄 때만 붙인다. 값에 넣으면 비교와
    // 파싱이 걸린다. (core/Version.h)
    auto *version = new QLabel(tr("버전 v%1").arg(ccm::core::applicationVersion()), this);
    applyHintTextColor(version);
    titleBox->addWidget(version);

    // 세로로 늘이지 않는다. 늘이면 창에 남는 높이를 이 자리가 다 먹어서
    // 이름과 설명 사이가 휑하게 벌어진다.
    header->addLayout(titleBox, 1);
    layout->addLayout(header);

    // --- 한 줄 설명 ---
    auto *summary = new QLabel(
        tr("Claude Code 사용 한도를 트레이에서 지켜봅니다. 세 지표(현재 세션, "
           "모든 모델, 모델별)의 사용률을 주기적으로 조회해 추이와 현재값을 "
           "보여 주고, 정해 둔 임계치를 넘으면 알립니다."),
        this);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    // --- 법적 고지 ---
    //
    // GPLv3 제0조가 요구하는 넷을 모두 담는다. 저작권, 보증 없음, 재배포 조건,
    // 사본을 보는 방법(아래 단추).
    auto *legal = new QLabel(
        tr("Copyright (C) 2026 Innogrid Co., Ltd.\n"
           "\n"
           "이 프로그램은 자유 소프트웨어입니다. GNU General Public License(GPL) "
           "버전 3의 조건에 따라 자유롭게 재배포하거나 수정할 수 있습니다.\n"
           "\n"
           "이 프로그램은 쓸모가 있기를 바라며 배포되지만 어떠한 보증도 하지 "
           "않습니다. 상품성이나 특정 목적 적합성에 대한 묵시적 보증도 "
           "없습니다. 자세한 내용은 아래 [라이선스 전문] 을 보십시오."),
        this);
    legal->setWordWrap(true);
    legal->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(legal);

    // Qt 버전과 설정 폴더를 적던 자리였다. 지웠다.
    //
    // Qt 를 적었던 것은 LGPLv3 제4조(a)가 "이 라이브러리를 썼다" 는 고지를
    // 요구하기 때문이었는데, 우리는 LGPL 에 기대지 않는다. Qt Charts 에 LGPL
    // 선택지가 없어 결합 저작물이 GPL-3.0-only 로 고정되고, 그러면 Qt 전체를
    // GPL-3.0-only 로 받는 것이 앞뒤가 맞는다. GPLv3 에는 쓴 라이브러리를
    // 열거하라는 조항이 없다.
    //
    // 나중에 Qt Charts 를 걷어내고 LGPL 경로로 가게 되면 그 고지가 다시
    // 필요해진다.

    // --- 문의 ---
    //
    // 누르면 메일 프로그램이 열리도록 서식 있는 글로 둔다. 주소를 손으로
    // 옮겨 적게 하면 오타가 난다.
    auto *contact = new QLabel(this);
    contact->setText(tr("문의: <a href=\"mailto:%1\">%1</a>")
                         .arg(QLatin1String(kContactAddress)));
    contact->setTextFormat(Qt::RichText);
    contact->setOpenExternalLinks(true);
    contact->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(contact);

    // --- 단추 ---
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *license = buttons->addButton(tr("라이선스 전문"),
                                              QDialogButtonBox::ActionRole);
    connect(license, &QPushButton::clicked, this, &AboutDialog::showLicense);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    localizeButtons(buttons);
    layout->addWidget(buttons);

    setMinimumWidth(470);
}

void AboutDialog::showLicense()
{
    // QFile file(QLatin1String(...)) 로 적으면 컴파일러가 함수 선언으로 읽는다.
    // (most vexing parse) QString 으로 만들어 넘긴다.
    QFile file(QString::fromLatin1(kLicenseResource));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 리소스는 실행 파일 안에 있으므로 여기 올 일이 없다. 그래도 말없이
        // 아무 일도 일어나지 않는 것보다는 낫다.
        showWarning(this, tr("라이선스 전문"),
                    tr("라이선스 전문을 읽지 못했습니다."));
        return;
    }
    const QString text = QString::fromUtf8(file.readAll());
    file.close();

    auto *window = new QDialog(this);
    window->setWindowTitle(tr("GNU General Public License v3.0"));
    window->setAttribute(Qt::WA_DeleteOnClose);

    auto *layout = new QVBoxLayout(window);

    auto *view = new QPlainTextEdit(window);
    view->setReadOnly(true);
    // 원문은 74칸에서 손으로 줄바꿈되어 있다. 가변폭 글꼴로 보거나 자동
    // 줄바꿈을 켜면 들쭉날쭉해져 읽기 어렵다. 등폭에 줄바꿈을 끄고, 대신
    // 가로 스크롤을 준다.
    //
    // 글꼴과 줄바꿈을 글보다 먼저 정한다. 글을 넣은 뒤에 바꾸면 다시 배치하면서
    // 보던 자리가 밀린다.
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    view->setPlainText(text);
    view->moveCursor(QTextCursor::Start);
    layout->addWidget(view);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, window);
    connect(buttons, &QDialogButtonBox::rejected, window, &QDialog::accept);
    localizeButtons(buttons);
    layout->addWidget(buttons);

    window->resize(kLicenseWidth, kLicenseHeight);
    window->show();

    // 맨 위에서 시작한다. 제목부터 보여야 무엇을 보고 있는지 알 수 있다.
    //
    // 창을 띄운 뒤에 한 번 더 미뤄 잡는다. 긴 글은 배치가 조금씩 나뉘어
    // 진행되고, 그 도중에 커서를 따라 보던 자리가 끝으로 밀린다. 배치가
    // 끝난 뒤에 정해야 그대로 남는다.
    QTimer::singleShot(0, view, [view] {
        view->verticalScrollBar()->setValue(0);
    });
}

} // namespace ccm::ui
