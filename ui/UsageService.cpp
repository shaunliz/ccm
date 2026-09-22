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

#include "ui/UsageService.h"

#include "core/TimeFormat.h"
#include "core/UsageReportParser.h"
#include "infra/ClaudeCli.h"
#include "infra/Logger.h"
#include "infra/Settings.h"
#include "infra/StatusLineCache.h"

#include <QProcess>
#include <QTimer>

namespace ccm::ui {
namespace {

constexpr int kMinutesToMs = 60 * 1000;

} // namespace

UsageService::UsageService(QObject *parent)
    : QObject(parent)
    , m_timeoutTimer(new QTimer(this))
    , m_pollTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &UsageService::onTimeout);
    connect(m_pollTimer, &QTimer::timeout, this, [this] {
        qCDebug(ccmApp) << "자동 조회 시각입니다. 간격" << m_pollMinutes << "분";
        refresh();
    });
}

UsageService::~UsageService()
{
    teardownProcess();
}

void UsageService::start()
{
    m_history.load();
    m_alerts.load();
    m_thresholds = ccm::infra::Settings::thresholds();
    m_marks = ccm::infra::Settings::firedMarks();

    // 켤 때 무엇을 들고 시작하는지 남긴다. 나중에 "왜 안 알렸나" 를 쫓을 때
    // 첫 번째로 보는 줄이다.
    qCDebug(ccmApp) << "서비스를 시작합니다. 이력 표본" << m_history.samples().size()
                    << "개, 알림" << m_alerts.alerts().size() << "건, 표식"
                    << m_marks.size() << "개";
    qCDebug(ccmApp) << "임계치를 설정한 지표" << m_thresholds.configuredKeys().size()
                    << "개:" << m_thresholds.configuredKeys();

    setPollIntervalMinutes(ccm::infra::Settings::pollIntervalMinutes());
}

void UsageService::setPollIntervalMinutes(int minutes)
{
    m_pollMinutes = minutes;
    if (minutes <= 0) {
        m_pollTimer->stop();
        qCInfo(ccmApp) << "자동 조회를 끕니다.";
        return;
    }
    m_pollTimer->start(minutes * kMinutesToMs);
    qCInfo(ccmApp) << "자동 조회 간격:" << minutes << "분";
}

ccm::core::AlertLevel UsageService::levelFor(const QString &windowKey) const
{
    const ccm::core::UsageWindow *window = m_lastReport.find(windowKey);
    if (!window) {
        return ccm::core::AlertLevel::Normal;
    }

    // 판정은 core 에 하나뿐이다. 알림을 내는 쪽과 같은 답이 나와야 한다.
    // (core/AlertTypes.h 의 levelAt 주석 참조)
    return ccm::core::levelAt(m_thresholds.rulesFor(*window), window->usedPercent);
}

bool UsageService::refresh()
{
    if (m_busy) {
        qCDebug(ccmApp) << "이미 조회 중이라 요청을 무시합니다.";
        return false;
    }

    const QString executable = ccm::infra::ClaudeCli::findExecutable();
    if (executable.isEmpty()) {
        qCWarning(ccmApp) << "claude 를 PATH 에서 찾지 못했습니다.";
        finishWithFailure(tr("claude 실행 파일을 찾지 못했습니다. "
                             "PATH 에 claude 가 있는지 확인하십시오."));
        return false;
    }

    m_busy = true;
    m_pending.clear();
    m_skippedLines = 0;
    m_elapsed.start();
    emit started();

    m_process = new QProcess(this);
    m_process->setProgram(executable);
    m_process->setArguments(ccm::infra::ClaudeCli::sdkArguments());
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &UsageService::onReadyRead);
    connect(m_process, &QProcess::finished, this, &UsageService::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &UsageService::onProcessError);

    // 띄운 것을 확인해 두면, 실패했을 때 "실행이 안 된 것" 과 "실행은 됐는데
    // 응답이 없는 것" 을 구별할 수 있다.
    connect(m_process, &QProcess::started, this, [this] {
        qCDebug(ccmApp) << "claude 를 띄웠습니다. pid" << m_process->processId();
    });

    qCDebug(ccmApp).noquote() << QStringLiteral("조회를 시작합니다: %1").arg(executable);
    qCDebug(ccmApp).noquote()
        << QStringLiteral("  인자: %1")
               .arg(ccm::infra::ClaudeCli::sdkArguments().join(QLatin1Char(' ')));

    m_process->start();

    const QByteArray request = ccm::infra::ClaudeCli::usageRequestLine();
    const qint64 written = m_process->write(request);
    qCDebug(ccmApp) << "요청을 보냈습니다." << written << "/" << request.size() << "바이트";

    const int timeoutMs = ccm::infra::ClaudeCli::defaultTimeoutMs();
    m_timeoutTimer->start(timeoutMs);
    qCDebug(ccmApp) << "응답을 기다립니다. 제한" << timeoutMs / 1000 << "초";
    return true;
}

