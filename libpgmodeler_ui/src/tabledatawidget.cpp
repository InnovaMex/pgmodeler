/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2016 - Raphael Araújo e Silva <raphael@pgmodeler.com.br>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

#include "tabledatawidget.h"
#include "htmlitemdelegate.h"

const QChar TableDataWidget::UNESC_VALUE_START='{';
const QChar	TableDataWidget::UNESC_VALUE_END='}';

TableDataWidget::TableDataWidget(QWidget *parent): BaseObjectWidget(parent, BASE_OBJECT)
{
	Ui_TableDataWidget::setupUi(this);
	configureFormLayout(tabledata_grid, BASE_OBJECT);

	obj_icon_lbl->setPixmap(QPixmap(QString(":/icones/icones/") +
									BaseObject::getSchemaName(OBJ_TABLE) + QString(".png")));

	comment_lbl->setVisible(false);
	comment_edt->setVisible(false);

	QFont font=name_edt->font();
	font.setItalic(true);
	name_edt->setReadOnly(true);
	name_edt->setFont(font);

	add_row_tb->setToolTip(add_row_tb->toolTip() + QString(" (%1)").arg(add_row_tb->shortcut().toString()));
	del_rows_tb->setToolTip(del_rows_tb->toolTip() + QString(" (%1)").arg(del_rows_tb->shortcut().toString()));
	dup_rows_tb->setToolTip(dup_rows_tb->toolTip() + QString(" (%1)").arg(dup_rows_tb->shortcut().toString()));
	clear_rows_tb->setToolTip(clear_rows_tb->toolTip() + QString(" (%1)").arg(clear_rows_tb->shortcut().toString()));
	clear_cols_tb->setToolTip(clear_cols_tb->toolTip() + QString(" (%1)").arg(clear_cols_tb->shortcut().toString()));

	add_col_tb->setMenu(&col_names_menu);
	data_tbw->removeEventFilter(this);

	setMinimumSize(640, 480);

	connect(add_row_tb, SIGNAL(clicked(bool)), this, SLOT(addRow()));
	connect(dup_rows_tb, SIGNAL(clicked(bool)), this, SLOT(duplicateRows()));
	connect(del_rows_tb, SIGNAL(clicked(bool)), this, SLOT(deleteRows()));
	connect(del_cols_tb, SIGNAL(clicked(bool)), this, SLOT(deleteColumns()));
	connect(clear_rows_tb, SIGNAL(clicked(bool)), this, SLOT(clearRows()));
	connect(clear_cols_tb, SIGNAL(clicked(bool)), this, SLOT(clearColumns()));
	connect(data_tbw, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(insertRowOnTabPress(int,int,int,int)), Qt::QueuedConnection);
	connect(&col_names_menu, SIGNAL(triggered(QAction*)), this, SLOT(addColumn(QAction *)));
	connect(data_tbw, SIGNAL(itemSelectionChanged()), this, SLOT(enableButtons()));
	connect(data_tbw->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(fixInvalidColumn(int)));
}

void TableDataWidget::insertRowOnTabPress(int curr_row, int curr_col, int prev_row, int prev_col)
{
	if(qApp->mouseButtons()==Qt::NoButton &&
			curr_row==0 && curr_col==0 &&
			prev_row==data_tbw->rowCount()-1 && prev_col==data_tbw->columnCount()-1)
	{
		addRow();
	}
}

void TableDataWidget::duplicateRows(void)
{
	QList<QTableWidgetSelectionRange> sel_ranges=data_tbw->selectedRanges();

	if(!sel_ranges.isEmpty())
	{
		for(auto &sel_rng : sel_ranges)
		{
			for(int row=sel_rng.topRow(); row <= sel_rng.bottomRow(); row++)
			{
				addRow();

				for(int col=0; col < data_tbw->columnCount(); col++)
				{
					data_tbw->item(data_tbw->rowCount() - 1, col)
							->setText(data_tbw->item(row, col)->text());
				}
			}
		}

		data_tbw->clearSelection();
	}
}

