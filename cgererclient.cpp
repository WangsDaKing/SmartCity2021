#include "cgererclient.h"
#include "cgererclient.h"

CGererClient::CGererClient(qintptr sd, QObject *parent) : QObject(parent)
{
    _sd = sd;
}

CGererClient::~CGererClient()
{
    qDebug() << "GCererClient::~CGererClient : destruction !";
    on_info("GCererClient::~CGererClient : destruction !");
    _sock->close();
    delete _modbus;
    delete _sock;
}


void CGererClient::on_goGestionClient()
{
    _modbus = new CModbusTcp;
    connect(_modbus, &CModbusTcp::sig_info, this, &CGererClient::on_info);
    connect(_modbus, &CModbusTcp::sig_erreur, this, &CGererClient::on_erreur);

    _sock = new QTcpSocket();  // la socket est gérée par le thread

    // init des params du client et stockage dans variable commune
    if (_sock->setSocketDescriptor(_sd)) {
        _hostAddress = _sock->peerAddress();//addr IP du client
        _localAddress = _sock->localAddress();//addr IP du serveur
        _peerPort = _sock->peerPort();//port du client
        _localPort = _sock->localPort();//port du serveur
        on_info("CGererClient::on_goGestionClient : Connexion de IP="+_hostAddress.toString()
                +" Port="+QString::number(_peerPort)); //affichage dans l'Ihm

        // signaux de fonctionnement de la socket
        connect(_sock, &QTcpSocket::readyRead, this, &CGererClient::on_readyRead);
        connect(_sock, &QTcpSocket::disconnected, this, &CGererClient::on_disconnected);

        connect(_sock, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
                [=](QAbstractSocket::SocketError socketError) {
            switch(socketError) {
            case QAbstractSocket::RemoteHostClosedError:
                on_erreur("Erreur socket : Remote Host Closed !");
                break;
            default:
                on_erreur("Erreur socket : Erreur non précisée : "+QString::number(socketError));
            } // sw
        });// cette longue commande gère les erreurs de manière intelligente et
        // l'affiche ainsi que son code d'erreur sauf si c'est une déconnection

    } // if setsocket....
}

void CGererClient::on_readyRead()
{
    QTcpSocket *client = static_cast<QTcpSocket *>(sender()); //on détermine qui envoie le message
    qint64 nb = client->bytesAvailable();// aquisition du nombre d'octets reçus
    QByteArray trameClient=client->readAll();// lecture de la trame qui nous est retourné en ASCII
    //on_info("IP Local="+_localAddress.toString()+" Port="+QString::number(_localPort));//affichage le l'ip et le port du serveur
    emit sig_info(QString::number(nb)+" caractères reçus de IP="+_hostAddress.toString()+" Port="+QString::number(_peerPort)+" :" );
    //affichage du nombre de d'octets reçus, de l'IP, du port de la source, et le contenu du message
    emit sig_info("trame reçue -> " + trameClient);
    //on_writeToClients("Bien reçu !\n"); // ACK (acquittement : informer à l'émmeteur de la bonne reception du message)
    int commande = _modbus->on_trameClient(trameClient);//retourne -1 si mal passé, 0 si trame non complète, 1 si c'est bon

    switch(commande){

    case -1:    //si mal passé
        emit sig_erreur("CGererClient::on_readyRead : Erreur dans le format de la trame, requette supprimée");
        //on_writeToClients("format de requette incorrecte, requete supprimée");
        _modbus->deleteTc();
        break;

    case 0:     //si trame non complète
        part += 1;
        emit sig_erreur("Trame incomplète, attente de la "+QByteArray::number(part)+"ème partie de la requette.");
        //on_writeToClients("attente de la "+QByteArray::number(part)+"ème partie de la requette");
        break;

    case 1:     //trame complète
        part = 1;
        emit sig_info("Trame complète.");
        bool verifier =_modbus->verifier();
        if (!verifier){
            emit sig_erreur("CGererClient::on_readyRead : erreur verification");
            //on_writeToClients("Vérification de la trame échouée");
            _modbus->deleteTc();
            break;
        }//if(!verifier)
        //si le MBAP header + CRC sont bon
        int ordre = _modbus->decoder();
        QByteArray reponse;
        QByteArray val;
        switch (_modbus->get_functionCode()) {
        case 1://Lecture
            val = read(ordre);
            reponse = _modbus->reponseLecture(val);
            break;
        case 2://Ecriture
            bool exec = write(ordre);
            reponse = _modbus->reponseEcriture(exec);
            break;
        }
        on_writeToClients(reponse);
        _modbus->deleteTc();
        emit sig_info("Réponse envoyée.");
        break;

    }//sw
}

QByteArray CGererClient::read(int ordre)
{
    QByteArray data = "";
    switch (ordre) {
    //case :
    default :
        on_erreur("CGererClient::read : erreur de décodage");
        break;
    }
    return data;
}

bool CGererClient::write(int ordre)
{
    bool REturn = false;

    switch(ordre){
    case 1://authentification
        emit sig_info("Requette d'authentification.");
        REturn = _modbus->verificationMdp();
        break;
    case 2://suite
        //setter
        //return si ça c'est bien passé
        break;
    default :
        on_erreur("CGererClient::write : Erreur de décodage");
        break;
    }
    return REturn;
}

void CGererClient::on_writeToClients(QByteArray rep)
{
    qDebug() <<"méssage :" << rep;
    qint64 nb = _sock->write(rep);
    if (nb == -1){
        emit sig_erreur("Erreur d'envoi");
    }
}

void CGererClient::on_disconnected()
{
    emit sig_info("CGererClient::on_disconnected : Déconnexion de IP="+_hostAddress.toString()+" Port="+QString::number(_peerPort));
    emit sig_disconnected();
}

void CGererClient::on_erreur(QString mess)
{
    emit sig_erreur(mess);
}

void CGererClient::on_info(QString mess)
{
    emit sig_info(mess);
}

