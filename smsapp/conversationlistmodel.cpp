/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "conversationlistmodel.h"

#include <QLoggingCategory>
#include "interfaces/conversationmessage.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL, "kdeconnect.sms.conversations_list")

ConversationListModel::ConversationListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    qCCritical(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Constructing" << this;
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(AddressRole, "address");
    roles.insert(PersonUriRole, "personUri");
    roles.insert(ConversationIdRole, "conversationId");
    roles.insert(DateRole, "date");
    setItemRoleNames(roles);

    ConversationMessage::registerDbusType();
}

ConversationListModel::~ConversationListModel()
{
}

void ConversationListModel::setDeviceId(const QString& deviceId)
{
    qCCritical(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "setDeviceId" << deviceId << "of" << this;
    if (deviceId == m_deviceId)
        return;

    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, SIGNAL(conversationCreated(QString)), this, SLOT(handleCreatedConversation(QString)));
        delete m_conversationsInterface;
    }

    m_deviceId = deviceId;
    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationCreated(QString)), this, SLOT(handleCreatedConversation(QString)));
    prepareConversationsList();

    m_conversationsInterface->requestAllConversationThreads();
}

void ConversationListModel::prepareConversationsList()
{
    clear();

    QDBusPendingReply<QStringList> validThreadIDsReply = m_conversationsInterface->activeConversations();
    validThreadIDsReply.waitForFinished();

    for (const QString& conversationId : validThreadIDsReply.value())
    {
        handleCreatedConversation(conversationId);
    }

}

void ConversationListModel::handleCreatedConversation(const QString& conversationId)
{
    QVariantList args;
    args.append(conversationId);

    m_conversationsInterface->callWithCallback("getFirstFromConversation", args,
                                               this, SLOT(createRowFromMessage(ConversationMessage)), SLOT(printDBusError(QDBusError)));
}

void ConversationListModel::printDBusError(const QDBusError& error)
{
    qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << error;
}

void ConversationListModel::createRowFromMessage(const ConversationMessage& message)
{
    if (message.type() == -1)
    {
        // The Android side currently hacks in -1 if something weird comes up
        // TODO: Remove this hack when MMS support is implemented
        return;
    }

    QStandardItem* item = new QStandardItem();

    KPeople::PersonData* personData = lookupPersonByAddress(message.address());
    if (personData)
    {
        item->setText(personData->name());
        item->setIcon(QIcon(personData->photo()));
        item->setData(personData->personUri(), PersonUriRole);
    }
    else
    {
        item->setData(QString(), PersonUriRole);
        item->setText(message.address());
    }
    item->setData(message.address(), AddressRole);

    item->setData(message.threadID(), ConversationIdRole);
    item->setData(message.type() == ConversationMessage::MessageTypeSent, FromMeRole);
    item->setData(message.date(), DateRole);

    appendRow(item);
    delete personData;
}

KPeople::PersonData* ConversationListModel::lookupPersonByAddress(const QString& address)
{
    int rowIndex = 0;
    for (rowIndex = 0; rowIndex < m_people.rowCount(); rowIndex++)
    {
        const QString& uri = m_people.get(rowIndex, KPeople::PersonsModel::PersonUriRole).toString();
        KPeople::PersonData* person = new KPeople::PersonData(uri);

        const QString& email = person->email();
        const QString& phoneNumber = canonicalizePhoneNumber(person->contactCustomProperty("phoneNumber").toString());

        if (address == email || canonicalizePhoneNumber(address) == phoneNumber)
        {
            qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Matched" << address << "to" << person->name();
            return person;
        }

        delete person;
    }

    return nullptr;
}

QString ConversationListModel::canonicalizePhoneNumber(const QString& phoneNumber)
{
    QString toReturn(phoneNumber);
    toReturn = toReturn.remove(' ');
    toReturn = toReturn.remove('-');
    toReturn = toReturn.remove('(');
    toReturn = toReturn.remove(')');
    toReturn = toReturn.remove('+');
    return toReturn;
}