void UsageService::onReadyRead()
{
    if (!m_process) {
        return;
    }
    const QByteArray chunk = m_process->readAllStandardOutput();
    m_pending += chunk;
    qCDebug(ccmApp) << "표준 출력" << chunk.size() << "바이트를 받았습니다. (대기"
                    << m_pending.size() << "바이트)";

    // claude 는 control_response 앞에 system 이벤트를 여러 줄 낸다.
    const QByteArray marker = ccm::infra::ClaudeCli::responseMarker();
    for (int newline = m_pending.indexOf('\n'); newline >= 0;
         newline = m_pending.indexOf('\n')) {
        const QByteArray line = m_pending.left(newline);
        m_pending.remove(0, newline + 1);
        if (line.contains(marker)) {
            qCDebug(ccmApp) << "응답 줄을 찾았습니다. 앞서 지나간 줄"
                            << m_skippedLines << "개";
            handleResponse(line);
            return;
        }
        ++m_skippedLines;
    }
}

void UsageService::handleResponse(const QByteArray &line)
{
    m_timeoutTimer->stop();
    m_lastRaw = line;

    const qint64 elapsedMs = m_elapsed.isValid() ? m_elapsed.elapsed() : -1;
    qCDebug(ccmApp) << "응답을 받았습니다." << line.size() << "바이트, 걸린 시간"
                    << elapsedMs << "ms";

    const ccm::core::UsageReportParseResult parsed =
        ccm::core::UsageReportParser::parse(line);

    // 응답을 받았으니 claude 세션은 더 필요 없다. 표준 입력을 닫아 스스로
    // 끝나게 하고, 남은 정리는 finished 처리에서 한다.
    if (m_process) {
        m_process->closeWriteChannel();
    }

    if (!parsed.ok) {
        qCWarning(ccmApp).noquote()
            << QStringLiteral("응답을 해석하지 못했습니다: %1").arg(parsed.error);
        // 무엇을 받았는지 남겨 둔다. 프로토콜이 바뀌면 이 줄이 유일한 단서다.
        qCDebug(ccmApp).noquote()
            << QStringLiteral("받은 줄(앞 500자): %1")
                   .arg(QString::fromUtf8(line.left(500)));
        finishWithFailure(parsed.error);
        return;
    }

    m_busy = false;
    m_lastError.clear();
    m_lastReport = parsed.report;

    // 창마다 한 줄씩. 콘솔에서 값이 어떻게 움직이는지 바로 보인다.
    for (const ccm::core::UsageWindow &window : m_lastReport.windows) {
        qCDebug(ccmApp).noquote()
            << QStringLiteral("  %1 : %2%  재설정 %3")
                   .arg(window.label)
                   .arg(window.usedPercent, 0, 'f', 1)
                   .arg(window.resetsAt.isValid()
                            ? ccm::core::formatDateTime(window.resetsAt.toLocalTime())
                            : QStringLiteral("(없음)"));
    }

    // 훅이 상태줄에 쓸 수 있도록 모델별 값을 남긴다. 훅은 payload 로 이것을
    // 받지 못한다. (infra/StatusLineCache 주석 참조)
    ccm::infra::StatusLineCache().write(m_lastReport);

    QString historyError;
    if (!m_history.append(m_lastReport, &historyError)) {
        // 이력을 못 남겨도 조회 자체는 성공이다. 차트만 비게 된다.
        qCWarning(ccmApp).noquote() << historyError;
    } else {
        qCDebug(ccmApp) << "이력에 표본을 더했습니다. 보관 중인 표본"
                        << m_history.samples().size() << "개";
    }

    evaluateAlerts(m_lastReport);

    qCInfo(ccmApp) << "조회 완료. 창" << m_lastReport.windows.size() << "개, 걸린 시간"
                   << elapsedMs << "ms";
    emit succeeded(m_lastReport);
}

void UsageService::evaluateAlerts(const ccm::core::UsageReport &report)
{
    const ccm::core::EvaluationResult evaluation =
        ccm::core::AlertEvaluator::evaluate(report, m_thresholds, m_marks);

    const ccm::core::AlertLevel before = m_highestLevel;

    m_marks = evaluation.marks;
    m_highestLevel = evaluation.highestLevel;
    ccm::infra::Settings::setFiredMarks(m_marks);

    if (before != m_highestLevel) {
        qCInfo(ccmApp).noquote()
            << QStringLiteral("전체 수준이 바뀌었습니다: %1 -> %2")
                   .arg(ccm::core::alertLevelName(before),
                        ccm::core::alertLevelName(m_highestLevel));
    }

    if (evaluation.events.isEmpty()) {
        qCDebug(ccmApp) << "새로 알릴 것은 없습니다. 표식" << m_marks.size() << "개";
        return;
    }

    qCInfo(ccmApp) << "새 알림" << evaluation.events.size() << "건";

    QString storeError;
    if (!m_alerts.append(evaluation.events, &storeError)) {
        qCWarning(ccmApp).noquote() << storeError;
    }

    emit alertsRaised(evaluation.events);
    emit alertLogChanged();
}

