#include "task_manager.h"

#include <QtTest/QTest>

TaskManager generateTasks(size_t amount);

class QtSerializationBenchmarks : public QObject
{
    Q_OBJECT

public:
    QtSerializationBenchmarks()
        : m_testData(generateTasks(10'000))
    {}

private slots:
    void QtCoreSerialization_data() const;
    void QtCoreSerialization();

    void QtProtobuf();

private:
    TaskManager m_testData;
};

using namespace Qt::StringLiterals;

QByteArray serializeDataStream(const TaskManager &manager);
TaskManager deserializeDataStream(const QByteArray &data);

QByteArray serializeXml(const TaskManager &contacts);
TaskManager deserializeXml(const QByteArray &data);

QByteArray serializeJson(const TaskManager &contacts);
TaskManager deserializeJson(const QByteArray &data);

QByteArray serializeCbor(const TaskManager &contacts);
TaskManager deserializeCbor(const QByteArray &data);

struct SerializationFormat
{
    QByteArray (*serialize)(const TaskManager &);
    TaskManager (*deserialize)(const QByteArray &);
};

void QtSerializationBenchmarks::QtCoreSerialization_data() const
{
    QTest::addColumn<SerializationFormat>("format");

    QTest::newRow("QDataStream") << SerializationFormat {
        serializeDataStream, deserializeDataStream };
    QTest::newRow("XML") << SerializationFormat{
        serializeXml, deserializeXml };
    QTest::newRow("JSON") << SerializationFormat{
        serializeJson, deserializeJson };
    QTest::newRow("CBOR") << SerializationFormat{
        serializeCbor, deserializeCbor };
}
void QtSerializationBenchmarks::QtCoreSerialization()
{
    QFETCH(SerializationFormat, format);

    QByteArray encodedData;
    TaskManager taskManager;

    QBENCHMARK {
        encodedData = format.serialize(m_testData);
    }
    QBENCHMARK {
        taskManager = format.deserialize(encodedData);
    }
    QTest::setBenchmarkResult(encodedData.size(), QTest::BytesAllocated);

    QCOMPARE_EQ(taskManager, m_testData);
}

#include <QtCore/QDataStream>

QDataStream &operator<<(QDataStream &stream, const TaskHeader &header) {
    return stream << header.id << header.name << header.created << header.color;
}
QDataStream &operator<<(QDataStream &stream, const Task &task) {
    return stream << task.header << task.description << qint8(task.priority) << task.completed;
}
QDataStream &operator<<(QDataStream &stream, const TaskList &list) {
    return stream << list.header << list.tasks;
}
QDataStream &operator<<(QDataStream &stream, const TaskManager &manager) {
    return stream << manager.user << manager.version << manager.lists;
}

QByteArray serializeDataStream(const TaskManager &manager)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_10);
    stream << manager;
    return data;
}

QDataStream &operator>>(QDataStream &stream, TaskHeader &header) {
    return stream >> header.id >> header.name >> header.created >> header.color;
}
QDataStream &operator>>(QDataStream &stream, Task &task) {
    qint8 priority;
    stream >> task.header >> task.description >> priority >> task.completed;
    task.priority = Task::Priority(priority);
    return stream;
}
QDataStream &operator>>(QDataStream &stream, TaskList &list) {
    return stream >> list.header >> list.tasks;
}
QDataStream &operator>>(QDataStream &stream, TaskManager &manager) {
    return stream >> manager.user >> manager.version >> manager.lists;
}

TaskManager deserializeDataStream(const QByteArray &data)
{
    TaskManager manager;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_10);
    stream >> manager;
    return manager;
}

#include <QtCore/QXmlStreamWriter>

void encodeXmlHeader(QXmlStreamWriter &writer, const TaskHeader &header) {
    writer.writeAttribute("id"_L1, header.id.toString(QUuid::WithoutBraces));
    writer.writeAttribute("name"_L1, header.name);
    writer.writeAttribute("color"_L1, header.color.name());
    writer.writeAttribute("created"_L1, header.created.toString(Qt::ISODateWithMs));
}
void encodeXmlTask(QXmlStreamWriter &writer, const Task &task) {
    writer.writeStartElement("task"_L1);
    encodeXmlHeader(writer, task.header);
    writer.writeAttribute("priority"_L1, QString::number(qToUnderlying(task.priority)));
    writer.writeAttribute("completed"_L1, task.completed ? "true"_L1 : "false"_L1);
    writer.writeCharacters(task.description);
    writer.writeEndElement();
}
void encodeXmlTaskList(QXmlStreamWriter &writer, const TaskList &list) {
    writer.writeStartElement("tasklist"_L1);
    encodeXmlHeader(writer, list.header);
    for (const auto &task : list.tasks)
        encodeXmlTask(writer, task);
    writer.writeEndElement();
}
void encodeXmlTaskManager(QXmlStreamWriter &writer, const TaskManager &manager) {
    writer.writeStartElement("taskmanager"_L1);
    writer.writeAttribute("user"_L1, manager.user);
    writer.writeAttribute("version"_L1, manager.version.toString());
    for (const auto &list : manager.lists)
        encodeXmlTaskList(writer, list);
    writer.writeEndElement();
}

