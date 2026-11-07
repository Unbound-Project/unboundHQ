// Desktop-only layout verification. This compiles the real UI/model sources,
// uses the packaged fonts/artwork, and captures their SDL software output.
// It is not part of the Wii U app or its downloads/install path.
//
// Native build (SDL2, SDL2_image and SDL2_ttf development packages required):
// c++ -std=c++20 -O2 -Wall -Wextra -Werror -Iinclude -Isource (continued below)
//   tools/render_install_menu.cpp source/ui.cpp source/model.cpp (continued)
//   $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf) -o /tmp/render-install
// Run from a temporary directory with fs:/vol/content symlinked to the actual
// content directory, then pass the absolute screenshot output directory.

#include "ui.hpp"
#include "input.hpp"
#include "config.hpp"
#include <SDL2/SDL_image.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace unbound;

GameInstall fixtureInstall(const GameSku* sku, Device updateDevice,
                           Device gameDevice, const std::string& language) {
    GameInstall install;
    install.sku = sku;
    install.consoleRegion = Region::USA;
    install.base = {gameDevice, {}, 0, true};
    install.update.device = updateDevice;
    install.update.present = true;
    install.update.titleVersion = requiredOfficialUpdateVersion(sku);
    install.update.root = updateDevice == Device::USB ? "storage_usb01:/" : "storage_mlc01:/";
    const std::string titleKey = installTargetKey(sku->updateTitleId, updateDevice);
    install.update.root += "usr/title/0005000e/" + titleKey.substr(titleKey.size() - 8);
    install.languageFolder = language;
    install.modVersion = "0.0.05P2.1";
    return install;
}

bool screenshot(const std::filesystem::path& path) {
    SDL_Window* window = SDL_GetWindowFromID(1);
    SDL_Renderer* renderer = window ? SDL_GetRenderer(window) : nullptr;
    int width = 0, height = 0;
    if (!renderer || SDL_GetRendererOutputSize(renderer, &width, &height) != 0) {
        std::cerr << "Cannot locate UI renderer: " << SDL_GetError() << '\n';
        return false;
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
                                                         SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        std::cerr << "Cannot allocate capture: " << SDL_GetError() << '\n';
        return false;
    }
    const bool ok = SDL_RenderReadPixels(renderer, nullptr, surface->format->format,
                                         surface->pixels, surface->pitch) == 0 &&
                    IMG_SavePNG(surface, path.string().c_str()) == 0;
    if (!ok) std::cerr << "Capture failed: " << SDL_GetError() << ' ' << IMG_GetError() << '\n';
    SDL_FreeSurface(surface);
    if (ok) std::cout << path.string() << '\n';
    return ok;
}
}

