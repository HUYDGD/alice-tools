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

#ifndef ALICE_EX_H
#define ALICE_EX_H

#include <stdint.h>
#include <stdio.h>

struct ex;
struct port;

struct ex *ex_parse_file(const char *path);
void ex_write(FILE *out, struct ex *ex);
uint8_t *ex_write_mem(struct ex *ex, size_t *size_out);
void ex_write_file(const char *path, struct ex *ex);
void ex_dump(struct port *port, struct ex *ex);
void ex_dump_split(FILE *out, struct ex *ex, const char *dir);

#endif /* ALICE_EX_H */
