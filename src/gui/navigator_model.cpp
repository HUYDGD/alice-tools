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

#include <cstring>
#include "navigator_model.hpp"

extern "C" {
#include "system4/ain.h"
#include "system4/archive.h"
#include "system4/ex.h"
#include "system4/file.h"
#include "system4/flat.h"
#include "system4/string.h"
#include "alice.h"
#include "alice/ain.h"
#include "alice/port.h"
}

NavigatorModel::Node *NavigatorModel::Node::fromEx(struct ex *ex)
{
        Node *root = new Node(NavigatorNode::RootNode);
        root->appendExFileChildren(ex);
        return root;
}

NavigatorModel::Node *NavigatorModel::Node::fromExKeyValue(struct string *key, struct ex_value *value)
{
        Node *node = new Node(NavigatorNode::ExStringKeyValueNode);
        node->node.exKV.key.s = key;
        node->node.exKV.value = value;
        node->appendExValueChildren(value);
        return node;
}

NavigatorModel::Node *NavigatorModel::Node::fromExKeyValue(int key, struct ex_value *value)
{
        Node *node = new Node(NavigatorNode::ExIntKeyValueNode);
        node->node.exKV.key.i = key;
        node->node.exKV.value = value;
        node->appendExValueChildren(value);
        return node;
}

NavigatorModel::Node *NavigatorModel::Node::fromExRow(int index, struct ex_table *table, struct ex_field *fields, unsigned nFields)
{
        Node *node = new Node(NavigatorNode::ExRowNode);
        node->node.exRow.i = index;
        node->node.exRow.t = table;

        if (nFields != table->nr_fields) {
                qWarning("Field/Column count mismatch");
                if (table->nr_columns < nFields)
                        nFields = table->nr_columns;
        }

        // add columns for row
        for (unsigned col = 0; col < nFields; col++) {
                node->appendChild(Node::fromExColumn(&table->rows[index][col], &fields[col]));
        }

        return node;
}

NavigatorModel::Node *NavigatorModel::Node::fromExColumn(struct ex_value *value, struct ex_field *field)
{
        Node *node = new Node(NavigatorNode::ExStringKeyValueNode);
        node->node.exKV.key.s = field->name;
        node->node.exKV.value = value;

        if (value->type != EX_TABLE)
                return node;

        // add rows for sub-table
        for (unsigned i = 0; i < value->t->nr_rows; i++) {
                node->appendChild(Node::fromExRow(i, value->t, field->subfields, field->nr_subfields));
        }

        return node;
}

void NavigatorModel::Node::appendExValueChildren(struct ex_value *value)
{
        switch (value->type) {
        case EX_TABLE:
                for (unsigned i = 0; i < value->t->nr_rows; i++) {
                        appendChild(Node::fromExRow(i, value->t, value->t->fields, value->t->nr_fields));
                }
                break;
        case EX_LIST:
                for (unsigned i = 0; i < value->list->nr_items; i++) {
                        appendChild(Node::fromExKeyValue(i, &value->list->items[i].value));
                }
                break;
        case EX_TREE:
                if (value->tree->is_leaf) {
                        node.exKV.key.s = value->tree->leaf.name;
                        node.exKV.value = &value->tree->leaf.value;
                        appendExValueChildren(node.exKV.value);
                } else {
                        for (unsigned i = 0; i < value->tree->nr_children; i++) {
                                appendChild(Node::fromExKeyValue(value->tree->children[i].name, &value->tree->_children[i]));
                        }
                }
                break;
        default:
                break;
        }
}

void NavigatorModel::Node::appendExFileChildren(struct ex *exFile)
{
        for (unsigned i = 0; i < exFile->nr_blocks; i++) {
                appendChild(Node::fromExKeyValue(exFile->blocks[i].name, &exFile->blocks[i].val));
        }
}

NavigatorModel::Node *NavigatorModel::Node::fromAinClasses(struct ain *ain)
{
        Node *root = new Node(NavigatorNode::RootNode);

        // add classes
        for (int i = 0; i < ain->nr_structures; i++) {
                root->appendChild(Node::fromAinClass(ain, i));
        }
        // add methods
        for (int i = 0; i < ain->nr_functions; i++) {
                Node *node = root->child(ain->functions[i].struct_type);
                if (node)
                        node->appendChild(Node::fromAinFunction(ain, i));
        }

        return root;
}

NavigatorModel::Node *NavigatorModel::Node::fromAinFunctions(struct ain *ain)
{
        Node *root = new Node(NavigatorNode::RootNode);

        for (int i = 0; i < ain->nr_functions; i++) {
                root->appendChild(Node::fromAinFunction(ain, i));
        }

        return root;
}

