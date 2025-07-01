#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


// ���ණ����������ΪDELETE�ĺ꣬Ϊ���ָ�ʽͳһ�ż���HTTP_ǰ׺
enum HTTPMethodType {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE
};
class HTTPRequest;
class HTTPResponse;

class HTTPClient : public QObject
{
	Q_OBJECT
public:
	static HTTPClient& getInstance() {
		static HTTPClient instance;
		return instance;
	}
	
	HTTPResponse send(HTTPRequest& request);

private:
	HTTPClient(QObject* parent_object = nullptr);
	~HTTPClient() = default;

public:
	// ��̬���ߺ���������ƴ�ӳ�url��ʽ���ַ���
	static QString agrs_dict_to_urlencoded_string(QHash<QString, QString> args) {
		QString args_str = "";
		for (auto it = args.begin(); it != args.end(); ++it) {
			if (args_str.isEmpty()) {
				args_str += "?";
			}
			if (args_str != "?") {
				args_str = args_str + "&" + it.key() + "=" + it.value();
			}
			else {
				args_str = it.key() + "=" + it.value();
			}
		}
		return args_str;
	};

private:
	QNetworkAccessManager manager_;
};


// ����Ķ���Ӧ�������洴�������ú����٣���Ҫ���ڳ���
class HTTPRequest {

public:
	HTTPRequest(HTTPMethodType http_method, QString url, bool use_default_headers = true);
	~HTTPRequest() = default;

	// http��Ӧ�Ļ������ݣ����֮����Ҫ��չ���������public��Ա��������
	HTTPMethodType http_method_type;
	QString url;
	QHash<QString, QString> headers;
	QHash<QString, QString> url_args;
	QByteArray payload;

	QNetworkRequest create_QNetworkRequest() {
		QNetworkRequest request;
		// headers ����
		for (auto it = this->headers.begin(); it != this->headers.end(); ++it) {
			request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
		}

		// ���� url (����url����ƴ��)
		request.setUrl(this->get_final_url());

		return request;
	}

	// Ĭ��ʹ�õ�header����
	QHash<QString, QString> get_default_headers() {
		QHash<QString, QString> default_headers = QHash<QString, QString>();
		default_headers["Content-Type"] = "application/json";
		return default_headers;
	}
	// ����header
	void set_headers(QHash<QString, QString> new_headers) {
		this->headers = new_headers;
	}
	void clear_headers() {
		this->headers.clear();
	}
	void add_header(QString key, QString value) {
		this->headers[key] = value;
	}

	// ����url����
	void set_url_args(QHash<QString, QString> new_url_args) {
		this->url_args = new_url_args;
	}
	void clear_url_args() {
		this->url_args.clear();
	}
	void add_url_arg(QString key, QString value) {
		this->url_args[key] = value;
	}

	// ���� payload
	void set_payload(QByteArray byte_array) {
		this->payload = byte_array;
	}
	void clear_payload() {
		this->payload.clear();
	}

	// ��ݺ�����������ֱ�����payload
	void set_payload(QString qstring) {
		this->payload = qstring.toUtf8();
	}
	void set_payload(QJsonDocument json_document) {
		this->payload = json_document.toJson(QJsonDocument::Compact);
	}
	void set_payload(QJsonObject json_object) {
		this->payload = QJsonDocument(json_object).toJson(QJsonDocument::Compact);
	}
	void set_payload(QJsonArray json_array) {
		this->payload = QJsonDocument(json_array).toJson(QJsonDocument::Compact);
	}

	// �鿴url����
	QString get_url_args_string() {
		return HTTPClient::agrs_dict_to_urlencoded_string(this->url_args);
	}

