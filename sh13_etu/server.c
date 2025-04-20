
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define NB_PLAYERS 4
#define NB_CARDS   13
#define NB_OBJECTS 8

struct _client
{
        char ipAddress[40];
        int port;
        char name[40];
} tcpClients[NB_PLAYERS];

int nbClients = 0;
int fsmServer  = 0;        /* 0 : attente des connexions, 1 : partie en cours   */
int deck[NB_CARDS]   = {0,1,2,3,4,5,6,7,8,9,10,11,12};
int tableCartes[NB_PLAYERS][NB_OBJECTS];  /* comptage des symboles              */
char *nomcartes[]=
{
        "Sebastian Moran", "Irene Adler", "Inspector Lestrade",
        "Inspector Gregson", "Inspector Baynes", "Inspector Bradstreet",
        "Inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
        "Mrs. Hudson", "Mary Morstan", "James Moriarty"
};

int joueurCourant = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/* Mélange les 13 cartes. */
void melangerDeck()
{
        for (int i=0;i<1000;i++)
        {
                int index1 = rand()%NB_CARDS;
                int index2 = rand()%NB_CARDS;
                int tmp     = deck[index1];
                deck[index1]= deck[index2];
                deck[index2]= tmp;
        }
}

/* Remplit tableCartes : nombre de symboles par joueur. */
void createTable()
{
	/*  Le joueur 0 possède deck[0..2]
	    Le joueur 1 possède deck[3..5]
	    Le joueur 2 possède deck[6..8]
	    Le joueur 3 possède deck[9..11]
	    deck[12] est le coupable (carte retournée au centre)             */
	for (int i=0;i<NB_PLAYERS;i++)
		for (int j=0;j<NB_OBJECTS;j++)
			tableCartes[i][j]=0;

	for (int i=0;i<NB_PLAYERS;i++)
	{
		for (int j=0;j<3;j++)
		{
			int c=deck[i*3+j];
			switch (c)
			{
				case 0: /* Sebastian Moran   */ tableCartes[i][7]++; tableCartes[i][2]++; break;
				case 1: /* Irene Adler       */ tableCartes[i][7]++; tableCartes[i][1]++; tableCartes[i][5]++; break;
				case 2: /* Inspector Lestrade*/ tableCartes[i][3]++; tableCartes[i][6]++; tableCartes[i][4]++; break;
				case 3: /* Inspector Gregson */ tableCartes[i][3]++; tableCartes[i][2]++; tableCartes[i][4]++; break;
				case 4: /* Inspector Baynes  */ tableCartes[i][3]++; tableCartes[i][1]++; break;
				case 5: /* Inspector Bradstreet */ tableCartes[i][3]++; tableCartes[i][2]++; break;
				case 6: /* Inspector Hopkins */ tableCartes[i][3]++; tableCartes[i][0]++; tableCartes[i][6]++; break;
				case 7: /* Sherlock Holmes   */ tableCartes[i][0]++; tableCartes[i][1]++; tableCartes[i][2]++; break;
				case 8: /* John Watson       */ tableCartes[i][0]++; tableCartes[i][6]++; tableCartes[i][2]++; break;
				case 9: /* Mycroft Holmes    */ tableCartes[i][0]++; tableCartes[i][1]++; tableCartes[i][4]++; break;
				case 10:/* Mrs. Hudson       */ tableCartes[i][0]++; tableCartes[i][5]++; break;
				case 11:/* Mary Morstan      */ tableCartes[i][4]++; tableCartes[i][5]++; break;
				case 12:/* James Moriarty    */ tableCartes[i][7]++; tableCartes[i][1]++; break;
			}
		}
	}
}

/* Envoie un message à un client TCP mono-trame */
void sendMessageToClient(char *clientip,int clientport,char *mess)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0) return;

    server = gethostbyname(clientip);
    if (server == NULL) return;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],(char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(clientport);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        close(sockfd);
        return;
    }

    char buffer[256];
    snprintf(buffer,sizeof(buffer),"%s\n",mess);
    write(sockfd,buffer,strlen(buffer));
    close(sockfd);
}

