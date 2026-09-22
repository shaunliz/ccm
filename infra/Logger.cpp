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

#include "infra/Logger.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include <cstdio>

Q_LOGGING_CATEGORY(ccmCore, "ccm.core")
Q_LOGGING_CATEGORY(ccmInfra, "ccm.infra")
Q_LOGGING_CATEGORY(ccmApp, "ccm.app")

namespace ccm::infra {
namespace {

/// 로그 파일이 이 크기를 넘으면 1세대 회전한다.
constexpr qint64 kMaxLogFileBytes = 2 * 1024 * 1024;

constexpr auto kRotatedSuffix = ".1";
constexpr auto kTimestampFormat = "yyyy-MM-dd HH:mm:ss.zzz";

QMutex g_mutex;
LogLevel g_minimumLevel = LogLevel::Info;
QString g_logFilePath;
QtMessageHandler g_previousHandler = nullptr;
bool g_installed = false;

LogLevel toLogLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return LogLevel::Debug;
    case QtInfoMsg:     return LogLevel::Info;
    case QtWarningMsg:  return LogLevel::Warning;
    case QtCriticalMsg: return LogLevel::Error;
    case QtFatalMsg:    return LogLevel::Error;
    }
    return LogLevel::Info;
}

QString categoryOf(const QMessageLogContext &context)
{
    return (context.category != nullptr) ? QString::fromUtf8(context.category)
                                         : QStringLiteral("default");
}

QString formatLine(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QLatin1String(kTimestampFormat));
    QString line = QStringLiteral("%1 [%2] [%3] %4")
                       .arg(timestamp,
                            Logger::levelName(toLogLevel(type)).leftJustified(5),
                            categoryOf(context),
                            message);

    // 경고 이상에서는 발생 위치를 함께 남긴다. 디버깅 비용을 크게 줄여 준다.
    if (type != QtDebugMsg && type != QtInfoMsg && context.file != nullptr) {
        const QString fileName = QFileInfo(QString::fromUtf8(context.file)).fileName();
        line += QStringLiteral(" (%1:%2)").arg(fileName).arg(context.line);
    }
    return line;
}

/// 호출자가 g_mutex 를 보유한 상태에서 호출해야 한다.
void rotateIfNeededLocked(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || info.size() < kMaxLogFileBytes) {
        return;
    }

    const QString rotatedPath = path + QLatin1String(kRotatedSuffix);
    QFile::remove(rotatedPath);
    if (!QFile::rename(path, rotatedPath)) {
        // 회전 실패는 치명적이지 않다. 계속 append 하되 사실만 남긴다.
        std::fputs("[ccm] 로그 파일 회전에 실패했습니다.\n", stderr);
    }
}

/// 호출자가 g_mutex 를 보유한 상태에서 호출해야 한다.
void writeToFileLocked(const QString &line)
{
    if (g_logFilePath.isEmpty()) {
        return;
    }

    rotateIfNeededLocked(g_logFilePath);

    QFile file(g_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    file.write(line.toUtf8());
    file.write("\n");
    file.close();
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const LogLevel level = toLogLevel(type);

    QMutexLocker locker(&g_mutex);
    if (static_cast<int>(level) < static_cast<int>(g_minimumLevel)) {
        return;
    }

    const QString line = formatLine(type, context, message);

    // stdout 은 결과 전용이므로 로그는 반드시 stderr 로 보낸다.
    // UTF-8 로 내보낸다. 진입점이 콘솔 코드페이지를 CP_UTF8 로 바꾸고 파일 싱크도
    // UTF-8 이므로, 로컬 8비트(예: CP949)로 쓰면 한글 로그가 깨진다.
    std::fputs(line.toUtf8().constData(), stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);

    writeToFileLocked(line);

    if (type == QtFatalMsg) {
        locker.unlock();
        std::fflush(stderr);
        std::abort();
    }
}

} // namespace

void Logger::install(LogLevel minimumLevel, const QString &logFilePath)
{
    QMutexLocker locker(&g_mutex);

    g_minimumLevel = minimumLevel;
    g_logFilePath = logFilePath;

    if (!g_logFilePath.isEmpty()) {
        const QFileInfo info(g_logFilePath);
        if (!QDir().mkpath(info.absolutePath())) {
            std::fputs("[ccm] 로그 디렉터리를 만들지 못해 파일 로깅을 끕니다.\n", stderr);
            g_logFilePath.clear();
        }
    }

    if (!g_installed) {
        g_previousHandler = qInstallMessageHandler(messageHandler);
        g_installed = true;
    }
}

void Logger::shutdown()
{
    QMutexLocker locker(&g_mutex);
    if (!g_installed) {
        return;
    }
    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;
    g_installed = false;
    g_logFilePath.clear();
}

void Logger::setMinimumLevel(LogLevel level)
{
    QMutexLocker locker(&g_mutex);
    g_minimumLevel = level;
}

LogLevel Logger::minimumLevel()
{
    QMutexLocker locker(&g_mutex);
    return g_minimumLevel;
}

QString Logger::logFilePath()
{
    QMutexLocker locker(&g_mutex);
    return g_logFilePath;
}

bool Logger::parseLevel(const QString &text, LogLevel *out)
{
    if (out == nullptr) {
        return false;
    }

    const QString normalized = text.trimmed().toLower();
    if (normalized == QLatin1String("debug")) {
        *out = LogLevel::Debug;
        return true;
    }
    if (normalized == QLatin1String("info")) {
        *out = LogLevel::Info;
        return true;
    }
    if (normalized == QLatin1String("warning") || normalized == QLatin1String("warn")) {
        *out = LogLevel::Warning;
        return true;
    }
    if (normalized == QLatin1String("error")) {
        *out = LogLevel::Error;
        return true;
    }
    return false;
}

QString Logger::levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:   return QStringLiteral("DEBUG");
    case LogLevel::Info:    return QStringLiteral("INFO");
    case LogLevel::Warning: return QStringLiteral("WARN");
    case LogLevel::Error:   return QStringLiteral("ERROR");
    }
    return QStringLiteral("INFO");
}

} // namespace ccm::infra
