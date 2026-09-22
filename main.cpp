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
//  main.cpp
//
//  위젯 앱 진입점.
//
//  여기서 정하는 것 세 가지
//   1. 단일 인스턴스 : 시작프로그램 등록과 수동 실행이 겹치면 트레이 아이콘이
//                      두 개 뜬다. 먼저 뜬 쪽에 창을 띄우라고 알리고 물러난다.
//   2. --tray        : 로그인 직후 창이 튀어나오지 않게 트레이로만 올라간다.
//                      시작프로그램에 등록되는 명령이 이 인자를 붙인다.
//   3. 첫 실행 등록  : 처음 실행될 때 시작프로그램에 자동 등록한다. 설정에서
//                      끌 수 있고, 한 번 끄면 다시 켜지 않는다.
//
//  로그는 파일로도 남긴다. GUI 앱은 콘솔이 없어 stderr 가 어디에도 보이지
//  않으므로, 트레이에서 조용히 도는 동안의 문제를 추적할 방법이 파일뿐이다.
// ============================================================================
#include "mainwindow.h"

#include "core/Version.h"
#include "infra/AppIdentity.h"
#include "infra/AutoStart.h"
#include "infra/ClaudePaths.h"
#include "infra/Logger.h"
#include "infra/Settings.h"
#include "infra/Translations.h"
#include "ui/AlertLogDialog.h"
#include "ui/AppIcon.h"
#include "ui/SettingsDialog.h"
#include "ui/TrayIcon.h"
#include "ui/UsageService.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QString>
#include <QStringList>

namespace {

constexpr auto kSingleInstanceKey = "ClaudeCodeMonitor.singleInstance";

constexpr auto kShowWindowMessage = "show";
constexpr int kConnectTimeoutMs = 500;

/// 이미 떠 있는 인스턴스에 창을 띄우라고 알린다.
/// 알렸으면 true. 그 경우 이 프로세스는 조용히 끝나야 한다.
bool notifyRunningInstance()
{
    QLocalSocket socket;
    socket.connectToServer(QLatin1String(kSingleInstanceKey));
    if (!socket.waitForConnected(kConnectTimeoutMs)) {
        return false;
    }

    socket.write(QByteArrayLiteral(kShowWindowMessage));
    socket.flush();
    socket.waitForBytesWritten(kConnectTimeoutMs);
    socket.disconnectFromServer();
    return true;
}

/// 뒤따라 실행된 인스턴스의 신호를 받을 서버.
/// 앞서 죽은 인스턴스가 남긴 소켓이 있으면 치우고 다시 연다.
QLocalServer *startSingleInstanceServer(QObject *parent)
{
    auto *server = new QLocalServer(parent);
    server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!server->listen(QLatin1String(kSingleInstanceKey))) {
        QLocalServer::removeServer(QLatin1String(kSingleInstanceKey));
        if (!server->listen(QLatin1String(kSingleInstanceKey))) {
            qCWarning(ccmApp) << "단일 인스턴스 서버를 열지 못했습니다:"
                              << server->errorString();
        }
    }
    return server;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ClaudeCodeMonitor"));
    QApplication::setApplicationVersion(ccm::core::applicationVersion());
    QApplication::setQuitOnLastWindowClosed(false);
    // 창과 작업 표시줄이 쓸 아이콘. 실행 파일 리소스와 같은 그림이지만,
    // 이쪽은 화면 배율에 맞는 크기를 그때그때 그린다.
    QApplication::setWindowIcon(ccm::ui::appIcon());

    // 트레이 상주라 콘솔이 없다. 파일에 남겨야 추적이 가능하다.
    ccm::infra::Logger::install(ccm::infra::LogLevel::Debug,
                                ccm::infra::ClaudePaths::logFilePath());

    // 알림이 알림 센터에 앱 단위로 쌓이도록 신원을 밝힌다.
    // 운영체제마다 방법이 다르다. (infra/AppIdentity.h)
    ccm::infra::AppIdentity::declare();

