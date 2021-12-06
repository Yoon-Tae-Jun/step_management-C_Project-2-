#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
//#include <atlstr.h> 
#pragma comment(lib, "wsock32.lib")

typedef struct DATA  {
    char date[15];
    int steps;
}DATA;

typedef struct NODE {
    struct NODE* next;
    DATA data;
}NODE;

typedef struct LIST {
    NODE* head;
    NODE* final;
    int length;
}LIST;

int setup(WSADATA* ws, SOCKET* st);
void input_Node(DATA d1, LIST* L);
void Print_Data(NODE* head);
void Free_Node(NODE* head);
void Send_Data(SOCKET* clientSocket, char* Buffer, char* str);
void Recv_Data(SOCKET* clientSocket, char* Buffer);
int Find_Date(SOCKET* clientSocket, NODE* head, char* Buffer, char* Buffer2);
double Find_weekaverage(NODE* head, LIST* list);
double Find_totalaverage(NODE* head, LIST* list);

NODE* Find_Node(NODE* head, int n);
int main() {
    //데이터 관련 변수
    FILE* fp;
    DATA data;
    LIST list;
    NODE* head = malloc(sizeof(struct NODE));
    head->next = NULL;
    list.head = head;
    list.final = head;
    list.length = 1;
    int num = 0;
    NODE* buf;
    NODE* buf2;
    
    //소켓 관련 변수
    SOCKET mySocket, clientSocket; 
    WSADATA WSAdata; //winsock data
    struct sockaddr_in socketin; //socket struct
    struct sockaddr_in client_addr;
    int size = sizeof(client_addr);
    char Buffer[1024] = { '\0',};
    char Buffer2[512] = { '\0', };
    char* ch = '\0';
    int cnt;

    /**********************데이터 open 함수*****************************/
    printf("<--데이터 불러오는 중..-->\n");
    fp = fopen("results_stpes.txt", "r");
    while (EOF != fscanf(fp, "%s %d\n", &data.date, &data.steps))
    {
        //Input_Node(data, head, &list);
        input_Node(data, &list);

    }
    Print_Data(head);
    printf("<--데이터 불러오기 완료-->\n");
    printf("데이터 개수: %d개\n", list.length);
    fclose(fp);
    /**********************소켓 open 함수*****************************/
    if (setup(&WSAdata, &mySocket) == 0) {
        return 0;
    }

    socketin.sin_family = AF_INET; // 소켓 설정
    socketin.sin_port = htons(9999); //포트설정, 빅엔디안으로 변경
    socketin.sin_addr.s_addr = htonl(ADDR_ANY); //IP주소 설정, 리틀 엔디안으로 변경


    if (bind(mySocket, (struct sockaddr*)&socketin, sizeof(socketin)) == SOCKET_ERROR) {
        printf("bind ERROR");
        closesocket(mySocket);
        WSACleanup();
        return 0;
    }

    if (listen(mySocket, SOMAXCONN) != 0) {
        printf("listen ERROR");
        closesocket(mySocket);
        WSACleanup();
        return 0;
    }
    

    /**********************클라이언트 접속 대기*****************************/
    while (1)
    {
        printf("<--클라이언트 대기중..--> \n");
        clientSocket = accept(mySocket, (struct sockaddr*)&client_addr, &size);

        if (clientSocket == INVALID_SOCKET) {
            printf("access accept failed");
            closesocket(mySocket);
            WSACleanup();
            return 0;
        }
        /*클라이언트 접속 성공*/
        printf("<--클라이언트 접속 성공-->l\n");

        printf("<--기본 정보 송신 시작-->\n");
        strcpy(Buffer2, list.final->data.date);
        Send_Data(&clientSocket,Buffer, Buffer2);
        Sleep(100);
        sprintf(Buffer2, "%d", list.final->data.steps);
        Send_Data(&clientSocket,Buffer, Buffer2);
        Sleep(100);
        sprintf(Buffer2, "%.2lf", Find_weekaverage(head, &list));
        Send_Data(&clientSocket, Buffer, Buffer2);
        Sleep(100);
        sprintf(Buffer2, "%.2lf", Find_totalaverage(head, &list));
        Send_Data(&clientSocket, Buffer, Buffer2);
        printf("<--기본 정보 송신 완료-->\n");

        Recv_Data(&clientSocket, Buffer);
        while (strcmp(Buffer, "9") != 0)
        {
            if (strcmp(Buffer, "5") == 0)
            {
                //수정 버튼
                printf("<--수정 버튼 클릭-->\n");
                Recv_Data(&clientSocket, Buffer);
                cnt = Find_Date(&clientSocket, head, Buffer, Buffer2);

                Sleep(100);
                if (cnt != 0)
                {
                    printf("<--수정 가능..-->\n");
                    printf("<--수정 시작-->\n");
                    Recv_Data(&clientSocket, Buffer);
                    num = atoi(Buffer);
                    buf = Find_Node(head, cnt);
                    buf->data.steps = num;
                    printf("%s의 걸음수 수정: %d\n",buf->data.date, buf->data.steps);

                    printf("<--수정 완료-->\n");
                }
                else
                {
                    printf("<--수정 불가-->\n");
                }
            }
            else if (strcmp(Buffer, "6") == 0)
            {
                printf("<--추가 버튼 클릭-->\n");
                Recv_Data(&clientSocket, Buffer);
                if (strcmp(list.final->data.date, Buffer) != 0)
                {   
                    printf("<--추가 가능..-->\n");
                    printf("<--추가 시작-->\n");
                    strcpy(data.date, Buffer);
                    strcpy(Buffer2, "1");
                    Send_Data(&clientSocket, Buffer,Buffer2);
                    Recv_Data(&clientSocket, Buffer);
                    data.steps = atoi(Buffer);
                    input_Node(data, &list);
                    Print_Data(head);
                    printf("<--추가 완료-->\n");
                }
                else
                {
                    printf("<--추가 불가-->\n");
                    strcpy(Buffer2, "0");
                    Send_Data(&clientSocket, Buffer,Buffer2);
                }
               
            }

            Recv_Data(&clientSocket, Buffer);
            
        }
        printf("<--클라이언트 종료-->\n");
        
        printf("<--수정 사항 저장중..-->\n");
        fp = fopen("results_stpes.txt", "w");
        buf2 = head;
        buf = head->next;
        
        while (buf != NULL)
        {
            buf2 = buf;
            buf = buf->next;
            fprintf(fp, "%s %d\n", buf2->data.date, buf2->data.steps);
        }

        fclose(fp);
        printf("<--파일 저장 완료-->\n");
    }
    
   
    /**********************소켓 close 함수*****************************/
    if (closesocket(clientSocket) != 0) {
        printf("Remove socket failed");
        WSACleanup();
        return 0;
    }

    if (closesocket(mySocket) != 0) {
        printf("Remove socket failed");
        WSACleanup();
        return 0;
    }

    if (WSACleanup() != 0) {
        printf("WSACleanup failed");
        return 0;
    }

    Free_Node(head);

    return 0;
}


