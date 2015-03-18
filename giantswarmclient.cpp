#include <QDebug>
#include <QString>

#include "giantswarmclient.hpp"

#include "deps/cache/devnullcacheadapter.hpp"

#include "deps/qjson4/QJsonDocument.h"
#include "deps/qjson4/QJsonObject.h"
#include "deps/qjson4/QJsonArray.h"
#include "deps/qjson4/QJsonParseError.h"

using namespace Bidstack::Http;
using namespace Bidstack::Cache;
using namespace Bidstack::Giantswarm;
using namespace Bidstack::Giantswarm::Repositories;

GiantswarmClient::GiantswarmClient(QSqlDatabase& database, QObject *parent) : QObject(parent) {
    m_endpoint = "https://api.giantswarm.io/v1";
    m_httpclient = new HttpClient();
    m_cache = new DevNullCacheAdapter();
    m_environments = new EnvironmentRepository(database);
    m_token = "";
}

/**
 * API
 */

void GiantswarmClient::setEndpoint(QString endpoint) {
    m_endpoint = endpoint;
}

/**
 * Authentication
 */

bool GiantswarmClient::login(QString email, QString password) {
    assertNotLoggedIn();

    QJsonObject object;
    object["password"] = QJsonValue(QString(password.toUtf8().toBase64()));

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest *request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/user/" + email + "/login");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse *response = send(request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    QJsonObject data = extractDataAsObject(response);
    m_token = data.take("Id").toString();

    if (m_token.isEmpty()) {
        qWarning() << "Could not find token in response!";
        return false;
    }

    return true;
}

