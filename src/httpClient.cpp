#include "httpClient.h"

#include <QtCore/QEventLoop>


HTTPClient::HTTPClient(QObject* parent_object)
	: QObject(parent_object)
{

}

HTTPResponse HTTPClient::send(HTTPRequest& request) {
	// 发送请求
	QNetworkReply* q_reply = nullptr;
	switch (request.http_method_type)
	{
	case HTTPMethodType::HTTP_GET:
		q_reply = this->manager_.get(request.create_QNetworkRequest());
		break;
	case HTTPMethodType::HTTP_POST:
		q_reply = this->manager_.post(request.create_QNetworkRequest(), request.payload);
		break;
	case HTTPMethodType::HTTP_PUT:
		q_reply = this->manager_.put(request.create_QNetworkRequest(), request.payload);
		break;
	case HTTPMethodType::HTTP_DELETE:
		q_reply = this->manager_.deleteResource(request.create_QNetworkRequest());
		break;
	}

	// 获取响应
	HTTPResponse response;
	if (q_reply != nullptr) {	// 这里判一下nullptr只是为了不让IDE报警告

		// 同步阻塞，等待异步请求完成
		QEventLoop loop;
		QObject::connect(q_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		loop.exec();

		// 响应信息提取
		response = HTTPResponse(*q_reply);
		q_reply->deleteLater();
	}

	// Http响应有效，但状态码不是2xx
	if (!response.is_Status_2xx()) {

		// 打印 状态码(http) 和 提示短语(http)
		qDebug() << "服务器http响应异常：";
		qDebug() << "状态码：" << response.status_code
			<< "提示短语：" << response.reason_phrase;
	}

	return response;
}


// HTTPRequest 构造函数
HTTPRequest::HTTPRequest(HTTPMethodType http_method, QString url, bool use_default_headers)
{
	// http请求配置初始化
	this->http_method_type = http_method;
	this->url = url;

	if (use_default_headers) {
		this->headers = this->get_default_headers();
	}
	else {
		this->headers = QHash<QString, QString>();
	}

	// 发送参数/内容初始化置空
	this->url_args = QHash<QString, QString>();
	this->payload = QByteArray();
}

// HTTPResponse 构造函数
HTTPResponse::HTTPResponse()
{
	// 所有值初始化为默认
	this->use_default();
}

HTTPResponse::HTTPResponse(QNetworkReply& reply)
{
	// 所有值初始化为默认
	this->use_default();

	// 提取Qt层的错误信息
	this->error_code = reply.error();	// 正常应为 QNetworkReply::NetworkError::NoError
	this->error_string = reply.errorString();

	// 提取Http信息，就算Http没发出去或者没收到，也能放心提，不会报错的
	this->status_code = reply.attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
	this->reason_phrase = reply.attribute(QNetworkRequest::Attribute::HttpReasonPhraseAttribute).toString();
	for (QNetworkReply::RawHeaderPair header_pair : reply.rawHeaderPairs()) {
		this->headers[QString(header_pair.first)] = QString(header_pair.second);
	};
	this->payload = reply.readAll();
}

