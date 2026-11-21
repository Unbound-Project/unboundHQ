#include "custom_maps.hpp"
#include <cassert>
#include <iostream>
using namespace unbound;
int main(){
    const std::string hash(64,'a');
    const std::string header="# UNBOUND_CUSTOM_MAPS_V1\n";
    const std::string catalog=header+"PACK|custom-island|10|Island|A community map\n"
        "FILE|custom-island|FULL|-|1.0.1|https://example.test/Island-v1.0.1.zip|"+hash+"|1871727718\n"
        "FILE|custom-island|PATCH|1.0.0|1.0.1|https://example.test/Island-patch.zip|"+hash+"|1000\n";
    std::vector<MapPack> maps;std::string error;
    assert(parseCustomMaps(header,maps,error)&&maps.empty());
    assert(parseCustomMaps(catalog,maps,error)&&maps.size()==1);
    assert(mapPackRelease(maps[0],"")->kind==ReleaseKind::Full);
    assert(mapPackRelease(maps[0],"1.0.0")->kind==ReleaseKind::Patch);
    assert(!mapPackRelease(maps[0],"1.0.1"));
    assert(mapPackRelease(maps[0],"1.0.1",true)->kind==ReleaseKind::Full);
    assert(!mapPackRelease(maps[0],"1.0.2",true));
    const auto marker=markerWithPack("1.0.4\nDLC|pack-1|1.0.1|english|USB\n","custom-island","1.0.1","english",Device::USB);
    assert(packVersion(marker,"pack-1","english",Device::USB)=="1.0.1");
    assert(packVersion(marker,"custom-island","english",Device::USB)=="1.0.1");
    for(const auto& bad:{catalog.substr(header.size()),header+"UNIVERSAL|FULL|-|1.0.0|x|x|1\n",header+"PACK|pack-1|10|Official DLC|Wrong namespace\nFILE|pack-1|FULL|-|1.0.0|https://example.test/dlc.zip|"+hash+"|100\n",catalog+"PACK|custom-island|10|Duplicate|Bad\n"})
        assert(!parseCustomMaps(bad,maps,error)&&maps.empty());
    const auto bo2=std::string(config::gameCustomMapsUrl());config::activeGame=Game::Ghosts;
    assert(bo2!=config::gameCustomMapsUrl());config::activeGame=Game::BlackOpsII;
    std::cout<<"Custom maps: empty/full/patch catalogs, namespace isolation, receipt identity, reinstalls, downgrade prevention and game feeds passed\n";
}