NavigatorModel::Node *NavigatorModel::Node::fromAinClass(struct ain *ain, int i)
{
        Node *node = new Node(NavigatorNode::ClassNode);
        node->node.ainItem.ainFile = ain;
        node->node.ainItem.i = i;
        return node;
}

NavigatorModel::Node *NavigatorModel::Node::fromAinFunction(struct ain *ain, int i)
{
        Node *node = new Node(NavigatorNode::FunctionNode);
        node->node.ainItem.ainFile = ain;
        node->node.ainItem.i = i;
        return node;
}

void NavigatorModel::Node::fromArchiveIter(struct archive_data *data, void *user)
{
        Node *parent = static_cast<Node*>(user);
        Node *child = new Node(NavigatorNode::FileNode);
        child->node.ar.type = NavigatorNode::NormalFile;
        child->node.ar.file = archive_copy_descriptor(data);
        child->node.ar.data = NULL;
        parent->appendChild(child);

        const char *ext = file_extension(child->node.ar.file->name);
        if (!ext) {
                // nothing
        } else if (!strcasecmp(ext, "pactex") || !strcasecmp(ext, "ex")) {
                archive_load_file(data);
                set_input_encoding("CP932");
                set_output_encoding("UTF-8");
                struct ex *ex = ex_read_conv(data->data, data->size, string_conv_output);
                if (ex) {
                        child->appendExFileChildren(ex);
                        child->node.ar.type = NavigatorNode::ExFile;
                        child->node.ar.ex = ex;
                } else {
                        // TODO: status message?
                }
                archive_release_file(data);
        } else if (!strcasecmp(ext, "flat")) {
                // XXX: We need to keep a second descriptor with the .flat data
                //      persistently loaded.
                child->node.ar.data = archive_copy_descriptor(child->node.ar.file);
                archive_load_file(child->node.ar.data);
                int error = ARCHIVE_SUCCESS;
                child->node.ar.ar = (struct archive*)flat_open(child->node.ar.data->data,
				child->node.ar.data->size, &error);
                if (child->node.ar.ar) {
                        child->node.ar.type = NavigatorNode::ArFile;
                        child->appendArchiveChildren(child->node.ar.ar);
                } else {
                        // TODO: status message?
                }
        }
}

NavigatorModel::Node *NavigatorModel::Node::fromArchive(struct archive *ar)
{
        Node *root = new Node(NavigatorNode::RootNode);
        root->appendArchiveChildren(ar);
        return root;
}

void NavigatorModel::Node::appendArchiveChildren(struct archive *ar)
{
        archive_for_each(ar, fromArchiveIter, this);
}

NavigatorModel::Node::Node(NavigatorNode::NodeType type)
	: parentNode(nullptr)
{
	node.type = type;
}

NavigatorModel::Node::~Node()
{
        qDeleteAll(children);

        if (node.type == NavigatorNode::FileNode) {
                archive_free_data(node.ar.file);
                switch (node.ar.type) {
                case NavigatorNode::NormalFile:
                        break;
                case NavigatorNode::ExFile:
                        ex_free(node.ar.ex);
                        break;
                case NavigatorNode::ArFile:
                        archive_free(node.ar.ar);
                        if (node.ar.data)
                                archive_free_data(node.ar.data);
                        break;
                }
        }
}

NavigatorModel::Node *NavigatorModel::Node::child(int i)
{
        if (i < 0 || i >= children.size())
                return nullptr;
        return children[i];
}

NavigatorModel::Node *NavigatorModel::Node::parent()
{
        return parentNode;
}

void NavigatorModel::Node::appendChild(NavigatorModel::Node *child)
{
        children.append(child);
        child->parentNode = this;
}

int NavigatorModel::Node::row()
{
        if (parentNode)
                return parentNode->children.indexOf(const_cast<Node*>(this));
        return 0;
}

int NavigatorModel::Node::childCount()
{
        return children.size();
}

int NavigatorModel::Node::columnCount()
{
        return 3;
}

