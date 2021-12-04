/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#ifndef GALICE_EX_VIEW_HPP
#define GALICE_EX_VIEW_HPP

#include <QTreeView>
#include <QAction>
#include <QMenu>
#include "ex_model.hpp"

struct ex;
struct ex_value;

class ExView : public QTreeView
{
        Q_OBJECT

public:
        ExView(struct ex *exFile);
        ~ExView();
signals:
        void opened(const QString &name, struct ex_value *value, bool newTab);
protected:
        void contextMenuEvent(QContextMenuEvent *event) override;
private slots:
        void activate(const QModelIndex &index);
private:
        QAction *openAct;
        QAction *openNewTabAct;
        ExModel *model;
};

#endif /* GALICE_EX_VIEW_HPP */
