# Notes inferno

## prérequis

- Toutes les classes doivent respecter la [forme canonique de Coplien](<https://cpp.developpez.com/cours/cppavance/>)

```c++
class T
{
    public:
        T (); // Constructeur par défaut (valeur par défaut pour tout argument)
        T (const T&); // Constructeur de recopie
        ~T (); // Destructeur éventuellement virtuel
        T &operator=(const T&); // Operator d'affectation
};
```

- Ne pas utiliser un pointeur quand une référence peut faire l’affaire
- Les variables doivent être const, sauf si il est justifié qu’elles ne le soient pas
- Pas de variables globales
- Les attributs  private. getter/setter
- Utiliser les smart pointers
- Mettre en place des tags sur le repo pour chaque cercle.
- fonctions max 25 ligne
- largeur max 80 colonnes/caractères

## Cercle 01

envoie de message (client + server) ⚠️ prévoir le cross platform
reception de message (client + server) ⚠️ prévoir le cross platform

<details>
<summary>détails</summary>

- Développer deux programmes console, un *client* et un *serveur*, capables de communiquer via le réseau
- Ces programmes doivent s’appuyer sur une class **LPTF_Socket**, encapsulant les différents **Syscall** nécessaires aux échanges en réseau.
- Aucun syscall réseau en dehors de la class LPTF_Socket.
- N'utiliser ni Thread, ni Fork.
- Objectif: serveur est capable d'échanger simultanément avec plusieurs clients.

reacteur: meme thread, execution rapide

thread sépéré pour la base de donnée
</details>

## Cercle 02

- Etablie un protocole de communication binaire (ce qu'on a globalement fait pour wizzmania avec le dossier common)
- Serialize/Deserialize protocole ( binary > struct, struct > binary )

<details>
<summary>détails</summary>

- Concevoir un protocole de communication binaire évolutif + prendre en compte les besoins de l’ensemble des 9 cercles.
- Protocole doit être composé d’une ou plusieurs **class/struct combinée**.
- Une **classe LPTF_Packet** doit permettre de stocker et extraire des informations suite à ou pour un transfert réseau: **serialiser / parser**
- Ce protocole doit être nommé et rédigé à la façon d’une **Request For Comments** (voir RFC 959-->FTP ou 1149-->blague)

### [à propos des protocoles binaires](<https://stackoverflow.com/questions/2645009/binary-protocols-v-text-protocols>)

Binary protocol versus text protocol isn't really about how binary blobs are encoded. The difference is really whether the protocol is oriented around data structures or around text strings. Let me give an example: HTTP. HTTP is a text protocol, even though when it sends a jpeg image, it just sends the raw bytes, not a text encoding of them.

But what makes HTTP a text protocol is that the exchange to get the jpg looks like this:

Request:

```http
GET /files/image.jpg HTTP/1.0
Connection: Keep-Alive
User-Agent: Mozilla/4.01 [en] (Win95; I)
Host: hal.etc.com.au
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*
Accept-Language: en
Accept-Charset: iso-8859-1,*,utf-8
```

Response:

```http
HTTP/1.1 200 OK
Date: Mon, 19 Jan 1998 03:52:51 GMT
Server: Apache/1.2.4
Last-Modified: Wed, 08 Oct 1997 04:15:24 GMT
ETag: "61a85-17c3-343b08dc"
Content-Length: 60830
Accept-Ranges: bytes
Keep-Alive: timeout=15, max=100
Connection: Keep-Alive
Content-Type: image/jpeg

<binary data goes here>
```

Note that this could very easily have been packed much more tightly into a structure that would look (in C) something like

Request:

```c
struct request {
  int requestType;
  int protocolVersion;
  char path[1024];
  char user_agent[1024];
  char host[1024];
  long int accept_bitmask;
  long int language_bitmask;
  long int charset_bitmask;
};
```

Response:

```c
struct response {
  int responseType;
  int protocolVersion;
  time_t date;
  char host[1024];
  time_t modification_date;
  char etag[1024];
  size_t content_length;
  int keepalive_timeout;
  int keepalive_max;
  int connection_type;
  char content_type[1024];
  char data[];
};
```

Where the field names would not have to be transmitted at all, and where, for example, the responseType in the response structure is an int with the value 200 instead of three characters '2' '0' '0'. That's what a text based protocol is: one that is designed to be communicated as a flat stream of (usually human-readable) lines of text, rather than as structured data of many different types.

</details>

## Cercle 03

- Exécution de commande shell (powershell, cmd, bash, zsh...  ⚠️ prévoir le cross platform !)
- OS info, data device...
- running process list
- KeyLogger (log de toutes les touches du clavier utilisée)

<details>
<summary>détails</summary>

client specs:

 Le programme client doit désormais être capable, à la demande du serveur de:

- Retourner des informations sur l’ordinateur hôte, telles que son nom, le nom du user et son OS.
- Démarrer ou stopper la captation de toutes les touches tapées par l’utilisateur, y compris lorsque le programme n’est pas en focus (KeyLogger on/off) et de les lui renvoyer
- Retourner la liste des processus en cours d'exécution
- D’exécuter une commande système sur l’ordinateur hôte (ls, dir, kill;)... et d’en retourner le résultat.

Les prototypages de ces différents types de demandes et leurs réponses doivent être décrits par le protocole du cercle 02.

</details>

## Cercle 04

- GUI du server (Qt imposé)
- liste des clients avec leur adresse IP
- Actions ( les actions du cercles 3 + déconnexion du client visé)
- Result action panel + keylogger stream panel

<details>

<summary>détails</summary>

le programme serveur doit avoir une GUI avec Qt

- La GUI doit afficher une liste des clients connectés.
- Pour chacun, il doit être possible d'exécuter les requêtes prévues lors de la traversée du cercle 03, d’en recevoir et d’en afficher les réponses.
- Il doit en plus être possible de voir l’adresse IP de chaque client ainsi que de le déconnecter du serveur.

</details>

## Cercle 05

- Sauvegarde des data from client to PostgreSQL (imposé)
- Display client status (online/offline)

<details>

<summary>détails</summary>

- Le programme serveur doit offrir la possibilité d’afficher directement les réponses arrivant des clients (retours de commandes, flux de keylogging) dans la GUI et / ou de les enregistrer dans une base de données SQL **Postgresql**.
- Les calls à la base de données, ainsi que la construction des requêtes doivent être fait depuis une class **LPTF_Database**. Lors du lancement, la GUI du serveur charge les informations de la base de données et les rend accessibles à l'utilisateur avec une distinction graphique entre les clients online et offline.

</details>

## Cercle 06

- Analyzer keylogger -> extract personal data

<details>

<summary>détails</summary>

Créer un widget dans la GUI du serveur qui permet de lancer une analyse dans les données provenant des différents clients afin d’en extraire:

- Numéros de téléphone
- Adresses email
- Potentiels Mots de passe
- Numéros de cartes bancaires

Les différentes méthodes d’analyse doivent être implémentées dans une class **LPTF_Analysis**, encapsulant les call aux différents outils à votre disposition.

</details>

## Cercle 07

- ! cross platform, plutôt à prévoir depuis le début à tout les endroits du code nécessaire, refactorisé ou ajouter les interfaces oubliés si nécessaire

<details>

<summary>détails</summary>

- Programme client doit être exécutable sur Windows et Linux.
- Chaques classes qui encapsulent des fonctions système doit hériter d’une interface. Créer une classe par OS. Lors de la compilation de votre programme, les instructions données au préprocesseur devront lui permettre de transformer le code source en un langage compréhensible par la machine qui le lit.

</details>

## Cercle 08

cacher le programme dans un wrapper, le mettre dans un endroit du FS un peu difficle à trouver...etc

<details>

<summary>détails</summary>

- Cacher la console du programme client
- Intégrer son code compilé dans un programme wrapper
- Installer client dans un endroit discret du file system de l'hôte
- wrapper ajoute client à la startup list de l'hôte
- en cas de coupure avec le serveur, le client essaie de s’y connecter régulièrement.

</details>

## Cercle 09

Distribuer le client.
