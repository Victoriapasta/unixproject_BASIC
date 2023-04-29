#define NUM 10
#define START_ID 1

struct locker
{
    int id; //사물함 번호
    int used; //사물함 사용 여부
    char yesorno[NUM]; //사물함 이용여부 Yes or No
    char PW[NUM]; //비밀번호
    int space; //사물함공간
    int bigspace; //큰 사물함공간
};
