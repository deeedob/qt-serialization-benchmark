#pragma once

#include <QtGui/QColor>

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QVersionNumber>

struct TaskHeader
{
    QUuid id;
    QString name;
    QColor color;
    QDateTime created;

    bool operator==(const TaskHeader &other) const = default;
};

struct Task
{
    enum class Priority {
        Low = 0,
        Medium = 1,
        High = 2,
        Critical = 3,
    };

    TaskHeader header;
    Priority priority;
    bool completed;
    QString description;

    bool operator==(const Task &other) const = default;
};

struct TaskList
{
    TaskHeader header;
    QList<Task> tasks;

    bool operator==(const TaskList &other) const = default;
};

namespace proto { class TaskManager; }

struct TaskManager
{
    QString user;
    QVersionNumber version;
    QList<TaskList> lists;

    operator proto::TaskManager() const;
    bool operator==(const TaskManager &other) const = default;
};

