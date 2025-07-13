#pragma once

#include "Packet.h"

#include <unordered_map>
#include <mutex>

class MapPacket
{
public:
    struct ResourcePacket {
        friend class MapPacket;
    public:
        Packet* inPacket;
        Packet* outPacket;
        ResourcePacket() :
            inPacket(nullptr), outPacket(nullptr), isUsed(nullptr)
        {}
        ~ResourcePacket() = default;

        ResourcePacket& operator=(const ResourcePacket& other) {
            if (this == &other) { return *this; }
            inPacket = other.inPacket;
            outPacket = other.outPacket;
            isUsed = other.isUsed;
            return *this;
        }

    private:
        bool* isUsed;
        void Init() {
            inPacket = new Packet;
            outPacket = new Packet;
            isUsed = new bool(false);
        }
        void Free() {
            delete inPacket; inPacket = nullptr;
            delete outPacket; outPacket = nullptr;
            delete isUsed; isUsed = nullptr;
        }

        void Clear() {
            inPacket->Clear();
            outPacket->Clear();
            *isUsed = false;
        }
    };

    MapPacket();
    ~MapPacket();

    bool AppResourcePacket(UINT flag, ResourcePacket& rp);
    bool GetResourcePacket(UINT flag, ResourcePacket& rp);
    void PutResourcePacket(UINT flag);

private:
    std::unordered_map<UINT, ResourcePacket> resourcePacketPool;
    std::mutex mtx;
};

