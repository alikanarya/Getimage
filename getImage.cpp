#include "getimage.h"

static const char *user = "user";//anonymity";
static const char *password = "user";

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
    connect(&manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            SLOT(onAuthenticationRequestSlot(QNetworkReply*,QAuthenticator*)));
    connect(&manager, SIGNAL(finished(QNetworkReply*)),SLOT(downloadFinished(QNetworkReply*)));
}

void getImage::onAuthenticationRequestSlot(QNetworkReply *aReply, QAuthenticator *aAuthenticator){
    //qDebug() << Q_FUNC_INFO << aAuthenticator->realm();;
    aAuthenticator->setUser(user);
    aAuthenticator->setPassword(password);
    aAuthenticator->setOption("realm", "Digest-MD5");
}

QImage* getImage::toImage(QIODevice *data){
    QImage *temp = new QImage;
    temp->loadFromData(data->readAll());
    return temp;
}

void getImage::run(){
    time.getSystemTimeMsec();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization","Basic " + QByteArray(QString("%1:%2").arg("user").arg("user").toAscii()).toBase64());
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

    int replyDelay = time.getSystemTimeMsec() - calcTotalMsec(hour, minute, second, msec);

    if (replyDelay <= 1000){
        repliesAborted = false;

        if (reply->error()) {
            errorCount++;
/**/qDebug() << errorCount << "df" << reply->errorString();
        } else {
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

