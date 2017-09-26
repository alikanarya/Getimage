#include "getimage.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QCryptographicHash>
#include <QDebug>
#include <QTextCodec>

networkData::networkData(){
    image = new QImage;
    requestId = "0";
    shown = false;
}

networkData::~networkData(){
    delete image;
}

getImage::getImage(QString _url, bool _mode){

    if (_mode){             // live state checker mode
        url.setUrl(_url);
        cameraDown = true;
        requestTime = 0;
        replyTime = 0;
        connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(checkReplyFinished(QNetworkReply*)));
    } else {                // api request mode
        hostName = _url;
        Q_ASSERT(&manager);
        connect(&manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(onAuthenticationRequestSlot(QNetworkReply*,QAuthenticator*)));
        connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(replyFinished(QNetworkReply*)));
    }
}

getImage::getImage(QString _url,int _dataBuffer){

    requestId = 0;
    replyId = 0;
    errorCount = 0;
    url.setUrl(_url);
    cameraDown = true;
    dataBuffer = _dataBuffer;
    fpsRequest = 0;
    repliesAborted = false;
    _flag = true;
    //requestNo.clear();
    //reqNo = 0;

    //-manager.setCookieJar(&cookieJar);
    Q_ASSERT(&manager);
    connect(&manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(onAuthenticationRequestSlot(QNetworkReply*,QAuthenticator*)));
    connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(downloadFinished(QNetworkReply*)));

    requestTimeOut = new QTimer(this);
    connect(requestTimeOut, SIGNAL(timeout()), this, SLOT(timeOut()));

}

getImage::~getImage(){

    imageList.clear();
}

void getImage::run(){

    if (_flag) {
        makeRequest(-1, true);
        fpsRequest++;
        _flag = false;
        emit requestMade();
    }
}

void getImage::onAuthenticationRequestSlot(QNetworkReply *aReply, QAuthenticator *aAuthenticator){

    //qDebug() << " realm: " << aAuthenticator->options().values().at(0).toString();
    aAuthenticator->setUser(user);
    aAuthenticator->setPassword(password);
    aAuthenticator->setOption("realm", "Digest-MD5");
    //_flag = true;     // ENABLE MAKING REQUEST WHILE DOWNLOADING THE DATA JUST AFTER AUTHENTICATION
}


QImage* getImage::toImage(QIODevice *data){

    QImage *temp = new QImage;
    temp->loadFromData(data->readAll());
    return temp;
}

void getImage::makeRequest(unsigned int id, bool autoId){

    time.getSystemTimeMsec();
    QNetworkRequest request(url);
    if (autoId) {
        id = requestId++;
        //reqNo++; //requestNo.append(id); //qDebug() << id;
        //request.setRawHeader("Authorization","Basic " + QByteArray(QString("%1:%2").arg(user).arg(password).toAscii()));
    } else {
        request.setRawHeader("Authorization","Digest " + authorHeader);
    }

    request.setRawHeader(RequestID, QString::number(id).toUtf8());
    request.setRawHeader(RequestHour, QString::number(time.hour).toUtf8());
    request.setRawHeader(RequestMinute, QString::number(time.minute).toUtf8());
    request.setRawHeader(RequestSecond, QString::number(time.second).toUtf8());
    request.setRawHeader(RequestMSecond, QString::number(time.msec).toUtf8());
    manager.get(request);

    cameraDown = false;
    requestTimeOut->start(1000);

    //request.setRawHeader("Authorization","Digest " +  QByteArray(QString("username=\"%1\", nonce=\"%2\", created=\"%3\"").arg("admin").arg("boAIt2qKEgFyncxdP6R8MdbkV2c=").arg("2017-08-10T22:02:33.000Z").toAscii()));
}

