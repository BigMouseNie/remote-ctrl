# remote-ctrl

# 📡 remote-ctrl

基于 MFC 的远程控制工具，支持文件管理、远程监控、鼠标操作、锁机/解锁功能。

---

# 🧰 项目简介

`remote-ctrl` 是一个使用 C++ 和 MFC 构建的远程主机控制系统，主要功能包括：

- 📁 查看远程主机的文件
- 📥 远程下载文件
- 🗑️ 远程删除文件
- 📂 远程打开文件或文件夹
- 🖥️ 实时远程桌面监控
- 🖱️ 鼠标控制（移动、点击等操作）
- 🔒 锁定远程主机屏幕
- 🔓 解锁远程主机屏幕

---

# ⚙️ 技术栈

- **语言**：C++
- **框架**：MFC（Microsoft Foundation Classes）
- **开发环境**：Visual Studio 2019
- **平台支持**：仅限 Windows 10

---

# 🚀 快速开始

### 1. 环境准备

- Windows 10 系统
- 安装 [Visual Studio 2019](https://visualstudio.microsoft.com/)
- 安装 MFC 工具包

### 2. 编译步骤

1. 使用 Visual Studio 2019 打开 `remote-ctrl/RemoteCtl/RemoteCtrl.sln` 解决方案
2. 分别编译 `RemoteClient` 和 `RemoteCtrl` 项目
3. 在两台 Windows 10 主机（或虚拟机）中部署（都部署在本地也没有问题）：
	- `RemoteCtrl.exe`：运行在被控端
	- `RemoteClient.exe`：运行在控制端

------

## 📌 注意事项

- 所有功能仅在 **Windows 10** 下开发与测试
- 通信协议基于自定义 TCP 套接字实现
- 建议客户端与服务端在同一网段内测试以减少网络阻碍
- 锁机功能在测试阶段，所以只需要被控制端按下'A'键即可解锁（解锁功能也是发送'A'键的按下操作）
- 如需更改锁机和解锁的条件可在RemoteCtrl项目中的`CmdHandler.cpp`中`UNLOCK_MACHINE_handler`以及`ThreadLockMachine`中的消息循环处理

---