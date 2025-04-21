// #include <SDL.h>     
#include <SDL2/SDL.h>     
// #include <SDL_image.h>        
// #include <SDL_ttf.h>     
#include <SDL2/SDL_image.h>   
#include <SDL2/SDL_ttf.h>       
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

pthread_t thread_serveur_tcp_id;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char gbuffer[256];
char gServerIpAddress[256];
int  gServerPort;
char gClientIpAddress[256];
int  gClientPort;
char gName[256];
char gNames[4][256];
int  gId = -1;
int  joueurSel, objetSel, guiltSel;
int  guiltGuess[13];
int  tableCartes[4][8];
int  b[3];
int  goEnabled, connectEnabled;

char *nbobjets[] =
  {"5","5","5","5","4","3","3","3"};
char *nbnoms[] =
  {"Sebastian Moran", "Irene Adler", "Inspector Lestrade",
   "Inspector Gregson", "Inspector Baynes", "Inspector Bradstreet",
   "Inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
   "Mrs. Hudson", "Mary Morstan", "James Moriarty"};

volatile int synchro;

/* -------------------- petit serveur TCP local -------------------- */
void *fn_serveur_tcp(void *arg)
{
    (void)arg; // c'est pour eviter warning car arg non utilise
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(gClientPort);

    if (bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        perror("bind"); exit(1); }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
        if (newsockfd<0){ perror("accept"); exit(1); }

        bzero(gbuffer,256);
        if (read(newsockfd,gbuffer,255)<0){ perror("read"); exit(1); }

        synchro = 1;
        while (synchro);               /* attendre consommation */
        close(newsockfd);
    }
}

/* -------------------- envoyer au serveur principal --------------- */
void sendMessageToServer(char *ip,int port,char *mess)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char sendbuffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0) return;

    server = gethostbyname(ip);
    if (!server){ close(sockfd); return; }

    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        close(sockfd); return; }

    sprintf(sendbuffer,"%s\n",mess);
    //write(sockfd,sendbuffer,strlen(sendbuffer));
    if (write(sockfd, sendbuffer, strlen(sendbuffer)) < 0)
    perror("write");


    close(sockfd);
}