bool GiantswarmClient::logout() {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/token/logout");

    try {
        HttpResponse *response = send(request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    m_token = "";
    return true;
}

bool GiantswarmClient::isLoggedIn() {
    return !m_token.isEmpty();
}

void GiantswarmClient::setToken(QString token) {
    m_token = token;
}

/**
 * Companies
 */

QVariantList GiantswarmClient::getCompanies() {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/user/me/memberships");

    QVariantList companies;

    try {
        HttpResponse *response = send("companies", request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return companies;
    }

    QJsonArray data = extractDataAsArray(response);

    for (int i = 0; i < data.size(); ++i) {
        companies.append(data.takeAt(i).toString());
    }

    return companies;
}

bool GiantswarmClient::hasCompanies() {
    return getCompanies().size() > 0;
}

bool GiantswarmClient::createCompany(QString companyName) {
    assertLoggedIn();

    QJsonObject object;
    object["company_id"] = QJsonValue(QString(companyName.toUtf8().toBase64()));

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse *response = send(request);
        assertStatusCode(response, STATUS_CODE_CREATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::deleteCompany(QString companyName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("DELETE");
    request->setUrl(m_endpoint + "/company/" + companyName);

    try {
        HttpResponse *response = send(request);
        assertStatusCode(response, STATUS_CODE_DELETED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

/**
 * Company users
 */

QVariantList GiantswarmClient::getCompanyUsers(QString companyName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/company/" + companyName);

    QVariantList users;

    try {
        HttpResponse* response = send("company_users", request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return users;
    }

    QJsonObject data = extractDataAsObject(response);
    QJsonArray members = data["members"].toArray();

    for (int i = 0; i < members.size(); ++i) {
        users.append(members.takeAt(i).toString());
    }

    return users;
}

bool GiantswarmClient::addUserToCompany(QString companyName, QString username) {
    assertLoggedIn();

    QJsonObject object;
    object["username"] = QJsonValue(QString(username.toUtf8().toBase64()));

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/members/add");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_UPDATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::removeUserFromCompany(QString companyName, QString username) {
    assertLoggedIn();

    QJsonObject object;
    object["username"] = QJsonValue(QString(username.toUtf8().toBase64()));

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/members/remove");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_UPDATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

/**
 * Environments
 */

QVariantList GiantswarmClient::getEnvironments() {
    return m_environments->all();
}

bool GiantswarmClient::hasEnvironments() {
    return getEnvironments().size() >= 1;
}

bool GiantswarmClient::hasEnvironment(QString companyName, QString environmentName) {
    return m_environments->has(companyName, environmentName);
}

bool GiantswarmClient::createEnvironment(QString companyName, QString environmentName) {
    if (!hasEnvironment(companyName, environmentName)) {
        return m_environments->add(companyName, environmentName);
    }
    return true;
}

bool GiantswarmClient::deleteEnvironment(QString companyName, QString environmentName) {
    return m_environments->remove(companyName, environmentName);
}

/**
 * Applications
 */

QVariantList GiantswarmClient::getAllApplications() {
    QVariantList applications;

    foreach (QVariant company, getCompanies()) {
        QString companyName = company.toString();

        foreach (QVariant environment, getEnvironments()) {
            QString environmentName = environment.toMap()["name"].toString();

            foreach (QVariant application, getApplications(companyName, environmentName)) {
                applications.append(application);
            }
        }
    }

    return applications;
}

QVariantList GiantswarmClient::getApplications(QString companyName, QString environmentName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/");

    QVariantList applications;

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return applications;
    }

    QJsonArray data = extractDataAsArray(response);

    for (int i = 0; i < data.size(); ++i) {
        QJsonObject item = data.takeAt(i).toObject();

        QVariantMap application;
        application["company"] = item.take("company").toString();
        application["environment"] = item.take("env").toString();
        application["application"] = item.take("app").toString();
        application["created_at"] = item.take("created").toString();

        applications.append(application);
    }

    return applications;
}

QVariantMap GiantswarmClient::getApplicationStatus(QString companyName, QString environmentName, QString applicationName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/" + applicationName + "/status");

    QVariantList services;

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return services;
    }

    QJsonObject data = extractDataAsObject(response);

    foreach (QJsonValue serviceElement, data["services"].toArray()) {
        QJsonObject serviceItem = serviceElement.toObject();

        QVariantList components;
        foreach (QJsonValue componentElement, serviceItem["components"].toArray()) {
            QJsonObject componentItem = componentElement.toObject();

            QVariantList instances;
            foreach (QJsonValue instanceElement, componentItem["instances"].toArray()) {
                QJsonObject instanceItem = instanceElement.toObject();

                QVariantMap instance;
                instance["id"] = instanceItem["id"].toString();
                instance["status"] = instanceItem["status"].toString();
                instance["image"] = instanceItem["image"].toString();
                instance["created_at"] = instanceItem["create_date"].toString();
                instances.append(instance);
            }

            QVariantMap component;
            component["name"] = componentItem["name"].toString();
            component["status"] = componentItem["status"].toString();
            component["maximum"] = componentItem.take("max").toInt();
            component["minimum"] = componentItem.take("min").toInt();
            component["instances"] = instances;
            components.append(component);
        }

        QVariantMap service;
        service["name"] = serviceItem["name"].toString();
        service["status"] = serviceItem["status"].toString();
        service["maximum"] = serviceItem.take("max").toInt();
        service["minimum"] = serviceItem.take("min").toInt();
        service["components"] = components;
        services.append(service);
    }

    QVariantMap application;
    application["name"] = data["name"].toString();
    application["status"] = data["status"].toString();
    application["services"] = services;

    return application;
}

QVariantMap GiantswarmClient::getApplicationConfiguration(QString companyName, QString environmentName, QString applicationName) {
    Q_UNUSED(companyName);
    Q_UNUSED(environmentName);
    Q_UNUSED(applicationName);
    QVariantMap configuration;
    return configuration;
}

bool GiantswarmClient::startApplication(QString companyName, QString environmentName, QString applicationName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/" + applicationName + "/start");

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_STARTED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::stopApplication(QString companyName, QString environmentName, QString applicationName) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/" + applicationName + "/stop");

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_STOPPED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::scaleApplicationUp(QString companyName, QString environmentName, QString applicationName, QString serviceName, QString componentName) {
    return scaleApplicationUp(
        companyName,
        environmentName,
        applicationName,
        serviceName,
        componentName,
        1 // scale up by 1 instance
    );
}

bool GiantswarmClient::scaleApplicationUp(QString companyName, QString environmentName, QString applicationName, QString serviceName, QString componentName, int count) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/" + applicationName + "/service/" + serviceName + "/component/" + componentName + "/scaleup/" + QString::number(count));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_UPDATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::scaleApplicationDown(QString companyName, QString environmentName, QString applicationName, QString serviceName, QString componentName) {
    return scaleApplicationUp(
        companyName,
        environmentName,
        applicationName,
        serviceName,
        componentName,
        1 // scale down by 1 instance
    );
}

bool GiantswarmClient::scaleApplicationDown(QString companyName, QString environmentName, QString applicationName, QString serviceName, QString componentName, int count) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/company/" + companyName + "/env/" + environmentName + "/app/" + applicationName + "/service/" + serviceName + "/component/" + componentName + "/scaleup/" + QString::number(count));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_DELETED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

/**
 * Instances
 */

QVariantMap GiantswarmClient::getInstanceStatistics(QString companyName, QString instanceId) {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/company/" + companyName + "/instance/" + instanceId + "/stats");

    QVariantMap statistics;

    try {
        HttpResponse* response = send("", request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return statistics;
    }

    QJsonObject data = extractDataAsObject(response);

    statistics["component"] = data["ComponentName"].toString();
    statistics["memory_usage_mb"] = data["MemoryUsageMb"].toDouble();
    statistics["memory_capacity_mb"] = data["MemoryCapacityMb"].toDouble();
    statistics["memory_usage_percent"] = data["MemoryUsagePercent"].toDouble();
    statistics["cpu_usage_percent"] = data["CpuUsagePercent"].toDouble();

    return statistics;
}

/**
 * Account
 */

QVariantMap GiantswarmClient::getUser() {
    assertLoggedIn();

    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/user/me");

    QVariantMap user;
    user["name"] = "";
    user["email"] = "";

    try {
        HttpResponse* response = send("user", request);
        assertStatusCode(response, STATUS_CODE_SUCCESS);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return user;
    }

    QJsonObject data = extractDataAsObject(response);

    user["name"] = data.take("username").toString();
    user["email"] = data.take("email").toString();

    return user;
}

bool GiantswarmClient::updateEmail(QString email) {
    assertLoggedIn();

    QVariantMap user = getUser();

    QJsonObject object;
    object["old_email"] = QJsonValue(user.take("email").toString());
    object["new_email"] = QJsonValue(email);

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/user/me/email/update");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_UPDATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

bool GiantswarmClient::updatePassword(QString old_password, QString new_password) {
    assertLoggedIn();

    QVariantMap user = getUser();

    QJsonObject object;
    object["old_password"] = QJsonValue(QString(old_password.toUtf8().toBase64()));
    object["new_password"] = QJsonValue(QString(new_password.toUtf8().toBase64()));

    QJsonDocument doc;
    doc.setObject(object);

    HttpRequest* request = new HttpRequest();
    request->setMethod("POST");
    request->setUrl(m_endpoint + "/user/me/password/update");
    request->setBody(new HttpBody(doc.toJson()));

    try {
        HttpResponse* response = send(request);
        assertStatusCode(response, STATUS_CODE_UPDATED);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return true;
}

/**
 * Cluster
 */

bool GiantswarmClient::ping() {
    HttpRequest* request = new HttpRequest();
    request->setMethod("GET");
    request->setUrl(m_endpoint + "/ping");

    try {
        HttpResponse* response = send(request);
    } catch (GiantswarmError& e) {
        qWarning() << "Error:" << e.errorString();
        return false;
    }

    return response->body()->toString() == "\"OK\"\n";
}

/**
 * Caching
 */

void GiantswarmClient::setCache(AbstractCacheAdapter *cache) {
    m_cache = cache;
}

/**
 * HTTP handling
 */

HttpResponse* GiantswarmClient::send(QString cacheKey, HttpRequest *request) {
    if (m_cache->has(cacheKey)) {
        try {
            return generateResponseFromCachableString(m_cache->fetch(cacheKey));
        } catch (GiantswarmError& e) {
            qWarning() << "Failed to generate response from cache:" << e.errorString();
        }
    }

    HttpResponse *response = send(request);
    m_cache->store(cacheKey, generateCachableStringFromResponse(response));

    return response;
}

HttpResponse* GiantswarmClient::send(HttpRequest *request) {
    QMap<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["User-Agent"] = "bb-giantswarm/0.0.1";

    if (!m_token.isEmpty()) {
        headers["Authorization"] = "giantswarm " + m_token;
    }

    if (!request->body()->isEmpty()) {
        headers["Content-Type"] = "application/json";
    }

    request->setHeaders(headers);

    HttpResponse* response = m_httpclient->send(request);

    if (response->isForbidden()) {
        throwError(GiantswarmError::NotAllowedToRequestURI);
    } else if (response->isClientError()) {
        throwError(GiantswarmError::ClientError);
    } else if (response->isServerError()) {
        throwError(GiantswarmError::ServerError);
    } else if (response->isRedirection()) {
        throwError(GiantswarmError::ResponseContainsRedirection);
    } else if (response->isNotFound()) {
        throwError(GiantswarmError::NotFound);
    } else if (!response->isSuccessful()) {
        throwError(GiantswarmError::UnexpectedResponseStatus);
    }

    return response;
}

/**
 * Example:
 *
 *   {
 *     "status": 200,
 *     "headers": [ { "name": "Content-Type", "value": "application/json" } ],
 *     "body": "{\"result\":\"success\"}"
 *   }
 *
 */
QString GiantswarmClient::generateCachableStringFromResponse(HttpResponse* response) {
    QMap<QString, QString> responseHeaders = response->headers();

    QJsonArray headers;
    foreach (QString key, responseHeaders.keys()) {
        QJsonObject header;
        header["name"] = QJsonValue(key);
        header["value"] = QJsonValue(responseHeaders[key]);
        headers.append(QJsonValue(headers));
    }

    QJsonObject object;
    object["status"] = QJsonValue(response->status());
    object["headers"] = QJsonValue(headers);
    object["body"] = QJsonValue(response->body()->toString());

    QJsonDocument doc;
    doc.setObject(object);

    return QString(doc.toJson());
}

HttpResponse* GiantswarmClient::generateResponseFromCachableString(QString string) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8(), &err);

    if (doc.isNull()) {
        throwError(GiantswarmError::InvalidJsonFromCache);
    }

    QJsonObject object = doc.object();
    QJsonArray headers = object.take("headers").toArray();

    QMap<QString, QString> responseHeaders;
    for (int i = 0; i < headers.size(); ++i) {
        QJsonObject header = headers.at(i).toObject();
        QString name = header.take("name").toString();
        QString value = header.take("value").toString();
        responseHeaders[name] = value;
    }

    HttpResponse *response = new HttpResponse(
        object.take("status").toInt(),
        responseHeaders,
        new HttpBody(object.take("body").toString())
    );

    return response;
}

/**
 * Helpers
 */

QJsonObject GiantswarmClient::extractDataAsObject(HttpResponse* response) {
    QByteArray json = response->body()->toByteArray();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);

    if (doc.isNull()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        return QJsonObject();
    }

    return doc.object()["data"].toObject();
}

QJsonArray GiantswarmClient::extractDataAsArray(HttpResponse* response) {
    QByteArray json = response->body()->toByteArray();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);

    if (doc.isNull()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        return QJsonArray();
    }

    return doc.object()["data"].toArray();
}

/**
 * Assertions
 */

void GiantswarmClient::assertLoggedIn() {
    if (!isLoggedIn()) {
        throwError(GiantswarmError::LoginRequired);
    }
}

void GiantswarmClient::assertNotLoggedIn() {
    if (isLoggedIn()) {
        throwError(GiantswarmError::LogoutRequired);
    }
}

void GiantswarmClient::assertStatusCode(HttpResponse* response, int status) {
    QByteArray data = response->body()->toByteArray();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (doc.isNull()) {
        throwError(GiantswarmError::InvalidJsonFromAPI);
    }

    if ((int)doc.object()["status_code"].toDouble() != status) {
        throwError(GiantswarmError::ResponseStatusMismatch);
    }
}

/**
 * Exceptions
 */

void GiantswarmClient::throwError(GiantswarmError::Error e) {
    GiantswarmError err;
    err.error = e;
    throw err;
}
