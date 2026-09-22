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

#include "ui/AlertLogDialog.h"

#include "core/TimeFormat.h"
#include "infra/AlertStore.h"
#include "ui/AlertVisuals.h"
#include "ui/StandardButtons.h"
#include "ui/UsageService.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace ccm::ui {
namespace {

enum Column {
    ColumnState = 0,
    ColumnTime,
    ColumnLevel,
    ColumnTarget,
    ColumnMessage,
    ColumnCount
};

/// 항목의 식별자를 담아 두는 자리. 화면 줄에서 저장소 항목을 되찾는 데 쓴다.
constexpr int kIdRole = Qt::UserRole + 1;

} // namespace

AlertLogDialog::AlertLogDialog(UsageService *service, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowTitle(tr("알림 관리"));
    resize(760, 460);

    auto *layout = new QVBoxLayout(this);

    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    layout->addWidget(m_summary);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(ColumnCount);
    m_tree->setHeaderLabels({tr("확인"), tr("시각"), tr("수준"), tr("지표"), tr("내용")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setSectionResizeMode(ColumnMessage, QHeaderView::Stretch);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &AlertLogDialog::updateButtons);
    // 줄을 두 번 누르는 것이 확인의 가장 빠른 길이다.
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        if (item) {
            m_service->acknowledgeAlert(item->data(ColumnState, kIdRole).toString());
        }
    });

    auto *buttonRow = new QHBoxLayout();

    m_acknowledgeButton = new QPushButton(tr("선택 확인"), this);
    connect(m_acknowledgeButton, &QPushButton::clicked, this, [this] {
        const QList<QString> ids = selectedIds();
        for (const QString &id : ids) {
            m_service->acknowledgeAlert(id);
        }
    });
    buttonRow->addWidget(m_acknowledgeButton);

    m_acknowledgeAllButton = new QPushButton(tr("모두 확인"), this);
    connect(m_acknowledgeAllButton, &QPushButton::clicked,
            this, [this] { m_service->acknowledgeAllAlerts(); });
    buttonRow->addWidget(m_acknowledgeAllButton);

    buttonRow->addStretch();

    m_removeButton = new QPushButton(tr("선택 삭제"), this);
    connect(m_removeButton, &QPushButton::clicked, this, [this] {
        const QList<QString> ids = selectedIds();
        if (ids.isEmpty()) {
            return;
        }
        m_service->removeAlerts(ids);
    });
    buttonRow->addWidget(m_removeButton);

    m_clearButton = new QPushButton(tr("전체 삭제"), this);
    connect(m_clearButton, &QPushButton::clicked, this, [this] {
        const int total = m_service->alerts().alerts().size();
        const int unread = m_service->unacknowledgedAlertCount();

        QString question = tr("알림 %1건을 모두 지웁니다. 되돌릴 수 없습니다.").arg(total);
        if (unread > 0) {
            // 아직 보지 않은 것이 섞여 있으면 그 사실을 알린다.
            question += QChar::LineFeed;
            question += tr("그중 %1건은 아직 확인하지 않았습니다.").arg(unread);
        }
        question += QChar::LineFeed;
        question += tr("계속할까요?");

        if (askYesNo(this, tr("알림 관리"), question)) {
            m_service->clearAlerts();
        }
    });
    buttonRow->addWidget(m_clearButton);

    layout->addLayout(buttonRow);

    auto *closeBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    localizeButtons(closeBox);
    layout->addWidget(closeBox);

    connect(m_service, &UsageService::alertLogChanged, this, &AlertLogDialog::reload);

    reload();
}

QList<QString> AlertLogDialog::selectedIds() const
{
    QList<QString> ids;
    const QList<QTreeWidgetItem *> items = m_tree->selectedItems();
    for (const QTreeWidgetItem *item : items) {
        const QString id = item->data(ColumnState, kIdRole).toString();
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

void AlertLogDialog::reload()
{
    // 다시 그리면 선택이 날아간다. 같은 줄을 다시 고르도록 식별자를 기억해 둔다.
    const QList<QString> previouslySelected = selectedIds();

    m_tree->clear();

    const QList<ccm::core::AlertEvent> &alerts = m_service->alerts().alerts();
    for (const ccm::core::AlertEvent &event : alerts) {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setData(ColumnState, kIdRole, event.id);
        item->setText(ColumnState, event.acknowledged ? tr("확인") : tr("미확인"));
        item->setText(ColumnTime, ccm::core::formatDateTimeWithSeconds(event.at));
        item->setText(ColumnLevel, ccm::core::alertLevelName(event.level));
        item->setText(ColumnTarget, event.windowLabel);
        item->setText(ColumnMessage, event.message());

        if (event.level != ccm::core::AlertLevel::Normal) {
            item->setIcon(ColumnLevel, levelIcon(event.level));
            item->setForeground(ColumnLevel, levelColor(event.level));
        }

        if (!event.acknowledged) {
            // 미확인은 굵게. 색만으로는 정렬된 목록에서 눈에 띄지 않는다.
            QFont bold = item->font(ColumnState);
            bold.setBold(true);
            for (int column = 0; column < ColumnCount; ++column) {
                item->setFont(column, bold);
            }
        }

        if (previouslySelected.contains(event.id)) {
            item->setSelected(true);
        }
    }

    for (int column = 0; column < ColumnMessage; ++column) {
        m_tree->resizeColumnToContents(column);
    }

    const int unread = m_service->unacknowledgedAlertCount();
    if (alerts.isEmpty()) {
        m_summary->setText(tr("알림이 없습니다. 임계치를 넘으면 여기에 쌓입니다."));
    } else {
        m_summary->setText(tr("전체 %1건 · 미확인 %2건 (최대 %3건 보관, 확인한 것부터 버립니다)")
                               .arg(alerts.size())
                               .arg(unread)
                               .arg(ccm::infra::AlertStore::capacity()));
    }

    updateButtons();
}

void AlertLogDialog::updateButtons()
{
    const bool hasSelection = !m_tree->selectedItems().isEmpty();
    const bool hasAny = !m_service->alerts().alerts().isEmpty();

    m_acknowledgeButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);
    m_acknowledgeAllButton->setEnabled(m_service->unacknowledgedAlertCount() > 0);
    m_clearButton->setEnabled(hasAny);
}

} // namespace ccm::ui
