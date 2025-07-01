#include "httpClient.h"

#include <QtCore/QEventLoop>


HTTPClient::HTTPClient(QObject* parent_object)
	: QObject(parent_object)
{

}

HTTPResponse HTTPClient::send(HTTPRequest& request) {
	// ��������
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

	// ��ȡ��Ӧ
	HTTPResponse response;
	if (q_reply != nullptr) {	// ������һ��nullptrֻ��Ϊ�˲���IDE������

		// ͬ���������ȴ��첽�������
		QEventLoop loop;
		QObject::connect(q_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		loop.exec();

		// ��Ӧ��Ϣ��ȡ
		response = HTTPResponse(*q_reply);
		q_reply->deleteLater();
	}

	// Http��Ӧ��Ч����״̬�벻��2xx
	if (!response.is_Status_2xx()) {

		// ��ӡ ״̬��(http) �� ��ʾ����(http)
		qDebug() << "������http��Ӧ�쳣��";
		qDebug() << "״̬�룺" << response.status_code
			<< "��ʾ���" << response.reason_phrase;
	}

	return response;
}


// HTTPRequest ���캯��
HTTPRequest::HTTPRequest(HTTPMethodType http_method, QString url, bool use_default_headers)
{
	// http�������ó�ʼ��
	this->http_method_type = http_method;
	this->url = url;

	if (use_default_headers) {
		this->headers = this->get_default_headers();
	}
	else {
		this->headers = QHash<QString, QString>();
	}

	// ���Ͳ���/���ݳ�ʼ���ÿ�
	this->url_args = QHash<QString, QString>();
	this->payload = QByteArray();
}

// HTTPResponse ���캯��
HTTPResponse::HTTPResponse()
{
	// ����ֵ��ʼ��ΪĬ��
	this->use_default();
}

HTTPResponse::HTTPResponse(QNetworkReply& reply)
{
	// ����ֵ��ʼ��ΪĬ��
	this->use_default();

	// ��ȡQt��Ĵ�����Ϣ
	this->error_code = reply.error();	// ����ӦΪ QNetworkReply::NetworkError::NoError
	this->error_string = reply.errorString();

	// ��ȡHttp��Ϣ������Httpû����ȥ����û�յ���Ҳ�ܷ����ᣬ���ᱨ���
	this->status_code = reply.attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
	this->reason_phrase = reply.attribute(QNetworkRequest::Attribute::HttpReasonPhraseAttribute).toString();
	for (QNetworkReply::RawHeaderPair header_pair : reply.rawHeaderPairs()) {
		this->headers[QString(header_pair.first)] = QString(header_pair.second);
	};
	this->payload = reply.readAll();
}

