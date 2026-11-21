#pragma once
#include "map_packs.hpp"

namespace unbound {
inline bool parseCustomMaps(const std::string& text,std::vector<MapPack>& maps,std::string& error){
    maps.clear();std::istringstream lines(text);std::string line;bool header=false;
    while(std::getline(lines,line)){
        if(line.rfind("\xef\xbb\xbf",0)==0)line.erase(0,3);
        line=trim(line);
        if(line=="# UNBOUND_CUSTOM_MAPS_V1")header=true;
        if(!line.empty()&&line[0]!='#'&&line.rfind("PACK|",0)!=0&&line.rfind("FILE|",0)!=0){
            error="Custom map catalogs accept only PACK and FILE records.";return false;
        }
    }
    if(!header){error="The custom map catalog header is missing.";return false;}
    if(!parseMapPacks(text,maps,error))return false;
    for(const auto& map:maps)if(map.id.rfind("custom-",0)!=0||map.id.size()==7){
        maps.clear();error="Custom map IDs must start with custom- and include a unique name.";return false;
    }
    return true;
}
}
