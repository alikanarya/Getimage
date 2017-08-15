#ifndef GETIMAGE_H
#define GETIMAGE_H

#define RequestID "RequestID"
#define RequestHour "RequestHour"
#define RequestMinute "RequestMinute"
#define RequestSecond "RequestSecond"
#define RequestMSecond "RequestMSecond"

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrl>
#include <QImage>
//#include <QNetworkConfiguration>
//#include <QTcpSocket>

#include "../_Modules/Getsystemtime/getsystemtime.h"    // module global for applications
//#include "../../Getsystemtime/getsystemtime.h"          // module local for test build

// this class holds network data
class networkData {
    public:
        QImage *image;
        QString requestId;
        QString requestHour;
        QString requestMinute;
        QString requestSecond;
        QString requestMSecond;
        bool shown;

        networkData();                          // constructor
        ~networkData();                         // destructor
};

class getImage: public QObject {
    Q_OBJECT
    QNetworkAccessManager manager;

    public:
        QUrl url;

        //static const char *user = "admin";//anonymity";
        QString user = "admin";//anonymity";
        QString password = "admin";

        getSystemTime time;                     // system time class
        QList<networkData *> imageList;         // net. data buffer
        int dataBuffer;                         // size of the buffer
        int fpsRequest;
        unsigned int requestId;                 // send data id no
        int replyId;                            // receive no; to dump the buffer
        unsigned int errorCount;                // net. reply error count

        bool repliesAborted;

        QList<int> requestNo;           // not used
        int reqNo;                      // not used
        bool authenticated = false;     // not used

        int requestTime;
        int replyTime;
        bool cameraDown;
        bool _flag;

        QByteArray authorHeader = "";
        QByteArray dlm = QString(":").toLocal8Bit();

        getImage(QString _url);                 // constructor
        getImage(QString _url,int _dataBuffer); // constructor
        QImage* toImage(QIODevice *data);       // converts net. data to image
        void makeRequest(unsigned int id, bool autoId);
        void reset();                           // resets some parameters
        int calcTotalMsec(int hour, int min, int second, int msec);     // calc. total msec of time values
        void digestCalc(QNetworkReply *reply);

        ~getImage();                            // destructor
        void run();                             // sends net. request
        void checkHost();

    public slots:

        void downloadFinished(QNetworkReply *reply);    // receives data
        void checkReplyFinished(QNetworkReply *reply);
        void onAuthenticationRequestSlot(QNetworkReply *aReply, QAuthenticator *aAuthenticator);

    signals:

        void requestMade();
        void downloadCompleted();
};

#endif // GETIMAGE_H