QByteArray serializeXml(const TaskManager &manager)
{
    QByteArray data;
    QXmlStreamWriter writer(&data);

    writer.writeStartDocument();
    encodeXmlTaskManager(writer, manager);
    writer.writeEndDocument();

    return data;
}

#include <QtCore/QXmlStreamReader>

TaskHeader decodeXmlHeader(const QXmlStreamAttributes &attrs) {
    return TaskHeader {
        .id = QUuid(attrs.value("id"_L1).toString()),
        .name = attrs.value("name"_L1).toString(),
        .color = QColor(attrs.value("color"_L1).toString()),
        .created = QDateTime::fromString(attrs.value("created"_L1).toString(), Qt::ISODateWithMs)
    };
}
Task decodeXmlTask(QXmlStreamReader &reader) {
    const auto attrs = reader.attributes();
    return Task {
        .header = decodeXmlHeader(attrs),
        .priority = Task::Priority(attrs.value("priority"_L1).toInt()),
        .completed = attrs.value("completed"_L1) == "true"_L1,
        .description = reader.readElementText(),
    };
}
TaskList decodeXmlTaskList(QXmlStreamReader &reader) {
    const auto attrs = reader.attributes();
    return TaskList {
        .header = decodeXmlHeader(attrs),
        .tasks = [](auto &reader) {
            QList<Task> tasks;
            while (reader.readNextStartElement() && reader.name() == "task"_L1)
                tasks.append(decodeXmlTask(reader));
            return tasks;
        }(reader)
    };
}
TaskManager decodeXmlTaskManager(QXmlStreamReader &reader) {
    const auto attrs = reader.attributes();
    return TaskManager {
        .user = attrs.value("user"_L1).toString(),
        .version = QVersionNumber::fromString(attrs.value("version"_L1).toString()),
        .lists = [](auto &reader) {
            QList<TaskList> taskLists;
            while (reader.readNextStartElement() && reader.name() == "tasklist"_L1)
                taskLists.append(decodeXmlTaskList(reader));
            return taskLists;
        }(reader)
    };
}

TaskManager deserializeXml(const QByteArray &data)
{
    QXmlStreamReader reader(data);
    while (reader.readNextStartElement() && reader.name() == "taskmanager"_L1)
        return decodeXmlTaskManager(reader);
    return {};
}

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QJsonValue encodeJsonHeader(const TaskHeader &header) {
    return QJsonObject{
        { "id"_L1, header.id.toString(QUuid::WithoutBraces) },
        { "name"_L1, header.name },
        { "color"_L1, header.color.name() },
        { "created"_L1, header.created.toString(Qt::ISODateWithMs) }
    };
}
QJsonValue encodeJsonTask(const Task &task) {
    return QJsonObject{
        { "header"_L1, encodeJsonHeader(task.header) },
        { "description"_L1, task.description },
        { "priority"_L1, qToUnderlying(task.priority) },
        { "completed"_L1, task.completed }
    };
}
QJsonValue encodeJsonTaskList(const TaskList &list) {
    return QJsonObject{
        { "header"_L1, encodeJsonHeader(list.header) },
        { "tasks"_L1, [](const auto &tasks) {
            QJsonArray taskArray;
            for (const auto &t : tasks)
                taskArray.append(encodeJsonTask(t));
            return taskArray;
        }(list.tasks) }
    };
}
QJsonValue encodeJsonTaskManager(const TaskManager &manager) {
    return QJsonObject{
        { "user"_L1, manager.user },
        { "version"_L1, manager.version.toString() },
        { "lists"_L1, [](const auto &lists) {
            QJsonArray taskListArray;
            for (const auto &l : lists)
                taskListArray.append(encodeJsonTaskList(l));
            return taskListArray;
        }(manager.lists) }
    };
}

QByteArray serializeJson(const TaskManager &manager)
{
    return encodeJsonTaskManager(manager).toJson(QJsonValue::JsonFormat::Compact);
}

