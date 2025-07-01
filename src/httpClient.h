#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


// 有脏东西定义了名为DELETE的宏，为保持格式统一才加了HTTP_前缀
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
	// 静态工具函数，参数拼接成url格式的字符串
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


// 该类的对象应该随用随创建，不用后销毁，不要长期持有
class HTTPRequest {

public:
	HTTPRequest(HTTPMethodType http_method, QString url, bool use_default_headers = true);
	~HTTPRequest() = default;

	// http响应的基本内容，如果之后需要扩展，继续添加public成员变量即可
	HTTPMethodType http_method_type;
	QString url;
	QHash<QString, QString> headers;
	QHash<QString, QString> url_args;
	QByteArray payload;

	QNetworkRequest create_QNetworkRequest() {
		QNetworkRequest request;
		// headers 设置
		for (auto it = this->headers.begin(); it != this->headers.end(); ++it) {
			request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
		}

		// 最终 url (包含url参数拼接)
		request.setUrl(this->get_final_url());

		return request;
	}

	// 默认使用的header配置
	QHash<QString, QString> get_default_headers() {
		QHash<QString, QString> default_headers = QHash<QString, QString>();
		default_headers["Content-Type"] = "application/json";
		return default_headers;
	}
	// 设置header
	void set_headers(QHash<QString, QString> new_headers) {
		this->headers = new_headers;
	}
	void clear_headers() {
		this->headers.clear();
	}
	void add_header(QString key, QString value) {
		this->headers[key] = value;
	}

	// 设置url参数
	void set_url_args(QHash<QString, QString> new_url_args) {
		this->url_args = new_url_args;
	}
	void clear_url_args() {
		this->url_args.clear();
	}
	void add_url_arg(QString key, QString value) {
		this->url_args[key] = value;
	}

	// 设置 payload
	void set_payload(QByteArray byte_array) {
		this->payload = byte_array;
	}
	void clear_payload() {
		this->payload.clear();
	}

	// 便捷函数，传对象直接设成payload
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

	// 查看url参数
	QString get_url_args_string() {
		return HTTPClient::agrs_dict_to_urlencoded_string(this->url_args);
	}

	// 一些便捷函数，供外部打印Debug
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

	// 未成功收到响应时的错误信息
	QNetworkReply::NetworkError error_code;
	QString error_string;

	// http响应的基本内容，如果之后需要扩展，继续添加public成员变量即可
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

	// 重要！
	// 读取任何 HttpResponse 的内容前，应该先进行此检查
	// 否则成员变量读到的是对象构造时的默认值，而非服务器发来的响应值
	bool is_Valid() {
		return this->status_code < 1;	// 构造时初始值是-1，请求没发出去或响应没收到是0，0是.toInt()导致的
	}

	// 便捷函数，判断响应的状态码是否为200 OK
	bool is_Status_200() {
		return this->status_code == 200;
	}

	// 便捷函数，判断响应的状态码是否为2xx系列（Restful风格的部分api可能用非200的状态码表示成功）
	bool is_Status_2xx() {
		return (this->status_code < 300 && this->status_code >= 200);
	}

	// 便捷函数，直接获取payload解析后的对象
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

	// 一些便捷函数，供外部打印Debug
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
