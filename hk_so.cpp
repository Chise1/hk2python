//HKIPcamera.cpp
#include <iostream>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <list>
#include "HCNetSDK.h"
#include <unistd.h>
#include <pthread.h>

#define USECOLOR 1
#define WINAPI

using namespace std;

//--------------------------------------------
int iPicNum = 0; //Set channel NO.
LONG nPort = -1;
HWND hWnd = NULL;
pthread_mutex_t g_cs_frameList;
LONG lUserID;
NET_DVR_DEVICEINFO_V30 struDeviceInfo;
LONG lRealPlayHandle = -1;

void yv12toYUV(char *outYuv, char *inYv12, int width, int height, int widthStep)
{
    int col, row;
    unsigned int Y, U, V;
    int tmp;
    int idx;

    //printf("widthStep=%d.\n",widthStep);

    for (row = 0; row < height; row++)
    {
        idx = row * widthStep;
        int rowptr = row * width;

        for (col = 0; col < width; col++)
        {
            //int colhalf=col>>1;
            tmp = (row / 2) * (width / 2) + (col / 2);
            //         if((row==1)&&( col>=1400 &&col<=1600))
            //         {
            //          printf("col=%d,row=%d,width=%d,tmp=%d.\n",col,row,width,tmp);
            //          printf("row*width+col=%d,width*height+width*height/4+tmp=%d,width*height+tmp=%d.\n",row*width+col,width*height+width*height/4+tmp,width*height+tmp);
            //         }
            Y = (unsigned int)inYv12[row * width + col];
            U = (unsigned int)inYv12[width * height + width * height / 4 + tmp];
            V = (unsigned int)inYv12[width * height + tmp];
            //         if ((col==200))
            //         {
            //         printf("col=%d,row=%d,width=%d,tmp=%d.\n",col,row,width,tmp);
            //         printf("width*height+width*height/4+tmp=%d.\n",width*height+width*height/4+tmp);
            //         return ;
            //         }
            if ((idx + col * 3 + 2) > (1200 * widthStep))
            {
                //printf("row * widthStep=%d,idx+col*3+2=%d.\n",1200 * widthStep,idx+col*3+2);
            }
            outYuv[idx + col * 3] = Y;
            outYuv[idx + col * 3 + 1] = U;
            outYuv[idx + col * 3 + 2] = V;
        }
    }
    //printf("col=%d,row=%d.\n",col,row);
}


bool OpenCamera(char *ip, char *usr, char *password)
{
    lUserID = NET_DVR_Login_V30(ip, 8000, usr, password, &struDeviceInfo);
    if (lUserID == 0)
    {
        cout << "Log in success!" << endl;
        return TRUE;
    }
    else
    {
        printf("Login error, %d\n", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return FALSE;
    }
}
void *ReadCamera(void *IpParameter)
{
    //---------------------------------------
    //设置异常消息回调函数
    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);
    //cvNamedWindow("Mywindow", 0);
    //cvNamedWindow("IPCamera", 0);
    //HWND  h = (HWND)cvGetWindowHandle("Mywindow");
    //h = cvNamedWindow("IPCamera");
    //---------------------------------------
    //启动预览并设置回调数据流
    NET_DVR_CLIENTINFO ClientInfo;
    ClientInfo.lChannel = 1;    //Channel number 设备通道号
    ClientInfo.hPlayWnd = NULL; //窗口为空，设备SDK不解码只取流
    ClientInfo.lLinkMode = 1;   //Main Stream
    ClientInfo.sMultiCastIP = NULL;

    LONG lRealPlayHandle;
    lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, fRealDataCallBack, NULL, TRUE);
    if (lRealPlayHandle < 0)
    {
        printf("NET_DVR_RealPlay_V30 failed! Error number: %d\n", NET_DVR_GetLastError());
        //return -1;
    }
    else
        cout << "码流回调成功！" << endl;
    sleep(-1);
    //fclose(fp);
    //---------------------------------------
    //关闭预览
    if (!NET_DVR_StopRealPlay(lRealPlayHandle))
    {
        printf("NET_DVR_StopRealPlay error! Error number: %d\n", NET_DVR_GetLastError());
        return 0;
    }
    //注销用户
    NET_DVR_Logout(lUserID);
    NET_DVR_Cleanup();
    //return 0;
}

void init(char *ip, char *usr, char *password)
{
    pthread_t hThread;
    cout << "IP:" << ip << "    UserName:" << usr << "    PassWord:" << password << endl;
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);
    OpenCamera(ip, usr, password);
    pthread_mutex_init(&g_cs_frameList, NULL);
    //hThread = ::CreateThread(NULL, 0, ReadCamera, NULL, 0, 0);
    pthread_create(&hThread, NULL, ReadCamera, NULL);
}


void release()
{
    //close(hThread);
    NET_DVR_StopRealPlay(lRealPlayHandle);
    //注销用户
    NET_DVR_Logout(lUserID);
    NET_DVR_Cleanup();
}