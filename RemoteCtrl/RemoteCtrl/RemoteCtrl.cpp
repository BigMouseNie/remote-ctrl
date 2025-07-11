﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "CmdHandler.h"

#include <io.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);
    if (!CCmdHandler::Init()) {
        TRACE("%s : %s\n", __FUNCTION__, "CCmdHandler Init Error!");
        return nRetCode;
    }
    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            int res = CServerSocket::GetInstance()->Run(&CCmdHandler::DealPacket, 8081);
            if (res == -1) {
                TRACE("%s : %s\n", __FUNCTION__, "Run Error!");
            }
            else {
                TRACE("%s : %s\n", __FUNCTION__, "End Run!");
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    CCmdHandler::Release();
    return nRetCode;
}