void TableDataWidget::deleteRows(void)
{
	QTableWidgetSelectionRange sel_range;

	while(!data_tbw->selectedRanges().isEmpty())
	{
		sel_range=data_tbw->selectedRanges().at(0);

		for(int i = 0; i < sel_range.rowCount(); i++)
			data_tbw->removeRow(sel_range.topRow());
	}
}

void TableDataWidget::deleteColumns(void)
{
	Messagebox msg_box;

	msg_box.show(trUtf8("Delete columns is an irreversible action! Do you really want to proceed?"),
							 Messagebox::CONFIRM_ICON, Messagebox::YES_NO_BUTTONS);

	if(msg_box.result()==QDialog::Accepted)
	{
		QTableWidgetSelectionRange sel_range;

		while(!data_tbw->selectedRanges().isEmpty())
		{
			sel_range=data_tbw->selectedRanges().at(0);

			for(int i = 0; i < sel_range.columnCount(); i++)
				data_tbw->removeColumn(sel_range.leftColumn());
		}

		//Clears the entire table if no columns is left
		if(data_tbw->columnCount()==0)
		{
			clearRows(false);
			add_row_tb->setEnabled(false);
			clear_cols_tb->setEnabled(false);
		}

		del_cols_tb->setEnabled(false);

		toggleWarningFrame();
		configureColumnNamesMenu();
	}
}

void TableDataWidget::clearRows(bool confirm)
{
	Messagebox msg_box;

	if(confirm)
		msg_box.show(trUtf8("Remove all rows is an irreversible action! Do you really want to proceed?"),
								 Messagebox::CONFIRM_ICON, Messagebox::YES_NO_BUTTONS);

	if(!confirm || msg_box.result()==QDialog::Accepted)
	{
		data_tbw->clearContents();
		data_tbw->setRowCount(0);
		clear_rows_tb->setEnabled(false);
	}
}

void TableDataWidget::clearColumns(void)
{
	Messagebox msg_box;

		msg_box.show(trUtf8("Remove all columns is an irreversible action! Do you really want to proceed?"),
								 Messagebox::CONFIRM_ICON, Messagebox::YES_NO_BUTTONS);

	if(msg_box.result()==QDialog::Accepted)
	{
		clearRows(false);
		data_tbw->setColumnCount(0);
		clear_cols_tb->setEnabled(false);
		warn_frm->setVisible(false);
		add_row_tb->setEnabled(false);
		configureColumnNamesMenu();
	}
}

void TableDataWidget::fixInvalidColumn(int col_idx)
{
	QTableWidgetItem *item=data_tbw->horizontalHeaderItem(col_idx);

	if(item && item->flags()==Qt::NoItemFlags)
	{
		QAction * act=nullptr;

		col_names_menu.blockSignals(true);
		act=col_names_menu.exec(QCursor::pos());
		col_names_menu.blockSignals(false);

		if(act && act->isEnabled())
		{
			QTableWidgetItem *item=data_tbw->horizontalHeaderItem(col_idx);

			item->setText(act->text());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setForeground(data_tbw->horizontalHeader()->palette().color(QPalette::Foreground));

			for(int row = 0; row < data_tbw->rowCount(); row++)
			{
				item=data_tbw->item(row, col_idx);
				item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				item->setBackground(item->data(Qt::UserRole).value<QBrush>());
			}

			toggleWarningFrame();
			configureColumnNamesMenu();
			data_tbw->update();
		}
	}
}

void TableDataWidget::enableButtons(void)
{
	QList<QTableWidgetSelectionRange> sel_ranges=data_tbw->selectedRanges();
	bool cols_selected, rows_selected;

	cols_selected = rows_selected = !sel_ranges.isEmpty();

	for(auto &sel_rng : sel_ranges)
	{
		cols_selected &= (sel_rng.columnCount() == data_tbw->columnCount());
		rows_selected &= (sel_rng.rowCount() == data_tbw->rowCount());
	}

	del_rows_tb->setEnabled(cols_selected);
	add_row_tb->setEnabled(data_tbw->columnCount() > 0);
	del_cols_tb->setEnabled(rows_selected);
	dup_rows_tb->setEnabled(cols_selected);
}