/* Envoie à tous les joueurs. */
void broadcastMessage(char *mess)
{
        for (int i=0;i<nbClients;i++)
                sendMessageToClient(tcpClients[i].ipAddress,
                                    tcpClients[i].port,
                                    mess);
}

/* Renvoie l'index d'un joueur à partir de son nom. */
int findClientByName(char *name)
{
        for (int i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) {
         fprintf(stderr,"Usage: %s <port>\n",argv[0]);
         exit(1);
     }

     /* Lancement du serveur d'attente TCP */
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) error("socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("bind");

     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     /* Préparation de la partie (un premier mélange, sera refait quand les 4 joueurs sont là) */
     melangerDeck();
     createTable();
     joueurCourant = 0;

     /* Initialise structure client */
     for (int i=0;i<NB_PLAYERS;i++)
     {
        strcpy(tcpClients[i].ipAddress,"localhost");
        tcpClients[i].port=-1;
        strcpy(tcpClients[i].name,"-");
     }

     while (1)
     {    
        newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if (newsockfd < 0) error("accept");

        bzero(buffer,256);
        int n = read(newsockfd,buffer,255);
        if (n < 0) error("read");

        printf("Received %s:%d -> %s", inet_ntoa(cli_addr.sin_addr),
                                        ntohs(cli_addr.sin_port), buffer);

        /* --- FSM : 0 = attente connexion, 1 = partie en cours -------------------- */
        if (fsmServer==0)
        {
            char com;
            char clientIp[256], clientName[256];
            int  clientPort;
            if (buffer[0]=='C')
            {
                sscanf(buffer,"%c %s %d %s",&com,clientIp,&clientPort,clientName);

                /* Ajout du client dans la liste */
                strcpy(tcpClients[nbClients].ipAddress,clientIp);
                tcpClients[nbClients].port = clientPort;
                strcpy(tcpClients[nbClients].name, clientName);
                nbClients++;

                /* Envoi de l'id au joueur */
                int id = nbClients-1;
                char reply[256];
                sprintf(reply,"I %d",id);
                sendMessageToClient(tcpClients[id].ipAddress,
                                    tcpClients[id].port,
                                    reply);

                /* Broadcast de la liste des joueurs connectés */
                sprintf(reply,"L %s %s %s %s",
                        tcpClients[0].name, tcpClients[1].name,
                        tcpClients[2].name, tcpClients[3].name);
                broadcastMessage(reply);

                /* Quand on atteint 4 joueurs on lance réellement la partie */
                if (nbClients==NB_PLAYERS)
                {
                    /* Un nouveau mélange pour commencer la partie */
                    melangerDeck();
                    createTable();

                    /* Distribution des cartes + information privée (tableCartes personnelle) */
                    for (int p=0;p<NB_PLAYERS;p++)
                    {
                        /* Ses 3 cartes */
                        sprintf(reply,"D %d %d %d", deck[p*3], deck[p*3+1], deck[p*3+2]);
                        sendMessageToClient(tcpClients[p].ipAddress, tcpClients[p].port, reply);

                        /* Sa ligne de symboles (privée) */
                        for (int j=0;j<NB_OBJECTS;j++)
                        {
                            sprintf(reply,"V %d %d %d", p, j, tableCartes[p][j]);
                            sendMessageToClient(tcpClients[p].ipAddress, tcpClients[p].port, reply);
                        }
                    }

                    /* Définit le joueur 0 comme premier à jouer */
                    joueurCourant = 0;
                    sprintf(reply,"M %d",joueurCourant);
                    broadcastMessage(reply);

                    fsmServer = 1;   /* Passage en mode partie en cours */
                }
            }
        }
        else if (fsmServer==1)
        {
            char type = buffer[0];
            char reply[256];

            switch (type)
            {
                /* ---------- Accusation ---------------- */
                case 'G':   /* "G id suspect" */
                {
                    int idJ, suspect;
                    sscanf(buffer,"G %d %d",&idJ,&suspect);

                    /* Vérification de la victoire */
                    int correct = (suspect == deck[12]);

                    /* On informe tout le monde du résultat (simple) */
                    sprintf(reply,"G %d %d %d", idJ, suspect, correct);
                    broadcastMessage(reply);

                    if (correct)
                    {
                        // /* On arrête la partie – pour simplifier on quitte le programme */
                        // printf("Le joueur %d a gagné !\n",idJ);
                        // exit(0);
                        /* Nom du vainqueur */
                        char *winner = tcpClients[idJ].name;

                        /* Affichage serveur */
                        printf("Victoire : %s (joueur %d)\n", winner, idJ);

                        /* Message spécial pour tous les clients (facultatif) */
                        char reply[256];
                        sprintf(reply, "W %d %s", idJ, winner);   /* W = Win */
                        broadcastMessage(reply);

                        /* Terminer proprement le serveur */
                        exit(0);
                    }
                    else
                    {
                        /* Tour suivant */
                        joueurCourant = (idJ+1)%NB_PLAYERS;
                        sprintf(reply,"M %d",joueurCourant);
                        broadcastMessage(reply);
                    }
                    break;
                }

                /* ---------- Question globale sur un objet ------------- */
                case 'O':   /* "O id objet" : Compte sur tous les joueurs SAUF l'auteur */
                {
                    int idJ, objet;
                    sscanf(buffer,"O %d %d",&idJ,&objet);

                    for (int p=0;p<NB_PLAYERS;p++)
                    {
                        if (p==idJ) continue;
                        int val = tableCartes[p][objet];
                        sprintf(reply,"V %d %d %d", p, objet, val);
                        sendMessageToClient(tcpClients[idJ].ipAddress,
                                            tcpClients[idJ].port, reply);

                        /* Les autres joueurs voient juste un '*' (100) pour savoir
                           qu'une information a été révélée sans connaître la valeur   */
                        sprintf(reply,"V %d %d %d", p, objet, 100);
                        for (int q=0;q<NB_PLAYERS;q++)
                            if (q!=idJ)
                                sendMessageToClient(tcpClients[q].ipAddress,
                                                    tcpClients[q].port, reply);
                    }

                    /* Tour suivant */
                    joueurCourant = (idJ+1)%NB_PLAYERS;
                    sprintf(reply,"M %d",joueurCourant);
                    broadcastMessage(reply);
                    break;
                }

                /* ---------- Question ciblée à un joueur -------------- */
                case 'S':   /* "S id joueurCible objet" */
                {
                    int idJ, cible, objet;
                    sscanf(buffer,"S %d %d %d",&idJ,&cible,&objet);

                    /* Valeur envoyée uniquement au demandeur */
                    int val = tableCartes[cible][objet];
                    sprintf(reply,"V %d %d %d", cible, objet, val);
                    sendMessageToClient(tcpClients[idJ].ipAddress,
                                        tcpClients[idJ].port, reply);

                    /* Un '*' (100) pour les autres */
                    sprintf(reply,"V %d %d %d", cible, objet, 100);
                    for (int q=0;q<NB_PLAYERS;q++)
                        if (q!=idJ)
                            sendMessageToClient(tcpClients[q].ipAddress,
                                                tcpClients[q].port, reply);

                    /* Tour suivant */
                    joueurCourant = (idJ+1)%NB_PLAYERS;
                    sprintf(reply,"M %d",joueurCourant);
                    broadcastMessage(reply);
                    break;
                }
                default:
                    break;
            }
        }

        close(newsockfd);
     }

     close(sockfd);
     return 0; 
}
