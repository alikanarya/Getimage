#include "getimage.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QCryptographicHash>

static const char *user = "admin";//anonymity";
static const char *password = "admin";

networkData::networkData(){
    image = new QImage;
    requestId = "0";
    shown = false;
}

networkData::~networkData(){
    delete image;
}

getImage::getImage(QString _url){
    url.setUrl(_url);
    cameraDown = true;
    requestTime = 0;
    replyTime = 0;
    connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(checkReplyFinished(QNetworkReply*)));
}

getImage::getImage(QString _url,int _fpsTarget){
    requestId = 0;
    replyId = 0;
    errorCount = 0;
    url.setUrl(_url);
    fpsTarget = _fpsTarget;
    repliesAborted = false;

    Q_ASSERT(&manager);
    //connect(&manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(onAuthenticationRequestSlot(QNetworkReply*,QAuthenticator*)));
    connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(downloadFinished(QNetworkReply*)));
}

void getImage::onAuthenticationRequestSlot(QNetworkReply *aReply, QAuthenticator *aAuthenticator){
    qDebug() << " realm: " << aAuthenticator->options().values().at(0).toString()
             << " reply: " << aReply->rawHeaderList();
    //<< Q_FUNC_INFO << aAuthenticator->realm()
    aAuthenticator->setUser(user);
    aAuthenticator->setPassword(password);
}

QImage* getImage::toImage(QIODevice *data){
    QImage *temp = new QImage;
    temp->loadFromData(data->readAll());
    return temp;
}

void getImage::run(){
    time.getSystemTimeMsec();

    QNetworkRequest request(url);
    //request.setRawHeader("Authorization","Basic " + QByteArray(QString("%1:%2").arg("admin").arg("admin").toAscii()).toBase64());
    /*
    request.setRawHeader("Authorization","Digest " +  QByteArray(QString("username=\"%1\", nonce=\"%2\", created=\"%3\"").
                                                                 arg("admin").
                                                                 arg("boAIt2qKEgFyncxdP6R8MdbkV2c=").
                                                                 arg("2017-08-10T22:02:33.000Z").toAscii()));
    //arg("2017-08-10T19:17:22.000Z").toAscii()).toBase64());
    */
    request.setRawHeader(RequestID, QString::number(requestId).toUtf8());
    request.setRawHeader(RequestHour, QString::number(time.hour).toUtf8());
    request.setRawHeader(RequestMinute, QString::number(time.minute).toUtf8());
    request.setRawHeader(RequestSecond, QString::number(time.second).toUtf8());
    request.setRawHeader(RequestMSecond, QString::number(time.msec).toUtf8());
    manager.get(request);
    requestId++;
}