void TableDataWidget::setAttributes(DatabaseModel *model, Table *table)
{
	BaseObjectWidget::setAttributes(model, table, nullptr);
	bool enable=(object != nullptr);

	protected_obj_frm->setVisible(false);
	obj_id_lbl->setVisible(false);
	data_tbw->setEnabled(enable);
	add_row_tb->setEnabled(enable);

	if(object)
		populateDataGrid();
}

void TableDataWidget::populateDataGrid(void)
{
	Table *table=dynamic_cast<Table *>(this->object);
	QTableWidgetItem *item=nullptr;
	QString ini_data;
	int col=0, row=0;
	QStringList columns, aux_cols, buffer, values;
	QVector<int> invalid_cols;
	Column *column=nullptr;

	clearRows(false);
	ini_data=table->getInitialData();

	/* If the initial data buffer is preset the columns
	there have priority over the current table's columns */
	if(!ini_data.isEmpty())
	{		
		buffer=ini_data.split(Table::DATA_LINE_BREAK);

		//The first line of the buffer always have the column names
		if(!buffer.isEmpty() && !buffer[0].isEmpty())
			columns.append(buffer[0].split(Table::DATA_SEPARATOR));
	}
	else
	{
		for(auto object : *table->getObjectList(OBJ_COLUMN))
			columns.push_back(object->getName());
	}

	data_tbw->setColumnCount(columns.size());

	//Creating the header of the grid
	for(QString col_name : columns)
	{
		column = table->getColumn(col_name);
		item=new QTableWidgetItem(col_name);

		/* Marking the invalid columns. The ones which aren't present in the table
		or were already created in a previous iteration */
		if(!column || aux_cols.contains(col_name))
		{
			invalid_cols.push_back(col);

			if(!column)
				item->setToolTip(trUtf8("Unknown column"));
			else
				item->setToolTip(trUtf8("Duplicated column"));
		}
		else
			item->setToolTip(QString("%1 [%2]").arg(col_name).arg(~column->getType()));

		aux_cols.append(col_name);
		data_tbw->setHorizontalHeaderItem(col++, item);
	}

	buffer.removeAt(0);
	row=0;

	//Populating the grid with the data
	for(QString buf_row : buffer)
	{
		addRow();
		values = buf_row.split(Table::DATA_SEPARATOR);
		col = 0;

		for(QString val : values)
		{
			if(col < columns.size())
				data_tbw->item(row, col++)->setText(val);
		}

		row++;
	}

	//Disabling invalid columns avoiding the user interaction
	if(!invalid_cols.isEmpty())
	{
		for(int dis_col : invalid_cols)
		{
			for(row = 0; row < data_tbw->rowCount(); row++)
				setItemInvalid(data_tbw->item(row, dis_col));

			item=data_tbw->horizontalHeaderItem(dis_col);
			item->setFlags(Qt::NoItemFlags);
			item->setForeground(QColor(Qt::red));
		}
	}

	warn_frm->setVisible(!invalid_cols.isEmpty());
	data_tbw->resizeRowsToContents();
	data_tbw->resizeColumnsToContents();

	add_row_tb->setEnabled(!columns.isEmpty());
	clear_cols_tb->setEnabled(!columns.isEmpty());
	configureColumnNamesMenu();
}

void TableDataWidget::configureColumnNamesMenu(void)
{
	Table *table=dynamic_cast<Table *>(this->object);
	QStringList col_names;

	col_names_menu.clear();

	for(auto object : *table->getObjectList(OBJ_COLUMN))
		col_names.push_back(object->getName());

	for(int col = 0; col < data_tbw->columnCount(); col++)
		col_names.removeOne(data_tbw->horizontalHeaderItem(col)->text());

	if(col_names.isEmpty())
		col_names_menu.addAction(trUtf8("(no columns)"))->setEnabled(false);
	else
	{
		col_names.sort();

		for(QString col_name : col_names)
			col_names_menu.addAction(col_name);
	}
}