void getImage::downloadFinished(QNetworkReply *reply){

    requestTimeOut->stop();
    cameraDown = false;
    emit cameraOnlineSignal();

    _flag = true;
    emit downloadCompleted();

    //reqNo--;
    int hour = reply->request().rawHeader( RequestHour ).toInt();
    int minute = reply->request().rawHeader( RequestMinute ).toInt();
    int second = reply->request().rawHeader( RequestSecond ).toInt();
    int msec = reply->request().rawHeader( RequestMSecond ).toInt();

    //qDebug()<<Q_FUNC_INFO << reply->rawHeader("CONNECTION") << ":" << reply->rawHeader("CONTENT-LENGTH");

    int replyDelay = time.getSystemTimeMsec() - calcTotalMsec(hour, minute, second, msec);

    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        //qDebug() << reply->errorString();
    } else if (reply->error()){
        errorCount++;
        //qDebug() << errorCount << " downloadFinished() " << reply->errorString();
    } else {

        //requestNo.removeAt( requestNo.indexOf( reply->request().rawHeader(RequestID).toUInt() ));

        if (replyDelay <= 1000){
            repliesAborted = false;

            networkData *_data = new networkData();
            _data->image->loadFromData(reply->readAll());
            _data->requestId = reply->request().rawHeader( RequestID );
            _data->requestHour = reply->request().rawHeader( RequestHour );
            _data->requestMinute = reply->request().rawHeader( RequestMinute );
            _data->requestSecond = reply->request().rawHeader( RequestSecond );
            _data->requestMSecond = reply->request().rawHeader( RequestMSecond );
            _data->shown = false;

            if (_data->image->format() != QImage::Format_Invalid) {
                imageList.append(_data);

                replyId++;

                if (replyId >= dataBuffer) replyId = 0;

                if (imageList.size() == dataBuffer){
                    delete imageList[0];
                    imageList.removeFirst();
                }

                emit lastDataTaken();
            } else
                delete _data;

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
        //qDebug() << "rf" << reply->errorString();
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
    fpsRequest = 0;
    //requestNo.clear();
    //reqNo = 0;

}

int getImage::calcTotalMsec(int hour, int min, int second, int msec){

    return ( (hour * 3600 + min * 60 + second) * 1000 + msec );
}

void getImage::digestCalc(QNetworkReply *reply){

    QByteArray httpHeaders = reply->rawHeader("WWW-Authenticate");
    httpHeaders.replace(QByteArray("\""),QByteArray(""));
    httpHeaders += ",";
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
    //qDebug() << "realm=" << authResponse.at(0) << " qop=" << authResponse.at(1) << " nonce=" << authResponse.at(2) << " opaque=" << authResponse.at(3);

    //QByteArray HA1inp = QString("admin").toLocal8Bit() + dlm + authResponse.at(0).toLocal8Bit() + dlm + QString("admin").toLocal8Bit();
    QByteArray HA1inp = user.toLocal8Bit() + dlm + authResponse.at(0).toLocal8Bit() + dlm + password.toLocal8Bit();
    QByteArray HA1byte = QCryptographicHash::hash((HA1inp),QCryptographicHash::Md5).toHex();

    QByteArray HA2inp = QString("GET").toLocal8Bit() + dlm + QString("/cgi-bin/snapshot.cgi").toLocal8Bit();
    QByteArray HA2byte = QCryptographicHash::hash((HA2inp),QCryptographicHash::Md5).toHex();

    QByteArray inp = HA1byte + dlm +
            authResponse.at(2).toLocal8Bit() + dlm +
            QString("00000001").toLocal8Bit() + dlm +
            QString("d655ee94b416337a").toLocal8Bit() + dlm +
            authResponse.at(1).toLocal8Bit() + dlm + HA2byte;

    QString Resp =  QString(QCryptographicHash::hash((inp),QCryptographicHash::Md5).toHex());

    authorHeader = QByteArray(QString("username=\"%1\", realm=\"%2\", "
                       "nonce=\"%3\", uri=\"%4\", "
                       "response=\"%5\", opaque=\"%6\", "
                       "qop=%7, nc=%8, cnonce=\"%9\"").
                       arg(user).arg(authResponse.at(0)).
                       //arg("admin").arg(authResponse.at(0)).
                       arg(authResponse.at(2)).arg("/cgi-bin/snapshot.cgi").
                       arg(Resp).arg(authResponse.at(3)).
                       arg(authResponse.at(1)).arg("00000001").arg("d655ee94b416337a").toAscii());

    //qDebug() << "downloadFinished() request ID: " << reply->request().rawHeader(RequestID) << " " << httpHeaders;
    //qDebug() << "HA1=" << QString(HA1byte) << " HA2=" << QString(HA2byte) << " Resp=" << Resp;
    //qDebug() << QString(authorHeader);
}

void getImage::timeOut(){
    cameraDown = true;
    emit cameraDownSignal();
}

void getImage::apiDahuaGetFocusState(){

    busy = true;
    QString cmd = "http://" + hostName + API_DAHUA_getFocusState;
    //qDebug() << cmd;
    url.setUrl(cmd);
    QNetworkRequest request(url);
    apiCode = APICODE_apiDahuaGetFocusState;
    manager.get(request);
    //busy = false;
}

void getImage::apiDahuaAutoFocus(){

    busy = true;
    QString cmd = "http://" + hostName + API_DAHUA_autoFocus;
    //qDebug() << cmd;
    url.setUrl(cmd);
    QNetworkRequest request(url);
    apiCode = APICODE_apiDahuaAutoFocus;
    manager.get(request);
    //busy = false;
}

void getImage::apiDahuaGetFocusStatus(){

    busy = true;
    QString cmd = "http://" + hostName + API_DAHUA_getFocusStatus;
    //qDebug() << cmd;
    url.setUrl(cmd);
    QNetworkRequest request(url);
    apiCode = APICODE_apiDahuaGetFocusStatus;
    manager.get(request);
    //busy = false;
}

void getImage::replyFinished(QNetworkReply *reply){

    busy = false;

    if ( !reply->error() && apiCode != 1) {
//        qDebug() << QTextCodec::codecForMib(1015)->toUnicode(reply->readAll());
        //qDebug() << ((QString) reply->readAll());
        QByteArray replyContent = reply->readAll();
        //qDebug() << ((QString) replyContent);

        QRegularExpression regex("=(.*?)\r\n");
        QRegularExpressionMatchIterator i = regex.globalMatch(replyContent);
        QList<QString> authResponse;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
                authResponse.append(match.captured(1));
            }
        }
        //for (int j=0; j<authResponse.size(); j++) qDebug() << authResponse.at(j);

        switch ( apiCode ) {
            case APICODE_apiDahuaGetFocusState:
                if ( authResponse.at(0) == "0")
                    emit focusState(true);      // in focus, no alarm
                else if ( authResponse.at(0) == "1")
                    emit focusState(false);     // out of focus, alarm
                else
                    emit apiError();
                break;
            case APICODE_apiDahuaGetFocusStatus:
                focusPos = authResponse.at(0).toFloat();
                focusStatus = authResponse.at(2);
                if (focusStatus == "Normal")
                    emit focusingActionState(false);
                if (focusStatus == "Autofocus")
                    emit focusingActionState(true);

                qDebug() << focusPos << "/" << authResponse.at(1) << "/" << focusStatus;
                break;
        }

    }

}