/**********************함수 정의*****************************/
int setup(WSADATA* ws, SOCKET* st)
{
    if (WSAStartup(WINSOCK_VERSION, ws) != 0) { //winsock version check
        printf("WSAStartup failed, %d\n", WSAGetLastError());
        return 0;
    }

    *st = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //소켓 생성

    if (*st == INVALID_SOCKET) {
        printf("Create socket failed, %d\n", WSAGetLastError()); //소켓 생성 에러시
        WSACleanup(); //finish WS2_32.DLL
        return 0;
    }

    return 1;
}


void Send_Data(SOCKET* clientSocket,char *Buffer, char *str ) {
    memset(Buffer, 0, 1024);
    strcpy(Buffer, str);
    send(*clientSocket, Buffer, strlen(Buffer), 0);
    //("send msg: %s\n", Buffer);
    memset(str, 0, 1024);
}

void Recv_Data(SOCKET* clientSocket, char* Buffer) {
    memset(Buffer, 0, 1024);
    recv(*clientSocket, Buffer, 1024, 0);
    //printf("recv msg: %s\n", Buffer);
}

void input_Node(DATA d1, LIST* L)
{
    DATA inputData = d1;
    NODE* newNode = malloc(sizeof(NODE));
    newNode->next = NULL;
    L->final->next = newNode;
    newNode->data = inputData;
    L->final = newNode;
    L->length += 1;
}

void Free_Node(NODE* head)
{
    NODE* curr = head->next;

    while (curr != NULL)
    {
        NODE* next = curr->next;
        free(curr);
        curr = next;
    }
}


void Print_Data(NODE* head)
{
    //순회용 포인터
    NODE* curr = head->next;

    while (curr != NULL)
    {
        printf("%s %d\n", curr->data.date, curr->data.steps);
        curr = curr->next;
    }

}



double Find_weekaverage(NODE* head, LIST* list)
{
    NODE* ptr = head;
    NODE* curr = head->next;
    double averge = 0;
    
    for(int i =0; i<list->length-1; i++)
    {
        ptr = curr;
        curr = curr->next;
        if (i > (list->length) - 7)
        {
            
            averge += ptr->data.steps;
        }
    }
    return averge/7;
    
}

double Find_totalaverage(NODE* head, LIST* list)
{
    NODE* ptr = head;
    NODE* curr = head->next;
    double averge = 0;

    while (curr != NULL)
    {
        ptr = curr;
        curr = curr->next;
        averge += ptr->data.steps;
    }

    return averge / list->length;

}
int Find_Date(SOCKET* clientSocket, NODE* head,char* Buffer, char* Buffer2) {
    NODE* ptr = head;
    NODE* curr = head->next;
    int check= 0;
    int cnt = 0;
    while (curr != NULL)
    {
        ptr = curr;
        curr = curr->next;
        cnt++;
        if (strcmp(Buffer, ptr->data.date) == 0) {
            strcpy(Buffer2, "1");
            Send_Data(clientSocket, Buffer, Buffer2);
            check = 1;
            return cnt;
        }
    }
    strcpy(Buffer2, "0");
    Send_Data(clientSocket, Buffer, Buffer2);
    return 0;
}


NODE* Find_Node(NODE* head, int n)
{
    NODE* ptr = head;
    NODE* curr = head->next;
    for (int i = 0; i < n; i++)
    {
        ptr = curr;
        curr = curr->next;
    }
    
    return ptr;
   
}