    const bool trayRequested = QApplication::arguments().contains(QStringLiteral("--tray"));

    if (notifyRunningInstance()) {
        qCInfo(ccmApp) << "이미 실행 중인 인스턴스가 있어 종료합니다.";
        ccm::infra::Logger::shutdown();
        return 0;
    }
    QLocalServer *instanceServer = startSingleInstanceServer(&application);

    // 화면 글자의 언어를 정한다.
    //
    // 시스템 로케일에 맞는 번역 파일(:/i18n/ClaudeCodeMonitor_<로케일>.qm)을
    // 찾아 설치한다. 찾지 못하면 아무것도 설치하지 않고 소스에 적힌 글자가
    // 그대로 나온다.
    //
    // 이 프로그램의 소스 글자는 한국어다. 그래서 한국어 환경에서는 맞는 파일이
    // 없어 그냥 한국어가 나오고, 그것이 정상이다. 한국어용 번역 파일을 따로
    // 두지 않는 이유이기도 하다. (같은 글자를 같은 글자로 옮기는 파일이 된다)
    //
    // 영어 환경에서는 ClaudeCodeMonitor_en.qm 이 얹혀 영어가 나온다.
    // 상세는 infra/Translations.h 참조.
    ccm::infra::Translations::install();

    // 처음 실행이면 시작프로그램에 등록한다. 설정에서 끌 수 있다.
    // 구현이 없는 운영체제에서는 표시만 찍고 지나간다. 부를 때마다 실패 경고가
    // 쌓이는 것을 막는다.
    if (ccm::infra::Settings::isFirstRun() && ccm::infra::AutoStart::isSupported()) {
        QString error;
        if (!ccm::infra::AutoStart::enable(&error)) {
            qCWarning(ccmApp).noquote() << error;
        }
        ccm::infra::Settings::markFirstRunDone();
    }

    ccm::ui::UsageService service;
    service.start();

    const bool trayAvailable = ccm::ui::TrayIcon::isAvailable();
    if (!trayAvailable) {
        qCWarning(ccmApp) << "시스템 트레이를 쓸 수 없습니다. 창만으로 동작합니다.";
    }

    MainWindow window(&service, trayAvailable);

    ccm::ui::TrayIcon *tray = nullptr;
    if (trayAvailable) {
        tray = new ccm::ui::TrayIcon(&service, &application);
        QObject::connect(tray, &ccm::ui::TrayIcon::mainWindowRequested,
                         &window, &MainWindow::showAndRaise);
        QObject::connect(tray, &ccm::ui::TrayIcon::settingsRequested,
                         &window, &MainWindow::openSettings);
        QObject::connect(tray, &ccm::ui::TrayIcon::alertLogRequested,
                         &window, &MainWindow::openAlertLog);
        QObject::connect(tray, &ccm::ui::TrayIcon::quitRequested,
                         &application, &QApplication::quit);
        tray->show();
    }

    QObject::connect(&window, &MainWindow::quitRequested,
                     &application, &QApplication::quit);

    // 뒤따라 실행된 인스턴스가 알려 오면 창을 띄운다.
    QObject::connect(instanceServer, &QLocalServer::newConnection, &window, [&] {
        QLocalSocket *socket = instanceServer->nextPendingConnection();
        if (!socket) {
            return;
        }
        socket->deleteLater();
        qCInfo(ccmApp) << "다른 인스턴스가 창을 요청했습니다.";
        window.showAndRaise();
    });

    // 트레이가 없으면 --tray 를 무시한다. 숨을 곳이 없다.
    const bool hide = (trayRequested || ccm::infra::Settings::startMinimized())
                      && trayAvailable;
    if (!hide) {
        window.show();
    } else {
        qCInfo(ccmApp) << "트레이로만 시작합니다.";
    }

    // 첫 값은 바로 채운다. 창이든 트레이든 빈 상태로 두지 않는다.
    service.refresh();

    const int exitCode = QApplication::exec();
    ccm::infra::Logger::shutdown();
    return exitCode;
}
