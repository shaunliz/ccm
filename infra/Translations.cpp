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

#include "infra/Translations.h"

#include "infra/Logger.h"

#include <QCoreApplication>
#include <QLocale>
#include <QStringList>
#include <QTranslator>

namespace ccm::infra {
namespace {

/// qt_add_translations 가 .qm 을 올려 두는 자리.
constexpr auto kResourcePrefix = ":/i18n/";
constexpr auto kFileNameBase = "ClaudeCodeMonitor_";

} // namespace

QString Translations::install()
{
    // 번역기는 프로그램이 끝날 때까지 살아 있어야 한다. 지역 변수로 두면
    // 설치 직후 사라져 글자가 원문으로 돌아간다.
    static QTranslator translator;

    const QStringList languages = QLocale::system().uiLanguages();
    for (const QString &language : languages) {
        // 원문이 한국어다. 한국어가 목록에 나오면 거기서 멈춘다.
        //
        // 멈추지 않으면 뒤에 오는 언어가 자리를 가로챈다. 윈도우는 표시 언어를
        // 여러 개 둘 수 있어서 ["ko-KR", "en-US"] 같은 목록이 흔하다. 한국어는
        // 번역 파일이 없는 것이 정상(원문이므로)인데, 그것을 "없으니 다음"
        // 으로 다루면 영어가 얹힌다. 한국어를 쓰겠다는 사람에게 영어가 나온다.
        if (QLocale(language).language() == QLocale::Korean) {
            qCDebug(ccmInfra) << "한국어가 우선이라 원문을 씁니다. 후보:" << languages;
            return QString();
        }

        const QString name = QLatin1String(kFileNameBase) + QLocale(language).name();
        // load 는 뒤쪽 지역/문자 표기를 떼며 찾는다. en_US 로 시작해도
        // ClaudeCodeMonitor_en.qm 을 찾아낸다.
        if (translator.load(QLatin1String(kResourcePrefix) + name)) {
            QCoreApplication::installTranslator(&translator);
            qCInfo(ccmInfra).noquote()
                << QStringLiteral("번역을 설치했습니다: %1").arg(translator.filePath());
            return translator.language();
        }
    }

    qCDebug(ccmInfra) << "맞는 번역이 없어 원문(한국어)을 씁니다. 후보:" << languages;
    return QString();
}

} // namespace ccm::infra
