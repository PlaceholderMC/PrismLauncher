#include <QObject>
#include <QDnsLookup>
#include <QtNetwork/qtcpsocket.h>
#include <QHostInfo>

// resolve the IP and port of a Minecraft server
class MCResolver : public QObject {
    Q_OBJECT

    QString constrDomain;
    int constrPort;

public:
    explicit MCResolver(QObject *parent, QString domain, int port): QObject(parent), constrDomain(domain), constrPort(port) {}

    void ping() {
        pingWithDomainSRV(constrDomain, constrPort);
    }

private:

    void pingWithDomainSRV(QString domain, int port) {
        QDnsLookup *lookup = new QDnsLookup(this);
        lookup->setName(QString("_minecraft._tcp.%1").arg(domain));
        lookup->setType(QDnsLookup::SRV);

        connect(lookup, &QDnsLookup::finished, this, [&, domain, port]() {
            QDnsLookup *lookup = qobject_cast<QDnsLookup *>(sender());

            lookup->deleteLater();

            if (lookup->error() != QDnsLookup::NoError) {
                qDebug() << QString("Warning: SRV record lookup failed (%1), trying A record lookup").arg(lookup->errorString());
                pingWithDomainA(domain, port);
                return;
            }

            auto records = lookup->serviceRecords();
            if (records.isEmpty()) {
                qDebug() << "Warning: no SRV entries found for domain, trying A record lookup";
                pingWithDomainA(domain, port);
                return;
            }


            const auto& firstRecord = records.at(0);
            QString domain = firstRecord.target();
            int port = firstRecord.port();
            pingWithDomainA(domain, port);
        });

        lookup->lookup();
    }

    void pingWithDomainA(QString domain, int port) {
        QHostInfo::lookupHost(domain, this, [&, port](const QHostInfo &hostInfo){
            if (hostInfo.error() != QHostInfo::NoError) {
                emitFail("A record lookup failed");
                return;
            } else {
                auto records = hostInfo.addresses();
                if (records.isEmpty()) {
                    emitFail("No A entries found for domain");
                    return;
                }
                
                const auto& firstRecord = records.at(0);
                emitSucceed(firstRecord.toString(), port);
            }
        });        
    }

    void emitFail(std::string error) {
        qDebug() << "Ping error:" << QString::fromStdString(error);
        emit fail();
    }

    void emitSucceed(QString ip, int port) {
        emit succeed(ip, port);
    }

signals:
    void succeed(QString ip, int port);
    void fail();
};
