/*
 *  Copyright (C) 2009  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtDebug>
#include <QSqlError>

#include <QVBoxLayout>
#include <QSplitter>
#include <QSqlQuery>

#include "ComponentSearchWidget.h"

ComponentSearchWidget::ComponentSearchWidget(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	_componentsList = new QListWidget(this);
	_componentsList->setViewMode(QListView::IconMode);
	_componentsList->setSelectionMode(QAbstractItemView::MultiSelection);
	_componentsList->setMovement(QListView::Static);
	_componentsList->setResizeMode(QListView::Adjust);
	_componentsList->setSelectionRectVisible(false);
	connect(_componentsList, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));

	layout->addWidget(_componentsList);

	onSelectionChanged();
}

void ComponentSearchWidget::onSelectionChanged()
{	
	QList<QListWidgetItem *>selection = _componentsList->selectedItems();
	//if (selection == _prevSelection) return;
	_prevSelection = selection;
	QList<QChar> res;
	QStringList l;
	foreach (const QListWidgetItem *item, selection) {
		res << item->text()[0];
		l << QString::number(item->text()[0].unicode());
	}
	QSqlQuery query;
	QString queryString;
	if (!selection.isEmpty()) queryString = QString("select distinct c2.component, strokeCount from kanjidic2.components c1 join kanjidic2.components c2 on c1.kanji = c2.kanji left join kanjidic2.entries on entries.id = c2.component where c1.component in (%1) and c1.kanji in (select kanji from kanjidic2.components where component in (%1) group by kanji having count(component) >= %2) and c2.component not in (select component from components where kanji in (%1)) order by entries.frequency is null ASC, entries.frequency ASC").arg(l.join(",")).arg(selection.size());
	else queryString = "select distinct component, strokeCount from kanjidic2.components left join kanjidic2.entries on entries.id = components.component where component not in (select distinct(kanji) from kanjidic2.components) order by entries.strokeCount, entries.frequency is null ASC, entries.frequency ASC";
//	if (!query.exec(QString("select distinct component from kanjidic2.components join kanjidic2.entries on kanjidic2.components.component = kanjidic2.entries.id where kanjidic2.components.kanji in (select kanji from kanjidic2.components where component in (%1) group by kanji having count(component) >= %2)").arg(l.join(",")).arg(l.size()))) qDebug() << query.lastError();
	if (!query.exec(queryString)) qDebug() << query.lastError();
	populateList(query);
	if (!res.isEmpty()) emit componentsSelected(res);
}

void ComponentSearchWidget::populateList(QSqlQuery &query)
{
	QFont font;
	font.setPointSize(font.pointSize() + 2);
	int currentStrokeCount = 0;
	QFont nbrFont(font);
	nbrFont.setBold(true);
	QColor nbrBg(Qt::yellow);
	nbrBg = nbrBg.lighter();

	QList<QListWidgetItem *> selection = _componentsList->selectedItems();
	foreach (QListWidgetItem *item, _items) {
		if (!selection.contains(item)) {
			_items.removeOne(item);
			delete item;
		}
	}

	QList<QString> selectionText;
	foreach (QListWidgetItem *item, selection) selectionText << item->text();

	while (query.next()) {
		uint code = query.value(0).toInt();
		QString val;
		if (code < 0x10000) {
			val += QChar(code);
		} else {
			val += QChar(QChar::highSurrogate(code));
			val += QChar(QChar::lowSurrogate(code));
		}
		if (selectionText.contains(val)) continue;

		int strokeCount = query.value(1).toInt();
		if (strokeCount > currentStrokeCount) {
			currentStrokeCount = strokeCount;
			QListWidgetItem *item = new QListWidgetItem(QString::number(strokeCount), _componentsList);
			item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
			item->setBackground(nbrBg);
			item->setFont(nbrFont);
			_items << item;
		}
		QListWidgetItem *item = new QListWidgetItem(val, _componentsList);
		item->setFont(font);
		_items << item;
	}
}