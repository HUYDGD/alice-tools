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

#include <QVBoxLayout>
#include "ex_view.hpp"
#include "navigator_model.hpp"
#include "navigator_view.hpp"

extern "C" {
#include "system4/ex.h"
}

ExView::ExView(struct ex *exFile, QWidget *parent)
        : QWidget(parent)
{
        NavigatorModel *model = NavigatorModel::fromExFile(exFile);
        NavigatorView *view = new NavigatorView(model);

        connect(model, &NavigatorModel::requestedOpenExValue, this, &ExView::requestedOpenExValue);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(view);
        layout->setContentsMargins(0, 0, 0, 0);
}