QVariant NavigatorModel::Node::data(int column) const
{
        if (column == 0) {
                switch (node.type) {
		case NavigatorNode::RootNode:
                        return "Name";
                case NavigatorNode::ClassNode:
                        return QString::fromUtf8(node.ainItem.ainFile->structures[node.ainItem.i].name);
                case NavigatorNode::FunctionNode:
                        return QString::fromUtf8(node.ainItem.ainFile->functions[node.ainItem.i].name);
                case NavigatorNode::ExStringKeyValueNode:
                        return QString::fromUtf8(node.exKV.key.s->text);
                case NavigatorNode::ExIntKeyValueNode:
                        return "[" + QString::number(node.exKV.key.i) + "]";
                case NavigatorNode::ExRowNode:
                        return "[" + QString::number(node.exRow.i) + "]";
                case NavigatorNode::FileNode:
                        return node.ar.file->name;
                }
        } else if (column == 1) {
                switch (node.type) {
                case NavigatorNode::RootNode:
                        return "Type";
                case NavigatorNode::ClassNode:
                case NavigatorNode::FunctionNode:
                case NavigatorNode::FileNode:
                        break;
                case NavigatorNode::ExStringKeyValueNode:
                case NavigatorNode::ExIntKeyValueNode:
                        return ex_strtype(node.exKV.value->type);
                case NavigatorNode::ExRowNode:
                        return "row";
                }
        } else if (column == 2) {
                switch (node.type) {
                case NavigatorNode::RootNode:
                        return "Value";
                case NavigatorNode::ClassNode:
                case NavigatorNode::FunctionNode:
                case NavigatorNode::ExRowNode:
                case NavigatorNode::FileNode:
                        break;
                case NavigatorNode::ExStringKeyValueNode:
                case NavigatorNode::ExIntKeyValueNode:
                        switch (node.exKV.value->type) {
                        case EX_INT:    return node.exKV.value->i;
                        case EX_FLOAT:  return node.exKV.value->f;
                        case EX_STRING: return QString::fromUtf8(node.exKV.value->s->text);
                        default:        break;
                        }
                        break;
                }
        }
        return QVariant();
}

NavigatorModel *NavigatorModel::fromExFile(struct ex *ex, QObject *parent)
{
        NavigatorModel *model = new NavigatorModel(parent);
        model->root = Node::fromEx(ex);
        return model;
}

NavigatorModel *NavigatorModel::fromAinClasses(struct ain *ain, QObject *parent)
{
        NavigatorModel *model = new NavigatorModel(parent);
        model->root = Node::fromAinClasses(ain);
        return model;
}

NavigatorModel *NavigatorModel::fromAinFunctions(struct ain *ain, QObject *parent)
{
        NavigatorModel *model = new NavigatorModel(parent);
        model->root = Node::fromAinFunctions(ain);
        return model;
}

NavigatorModel *NavigatorModel::fromArchive(struct archive *ar, QObject *parent)
{
        NavigatorModel *model = new NavigatorModel(parent);
        model->root = Node::fromArchive(ar);
        return model;
}

NavigatorModel::~NavigatorModel()
{
        delete root;
}

QModelIndex NavigatorModel::index(int row, int column, const QModelIndex &parent) const
{
        if (!hasIndex(row, column, parent))
                return QModelIndex();

        Node *parentNode;
        if (!parent.isValid())
                parentNode = root;
        else
                parentNode = static_cast<Node*>(parent.internalPointer());

        Node *childNode = parentNode->child(row);
        if (childNode)
                return createIndex(row, column, childNode);
        return QModelIndex();
}

QModelIndex NavigatorModel::parent(const QModelIndex &index) const
{
        if (!index.isValid())
                return QModelIndex();

        Node *childNode = static_cast<Node*>(index.internalPointer());
        Node *parentNode = childNode->parent();

        if (!parentNode || parentNode == root)
                return QModelIndex();

        return createIndex(parentNode->row(), 0, parentNode);
}

int NavigatorModel::rowCount(const QModelIndex &parent) const
{
        if (parent.column() > 0)
                return 0;

        Node *parentNode;
        if (!parent.isValid())
                parentNode = root;
        else
                parentNode = static_cast<Node*>(parent.internalPointer());

        return parentNode->childCount();
}

int NavigatorModel::columnCount(const QModelIndex &parent) const
{
        if (parent.isValid())
                return static_cast<Node*>(parent.internalPointer())->columnCount();
        return root->columnCount();
}

QVariant NavigatorModel::data(const QModelIndex &index, int role) const
{
        if (!index.isValid())
                return QVariant();

        if (role != Qt::DisplayRole)
                return QVariant();

        Node *node = static_cast<Node*>(index.internalPointer());
        return node->data(index.column());
}

Qt::ItemFlags NavigatorModel::flags(const QModelIndex &index) const
{
        if (!index.isValid())
                return Qt::NoItemFlags;
        return QAbstractItemModel::flags(index);
}

QVariant NavigatorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
                return root->data(section);
        return QVariant();
}

NavigatorNode *NavigatorModel::getNode(const QModelIndex &index) const
{
	if (!index.isValid())
		return nullptr;
	return &static_cast<Node*>(index.internalPointer())->node;
}
