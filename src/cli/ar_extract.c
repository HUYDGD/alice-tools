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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <png.h>
#include <webp/encode.h>
#include "system4.h"
#include "system4/afa.h"
#include "system4/ald.h"
#include "system4/cg.h"
#include "system4/ex.h"
#include "system4/file.h"
#include "system4/flat.h"
#include "system4/png.h"
#include "system4/string.h"
#include "system4/utfsjis.h"
#include "system4/webp.h"
#include "alice.h"
#include "alice/ar.h"
#include "cli.h"

enum {
	LOPT_HELP = 256,
	LOPT_OUTPUT,
	LOPT_INDEX,
	LOPT_NAME,
	LOPT_FORCE,
	LOPT_IMAGE_FORMAT,
	LOPT_IMAGES_ONLY,
	LOPT_RAW,
	LOPT_TOC,
};

static char *toc_read_filename(char **data)
{
	char *p = *data;
	if (!p)
		return NULL;

	// skip initial ws
	while (*p == '\n' || *p == '\r') {
		p++;
	}

	// skip filename
	char *start = p;
	while (*p && *p != '\r' && *p != '\n') {
		p++;
	}

	// write null terminator
	if (*p == 0) {
		*data = NULL;
	} else {
		*p = 0;
		*data = p+1;
	}

	return start;
}

static char **parse_toc(const char *toc_file, size_t *_nr_files)
{
	size_t file_size;
	char *file_data = file_read(toc_file, &file_size);

	char *filename;
	char *p = file_data;
	char **files = NULL;
	size_t nr_files = 0;
	while ((filename = toc_read_filename(&p))) {
		files = xrealloc(files, (nr_files+1) * sizeof(char*));
		files[nr_files++] = strdup(filename);
	}

	free(file_data);
	*_nr_files = nr_files;
	return files;
}

int command_ar_extract(int argc, char *argv[])
{
	char *output_file = NULL;
	char *toc_file = NULL;
	char *file_name = NULL;
	int file_index = -1;

	uint32_t flags = 0;

	while (1) {
		int c = alice_getopt(argc, argv, &cmd_ar_extract);
		if (c == -1)
			break;

		switch (c) {
		case 'o':
		case LOPT_OUTPUT:
			output_file = optarg;
			break;
		case 'i':
		case LOPT_INDEX:
			// FIXME: use strtol with error checking
			file_index = atoi(optarg);
			break;
		case 'n':
		case LOPT_NAME:
			file_name = optarg;
			break;
		case 'f':
		case LOPT_FORCE:
			flags |= AR_FORCE;
			break;
		case LOPT_IMAGE_FORMAT:
			if (!strcasecmp(optarg, "png"))
				flags |= AR_IMGENC_BITS(ALCG_PNG);
			else if (!strcasecmp(optarg, "webp"))
				flags |= AR_IMGENC_BITS(ALCG_WEBP);
			else
				ERROR("Unsupported image format: \"%s\"", optarg);
			break;
		case LOPT_IMAGES_ONLY:
			flags |= AR_IMAGES_ONLY;
			break;
		case LOPT_RAW:
			flags |= AR_RAW;
			break;
		case LOPT_TOC:
			toc_file = optarg;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	// check argument count
	if (argc != 1) {
		USAGE_ERROR(&cmd_ar_extract, "Wrong number of arguments");
	}

	// open archive
	struct archive *ar;
	enum archive_type type;
	int error;
	ar = open_archive(argv[0], &type, &error);
	if (!ar) {
		ERROR("Opening archive: %s", archive_strerror(error));
	}

	size_t toc_size = 0;
	char **toc = NULL;
	if (toc_file) {
		toc = parse_toc(toc_file, &toc_size);
	}

	// run command
	if (file_index >= 0) {
		ar_extract_index(ar, file_index, output_file, flags);
	} else if (file_name) {
		ar_extract_file(ar, file_name, output_file, flags);
	} else {
		ar_extract_all(ar, output_file, flags, toc, toc_size);
	}

	archive_free(ar);
	for (size_t i = 0; i < toc_size; i++) {
		free(toc[i]);
	}
	free(toc);
	return 0;
}

struct command cmd_ar_extract = {
	.name = "extract",
	.usage = "[options...] <input-file>",
	.description = "Extract an archive file",
	.parent = &cmd_ar,
	.fun = command_ar_extract,
	.options = {
		{ "output",       'o', "Specify output file/directory",     required_argument, LOPT_OUTPUT },
		{ "index",        'i', "Specify file index",                required_argument, LOPT_INDEX },
		{ "name",         'n', "Specify file name",                 required_argument, LOPT_NAME },
		{ "force",        'f', "Allow overwriting existing files",  no_argument,       LOPT_FORCE },
		{ "image-format", 0,   "Image output format (png or webp)", required_argument, LOPT_IMAGE_FORMAT },
		{ "images-only",  0,   "Only extract images",               no_argument,       LOPT_IMAGES_ONLY },
		{ "raw",          0,   "Don't convert image files",         no_argument,       LOPT_RAW },
		{ "toc",          0,   "Specify table of contents file",    required_argument, LOPT_TOC },
		{ 0 }
	}
};