TaskHeader decodeJsonHeader(const QJsonObject &obj) {
    return {
        .id = QUuid(obj["id"_L1].toString()),
        .name = obj["name"_L1].toString(),
        .color = QColor(obj["color"_L1].toString()),
        .created = QDateTime::fromString(obj["created"_L1].toString(), Qt::ISODateWithMs)
    };
}
Task decodeJsonTask(const QJsonObject &obj) {
    return {
        .header = decodeJsonHeader(obj["header"_L1].toObject()),
        .priority = Task::Priority(obj["priority"_L1].toInt()),
        .completed = obj["completed"_L1].toBool(),
        .description = obj["description"_L1].toString()
    };
}
TaskList decodeJsonTaskList(const QJsonObject &obj) {
    return {
        .header = decodeJsonHeader(obj["header"_L1].toObject()),
        .tasks = [](const QJsonArray &array) {
            QList<Task> tasks; tasks.reserve(array.size());
            for (const auto &taskValue : array)
                tasks.append(decodeJsonTask(taskValue.toObject()));
            return tasks;
        }(obj["tasks"_L1].toArray())
    };
}
TaskManager decodeJsonTaskManager(const QJsonObject &obj) {
    return {
        .user = obj["user"_L1].toString(),
        .version = QVersionNumber::fromString(obj["version"_L1].toString()),
        .lists = [](const QJsonArray &array) {
            QList<TaskList> lists; lists.reserve(array.size());
            for (const auto &listValue : array)
                lists.append(decodeJsonTaskList(listValue.toObject()));
            return lists;
        }(obj["lists"_L1].toArray())
    };
}

TaskManager deserializeJson(const QByteArray &data)
{
    const auto jsonRoot = QJsonDocument::fromJson(data).object();
    return decodeJsonTaskManager(jsonRoot);
}

#include <QtCore/QCborArray>
#include <QtCore/QCborMap>
#include <QtCore/QCborValue>

QCborMap encodeCborHeader(const TaskHeader &header) {
    return {
        { "id"_L1, QCborValue(header.id) },
        { "name"_L1, header.name },
        { "color"_L1, header.color.name() },
        { "created"_L1, QCborValue(header.created) }
    };
}
QCborMap encodeCborTask(const Task &task) {
    return {
        {"header"_L1, encodeCborHeader(task.header)},
        {"description"_L1, task.description},
        {"priority"_L1, qToUnderlying(task.priority)},
        {"completed"_L1, task.completed}
    };
}
QCborMap encodeCborTaskList(const TaskList &list) {
    return {
        { "header"_L1, encodeCborHeader(list.header) },
        { "tasks"_L1, [](const auto &tasks) {
            QCborArray tasksArray;
            for (const auto &t : tasks)
                tasksArray.append(encodeCborTask(t));
            return tasksArray;
        }(list.tasks) }
    };
}
QCborMap encodeCborTaskManager(const TaskManager &manager) {
    return {
        { "user"_L1, manager.user },
        { "version"_L1, manager.version.toString() },
        { "lists"_L1, [](const auto &lists) {
            QCborArray listsArray;
            for (const auto &l : lists)
                listsArray.append(encodeCborTaskList(l));
            return listsArray;
        }(manager.lists) }
    };
}

QByteArray serializeCbor(const TaskManager &manager)
{
    return QCborValue(encodeCborTaskManager(manager)).toCbor();
}

TaskHeader decodeCborHeader(const QCborMap &map) {
    return TaskHeader{
        .id = map["id"_L1].toUuid(),
        .name = map["name"_L1].toString(),
        .color = QColor(map["color"_L1].toString()),
        .created = map["created"_L1].toDateTime(),
    };
}
Task decodeCborTask(const QCborMap &map) {
    return Task{
        .header = decodeCborHeader(map["header"_L1].toMap()),
        .priority = Task::Priority(map["priority"_L1].toInteger()),
        .completed = map["completed"_L1].toBool(),
        .description = map["description"_L1].toString()
    };
}
TaskList decodeCborTaskList(const QCborMap &map) {
    return TaskList {
        .header = decodeCborHeader(map["header"_L1].toMap()),
        .tasks = [](const QCborArray &array) {
            QList<Task> tasks; tasks.reserve(array.size());
            for (const auto &taskValue : array)
                tasks.append(decodeCborTask(taskValue.toMap()));
            return tasks;
        }(map["tasks"_L1].toArray())
    };
}
TaskManager decodeCborTaskManager(const QCborMap &map) {
    return TaskManager {
        .user = map["user"_L1].toString(),
        .version = QVersionNumber::fromString(map["version"_L1].toString()),
        .lists = [](const QCborArray &array) {
            QList<TaskList> lists; lists.reserve(array.size());
            for (const auto &listValue : array)
                lists.append(decodeCborTaskList(listValue.toMap()));
            return lists;
        }(map["lists"_L1].toArray())
    };
}