	// һЩ��ݺ��������ⲿ��ӡDebug
	void SimpleDebug() {
		qDebug() << "";
		qDebug() << "=====Request=====";
		switch (this->http_method_type)
		{
		case HTTPMethodType::HTTP_GET:
			qDebug() << "Http Method: GET";
			break;
		case HTTPMethodType::HTTP_POST:
			qDebug() << "Http Method: POST";
			break;
		case HTTPMethodType::HTTP_PUT:
			qDebug() << "Http Method: PUT";
			break;
		case HTTPMethodType::HTTP_DELETE:
			qDebug() << "Http Method: DELETE";
			break;
		}
		qDebug() << "url:";
		qDebug() << " " << this->get_final_url();
		qDebug() << "headers:";
		for (auto it = this->headers.begin(); it != this->headers.end(); it++) {
			qDebug() << " " << it.key() << ": " << it.value();
		}
		qDebug() << "payload:";
		qDebug() << " " << QString(this->payload);
		qDebug() << "=====Request=====";
	}
	QString get_final_url() {
		QString url_args_string = this->get_url_args_string();
		if (url_args_string.isEmpty()) {
			return this->url;
		}
		else {
			return this->url + "?" + url_args_string;
		}
	}
	QString get_headers_string_format() {
		QString str = "";
		for (auto it = this->headers.begin(); it != this->headers.end(); it++) {
			str = str + it.key() + ": " + it.value() + "\n";
		}
		return str;
	}
};


class HTTPResponse {

public:
	HTTPResponse();
	HTTPResponse(QNetworkReply& reply);
	~HTTPResponse() = default;

	// δ�ɹ��յ���Ӧʱ�Ĵ�����Ϣ
	QNetworkReply::NetworkError error_code;
	QString error_string;

	// http��Ӧ�Ļ������ݣ����֮����Ҫ��չ���������public��Ա��������
	int status_code;
	QString reason_phrase;
	QHash<QString, QString> headers;
	QByteArray payload;

public:
	void use_default() {
		this->error_code = QNetworkReply::NetworkError::UnknownNetworkError;
		this->error_string = "";
		this->status_code = -1;
		this->reason_phrase = QString();
		this->headers = QHash<QString, QString>();
		this->payload = QByteArray();
	}

	// ��Ҫ��
	// ��ȡ�κ� HttpResponse ������ǰ��Ӧ���Ƚ��д˼��
	// �����Ա�����������Ƕ�����ʱ��Ĭ��ֵ�����Ƿ�������������Ӧֵ
	bool is_Valid() {
		return this->status_code < 1;	// ����ʱ��ʼֵ��-1������û����ȥ����Ӧû�յ���0��0��.toInt()���µ�
	}

	// ��ݺ������ж���Ӧ��״̬���Ƿ�Ϊ200 OK
	bool is_Status_200() {
		return this->status_code == 200;
	}

	// ��ݺ������ж���Ӧ��״̬���Ƿ�Ϊ2xxϵ�У�Restful���Ĳ���api�����÷�200��״̬���ʾ�ɹ���
	bool is_Status_2xx() {
		return (this->status_code < 300 && this->status_code >= 200);
	}

	// ��ݺ�����ֱ�ӻ�ȡpayload������Ķ���
	QByteArray get_payload_ByteArray() {
		return this->payload;
	}
	QString get_payload_QString() {
		return QString(this->payload);
	};
	QJsonDocument get_payload_QJsonDocument() {
		return QJsonDocument::fromJson(this->payload);
	};
	QJsonObject get_payload_QJsonObject() {
		QJsonDocument json_document = QJsonDocument::fromJson(this->payload);
		QJsonObject json_object;
		if (json_document.isObject()) {
			json_object = json_document.object();
		}
		return json_object;
	};
	QJsonArray get_payload_QJsonArray() {
		QJsonDocument json_document = QJsonDocument::fromJson(this->payload);
		QJsonArray json_array;
		if (json_document.isArray()) {
			json_array = json_document.array();
		}
		return json_array;
	};

	// һЩ��ݺ��������ⲿ��ӡDebug
	void SimpleDebug() {
		qDebug() << "";
		qDebug() << "=====Response=====";
		if (this->error_code != QNetworkReply::NetworkError::NoError) {
			qDebug() << "error code: " << (int)this->error_code << "   " << "error string: " << this->error_string;
		}
		qDebug() << "status code: " << this->status_code << "   " << "reason phrase: " << this->reason_phrase;
		qDebug() << "headers:";
		for (auto it = this->headers.begin(); it != this->headers.end(); it++) {
			qDebug() << " " << it.key() << ": " << it.value();
		}
		qDebug() << "payload:";
		qDebug() << " " << this->get_payload_QString();
		qDebug() << "=====Response=====";
	}
	QString get_headers_string_format() {
		QString str = "";
		for (auto it = this->headers.begin(); it != this->headers.end(); it++) {
			str = str + it.key() + ": " + it.value() + "\n";
		}
		return str;
	}
};
