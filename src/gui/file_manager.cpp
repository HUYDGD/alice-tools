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

#include <QCursor>
#include <QFileInfo>
#include <QGuiApplication>
#include "file_manager.hpp"

extern "C" {
#include "system4/ain.h"
#include "alice.h"
#include "alice/ain.h"
}

FileManager::AliceFile::AliceFile(struct ain *ain)
        : type(FILE_TYPE_AIN)
        , ain(ain)
{
}

FileManager::AliceFile::~AliceFile()
{
        switch (type) {
        case FILE_TYPE_AIN:
                ain_free(ain);
                break;
        case FILE_TYPE_EX:
                // TODO
                break;
        case FILE_TYPE_ACX:
                // TODO
                break;
        case FILE_TYPE_ARCHIVE:
                // TODO
                break;
        }
}

FileManager::FileManager()
        : QObject()
{
}

FileManager::~FileManager()
{
        qDeleteAll(files);
}

void FileManager::openFile(const QString &path)
{
        QString suffix = QFileInfo(path).suffix();

        if (!suffix.compare("ain")) {
                openAinFile(path);
        } else if (suffix.compare("ex")) {
                openExFile(path);
        } else if (suffix.compare("acx")) {
                openAcxFile(path);
        } else if (suffix.compare("afa")) {
                openAfaFile(path);
        } else if (suffix.compare("ald")) {
                openAldFile(path);
        } else if (suffix.compare("alk")) {
                openAlkFile(path);
        } else {
                emit openFileError(path, tr("Unsupported file type"));
        }
}

void FileManager::openAinFile(const QString &path)
{
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);

        int error = AIN_SUCCESS;
        struct ain *ain = ain_open(path.toUtf8(), &error);
        if (!ain) {
                QGuiApplication::restoreOverrideCursor();
                emit openFileError(path, tr("Failed to read .ain file"));
                return;
        }
        // convert all text to UTF-8
        set_input_encoding("CP932");
        set_output_encoding("UTF-8");
        ain_transcode(ain);
        // initialize method-struct mappings
        ain_init_member_functions(ain, strdup);

        files.append(new AliceFile(ain));
        emit openedAinFile(path, ain);
        QGuiApplication::restoreOverrideCursor();
}

void FileManager::openExFile(const QString &path)
{
        emit openFileError(path, tr(".ex files not yet supported"));
}

void FileManager::openAcxFile(const QString &path)
{
        emit openFileError(path, tr(".acx files not yet supported"));
}

void FileManager::openAfaFile(const QString &path)
{
        emit openFileError(path, tr(".afa files not yet supported"));
}

void FileManager::openAldFile(const QString &path)
{
        emit openFileError(path, tr(".ald files not yet supported"));
}

void FileManager::openAlkFile(const QString &path)
{
        emit openFileError(path, tr(".alk files not yet supported"));
}