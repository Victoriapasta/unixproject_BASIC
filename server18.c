#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "locker.h"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 100

void update(FILE* fp, int ch, struct locker *locker);
void load(FILE* fp, struct locker *locker, int ch);
void printlockers(FILE *fp, struct locker *lockers, int ch, int total);
// void locking(struct flock lock, struct locker *locker, int fd, int f);

int main ()
 {
    struct locker *locker;
    struct flock lock;
    int listenfd, connfd, clientlen, size, ch, bigch, total, wantID, f, nowusing, fd;
    char count[MAXLINE], usedid[MAXLINE], setPW[MAXLINE], rpassword[MAXLINE], spassword[MAXLINE];
    char using, canUse;
    FILE *fp=fopen("info", "wb");
    fd = open("info", "wb");

    struct sockaddr_un serverUNIXaddr, clientUNIXaddr;

    signal(SIGCHLD, SIG_IGN);
    clientlen = sizeof(clientUNIXaddr);

    listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
    serverUNIXaddr.sun_family = AF_UNIX;
    strcpy(serverUNIXaddr.sun_path, "convert");
    unlink("convert");
    bind(listenfd, (struct sockaddr *)&serverUNIXaddr, sizeof(serverUNIXaddr));
    listen(listenfd, 5);

    //사물함 갯수 설정
    printf("사물함 갯수 설정: \n");
    scanf("%d", &ch);
    printf("큰 사물함 갯수 설정: ");
    scanf("%d", &bigch);

    total = ch + bigch;
    locker = (struct locker*)malloc((total+1)*sizeof(struct locker));
    
    printf("|------------------- 현재 사물함 정보 --------------------|\n");
    //사물함 초기화 과정
    for(int i=1; i<=ch; i++)
    {
        locker[i].id = i;
        locker[i].used = 0;
        locker[i].space = 10;
        strcpy(locker[i].PW, "0000");
        printf("|      locker ID : %d        |       Used :      No        |\n",i);
    }
    for(int i=ch+1; i<=total; i++)
    {
        locker[i].id = i;
        locker[i].used = 0;
        locker[i].bigspace = 20;
        strcpy(locker[i].PW, "0000");
        printf("|      Big locker ID : %d    |       Used :      No        |\n",i);
    }
    printf("|------------------- 현재 사물함 정보 --------------------|\n");

    while (1)
    { /* 소켓 연결 요청 수락 */

        connfd = accept(listenfd,(struct sockaddr *)&clientUNIXaddr, &clientlen);
        if(connfd == -1)
            printf("error\n");

        int fpipe[2];
        pipe(fpipe);

        if(fork()==0)
        {
            close(fpipe[0]);
            /* connfd에 사물함 개수 저장*/
            write(connfd, &ch, sizeof(int));
            write(connfd, &bigch, sizeof(int));
            /* 현재 사물함 이용여부를 클라이언트에 전달 */
            load(fp, locker, total);
            for(int i=1; i<=ch; i++)
            {
                if(locker[i].used == 0)
                {
                    using='0';
                    write(connfd, &using, sizeof(using));
                }
                else
                {
                    using='1';
                    write(connfd, &using, sizeof(using));
                }
            }
            /*BIG Locker 추가*/
            for(int i=ch+1; i<=total; i++)
            {
                if(locker[i].used == 0)
                {
                    using='0';
                    write(connfd, &using, sizeof(using));
                }
                else
                {
                    using='1';
                    write(connfd, &using, sizeof(using));
                }
            }
            /*Client의 사물함 이용*/
            while(1)
            {         

            	load(fp, locker, total);
                read(connfd, &f, sizeof(int));
                
                if(f == 1)
                {
                /*사물함 선택 및 비밀번호 설정*/
                    read(connfd, &wantID, sizeof(int));
                    
                    if(locker[wantID].used == 0)
                    {
                        canUse = 'Y';
                        // locking(lock, locker, fd, f);
                        write(connfd, &canUse, 1);
                        read(connfd, &setPW, sizeof(setPW));
                        strcpy(locker[wantID].PW, setPW);
                        locker[wantID].used = 1;
                        update(fp, ch, locker);
                        printlockers(fp, locker, ch, total);

                    }
                    else
                    {
                        // clientfd에 'N'를 write
                        canUse = 'N';
                        write(connfd, &canUse, 1);
                        // locking(lock, locker, fd, f);

						strcpy(spassword,locker[wantID].PW);

						// 클라이언트에서 입력한 비밀번호를 rpassword에 write 
						read(connfd,&rpassword,sizeof(rpassword));

						if(!strcmp(rpassword,locker[wantID].PW))
						{
                            locker[wantID].used = 0;
                            nowusing = 3;
                            write(connfd, &nowusing, sizeof(int));
                           update(fp, ch, locker);
                           printlockers(fp, locker, ch, total);
						}
						else 
							write(connfd, 0, sizeof(int));
                    }

            }
            /*물건 넣고 빼기 모드*/
            else if(f == 2)
            {
                // locking(lock, locker, fd, f);
                read(connfd, &nowusing, sizeof(int));
                if(nowusing <= total)
                {
                    write(connfd, &locker[nowusing].used, sizeof(int));
                    if(locker[nowusing].used == 1){
                    read(connfd, &rpassword, sizeof(rpassword));
                    if(strcmp(rpassword, locker[nowusing].PW))
                    {
                        canUse = 'N';
                        write(connfd, &canUse, 1);
                    }
                    else
                    {
                        /*소형사물함 물건 넣고 남은공간 출력*/
                        if(nowusing <= ch)
                        {
                            canUse = 'Y';
                            write(connfd, &canUse, 1);
                            read(connfd, &size, sizeof(int));
                            int tempsize = locker[nowusing].space - size;
                            write(connfd, &tempsize, sizeof(int));
                            if(tempsize>=0){
                            	locker[nowusing].space -= size;
                            update(fp, ch, locker);
                            printlockers(fp, locker, ch, total);
                            }
                        }
                        /*대형사물함 물건 넣고 남은공간 출력*/
                        else if(ch<nowusing && nowusing <= total)
                        {
                            canUse = 'Y';
                            write(connfd, &canUse, 1);
                            read(connfd, &size, sizeof(int));
                            int tempsize = locker[nowusing].bigspace - size;
                            write(connfd, &tempsize, sizeof(int));
                             if(tempsize>=0){
                            	 locker[nowusing].bigspace -= size;
                                 update(fp, ch, locker);
                                 printlockers(fp, locker, ch, total);
                             }
                        }
                    }
                }
                }
            }

        }
    }
        
    }

    close(connfd);
    return 0;
}
    void update(FILE* fp, int ch, struct locker *locker){
    	fp = fopen("info", "wb"); //open("info",O_WRONLY|O_CREAT|O_EXCL, 0640);
    	fseek(fp, 0L, SEEK_SET);
    	fwrite(locker, sizeof(struct locker), ch, fp);
    	fflush(fp);
    	fclose(fp);
    }

    void load(FILE* fp, struct locker *locker, int ch)
    {
    	fp = fopen("info", "r");
        fseek(fp, 0L, SEEK_SET);
        fread(locker, sizeof(struct locker), ch, fp);
        fclose(fp);
    }
    
  void printlockers(FILE* fp, struct locker *lockers, int ch, int total)
  {
    	load(fp, lockers, ch);
        
    	 printf("|------------------- 현재 사물함 정보 -------------------- |\n");
    	    //사물함 초기화 과정
    	    for(int i=1; i<=ch; i++)
    	    {
                if(lockers[i].used == 0)
                {
                    char yesno[] = "No ";
                    strcpy(lockers[i].yesorno, yesno);
                }  
                else
                {
                    char yesno[] = "Yes";
                    strcpy(lockers[i].yesorno, yesno);
                }
    	        printf("|      locker ID : %d        |       Used :      %s        |\n",i, lockers[i].yesorno);
    	    }
    	    for(int i=ch+1; i<=total; i++)
    	    {
                if(lockers[i].used == 0)
                {
                    char yesno[] = "No ";
                    strcpy(lockers[i].yesorno, yesno);
                }  
                else
                {
                    char yesno[] = "Yes";
                    strcpy(lockers[i].yesorno, yesno);
                }
    	        printf("|      Big locker ID : %d    |       Used :      %s        |\n",i, lockers[i].yesorno);
    	    }
    	    printf("|------------------- 현재 사물함 정보 -------------------- |\n");
    }

/*
void locking(struct flock lock, struct locker *locker, int fd, int f)
{
    lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = (f-START_ID)*sizeof(locker);
	lock.l_len = sizeof(locker);
    lseek(fd, (f-START_ID)*sizeof(locker), SEEK_SET);
         
    lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
}
*/