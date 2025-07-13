#include "pch.h"
#include "MapPacket.h"



MapPacket::MapPacket()
{}

MapPacket::~MapPacket()
{
    for (auto& pair : resourcePacketPool) {
        pair.second.Free();
    }
}

bool MapPacket::AppResourcePacket(UINT flag, ResourcePacket& rp)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto& it = resourcePacketPool.find(flag);
    if (it == resourcePacketPool.end()) {
        ResourcePacket rp;
        rp.Init();
        auto insertRes = resourcePacketPool.insert({ flag, rp });
        if (!insertRes.second) { return false; }
        it = insertRes.first;
    }
    if (*(it->second.isUsed)) { return false; }

    rp = it->second;
    *rp.isUsed = true;
    return true;
}

bool MapPacket::GetResourcePacket(UINT flag, ResourcePacket& rp)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto& it = resourcePacketPool.find(flag);
    if (it == resourcePacketPool.end()) {
        return false;
    }
    if (!*(it->second.isUsed)) { return false; }

    rp = it->second;
    return true;
}

void MapPacket::PutResourcePacket(UINT flag)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto& it = resourcePacketPool.find(flag);
    if (it == resourcePacketPool.end()) {
        return;
    }
    it->second.Clear();
}
