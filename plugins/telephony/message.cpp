/**
 * Copyright 2018 Simon Redman <simon@ergotech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore>
#include <QVariantMap>

#include "message.h"

Message::Message(const QVariantMap& args, QObject* parent)
    : QObject(parent),
      m_body(args["body"].toString()),
      m_address(args["address"].toString()),
      m_date(args["date"].toLongLong()),
      m_type(args["type"].toString()),
      m_person(args["person"].toString()),
      m_read(args["read"].toInt()),
      m_threadID(args["thread_id"].toString())
      {
}

Message::~Message() { }