TaskManager deserializeCbor(const QByteArray &data) {
    const auto cborRoot = QCborValue::fromCbor(data).toMap();
    return decodeCborTaskManager(cborRoot);
}

#include "task_manager.qpb.h"
#include <QtProtobuf/QProtobufSerializer>
#include <QtProtobufQtCoreTypes/QtProtobufQtCoreTypes>
#include <QtProtobufQtGuiTypes/QtProtobufQtGuiTypes>

TaskManager::operator proto::TaskManager() const
{
    proto::TaskManager protoManager;
    protoManager.setUser(user);
    protoManager.setVersion(version);

    auto readHeader = [](TaskHeader header) {
        proto::TaskHeader h;
        h.setId_proto(header.id);
        h.setName(header.name);
        h.setColor(header.color);
        h.setCrated(header.created);
        return h;
    };

    QList<proto::TaskList> protoLists;
    for (const auto &list : lists) {
        proto::TaskList protoList;
        protoList.setHeader(readHeader(list.header));
        QList<proto::Task> protoTasks;
        for (const auto &task : list.tasks) {
            proto::Task t;
            t.setHeader(readHeader(task.header));
            t.setDescription(task.description);
            t.setPriority(proto::Task::Priority(task.priority));
            t.setCompleted(task.completed);
            protoTasks.append(t);
        }
        protoList.setTasks(std::move(protoTasks));
        protoLists.append(std::move(protoList));
    }
    protoManager.setLists(std::move(protoLists));
    return protoManager;
}

void QtSerializationBenchmarks::QtProtobuf()
{
    const proto::TaskManager protoTestData = m_testData;

    QtProtobuf::registerProtobufQtCoreTypes();
    QtProtobuf::registerProtobufQtGuiTypes();
    QProtobufSerializer serializer;

    QByteArray encodedData;
    proto::TaskManager protoTaskManager;

    QBENCHMARK {
        encodedData = serializer.serialize(&protoTestData);
    }
    QBENCHMARK {
        serializer.deserialize(&protoTaskManager, encodedData);
    }
    QTest::setBenchmarkResult(encodedData.size(), QTest::BytesAllocated);

    QCOMPARE_EQ(protoTaskManager, protoTestData);
}

// ## Helpers

TaskManager generateTasks(size_t count)
{
    TaskManager manager;
    manager.user = "Demo User"_L1;
    manager.version = QVersionNumber{ 1, 0, 0 };

    QStringList listNames = { "Work"_L1, "Personal"_L1, "Shopping"_L1, "Home"_L1, "Learning"_L1, "Health"_L1, "Finance"_L1, "Travel"_L1 };
    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::cyan, Qt::magenta, Qt::gray, Qt::darkGreen };
    QStringList descriptions = {
        "Finish quarterly report"_L1, "Buy groceries for the week"_L1, "Call important client"_L1,
        "Study new documentation"_L1, "Fix critical production bugs"_L1, "Schedule team meeting"_L1,
        "Review project budget"_L1, "Plan vacation itinerary"_L1, "Update portfolio"_L1,
        "Research new technologies"_L1, "Write unit tests"_L1, "Deploy to staging"_L1
    };

    int targetListCount = 10 + (count % 3); // 10-12 lists
    int baseTasksPerList = count / targetListCount;

    size_t tasksLeft = count;
    int listIndex = 0;

    while (tasksLeft > 0 && listIndex < targetListCount) {
        TaskList list;
        list.header = {
            QUuid::createUuid(),
            listNames[listIndex % listNames.size()],
            colors[listIndex % colors.size()],
            QDateTime::currentDateTimeUtc().addDays(-listIndex * 7).addSecs(listIndex * 3600),
        };

        int listSize = baseTasksPerList + ((listIndex * 17) % (baseTasksPerList / 5)) - (baseTasksPerList / 10);
        listSize = std::min(static_cast<size_t>(listSize), tasksLeft);
        listSize = std::max(1, listSize); // Ensure at least 1 task

        for (int i = 0; i < listSize; ++i) {
            Task task;
            task.header = {
                QUuid::createUuid(),
                QString("Task %1-%2"_L1).arg(listIndex).arg(i),
                colors[(listIndex + i) % colors.size()],
                QDateTime::currentDateTimeUtc().addDays(-listIndex * 7).addSecs(i * 120),
            };
            task.description = descriptions[(listIndex * 13 + i * 7) % descriptions.size()];
            task.priority = static_cast<Task::Priority>((listIndex + i) % 4);
            task.completed = (i % 7) == 0;
            list.tasks.append(task);
        }

        manager.lists.append(list);
        tasksLeft -= listSize;
        listIndex++;
    }

    return manager;
}

QTEST_MAIN(QtSerializationBenchmarks)

#include "qtserializationbenchmark.moc"
