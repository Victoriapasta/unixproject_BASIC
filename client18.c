#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#define DEFAULT_PROTOCOL 0
#define MAXLINE 100

int main ( )
{
  int clientfd, result, ch, bigch, total, status, wantID, f, useid, nowusing, cl, size;
  char count[MAXLINE], password[MAXLINE], repassword[MAXLINE];
  char using, canUse;

  struct sockaddr_un serverUNIXaddr;
  clientfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
  serverUNIXaddr.sun_family = AF_UNIX;
  strcpy(serverUNIXaddr.sun_path, "convert");

  do
  { /* 연결 요청 */
    result = connect(clientfd, (struct sockaddr *)&serverUNIXaddr, sizeof(serverUNIXaddr));
      if (result == -1) sleep(1);
  } while (result == -1);


  /*clientfd를 열어서 서버에서 작성한 정보 읽기*/
  read(clientfd, &ch, sizeof(int));
  read(clientfd, &bigch, sizeof(int));
  total = ch + bigch;
  /*현재 Locker 이용가능여부 출력*/
  for(int i=1; i<=ch; i++)
  {
    read(clientfd, &using, 1);
    status = using - '0';
    if(status == 1)
    {
      printf("|      Locker ID : %d        |       Used :     Yes      |\n",i);
      printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
    }
    else
    {
      printf("|      Locker ID : %d        |       Used :     No      |\n",i);
      printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
    }
  }

  /*BIG LOCKER 이용가능여부 출력*/
  for(int i=ch+1; i<=total; i++)
  {
    read(clientfd, &using, 1);
    status = using - '0';
    if(status == 1)
    {
      printf("|      Big Locker ID : %d    |       Used :     Yes     |\n",i);
      printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
    }
    else
    {
      printf("|      Big Locker ID : %d    |       Used :     No      |\n",i);
      printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
    }
  }
  
  while(1)
  {
    /*사물함 선택모드 or 사물함 이용모드*/
    printf("사용할 사물한 선택은 1번, 물건 저장은 2번을 누르십시오. ");
    scanf("%d", &f);
    write(clientfd, &f, sizeof(int));

    /*사물함 선택모드*/
    if(f == 1)
    {
      printf("\n사용할 사물함 번호를 입력하세요. (1번 사물함부터 %d번 사물함)\n", total);
      scanf("%d", &wantID);

      write(clientfd, &wantID, sizeof(int));
      read(clientfd, &canUse, 1);
      
      /*사용 가능한지 검사 후 가능하면 비밀번호 설정*/
      if(canUse == 'Y')
      {
        // 비밀번호 설정
        	printf("%d번 사물함의 비밀번호를 설정해주세요 : ", wantID);
					scanf("%s",password);
					
					//비밀번호 확인을위해 재입력 
					while(1)
					{
					printf("설정하실 비밀번호를 다시 입력해주세요 : ");
					scanf("%s",repassword);
					if(!strcmp(password,repassword))
            break;
					}

					//비밀번호를 client에 write() 
					write(clientfd,&password,sizeof(password));

      }
      /*사물함 반납하기*/
      else if(canUse == 'N')
      {
    	  printf("%d번 사물함의 비밀번호를 입력해주세요: ",wantID);
        scanf("%s", password);
        write(clientfd, &password, sizeof(password));
        read(clientfd, &cl, sizeof(int));

        if(cl == 3)
          printf("해당 사물함이 반납되었습니다. \n");
        
        else
          printf("비밀번호가 틀립니다. \n");
      }
    }
    /*물건 넣고빼기 모드*/
    else if(f == 2)
    {
      printf("이용할 사물함의 번호를 누르십시오. ");
      scanf("%d", &useid);
      /*소형사물함일때*/
      if(useid <= ch)
      {
        write(clientfd, &useid, sizeof(int));
        read(clientfd, &nowusing, sizeof(int));
        if(nowusing == 0 ){
        	printf("이 사물함은 소유자가 없습니다.\n ");
        }
        else if(nowusing == 1)
        {
          /*비밀번호 일치시 넣고빼기가능*/
          printf("해당 사물함의 비밀번호를 입력하세요: \n");
          scanf("%s", password);
          write(clientfd, &password, sizeof(password));
          read(clientfd, &canUse, 1);
          if(canUse == 'N')
            printf("비밀번호가 틀립니다. \n");
          /*넣고빼기 기능 구현*/
          else if(canUse == 'Y')
          {
            /*빼고싶으면 음수값 입력*/
            printf("보관할 물건의 크기를 말해주세요: ");
            scanf("%d", &size);
            write(clientfd, &size, sizeof(int));
            read(clientfd, &size, sizeof(int));
            if(size>=0) printf("해당 사물함의 남은 공간은 %d칸 입니다.\n", size);
              else printf("사물함의 공간이 부족합니다. \n");
          }

        }
      }
      /*대형사물함일때*/
      else if(ch<useid && useid<= total)
      {
        write(clientfd, &useid, sizeof(int));
        read(clientfd, &nowusing, 1);
        if(nowusing == 1)
        {
          /*비밀번호 일치시 넣고빼기가능*/
          printf("해당 사물함의 비밀번호를 입력하세요: \n");
          scanf("%s", password);
          write(clientfd, &password, sizeof(password));
          read(clientfd, &canUse, 1);
          if(canUse == 'N')
            printf("비밀번호가 틀립니다. \n");

          /*넣고빼기 기능 구현*/
          else if(canUse == 'Y')
          {
            printf("보관할 물건의 크기를 말해주세요: ");
            scanf("%d", &size);
            write(clientfd, &size, sizeof(int));
            read(clientfd, &size, sizeof(int));
            if(size>=0) printf("해당 사물함의 남은 공간은 %d칸 입니다. \n", size);
                       else printf("사물함의 공간이 부족합니다.\n");
          }
        }
      }
      
    }
  }
  close(clientfd);
  exit(0);
}
