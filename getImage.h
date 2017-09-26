#ifndef GETIMAGE_H
#define GETIMAGE_H

#define RequestID "RequestID"
#define RequestHour "RequestHour"
#define RequestMinute "RequestMinute"
#define RequestSecond "RequestSecond"
#define RequestMSecond "RequestMSecond"

#define API_DAHUA_getFocusState     "/cgi-bin/alarm.cgi?action=getOutState"
#define API_DAHUA_autoFocus         "/cgi-bin/devVideoInput.cgi?action=autoFocus"
#define API_DAHUA_getFocusStatus    "/cgi-bin/devVideoInput.cgi?action=getFocusStatus"
#define API_DAHUA_setFocusPosition1 "/cgi-bin/devVideoInput.cgi?action=adjustFocus&focus="
#define API_DAHUA_setFocusPosition2 "&zoom=0"

#define APICODE_apiDahuaGetFocusState   0
#define APICODE_apiDahuaAutoFocus       1
#define APICODE_apiDahuaGetFocusStatus  2
#define APICODE_apiDahuaSetFocusPos     3

#include <QList>
#include <QNetworkAccessManager>
//-#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrl>
#include <QImage>
#include <QTimer>
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
    //-QNetworkCookieJar cookieJar;

    public:
        QUrl url;
        QString hostName = "";

        //static const char *user = "admin";//anonymity";
        QString user = "admin";//"anonymity";
        QString password = "admin";

        getSystemTime time;                     // system time class
        QList<networkData *> imageList;         // net. data buffer
        int dataBuffer;                         // size of the buffer
        int fpsRequest;
        unsigned int requestId;                 // send data id no
        int replyId;                            // receive no; to dump the buffer
        unsigned int errorCount;                // net. reply error count

        QTimer *requestTimeOut;
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

        int apiCode = 0;
        bool busy = false;
        float focusPos = 0;
        QString focusMotorSteps = "";
        QString focusStatus = "";

        getImage(QString _url, bool _mode);     // constructor
        getImage(QString _url,int _dataBuffer); // constructor
        QImage* toImage(QIODevice *data);       // converts net. data to image
        void makeRequest(unsigned int id, bool autoId);
        void reset();                           // resets some parameters
        int calcTotalMsec(int hour, int min, int second, int msec);     // calc. total msec of time values
        void digestCalc(QNetworkReply *reply);

        void apiDahuaGetFocusState();
        void apiDahuaAutoFocus();
        void apiDahuaGetFocusStatus();
        void apiDahuaSetFocusPos(float value);

        ~getImage();                            // destructor
        void run();                             // sends net. request
        void checkHost();

    public slots:

        void downloadFinished(QNetworkReply *reply);    // receives data
        void checkReplyFinished(QNetworkReply *reply);
        void onAuthenticationRequestSlot(QNetworkReply *aReply, QAuthenticator *aAuthenticator);
        void timeOut();
        void replyFinished(QNetworkReply *reply);

    signals:

        void requestMade();
        void downloadCompleted();
        void lastDataTaken();
        void cameraDownSignal();
        void cameraOnlineSignal();
        void apiError();
        void focusState(bool);              // true; in focus, false; out of focus
        void focusingActionState(bool);     // true; focusing in action, false; stable
};

#endif // GETIMAGE_H