void UsageService::reloadThresholds()
{
    m_thresholds = ccm::infra::Settings::thresholds();
    qCDebug(ccmApp) << "임계치를 다시 읽었습니다. 설정된 지표"
                    << m_thresholds.configuredKeys().size() << "개";
    if (m_lastReport.valid) {
        // 임계치를 낮추면 지금 값으로도 알림이 떠야 한다.
        evaluateAlerts(m_lastReport);
    }
}

void UsageService::acknowledgeAlert(const QString &id)
{
    QString error;
    if (!m_alerts.acknowledge(id, &error)) {
        if (!error.isEmpty()) {
            qCWarning(ccmApp).noquote() << error;
        }
        return;
    }
    emit alertLogChanged();
}

void UsageService::acknowledgeAllAlerts()
{
    QString error;
    if (!m_alerts.acknowledgeAll(&error)) {
        qCWarning(ccmApp).noquote() << error;
    }
    emit alertLogChanged();
}

void UsageService::removeAlerts(const QList<QString> &ids)
{
    QString error;
    const int removed = m_alerts.removeMany(ids, &error);
    if (!error.isEmpty()) {
        qCWarning(ccmApp).noquote() << error;
    }
    if (removed > 0) {
        qCInfo(ccmApp) << "알림" << removed << "건을 지웠습니다.";
        emit alertLogChanged();
    }
}

void UsageService::clearAlerts()
{
    QString error;
    if (!m_alerts.clear(&error)) {
        qCWarning(ccmApp).noquote() << error;
    }
    emit alertLogChanged();
}

void UsageService::onTimeout()
{
    // 어디까지 왔다가 멈췄는지 남긴다. 한 바이트도 못 받은 것과 받다 만 것은
    // 원인이 다르다.
    qCWarning(ccmApp) << "응답 대기가 끝났습니다. 받은 바이트" << m_pending.size()
                      << ", 지나간 줄" << m_skippedLines << "개, 프로세스 상태"
                      << (m_process ? int(m_process->state()) : -1);
    finishWithFailure(
        tr("%1초 안에 응답이 오지 않았습니다.")
            .arg(ccm::infra::ClaudeCli::defaultTimeoutMs() / 1000));
}

void UsageService::onProcessError()
{
    if (!m_busy || !m_process) {
        return;
    }
    qCWarning(ccmApp) << "프로세스 오류. 코드" << int(m_process->error()) << ":"
                      << m_process->errorString();
    finishWithFailure(
        tr("claude 실행에 실패했습니다: %1").arg(m_process->errorString()));
}

void UsageService::onProcessFinished()
{
    if (m_process) {
        qCDebug(ccmApp) << "claude 가 끝났습니다. 종료 코드" << m_process->exitCode()
                        << ", 상태" << int(m_process->exitStatus());
    }

    if (!m_busy) {
        // 응답을 이미 처리한 뒤의 정상 종료다.
        teardownProcess();
        return;
    }

    // 응답 없이 끝났다. stderr 가 원인을 담고 있을 수 있다.
    QString detail;
    if (m_process) {
        detail = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    }
    qCWarning(ccmApp) << "응답 없이 끝났습니다. 받은 바이트" << m_pending.size()
                      << ", 지나간 줄" << m_skippedLines << "개";
    if (!detail.isEmpty()) {
        qCWarning(ccmApp).noquote() << QStringLiteral("표준 오류: %1").arg(detail);
    }
    finishWithFailure(detail.isEmpty()
                          ? tr("claude 가 응답 없이 종료했습니다.")
                          : tr("claude 가 응답 없이 종료했습니다: %1").arg(detail));
}

void UsageService::finishWithFailure(const QString &error)
{
    m_timeoutTimer->stop();
    m_busy = false;
    m_lastError = error;
    teardownProcess();

    qCWarning(ccmApp).noquote() << error;
    emit failed(error);
}

void UsageService::teardownProcess()
{
    if (!m_process) {
        return;
    }

    // 시그널을 먼저 끊는다. 정리 도중의 finished/errorOccurred 가 다시
    // 들어오면 같은 실패를 두 번 처리하게 된다.
    m_process->disconnect(this);

    if (m_process->state() != QProcess::NotRunning) {
        m_process->closeWriteChannel();
        if (!m_process->waitForFinished(2000)) {
            // 스스로 끝나지 않았다. 흔한 일이 아니므로 남겨 둔다.
            qCWarning(ccmApp) << "claude 가 스스로 끝나지 않아 강제로 끝냅니다. pid"
                              << m_process->processId();
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
    m_process->deleteLater();
    m_process = nullptr;
}

} // namespace ccm::ui