/* ============================ MAIN ============================ */
int main(int argc,char **argv)
{
    if (argc<6){
        printf("%s <IPserveur> <Portserv> <IPclient> <Portclient> <Nom>\n",argv[0]);
        exit(1);
    }

    /* ---- paramètres ---- */
    strcpy(gServerIpAddress,argv[1]);
    gServerPort  = atoi(argv[2]);
    strcpy(gClientIpAddress,argv[3]);
    gClientPort  = atoi(argv[4]);
    strcpy(gName,argv[5]);

    /* ---- SDL ---- */
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window  *win  = SDL_CreateWindow("SDL2 SH13",
                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           1024,768,0);
    SDL_Renderer* ren = SDL_CreateRenderer(win,-1,0);

    /* ---- chargement images ---- */
    SDL_Surface *deck[13],*objet[8],*gobutton,*connectbutton;
    char path[64];
    for(int i=0;i<13;i++){ sprintf(path,"SH13_%d.png",i);  deck[i]=IMG_Load(path); }
    objet[0] = IMG_Load("SH13_pipe_120x120.png");
    objet[1] = IMG_Load("SH13_ampoule_120x120.png");
    objet[2] = IMG_Load("SH13_poing_120x120.png");
    objet[3] = IMG_Load("SH13_couronne_120x120.png");
    objet[4] = IMG_Load("SH13_carnet_120x120.png");
    objet[5] = IMG_Load("SH13_collier_120x120.png");
    objet[6] = IMG_Load("SH13_oeil_120x120.png");
    objet[7] = IMG_Load("SH13_crane_120x120.png");

    gobutton      = IMG_Load("gobutton.png");
    connectbutton = IMG_Load("connectbutton.png");

    SDL_Texture *tdeck[13],*tobj[8],*tgo,*tconnect;
    for(int i=0;i<13;i++) tdeck[i]=SDL_CreateTextureFromSurface(ren,deck[i]);
    for(int i=0;i<8;i++)  tobj[i]=SDL_CreateTextureFromSurface(ren,objet[i]);
    tgo      = SDL_CreateTextureFromSurface(ren,gobutton);
    tconnect = SDL_CreateTextureFromSurface(ren,connectbutton);

    TTF_Font* font = TTF_OpenFont("sans.ttf",15);

    /* ---- structures jeu ---- */
    for(int i=0;i<4;i++) strcpy(gNames[i],"-");
    joueurSel=objetSel=guiltSel=-1;
    for(int i=0;i<13;i++) guiltGuess[i]=0;
    for(int i=0;i<4;i++)for(int j=0;j<8;j++)tableCartes[i][j]=-1;
    b[0]=b[1]=b[2]=-1;
    goEnabled=0; connectEnabled=1;

    /* ---- thread serveur local ---- */
    synchro=0;
    pthread_create(&thread_serveur_tcp_id,NULL,fn_serveur_tcp,NULL);

    /* ================= boucle principale SDL ================= */
    int quit=0; SDL_Event ev;
    while(!quit)
    {
        /* ---- events ---- */
        while(SDL_PollEvent(&ev))
        {
            if (ev.type==SDL_QUIT){ quit=1; }
            else if (ev.type==SDL_MOUSEBUTTONDOWN)
            {
                int mx,my; SDL_GetMouseState(&mx,&my);

                /* bouton CONNECT */
                if (mx<200 && my<50 && connectEnabled){
                    char buf[1024];
                    sprintf(buf,"C %s %d %s",gClientIpAddress,gClientPort,gName);
                    sendMessageToServer(gServerIpAddress,gServerPort,buf);
                    connectEnabled=0;
                }
                /* sélections */
                else if (mx>=0 && mx<200 && my>=90 && my<330){
                    joueurSel=(my-90)/60; guiltSel=-1;
                }
                else if (mx>=200 && mx<680 && my>=0 && my<90){
                    objetSel=(mx-200)/60; guiltSel=-1;
                }
                else if (mx>=100 && mx<250 && my>=350 && my<740){
                    joueurSel=objetSel=-1; guiltSel=(my-350)/30;
                }
                else if (mx>=250 && mx<300 && my>=350 && my<740){
                    int k=(my-350)/30; guiltGuess[k]=1-guiltGuess[k];
                }
                /* GO */
                else if (mx>=500 && mx<700 && my>=350 && my<450 && goEnabled){
                    char buf[256];
                    if (guiltSel!=-1){
                        sprintf(buf,"G %d %d",gId,guiltSel);
                        sendMessageToServer(gServerIpAddress,gServerPort,buf);
                    }
                    else if (objetSel!=-1 && joueurSel==-1){
                        sprintf(buf,"O %d %d",gId,objetSel);
                        sendMessageToServer(gServerIpAddress,gServerPort,buf);
                    }
                    else if (objetSel!=-1 && joueurSel!=-1){
                        sprintf(buf,"S %d %d %d",gId,joueurSel,objetSel);
                        sendMessageToServer(gServerIpAddress,gServerPort,buf);
                    }
                    goEnabled=0;
                }
                else { joueurSel=objetSel=guiltSel=-1; }
            }
        }

        /* ---- traitement message réseau ---- */
        if (synchro)
        {
            char c=gbuffer[0];
            if (c=='I'){ sscanf(gbuffer,"I %d",&gId); }
            else if (c=='L'){
                sscanf(gbuffer,"L %s %s %s %s",
                       gNames[0],gNames[1],gNames[2],gNames[3]);
            }
            else if (c=='D'){
                sscanf(gbuffer,"D %d %d %d",&b[0],&b[1],&b[2]);
            }
            else if (c=='M'){
                int cur; sscanf(gbuffer,"M %d",&cur);
                goEnabled = (cur==gId);
            }
            else if (c=='V'){
                int i,j,v; sscanf(gbuffer,"V %d %d %d",&i,&j,&v);
                tableCartes[i][j]=v;
            }
            
        
            synchro=0;
        }

        /* ================== RENDER ================== */
        SDL_SetRenderDrawColor(ren,255,230,230,255);
        SDL_Rect full={0,0,1024,768}; SDL_RenderFillRect(ren,&full);

        /* highlights sélection */
        if (joueurSel!=-1){
            SDL_SetRenderDrawColor(ren,255,180,180,255);
            SDL_Rect r={0,90+joueurSel*60,200,60}; SDL_RenderFillRect(ren,&r);
        }
        if (objetSel!=-1){
            SDL_SetRenderDrawColor(ren,180,255,180,255);
            SDL_Rect r={200+objetSel*60,0,60,90}; SDL_RenderFillRect(ren,&r);
        }
        if (guiltSel!=-1){
            SDL_SetRenderDrawColor(ren,180,180,255,255);
            SDL_Rect r={100,350+guiltSel*30,150,30}; SDL_RenderFillRect(ren,&r);
        }

        /* objets en haut */
        SDL_Rect r; r.w=r.h=40; r.y=10;
        for(int i=0;i<8;i++){
            r.x=210+i*60; SDL_RenderCopy(ren,tobj[i],NULL,&r);
        }

        /* nb d'objets */
        SDL_Color black={0,0,0,255};
        for(int i=0;i<8;i++){
            SDL_Surface *s=TTF_RenderText_Solid(font,nbobjets[i],black);
            SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
            SDL_Rect m={230+i*60,50,s->w,s->h};
            SDL_RenderCopy(ren,t,NULL,&m);
            SDL_DestroyTexture(t); SDL_FreeSurface(s);
        }

        /* noms suspects */
        for(int i=0;i<13;i++){
            SDL_Surface *s=TTF_RenderText_Solid(font,nbnoms[i],black);
            SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
            SDL_Rect m={105,350+i*30,s->w,s->h};
            SDL_RenderCopy(ren,t,NULL,&m);
            SDL_DestroyTexture(t); SDL_FreeSurface(s);
        }

        /* tableCartes */
        for(int i=0;i<4;i++)
            for(int j=0;j<8;j++)
                if (tableCartes[i][j]!=-1){
                    char buf[8];
                    sprintf(buf,"%s",tableCartes[i][j]==100?"*":
                                   (sprintf(buf,"%d",tableCartes[i][j]),buf));
                    SDL_Surface *s=TTF_RenderText_Solid(font,buf,black);
                    SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
                    SDL_Rect m={230+j*60,110+i*60,s->w,s->h};
                    SDL_RenderCopy(ren,t,NULL,&m);
                    SDL_DestroyTexture(t); SDL_FreeSurface(s);
                }

        /* cartes en main */
        SDL_Rect cpos={750,0,1000/4,660/4};
        for(int i=0;i<3;i++){
            if (b[i]==-1) continue;
            cpos.y=i*200;
            SDL_RenderCopy(ren,tdeck[b[i]],NULL,&cpos);
        }

        /* boutons */
        if (goEnabled){ SDL_Rect q={500,350,200,150};
                        SDL_RenderCopy(ren,tgo,NULL,&q); }
        if (connectEnabled){ SDL_Rect q={0,0,200,50};
                             SDL_RenderCopy(ren,tconnect,NULL,&q); }

        /* croix sur suppositions */
        SDL_SetRenderDrawColor(ren,255,0,0,255);
        for(int i=0;i<13;i++) if (guiltGuess[i]){
            SDL_RenderDrawLine(ren,250,350+i*30,300,380+i*30);
            SDL_RenderDrawLine(ren,250,380+i*30,300,350+i*30); }

        /* grilles */
        SDL_SetRenderDrawColor(ren,0,0,0,255);
        SDL_RenderDrawLine(ren,0,90+60,680,90+60);
        SDL_RenderDrawLine(ren,0,90+120,680,90+120);
        SDL_RenderDrawLine(ren,0,90+180,680,90+180);
        SDL_RenderDrawLine(ren,0,90+240,680,90+240);
        SDL_RenderDrawLine(ren,0,90+300,680,90+300);

        SDL_RenderDrawLine(ren,200,0,200,330);
        for(int x=260;x<=620;x+=60) SDL_RenderDrawLine(ren,x,0,x,330);
        SDL_RenderDrawLine(ren,680,0,680,330);
        

        for(int y=0;y<14;y++) SDL_RenderDrawLine(ren,0,350+y*30,300,350+y*30);
        SDL_RenderDrawLine(ren,100,350,100,740);
        SDL_RenderDrawLine(ren,250,350,250,740);
        SDL_RenderDrawLine(ren,300,350,300,740);

        /* noms joueurs */
        for(int i=0;i<4;i++) if (strlen(gNames[i])>0){
            SDL_Surface *s=TTF_RenderText_Solid(font,gNames[i],black);
            SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
            SDL_Rect m={10,110+i*60,s->w,s->h};
            SDL_RenderCopy(ren,t,NULL,&m);
            SDL_DestroyTexture(t); SDL_FreeSurface(s);
        }

        SDL_RenderPresent(ren);
    }

    SDL_Quit();
    return 0;
}
