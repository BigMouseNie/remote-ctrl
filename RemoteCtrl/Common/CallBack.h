#pragma 

#include "Packet.h"

/**
* @return : 如果处理失败返回-1，成功返回大于等于0的数
*/
using DealPacketCallBack = int(*)(const Packet&, Packet**);