int main(int argc,char** argv){
 if(argc!=2)return 2;
 std::filesystem::create_directories(argv[1]);
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");
 unbound::Ui ui;std::string error;if(!ui.initialize(error)){std::cerr<<error;return 1;}
 auto active=fixtureInstall(&unbound::SupportedSkus[0],unbound::Device::USB,unbound::Device::Disc,"english");
 active.modVersion="1.0.0";
 const std::vector<unbound::MapPack> releasePacks{
     {"pack-1","Unbound DLC Pack 1","Adds Downhill, Hydro, Mirage, and Grind for Multiplayer, plus Die Rise and Nuketown Zombies.",10,
      {"UNIVERSAL",unbound::ReleaseKind::Full,"-","1.0.0","","",1366100485},{}}
 };
 ui.setNavigation(1,false);
 ui.mapPacks(releasePacks,{true},{""},0,active,"Unchecking a pack keeps its files and pauses future updates.");
 if(!screenshot(std::filesystem::path(argv[1])/"dlc-map-pack-picker.png"))return 1;
 const auto nand=fixtureInstall(&unbound::SupportedSkus[0],unbound::Device::MLC,unbound::Device::Disc,"english");
 ui.mapPacks(releasePacks,{true},{""},0,nand,"Unchecking a pack keeps its files and pauses future updates.");
 if(!screenshot(std::filesystem::path(argv[1])/"dlc-map-pack-picker-nand.png"))return 1;
 std::vector<unbound::MapPack> packs(5);
 for(int i=0;i<5;++i){packs[i].id="pack-"+std::to_string(i+1);packs[i].name="Unbound DLC Pack "+std::to_string(i+1);packs[i].description="Optional maps for Multiplayer and Zombies. Keep this pack up to date independently of Core.";packs[i].full.version="1.0.0";packs[i].full.compressedBytes=620*1024*1024;}
 ui.setNavigation(1,false);
 ui.mapPacks(packs,{true,false,false,false,false},{"","","","",""},0,active,"Select the maps you want. Installed files stay when unchecked.");
 if(!screenshot(std::filesystem::path(argv[1])/"choose-packs.png"))return 1;
 ui.touchAction({true,false,0,0});ui.touchAction({true,true,400,340});auto tap=ui.touchAction({true,false,400,340});
 if(!tap||tap->kind!=unbound::TouchActionKind::MapPackRow||tap->value!=0)return 3;
 ui.touchAction({true,true,900,520});tap=ui.touchAction({true,false,900,520});
 if(!tap||tap->kind!=unbound::TouchActionKind::MapPackDetails||tap->value!=0)return 6;
 ui.touchAction({true,true,400,625});tap=ui.touchAction({true,false,400,625});
 if(!tap||tap->kind!=unbound::TouchActionKind::MapPackRow||tap->value!=5)return 7;
 ui.touchAction({true,true,900,625});tap=ui.touchAction({true,false,900,625});
 if(!tap||tap->kind!=unbound::TouchActionKind::Button||tap->value!=int(unbound::button::X))return 4;

 ui.mapPacks(packs,{true,false,false,false,true},{"1.0.0","","","",""},4,active,"SD packages are verified before installation.");
 if(!screenshot(std::filesystem::path(argv[1])/"choose-packs-scrolled.png"))return 1;
 ui.mapPacks({}, {}, {},0,active,"Map catalog offline. Core can still be installed.");
 if(!screenshot(std::filesystem::path(argv[1])/"catalog-offline.png"))return 1;
 ui.contentQueue({{"Unbound Core 1.0.0","180 MiB / USB storage"},{"Unbound DLC Pack 1 1.0.0","620 MiB / USB storage"}},active,"800 MiB",0);
 if(!screenshot(std::filesystem::path(argv[1])/"review-install.png"))return 1;
 ui.touchAction({true,false,0,0});ui.touchAction({true,true,400,615});tap=ui.touchAction({true,false,400,615});
 if(!tap||tap->kind!=unbound::TouchActionKind::Button||tap->value!=int(unbound::button::A))return 5;
 std::cout<<"Actual UI map selection, reinstall and start touch targets passed\n";

 for(const auto game : {unbound::Game::BlackOpsII,unbound::Game::Ghosts}){
     unbound::config::activeGame=game;
     const auto& sku=unbound::gameSkus(game).front();
     const auto gameInstall=fixtureInstall(&sku,unbound::Device::USB,unbound::Device::Disc,"english");
     const bool ghosts=game==unbound::Game::Ghosts;
     ui.setNavigation(0,false);
     ui.home(gameInstall,nullptr,"",0,"",false,false);
     if(!screenshot(std::filesystem::path(argv[1])/(ghosts?"hq-ghosts.png":"hq-bo2.png")))return 9;
     ui.touchAction({true,false,0,0});
     const int otherGameX=ghosts?660:830;
     ui.touchAction({true,true,otherGameX,51});
     const auto gameTap=ui.touchAction({true,false,otherGameX,51});
     if(!gameTap||gameTap->kind!=unbound::TouchActionKind::GameChoice||gameTap->value!=(ghosts?0:1))return 10;
 }
 std::cout<<"Both game dashboard switch targets passed\n";
 ui.loading("Downloading Ghosts Core","Receiving package",42,false);
 if(!screenshot(std::filesystem::path(argv[1])/"ghosts-download.png"))return 11;
 ui.modal("Install complete","Your Ghosts update is ready.",false,"OK");
 if(!screenshot(std::filesystem::path(argv[1])/"ghosts-dialog.png"))return 12;
 ui.shutdown();return 0;
}