void TableDataWidget::toggleWarningFrame(void)
{
	bool has_inv_cols=false;

	for(int col = 0; col < data_tbw->columnCount() && !has_inv_cols; col++)
		has_inv_cols = data_tbw->horizontalHeaderItem(col)->flags()==Qt::NoItemFlags;

	warn_frm->setVisible(has_inv_cols);
}

void TableDataWidget::setItemInvalid(QTableWidgetItem *item)
{
	if(item)
	{
		item->setData(Qt::UserRole, item->background());
		item->setBackgroundColor(QColor(QString("#FFC0C0")));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	}
}

QString TableDataWidget::generateDataBuffer(void)
{
	QStringList val_list, col_names, buffer;
	QString value;
	int col = 0, col_count = data_tbw->horizontalHeader()->count();

	for(int col=0; col < col_count; col++)
		col_names.push_back(data_tbw->horizontalHeaderItem(col)->text());

	//The first line of the buffer consists in the column names
	buffer.push_back(col_names.join(Table::DATA_SEPARATOR));

	for(int row = 0; row < data_tbw->rowCount(); row++)
	{
		for(col = 0; col < col_count; col++)
		{
			value = data_tbw->item(row, col)->text();

			//Checking if the value is a malformed unescaped value, e.g., {value, value}, {value\}
			if((value.startsWith(Table::UNESC_VALUE_START) && value.endsWith(QString("\\") + Table::UNESC_VALUE_END)) ||
					(value.startsWith(Table::UNESC_VALUE_START) && !value.endsWith(Table::UNESC_VALUE_END)) ||
					(!value.startsWith(Table::UNESC_VALUE_START) && !value.endsWith(QString("\\") + Table::UNESC_VALUE_END) && value.endsWith(Table::UNESC_VALUE_END)))
				throw Exception(Exception::getErrorMessage(ERR_MALFORMED_UNESCAPED_VALUE)
								.arg(row + 1).arg(col_names[col]),
								ERR_MALFORMED_UNESCAPED_VALUE,__PRETTY_FUNCTION__,__FILE__,__LINE__);

			val_list.push_back(value);
		}

		buffer.push_back(val_list.join(Table::DATA_SEPARATOR));
		val_list.clear();
	}

	if(buffer.size() <= 1)
		return(QString());

	return(buffer.join(Table::DATA_LINE_BREAK));
}

void TableDataWidget::addRow(void)
{
	int row=data_tbw->rowCount();
	QTableWidgetItem *item = nullptr;

	data_tbw->blockSignals(true);
	data_tbw->insertRow(row);

	for(int col=0; col < data_tbw->columnCount(); col++)
	{
		item=new QTableWidgetItem;

		if(data_tbw->horizontalHeaderItem(col)->flags()==Qt::NoItemFlags)
			setItemInvalid(item);
		else
			item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		data_tbw->setItem(row, col, item);
	}

	data_tbw->clearSelection();
	data_tbw->setCurrentCell(row, 0, QItemSelectionModel::ClearAndSelect);

	if(item->flags()!=Qt::NoItemFlags)
		data_tbw->editItem(data_tbw->item(row, 0));

	data_tbw->blockSignals(false);

	clear_rows_tb->setEnabled(true);
}

void TableDataWidget::addColumn(QAction *action)
{
	if(action)
	{
		QTableWidgetItem *item=nullptr;
		int col = data_tbw->columnCount();

		data_tbw->insertColumn(col);

		item=new QTableWidgetItem;
		item->setText(action->text());
		data_tbw->setHorizontalHeaderItem(col, item);

		for(int row=0; row < data_tbw->rowCount(); row++)
		{
			item=new QTableWidgetItem;
			item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			data_tbw->setItem(row, col, item);
		}

		add_row_tb->setEnabled(true);
		clear_cols_tb->setEnabled(true);
		data_tbw->resizeColumnsToContents();
		configureColumnNamesMenu();
	}
}

void TableDataWidget::applyConfiguration(void)
{
	try
	{
		Table *table = dynamic_cast<Table *>(this->object);
		table->setInitialData(generateDataBuffer());
		emit s_closeRequested();
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

