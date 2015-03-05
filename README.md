# bidstack-giantswarm

GiantSwarm API client written in Qt/C++

## Usage

```c++
#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>

#include "giantswarmclient.hpp"

using namespace Bidstack::Giantswarm;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", "giantswarm");
    database.setDatabaseName("giantswarm.db");

    if (!database.isValid()) {
        qWarning() << "Could not set database name probably due to invalid driver.";
    } else if (!database.open()) {
        qWarning() << "Could not open connection to database:" << database.lastError().text();
    }

    GiantswarmClient giantswarm(database);
    qDebug() << "Ping:" << (giantswarm.ping() ? "successful" : "failed");

    return 0;
}
```
