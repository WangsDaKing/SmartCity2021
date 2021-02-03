#ifndef CSERVERTCP_H
#define CSERVERTCP_H


#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QThread>
#include <stdio.h>

#include "cgestionclient.h"

class CServerTcp : public QTcpServer
{
    Q_OBJECT

public:
    explicit CServerTcp(QObject *parent = nullptr, quint16 noPort = 2222);
    ~CServerTcp();

private:
    //variables
    quint16 m_noPort;
    QList<QTcpSocket*> _listeClients;//liste des clients connectés
    QList<CGestionClient*> _listeGestion;//liste gestionClients
    QList<QThread*> _listeThread;//liste Thread
    char ch[30];

    //objects
    CGestionClient *_client;
    QThread *_gthc;//gestion Thread Client

    //méthodes
    int init();


signals:
    void sig_erreur(QString mess);
    void sig_info(QString mess);

private slots:

    void onNewConnectionClient();
    void onDisconnectedClient();
    void on_erreur(QString mess);
    void on_info(QString mess);
};

#endif // CSERVERTCP_H