void getImage::downloadFinished(QNetworkReply *reply){

    int hour = reply->request().rawHeader( RequestHour ).toInt();
    int minute = reply->request().rawHeader( RequestMinute ).toInt();
    int second = reply->request().rawHeader( RequestSecond ).toInt();
    int msec = reply->request().rawHeader( RequestMSecond ).toInt();


//qDebug()<<Q_FUNC_INFO << reply->rawHeader("CONNECTION") << ":" << reply->rawHeader("CONTENT-LENGTH");

    int replyDelay = time.getSystemTimeMsec() - calcTotalMsec(hour, minute, second, msec);

    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {

        QByteArray httpHeaders = reply->rawHeader("WWW-Authenticate");
        httpHeaders.replace(QByteArray("\""),QByteArray(""));
        httpHeaders += ",";
        qDebug() << httpHeaders;
        QRegularExpression regex("=(.*?),");
        QRegularExpressionMatchIterator i = regex.globalMatch(httpHeaders);
        QList<QString> authResponse;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) { // realm, qop, nonce, opaque
                authResponse.append(match.captured(1));
            }
        }

        //for (int j=0; j<authResponse.size(); j++) qDebug() << authResponse.at(j);
        qDebug() << "realm=" << authResponse.at(0) << " qop=" << authResponse.at(1) << " nonce=" << authResponse.at(2) << " opaque=" << authResponse.at(3);

        QByteArray dlm = QString(":").toLocal8Bit();
        QByteArray HA1inp = QString("admin").toLocal8Bit() + dlm + authResponse.at(0).toLocal8Bit() + dlm + QString("admin").toLocal8Bit();
        QByteArray HA1byte = QCryptographicHash::hash((HA1inp),QCryptographicHash::Md5).toHex();
        //QByteArray HA1byte = QCryptographicHash::hash(("admin:Login to 2C003E7PAW00006:admin"),QCryptographicHash::Md5).toHex();
        QString HA1 =  QString(HA1byte);

        QByteArray HA2inp = QString("GET").toLocal8Bit() + dlm + QString("/cgi-bin/snapshot.cgi").toLocal8Bit();
        QByteArray HA2byte = QCryptographicHash::hash((HA2inp),QCryptographicHash::Md5).toHex();
        //QByteArray HA2byte = QCryptographicHash::hash(("GET:/cgi-bin/mjpg/video.cgi?channel=1&subtype=1"),QCryptographicHash::Md5).toHex();
        QString HA2 =  QString(HA2byte);

        QByteArray inp = HA1byte + dlm +
                authResponse.at(2).toLocal8Bit() + dlm +
                QString("00000001").toLocal8Bit() + dlm +
                QString("d655ee94b416337a").toLocal8Bit() + dlm +
                authResponse.at(1).toLocal8Bit() + dlm + HA2byte;

        //QString(":425529402:00000001:d655ee94b416337a:auth:").toLocal8Bit() + HA2byte;
        //QByteArray inp = HA1byte + QString(":425529402:00000001:d655ee94b416337a:auth:").toLocal8Bit() + HA2byte;
        QString Resp =  QString(QCryptographicHash::hash((inp),QCryptographicHash::Md5).toHex());
        qDebug() << "HA1=" << HA1 << " HA2=" << HA2 << " Resp=" << Resp;

        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Digest " +
                             QByteArray(QString("username=\"%1\", realm=\"%2\", "
                                                "nonce=\"%3\", uri=\"%4\", "
                                                "response=\"%5\", opaque=\"%6\", "
                                                "qop=\"%7\", nc=\"%8\", cnonce=\"%9\"").
                                                arg("admin").arg(authResponse.at(0)).
                                                arg(authResponse.at(2)).arg("/cgi-bin/snapshot.cgi").
                                                arg(Resp).arg(authResponse.at(3)).
                                                arg(authResponse.at(1)).arg("00000001").arg("d655ee94b416337a").
                                                toAscii()));
        manager.get(request);

    } else {

        if (replyDelay <= 1000){
            repliesAborted = false;

            if (reply->error()) {
                errorCount++;
    //qDebug() << errorCount << "df" << reply->errorString();
            } else {
                networkData *_data = new networkData();
                _data->image->loadFromData(reply->readAll());
                _data->requestId = reply->request().rawHeader( RequestID );
                _data->requestHour = reply->request().rawHeader( RequestHour );
                _data->requestMinute = reply->request().rawHeader( RequestMinute );
                _data->requestSecond = reply->request().rawHeader( RequestSecond );
                _data->requestMSecond = reply->request().rawHeader( RequestMSecond );
                _data->shown = false;

    //qDebug() << "df request ID: " << _data->requestId;

                if (_data->image->format() != QImage::Format_Invalid) {
                    imageList.append(_data);

                    replyId++;
    //qDebug() << "df reply ID: " << replyId;

                    if (replyId >= fpsTarget) replyId = 0;

                    if (imageList.size() == fpsTarget){
                        delete imageList[0];
                        imageList.removeFirst();
                    }
                } else
                    delete _data;
            }

        } else {
            repliesAborted = true;

            reply->abort();
        }

    }



    reply->deleteLater();
}


void getImage::checkHost(){
    int replyDelay = replyTime - requestTime;

    if (replyDelay >=0 && replyDelay <= 1000)
        cameraDown = false;
    else
        cameraDown = true;

    requestTime = time.getSystemTimeMsec();

    QNetworkRequest request(url);
    manager.get(request);
}

void getImage::checkReplyFinished(QNetworkReply *reply){
    if (reply->error()) {
        //errorCount++;
/**/qDebug() << "rf" << reply->errorString();
    } else {
        replyTime = time.getSystemTimeMsec();
    }

    reply->deleteLater();
}


void getImage::reset(){
    requestId = 0;
    replyId = 0;
    errorCount = 0;
    imageList.clear();
}

int getImage::calcTotalMsec(int hour, int min, int second, int msec){
    return ( (hour * 3600 + min * 60 + second) * 1000 + msec );
}

getImage::~getImage(){
    imageList.clear();
}

