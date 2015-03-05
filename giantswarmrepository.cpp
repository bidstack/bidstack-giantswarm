#include "giantswarmrepository.hpp"

using namespace Bidstack::Giantswarm;

GiantswarmRepository::GiantswarmRepository(QSqlDatabase& database, QObject *parent) : QObject(parent) {
    m_database = database;
}

QSqlDatabase& GiantswarmRepository::database() {
    return m_database;
}
