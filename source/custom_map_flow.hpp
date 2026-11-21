// Included after the shared map installer in main.cpp's application namespace.
std::vector<MapPack> sessionCustomMaps;
std::string sessionCustomMapsNotice;
bool sessionCustomMapsLoaded=false;

void loadCustomMaps(Ui& ui,bool refresh=false){
    if(sessionCustomMapsLoaded&&!refresh)return;
    sessionCustomMapsLoaded=true;sessionCustomMaps.clear();sessionCustomMapsNotice.clear();
    ui.loading("Checking custom maps","Reading community map listings...",-1);
    const auto cache=std::string(config::gameCacheRoot())+"/custom-maps.txt";
    std::string text,error;
    if(downloadText(config::gameCustomMapsUrl(),text,error)&&parseCustomMaps(text,sessionCustomMaps,error)){
        if(!writeTextFileAtomic(cache,text,error))sessionCustomMapsNotice="Catalog loaded; its saved copy could not be updated.";
        return;
    }
    std::string saved;
    if(readInstallState(cache,saved,error)&&!saved.empty()&&parseCustomMaps(saved,sessionCustomMaps,error))
        sessionCustomMapsNotice="Using the saved custom map catalog.";
    else{sessionCustomMaps.clear();sessionCustomMapsNotice="Custom map catalog unavailable. Try Refresh.";}
}

void refreshCustomMapVersions(const GameInstall& install,std::vector<std::string>& versions){
    versions.assign(sessionCustomMaps.size(),{});
    if(!mapPackTargetReady(install))return;
    std::string marker,error;
    if(!readInstallState(install.update.root+"/"+config::VersionRelativePath,marker,error)){
        appendWarning(sessionCustomMapsNotice,"Installed map records could not be read.");return;
    }
    for(std::size_t i=0;i<sessionCustomMaps.size();++i){
        error.clear();versions[i]=installedMapVersion(install,marker,sessionCustomMaps[i].id,error);
        if(!error.empty())appendWarning(sessionCustomMapsNotice,"An installed map receipt could not be read.");
    }
}
