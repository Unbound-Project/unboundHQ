#include "ui.hpp"
#include "config.hpp"
#include "input.hpp"
#include "plugin_policy.hpp"
#include "install_feedback.hpp"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <sstream>

namespace unbound {
namespace {
constexpr int W=1280,H=720;
constexpr SDL_Color White{255,255,255,255},Ink{25,30,34,255},Muted{151,161,167,255};
constexpr SDL_Color Blue{0,159,207,255},Pale{238,244,247,255};
SDL_Color accentColor(){return config::ghostsMode()?SDL_Color{167,190,203,255}:SDL_Color{255,92,0,255};}
SDL_Color darkColor(){return config::ghostsMode()?SDL_Color{16,22,28,255}:SDL_Color{10,12,14,255};}
SDL_Color panelColor(){return config::ghostsMode()?SDL_Color{25,33,42,248}:SDL_Color{18,21,23,248};}
SDL_Color raisedColor(){return config::ghostsMode()?SDL_Color{33,44,53,255}:SDL_Color{27,31,34,255};}
constexpr SDL_Color Green{46,174,119,255},Red{218,67,72,255};
constexpr SDL_Color Steam{12,15,17,255},SteamCard{27,31,34,255};
constexpr SDL_Color PanelBorder{77,84,89,255};
constexpr SDL_Color LightText{205,211,215,255};
constexpr int NewsClipTop=102,NewsClipBottom=658;
// Retain the originally centered clip; the optional left/right clips are no
// longer loaded or drawn. Keeping the generated index preserves its timing.
constexpr std::size_t DownloadClip=1;
bool newsVisible(int y,int height){return y<NewsClipBottom&&y+height>NewsClipTop;}
const std::vector<std::vector<std::string>> KeyboardRows={
    {"Q","W","E","R","T","Y","U","I","O","P"},
    {"A","S","D","F","G","H","J","K","L"},
    {"Z","X","C","V","B","N","M"},
    {"SPACE","BACKSPACE",".",",","!","?","POST","CANCEL"}
};
int rating(const WorkshopMod& mod){const unsigned total=mod.upvotes+mod.downvotes;return total?int((mod.upvotes*5+total/2)/total):0;}
std::string stars(const WorkshopMod& mod){std::string result;const int score=rating(mod);for(int i=0;i<5;++i)result+=i<score?"*":"-";return result;}
bool loadFontData(const char* path,std::vector<unsigned char>& data){
    constexpr Sint64 MaximumFontBytes=16*1024*1024;
    SDL_RWops* source=SDL_RWFromFile(path,"rb");if(!source)return false;
    const Sint64 length=SDL_RWsize(source);bool ok=length>0&&length<=MaximumFontBytes;
    if(ok){data.resize(static_cast<std::size_t>(length));ok=SDL_RWread(source,data.data(),1,data.size())==data.size();}
    SDL_RWclose(source);if(!ok)data.clear();return ok;
}
TTF_Font* openMemoryFont(const std::vector<unsigned char>& data,int points){
    SDL_RWops* source=SDL_RWFromConstMem(data.data(),static_cast<int>(data.size()));
    if(!source)return nullptr;
    TTF_Font* font=TTF_OpenFontRW(source,1,points);return font;
}
// Icons use a consistent 32-unit design grid. Supersample alpha once, then
// linearly sample the alpha silhouettes. Unlike integer SDL line primitives,
// the stroke weight and diagonals remain consistent at GamePad scale.
bool iconPolygon(double x,double y,std::initializer_list<std::pair<double,double>> vertices){
    bool inside=false;auto previous=*(vertices.end()-1);
    for(const auto& point:vertices){
        if((point.second>y)!=(previous.second>y)&&
           x<(previous.first-point.first)*(y-point.second)/(previous.second-point.second)+point.first)
            inside=!inside;
        previous=point;
    }
    return inside;
}
bool iconBox(double x,double y,double left,double top,double right,double bottom){
    return x>=left&&x<=right&&y>=top&&y<=bottom;
}
bool iconCircle(double x,double y,double cx,double cy,double radius){
    return (x-cx)*(x-cx)+(y-cy)*(y-cy)<=radius*radius;
}
bool iconShape(int icon,double x,double y){
    if(icon==10){
        const double dx=x-16,dy=y-16;
        const bool disk=dx*dx+dy*dy<=169;
        const double ellipse=dx*dx/49+dy*dy/169;
        return disk&&(dx*dx+dy*dy>=121||std::abs(dy)<1.1||std::abs(ellipse-1)<.17);
    }
    if(icon==9)return iconPolygon(x,y,{{7,3},{29,16},{7,29}});
    if(icon>=5){
        double ax=x,ay=y;
        if(icon==6)ay=32-y;
        else if(icon==7){ax=32-y;ay=x;}
        else if(icon==8){ax=y;ay=32-x;}
        return iconBox(ax,ay,14,10,18,29)||
               iconPolygon(ax,ay,{{3,14},{16,2},{29,14},{25,18},{16,10},{7,18}});
    }
    if(icon==0){
        const bool roof=iconPolygon(x,y,{{2,15},{16,3},{30,15},{27,18},{16,8},{5,18}});
        const bool house=iconBox(x,y,7,13,25,29)&&!iconBox(x,y,13,21,19,30);
        return roof||house;
    }
    if(icon==1){
        const bool arrow=iconBox(x,y,14,3,18,18)||
            iconPolygon(x,y,{{6,14},{10,11},{16,18},{22,11},{26,14},{16,25}});
        const bool tray=iconBox(x,y,4,25,28,29)||iconBox(x,y,4,21,8,29)||iconBox(x,y,24,21,28,29);
        return arrow||tray;
    }
    if(icon==2){
        return iconBox(x,y,4,4,14,14)||iconBox(x,y,18,4,28,14)||
               iconBox(x,y,4,18,14,28)||iconBox(x,y,18,18,28,28);
    }
    if(icon==3){
        return iconPolygon(x,y,{{3,7},{13,7},{17,11},{29,11},{29,28},{3,28}});
    }
    const bool disk=iconCircle(x,y,16,16,13);
    const bool question=(iconCircle(x,y,16,12,5)&&!iconCircle(x,y,16,12,2.5)&&y<14)||
        iconPolygon(x,y,{{18,11},{21,13},{18,17},{17.5,20},{14,20},{14,16},{17,13}})||
        iconCircle(x,y,16,24,1.7);
    return disk&&!question;
}
SDL_Texture* createNavigationIcon(SDL_Renderer* renderer,int icon){
    // Official Lucide geometry is rasterized offline at 128 px, then tinted
    // and sampled proportionally. See content/licenses/Lucide-LICENSE.txt.
    constexpr std::array<const char*,10> names{{"house","download","layout-grid","folder",
        "circle-question-mark","arrow-up","arrow-down","arrow-left","arrow-right","play"}};
    if(icon>=0&&static_cast<std::size_t>(icon)<names.size()){
        const std::string path=std::string("fs:/vol/content/assets/icons/")+names[icon]+".png";
        if(SDL_Texture* texture=IMG_LoadTexture(renderer,path.c_str())){
            SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
            return texture;
        }
    }
    // An optional artwork problem must not prevent navigation or startup.
    constexpr int size=64,samples=4;
    SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,size,size,32,SDL_PIXELFORMAT_RGBA32);
    if(!surface)return nullptr;
    if(SDL_LockSurface(surface)!=0){SDL_FreeSurface(surface);return nullptr;}
    for(int y=0;y<size;++y){
        auto* row=reinterpret_cast<Uint32*>(static_cast<unsigned char*>(surface->pixels)+y*surface->pitch);
        for(int x=0;x<size;++x){
            unsigned covered=0;
            for(int sy=0;sy<samples;++sy)for(int sx=0;sx<samples;++sx)
                covered+=iconShape(icon,(x+(sx+0.5)/samples)/2.0,(y+(sy+0.5)/samples)/2.0)?1U:0U;
            row[x]=SDL_MapRGBA(surface->format,255,255,255,static_cast<Uint8>(covered*255/(samples*samples)));
        }
    }
    SDL_UnlockSurface(surface);
    SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,surface);SDL_FreeSurface(surface);
    if(texture)SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
    return texture;
}
}

bool Ui::initialize(std::string& error){
    if(SDL_Init(SDL_INIT_VIDEO)!=0){error=SDL_GetError();return false;}
    if((IMG_Init(IMG_INIT_PNG)&IMG_INIT_PNG)==0){error=IMG_GetError();return false;}
    if(TTF_Init()!=0){error=TTF_GetError();return false;}
    window_=SDL_CreateWindow(config::AppName,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
    if(!window_){error=SDL_GetError();return false;}
    renderer_=SDL_CreateRenderer(window_,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer_)renderer_=SDL_CreateRenderer(window_,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer_){error=SDL_GetError();return false;}
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
    SDL_RenderSetLogicalSize(renderer_,W,H);SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);
    touchTargets_.reserve(48);pendingTouchTargets_.reserve(48);
    const char* regular="fs:/vol/content/fonts/NotoSans-Regular.ttf";const char* bold="fs:/vol/content/fonts/NotoSans-Bold.ttf";
    if(!loadFontData(regular,regularFontData_)||!loadFontData(bold,boldFontData_)){error=SDL_GetError();return false;}
    regular18_=openMemoryFont(regularFontData_,18);regular22_=openMemoryFont(regularFontData_,22);bold20_=openMemoryFont(boldFontData_,20);bold28_=openMemoryFont(boldFontData_,28);bold38_=openMemoryFont(boldFontData_,38);
    if(!regular18_||!regular22_||!bold20_||!bold28_||!bold38_){error=TTF_GetError();return false;}
    boot_=IMG_LoadTexture(renderer_,"fs:/vol/content/assets/Boot_Screen.png");logo_=IMG_LoadTexture(renderer_,"fs:/vol/content/assets/unbound_short.png");squareLogo_=IMG_LoadTexture(renderer_,"fs:/vol/content/assets/fixed_logo.png");
    if(!boot_||!logo_||!squareLogo_){error=IMG_GetError();return false;}
    for(std::size_t i=0;i<navigationIcons_.size();++i){
        navigationIcons_[i]=createNavigationIcon(renderer_,static_cast<int>(i));
        if(!navigationIcons_[i]){error=SDL_GetError();return false;}
    }
    if(SDL_Texture* artwork=IMG_LoadTexture(renderer_,"fs:/vol/content/assets/gamepad_mic_redirect.png")){
        SDL_SetTextureBlendMode(artwork,SDL_BLENDMODE_BLEND);
        pluginArtwork_["GAMEPAD_MIC_REDIRECT"]=artwork;
    }
    if(SDL_Texture* artwork=IMG_LoadTexture(renderer_,"fs:/vol/content/assets/protarium_network.png")){
        SDL_SetTextureBlendMode(artwork,SDL_BLENDMODE_BLEND);
        pluginArtwork_["PROTARIUM"]=artwork;
    }
    // Audio is optional and memory-backed. It keeps playing during network and
    // install workers without any music-file reads or UI-frame polling.
    backgroundMusic_.start("fs:/vol/content/audio/hq_theme.wav");
    loadDownloadAnimations();
    return true;
}
void Ui::shutdown(){
    networkGlobe_.clear();
    backgroundMusic_.stop();
    downloadAnimationActive_=false;
    for(auto& texture:downloadAtlases_){if(texture)SDL_DestroyTexture(texture);texture=nullptr;}
    downloadClipsReady_.fill(false);
    if(downloadScreenCache_)SDL_DestroyTexture(downloadScreenCache_);
    downloadScreenCache_=nullptr;
    newsHero_=nullptr;
    for(auto& texture:navigationIcons_){if(texture)SDL_DestroyTexture(texture);texture=nullptr;}
    for(auto& document:flowDocuments_)for(auto& chunk:document.second.chunks)if(chunk.text.texture)SDL_DestroyTexture(chunk.text.texture);
    flowDocuments_.clear();
    for(auto& item:textCache_)if(item.second.texture)SDL_DestroyTexture(item.second.texture);
    textCache_.clear();textMeasureCache_.clear();
    for(auto& item:newsTextCache_)if(item.second.texture)SDL_DestroyTexture(item.second.texture);
    newsTextCache_.clear();newsBlocks_.clear();
    for(auto& item:supportBlocks_)if(item.text.texture)SDL_DestroyTexture(item.text.texture);
    supportBlocks_.clear();preparedSupportSource_.clear();supportMaximumScroll_=0;
    for(auto& item:newsImages_)if(item.second)SDL_DestroyTexture(item.second);
    newsImages_.clear();
    for(auto& item:pluginArtwork_)if(item.second)SDL_DestroyTexture(item.second);
    pluginArtwork_.clear();
    for(auto* t:{boot_,logo_,squareLogo_})if(t)SDL_DestroyTexture(t);
    for(auto* f:{regular18_,regular22_,bold20_,bold28_,bold38_})if(f)TTF_CloseFont(f);
    boot_=logo_=squareLogo_=nullptr;regular18_=regular22_=bold20_=bold28_=bold38_=nullptr;
    regularFontData_.clear();boldFontData_.clear();
    if(renderer_)SDL_DestroyRenderer(renderer_);
    if(window_)SDL_DestroyWindow(window_);
    renderer_=nullptr;window_=nullptr;TTF_Quit();IMG_Quit();SDL_Quit();
}
void Ui::setNewsImages(const std::vector<NewsImage>& images){
    newsHero_=nullptr;
    for(auto& item:newsTextCache_)if(item.second.texture)SDL_DestroyTexture(item.second.texture);
    newsTextCache_.clear();newsBlocks_.clear();preparedNewsSource_.clear();
    for(auto& item:newsImages_)if(item.second)SDL_DestroyTexture(item.second);
    newsImages_.clear();
    constexpr int maxWidth=940,maxHeight=310;
    for(const auto& image:images){
        if(image.cachePath.empty())continue;
        const std::string scaledPath=image.cachePath+".bmp";SDL_Surface* source=nullptr;SDL_Surface* prepared=SDL_LoadBMP(scaledPath.c_str());
        if(prepared&&(prepared->w<=0||prepared->h<=0||prepared->w>maxWidth||prepared->h>maxHeight)){SDL_FreeSurface(prepared);prepared=nullptr;}
        if(!prepared){
            source=IMG_Load(image.cachePath.c_str());if(!source)continue;
            const double scale=std::min({1.0,double(maxWidth)/std::max(1,source->w),double(maxHeight)/std::max(1,source->h)});const int width=std::max(1,int(source->w*scale)),height=std::max(1,int(source->h*scale));prepared=source;
            if(width!=source->w||height!=source->h){prepared=SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32);if(prepared){SDL_FillRect(prepared,nullptr,SDL_MapRGBA(prepared->format,0,0,0,0));SDL_Rect destination{0,0,width,height};if(SDL_BlitScaled(source,nullptr,prepared,&destination)!=0){SDL_FreeSurface(prepared);prepared=nullptr;}}}
            if(prepared)SDL_SaveBMP(prepared,scaledPath.c_str());
        }
        if(prepared){if(SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer_,prepared)){SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);newsImages_[image.url]=texture;if(!newsHero_)newsHero_=texture;}}
        if(prepared&&prepared!=source)SDL_FreeSurface(prepared);
        if(source)SDL_FreeSurface(source);
    }
}
void Ui::pump(){
    // A screen without explicit touch registration publishes an inert map,
    // never the previous screen's controls.
    pendingTouchTargets_.clear();pendingTouchIdentity_="inactive";suppressedTouchButtons_=0;
    SDL_Event event{};while(SDL_PollEvent(&event)){};
}
void Ui::circle(int x,int y,int radius,SDL_Color color){SDL_SetRenderDrawColor(renderer_,color.r,color.g,color.b,color.a);for(int dy=-radius;dy<=radius;++dy){const int dx=int(std::sqrt(double(radius*radius-dy*dy)));SDL_RenderDrawLine(renderer_,x-dx,y+dy,x+dx,y+dy);}}
void Ui::present(){
    if(pendingTouchIdentity_!=touchIdentity_||pendingTouchTargets_!=touchTargets_){
        touchIdentity_=pendingTouchIdentity_;touchTargets_=pendingTouchTargets_;++touchRevision_;
        // A page can change during a synchronous rescan/install that does not
        // poll touch. Require a release even if that held press was never seen.
        tapGesture_.reset();
    }
    SDL_RenderPresent(renderer_);
    if(textCache_.size()>256){for(auto& item:textCache_)if(item.second.texture)SDL_DestroyTexture(item.second.texture);textCache_.clear();}
    if(textMeasureCache_.size()>256)textMeasureCache_.clear();
}
void Ui::beginTouch(const std::string& identity){
    if(identity!="loading-cancel")downloadAnimationActive_=false;
    pendingTouchIdentity_=identity;pendingTouchTargets_.clear();suppressedTouchButtons_=0;
}
void Ui::touchTarget(SDL_Rect rect,TouchActionKind kind,int value,const SDL_Rect* clip){
    if(clip){SDL_Rect intersection{};if(!SDL_IntersectRect(&rect,clip,&intersection))return;rect=intersection;}
    if(rect.w<=0||rect.h<=0)return;
    pendingTouchTargets_.push_back({{rect.x,rect.y,rect.w,rect.h},kind,value});
}
std::optional<TouchAction> Ui::touchAction(const TouchSample& sample){
    return tapGesture_.update(sample,touchTargets_,touchRevision_);
}
void Ui::invalidateTouch(){
    downloadAnimationActive_=false;
    tapGesture_.reset();touchTargets_.clear();pendingTouchTargets_.clear();
    touchIdentity_.clear();pendingTouchIdentity_.clear();++touchRevision_;
}
void Ui::cancelTouchGesture(){tapGesture_.reset();}
void Ui::roundRect(SDL_Rect r,int radius,SDL_Color color){
    if(r.w<=0||r.h<=0)return;
    radius=std::min({radius,r.w/2,r.h/2,64});SDL_SetRenderDrawColor(renderer_,color.r,color.g,color.b,color.a);
    if(radius<=0){SDL_RenderFillRect(renderer_,&r);return;}
    std::array<SDL_Rect,129> spans{};int count=0;spans[count++]={r.x,r.y+radius,r.w,r.h-radius*2};
    for(int row=0;row<radius;++row){const int topDy=row-radius;const int topDx=int(std::sqrt(double(radius*radius-topDy*topDy)));spans[count++]={r.x+radius-topDx,r.y+row,r.w-radius*2+topDx*2,1};const int bottomDy=row+1;const int bottomDx=int(std::sqrt(double(radius*radius-bottomDy*bottomDy)));spans[count++]={r.x+radius-bottomDx,r.y+r.h-radius+row,r.w-radius*2+bottomDx*2,1};}
    SDL_RenderFillRects(renderer_,spans.data(),count);
}
void Ui::outline(SDL_Rect r,int radius,SDL_Color color,int thickness){roundRect(r,radius,color);SDL_Rect inner{r.x+thickness,r.y+thickness,r.w-thickness*2,r.h-thickness*2};roundRect(inner,std::max(0,radius-thickness),White);}
void Ui::text(TTF_Font* font,const std::string& value,int x,int y,SDL_Color color,int maxWidth,bool centered){if(value.empty())return;const std::string key="T|"+std::to_string(reinterpret_cast<std::uintptr_t>(font))+"|"+std::to_string(maxWidth)+"|"+std::to_string(color.r)+","+std::to_string(color.g)+","+std::to_string(color.b)+"|"+value;auto found=textCache_.find(key);if(found==textCache_.end()){std::string shown=value;int width=0,height=0;TTF_SizeUTF8(font,shown.c_str(),&width,&height);while(maxWidth>0&&width>maxWidth&&shown.size()>4){shown.pop_back();TTF_SizeUTF8(font,(shown+"...").c_str(),&width,&height);}if(shown!=value)shown+="...";SDL_Surface* surface=TTF_RenderUTF8_Blended(font,shown.c_str(),color);if(!surface)return;SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer_,surface);CachedText cached{texture,surface->w,surface->h};SDL_FreeSurface(surface);if(!texture)return;SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);found=textCache_.emplace(key,cached).first;}const auto& cached=found->second;SDL_Rect target{x-(centered?cached.width/2:0),y,cached.width,cached.height};SDL_RenderCopy(renderer_,cached.texture,nullptr,&target);}
const Ui::CachedText* Ui::wrappedText(TTF_Font* font,const std::string& value,int width,SDL_Color color){
    if(value.empty())return nullptr;
    const std::string key="W|"+std::to_string(reinterpret_cast<std::uintptr_t>(font))+"|"+
        std::to_string(width)+"|"+std::to_string(color.r)+","+std::to_string(color.g)+","+
        std::to_string(color.b)+"|"+value;
    auto found=textCache_.find(key);
    if(found==textCache_.end()){
        SDL_Surface* surface=TTF_RenderUTF8_Blended_Wrapped(font,value.c_str(),color,width);
        if(!surface)return nullptr;
        SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer_,surface);
        const CachedText cached{texture,surface->w,surface->h};SDL_FreeSurface(surface);
        if(!texture)return nullptr;
        SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
        found=textCache_.emplace(key,cached).first;
    }
    return &found->second;
}
void Ui::wrapped(TTF_Font* font,const std::string& value,SDL_Rect box,SDL_Color color){
    if(const CachedText* cached=wrappedText(font,value,box.w,color)){
        SDL_Rect target{box.x,box.y,std::min(box.w,cached->width),std::min(box.h,cached->height)},
                 source{0,0,target.w,target.h};
        SDL_RenderCopy(renderer_,cached->texture,&source,&target);
    }
}
int Ui::wrappedScrollable(TTF_Font* font,const std::string& value,SDL_Rect box,
                          SDL_Color color,int scrollOffset){
    const int maximum=std::max(0,flowDocument(font,value,box.w,color).height-box.h);
    const int offset=std::clamp(scrollOffset,0,maximum);
    SDL_Rect previous{};SDL_RenderGetClipRect(renderer_,&previous);
    const bool previousEnabled=SDL_RenderIsClipEnabled(renderer_);
    SDL_RenderSetClipRect(renderer_,&box);
    flowText(font,value,box.x,box.y-offset,box.w,color);
    SDL_RenderSetClipRect(renderer_,previousEnabled?&previous:nullptr);
    return maximum;
}
std::pair<int,int> Ui::measure(TTF_Font* font,const std::string& value){const std::string key=std::to_string(reinterpret_cast<std::uintptr_t>(font))+"|"+value;const auto found=textMeasureCache_.find(key);if(found!=textMeasureCache_.end())return found->second;int width=0,height=0;TTF_SizeUTF8(font,value.c_str(),&width,&height);return textMeasureCache_.emplace(key,std::make_pair(width,height)).first->second;}
Ui::FlowDocument& Ui::flowDocument(TTF_Font* font,const std::string& value,int width,
                                   SDL_Color color){
    const std::string key=std::to_string(reinterpret_cast<std::uintptr_t>(font))+"|"+
        std::to_string(width)+"|"+std::to_string(color.r)+","+std::to_string(color.g)+","+
        std::to_string(color.b)+"|"+value;
    const auto found=flowDocuments_.find(key);if(found!=flowDocuments_.end())return found->second;
    // Layout is cached, and raster chunks are bounded. Very long descriptions
    // must not become a single texture larger than the Wii U renderer supports.
    if(flowDocuments_.size()>=24){
        const auto old=flowDocuments_.begin();
        for(auto& chunk:old->second.chunks)if(chunk.text.texture)SDL_DestroyTexture(chunk.text.texture);
        flowDocuments_.erase(old);
    }
    FlowDocument document;std::string chunk;unsigned count=0;
    const auto flush=[&](){
        if(chunk.empty())return;
        if(chunk.back()=='\n')chunk.pop_back();
        SDL_Surface* surface=TTF_RenderUTF8_Blended_Wrapped(font,chunk.c_str(),color,width);
        if(surface){
            document.chunks.push_back({chunk,{nullptr,surface->w,surface->h},document.height});
            document.height+=surface->h+std::max(0,TTF_FontLineSkip(font)-TTF_FontHeight(font));
            SDL_FreeSurface(surface);
        }
        chunk.clear();count=0;
    };
    const auto append=[&](const std::string& line){
        if(count>=12||chunk.size()+line.size()>1536)flush();
        chunk+=line;chunk.push_back('\n');++count;
    };
    std::istringstream lines(value);std::string line;
    while(std::getline(lines,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()){append(" ");continue;}
        std::size_t start=0;
        while(start<line.size()){
            std::vector<std::size_t> boundaries;
            const std::size_t limit=std::min(line.size(),start+1536);
            for(std::size_t end=start+1;end<=limit;++end)
                if(end==line.size()||(static_cast<unsigned char>(line[end])&0xc0U)!=0x80U)
                    boundaries.push_back(end);
            if(boundaries.empty())boundaries.push_back(limit);
            std::size_t low=0,high=boundaries.size();
            while(low<high){
                const auto middle=low+(high-low)/2;int pixels=0,height=0;
                const auto prefix=line.substr(start,boundaries[middle]-start);
                TTF_SizeUTF8(font,prefix.c_str(),&pixels,&height);
                // SDL_ttf's wrapped glyph advances can exceed SizeUTF8's
                // bounding width slightly. Leave a small inset so an already
                // wrapped final word is not wrapped again onto an orphan line.
                if(pixels<=std::max(1,width-16))low=middle+1;else high=middle;
            }
            std::size_t end=boundaries[low?low-1:0];
            if(end<line.size()){
                const auto space=line.find_last_of(" \t",end-1);
                if(space!=std::string::npos&&space>start)end=space;
            }
            append(line.substr(start,end-start));start=end;
            while(start<line.size()&&(line[start]==' '||line[start]=='\t'))++start;
        }
    }
    flush();return flowDocuments_.emplace(key,std::move(document)).first->second;
}
int Ui::flowText(TTF_Font* font,const std::string& value,int x,int y,int width,
                  SDL_Color color){
    auto& document=flowDocument(font,value,width,color);
    SDL_Rect clip{};SDL_RenderGetClipRect(renderer_,&clip);
    if(!SDL_RenderIsClipEnabled(renderer_))clip={0,102,W,556};
    for(auto& chunk:document.chunks){
        SDL_Rect destination{x,y+chunk.y,chunk.text.width,chunk.text.height};
        if(destination.y+destination.h<=clip.y||destination.y>=clip.y+clip.h){
            // Retain layout, not offscreen raster memory. Entering a new page
            // cannot keep megabytes of unrelated long descriptions resident.
            if(chunk.text.texture){SDL_DestroyTexture(chunk.text.texture);chunk.text.texture=nullptr;}
            continue;
        }
        if(!chunk.text.texture){
            SDL_Surface* surface=TTF_RenderUTF8_Blended_Wrapped(font,chunk.source.c_str(),color,width);
            if(surface){chunk.text.texture=SDL_CreateTextureFromSurface(renderer_,surface);SDL_FreeSurface(surface);}
            if(chunk.text.texture)SDL_SetTextureBlendMode(chunk.text.texture,SDL_BLENDMODE_BLEND);
        }
        if(chunk.text.texture)SDL_RenderCopy(renderer_,chunk.text.texture,nullptr,&destination);
    }
    return document.height;
}
void Ui::quietPanel(SDL_Rect rect,bool focused){
    roundRect(rect,8,focused?accentColor():SDL_Color{43,49,53,255});
    roundRect({rect.x+1,rect.y+1,rect.w-2,rect.h-2},7,focused?raisedColor():panelColor());
    if(focused)roundRect({rect.x,rect.y+10,3,rect.h-20},1,accentColor());
}
void Ui::scrollBar(SDL_Rect viewport,int offset,int maximum){
    if(maximum<=0)return;
    const int thumbHeight=std::max(24,viewport.h*viewport.h/(viewport.h+maximum));
    roundRect({viewport.x+viewport.w+12,viewport.y,4,viewport.h},2,raisedColor());
    roundRect({viewport.x+viewport.w+12,viewport.y+(viewport.h-thumbHeight)*
        std::clamp(offset,0,maximum)/maximum,4,thumbHeight},2,Muted);
}
Ui::CachedText Ui::richTexture(TTF_Font* font,const std::string& value,int width,SDL_Color color){
    const std::string key=std::to_string(reinterpret_cast<std::uintptr_t>(font))+"|"+std::to_string(width)+"|"+std::to_string(color.r)+","+std::to_string(color.g)+","+std::to_string(color.b)+"|"+value;
    const auto existing=newsTextCache_.find(key);if(existing!=newsTextCache_.end())return existing->second;
    const int lineHeight=TTF_FontHeight(font)+7;
    struct Run{std::string value;bool bold;bool italic;};std::vector<Run> runs;std::string current;bool bold=false,italic=false;
    auto flush=[&](){if(!current.empty()){runs.push_back({current,bold,italic});current.clear();}};
    for(std::size_t i=0;i<value.size();){if(i+1<value.size()&&value[i]=='*'&&value[i+1]=='*'){flush();bold=!bold;i+=2;}else if(value[i]=='*'){flush();italic=!italic;++i;}else current.push_back(value[i++]);}flush();
    struct Word{std::string value;int x;int y;int style;};std::vector<Word> layout;
    const int originalStyle=TTF_GetFontStyle(font);int activeStyle=originalStyle;
    const auto useStyle=[&](int style){if(activeStyle!=style){TTF_SetFontStyle(font,style);activeStyle=style;}};
    int cursorX=0,cursorY=0,usedWidth=0;bool needsSpace=false;
    for(const auto& run:runs){const int style=(run.bold?TTF_STYLE_BOLD:TTF_STYLE_NORMAL)|(run.italic?TTF_STYLE_ITALIC:TTF_STYLE_NORMAL);useStyle(style);int spaceW=0,spaceH=0;TTF_SizeUTF8(font," ",&spaceW,&spaceH);std::istringstream words(run.value);std::string word;while(words>>word){int wordW=0,wordH=0;TTF_SizeUTF8(font,word.c_str(),&wordW,&wordH);const int added=(needsSpace?spaceW:0)+wordW;if(cursorX+added>width){cursorX=0;cursorY+=lineHeight;needsSpace=false;}if(needsSpace)cursorX+=spaceW;layout.push_back({word,cursorX,cursorY,style});cursorX+=wordW;usedWidth=std::max(usedWidth,cursorX);needsSpace=true;}}
    struct Segment{std::string value;int x;int y;int style;};std::vector<Segment> segments;
    for(const auto& word:layout){if(!segments.empty()&&segments.back().y==word.y&&segments.back().style==word.style)segments.back().value+=" "+word.value;else segments.push_back({word.value,word.x,word.y,word.style});}
    useStyle(originalStyle);const int totalHeight=cursorY+lineHeight,textureWidth=std::max(1,std::min(width,usedWidth+2));SDL_Texture* texture=nullptr;
    SDL_Surface* composed=SDL_CreateRGBSurfaceWithFormat(0,textureWidth,totalHeight,32,SDL_PIXELFORMAT_RGBA32);
    if(composed){SDL_FillRect(composed,nullptr,SDL_MapRGBA(composed->format,0,0,0,0));for(const auto& item:segments){useStyle(item.style);SDL_Surface* segment=TTF_RenderUTF8_Blended(font,item.value.c_str(),color);if(segment){SDL_SetSurfaceBlendMode(segment,SDL_BLENDMODE_NONE);SDL_Rect destination{item.x,item.y,segment->w,segment->h};SDL_BlitSurface(segment,nullptr,composed,&destination);SDL_FreeSurface(segment);}}useStyle(originalStyle);texture=SDL_CreateTextureFromSurface(renderer_,composed);SDL_FreeSurface(composed);}CachedText cached{texture,textureWidth,totalHeight};if(texture){SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);newsTextCache_[key]=cached;}return cached;
}

void Ui::prepareNews(const std::string& changes){
    for(auto& item:newsTextCache_)if(item.second.texture)SDL_DestroyTexture(item.second.texture);
    newsTextCache_.clear();newsBlocks_.clear();preparedNewsSource_=changes;
    preparedNewsPreview_=plainNewsPreview(changes.substr(0,std::min<std::size_t>(changes.size(),64*1024)));
    if(preparedNewsPreview_.size()>360)preparedNewsPreview_.resize(360);
    auto addTexture=[this](const CachedText& cached,int x,int y){if(cached.texture)newsBlocks_.push_back({cached.texture,{x,y,cached.width,cached.height},0});};
    const auto badge=richTexture(bold20_,"WHAT'S NEW",180,accentColor());const int badgeWidth=measure(bold20_,"WHAT'S NEW").first+28;newsBlocks_.push_back({nullptr,{254,128,badgeWidth,36},3});addTexture(badge,268,132);
    int cursor=178;std::istringstream stream(changes);std::string line;
    std::size_t blockCount=0;
    while(blockCount<256&&cursor<16000&&std::getline(stream,line)){
        if(line.size()>4096)line.resize(4096);
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        const std::string clean=trim(line);
        if(clean.empty()){cursor+=12;continue;}
        const auto imageRefs=parseNewsImages(clean);
        if(!imageRefs.empty()){
            const auto& ref=imageRefs.front();const auto found=newsImages_.find(ref.url);
            if(found!=newsImages_.end()){int width=0,height=0;SDL_QueryTexture(found->second,nullptr,nullptr,&width,&height);if(width>0&&height>0){newsBlocks_.push_back({found->second,{268,cursor,width,height},0});cursor+=height+8;}}
            else{newsBlocks_.push_back({nullptr,{268,cursor,960,90},2});addTexture(richTexture(bold20_,"IMAGE UNAVAILABLE",400,accentColor()),288,cursor+18);addTexture(richTexture(regular18_,ref.url,920,Muted),288,cursor+49);cursor+=98;}
            if(!ref.caption.empty()){const auto caption=richTexture(regular18_,ref.caption,960,Muted);addTexture(caption,268,cursor);cursor+=caption.height+5;}cursor+=10;++blockCount;continue;
        }
        if(clean=="---"){newsBlocks_.push_back({nullptr,{268,cursor+6,960,1},1});cursor+=22;++blockCount;continue;}
        TTF_Font* font=regular18_;SDL_Color color=LightText;int x=268,width=960,spacing=5;std::string content=clean;
        if(clean.rfind("### ",0)==0){font=bold20_;color=accentColor();content=clean.substr(4);spacing=5;}
        else if(clean.rfind("## ",0)==0){font=bold20_;color=White;content=clean.substr(3);spacing=7;}
        else if(clean.rfind("# ",0)==0){font=bold28_;color=White;content=clean.substr(2);spacing=9;}
        else if(clean.rfind("- ",0)==0){const auto bullet=richTexture(bold20_,"•",24,accentColor());addTexture(bullet,270,cursor);x=298;width=930;content=clean.substr(2);spacing=3;}
        const auto rendered=richTexture(font,content,width,color);addTexture(rendered,x,cursor);cursor+=rendered.height+spacing;++blockCount;
    }
    newsPageBottom_=std::max(670,cursor+25);newsMaximumScroll_=std::max(0,cursor-630);
    header("What's new");footer({{"UP/DN","Scroll / swipe page"},{"B","Back"}});
}
void Ui::backdrop(){SDL_SetRenderDrawColor(renderer_,12,15,17,255);SDL_RenderClear(renderer_);}
void Ui::setNavigation(int selected,bool focused){navigationSelected_=std::clamp(selected,0,5);navigationFocused_=focused;}
void Ui::directionIcon(SDL_Rect rect,std::uint32_t direction,SDL_Color color){
    const std::size_t index=direction==button::Up?5:(direction==button::Down?6:
                            (direction==button::Left?7:8));
    SDL_Texture* icon=navigationIcons_[index];SDL_SetTextureColorMod(icon,color.r,color.g,color.b);
    SDL_RenderCopy(renderer_,icon,nullptr,&rect);
}
void Ui::navigationRail(int activeSection){
    SDL_SetRenderDrawColor(renderer_,10,12,14,255);
    SDL_Rect rail{0,102,NavigationRailWidth,556};SDL_RenderFillRect(renderer_,&rail);
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,NavigationRailWidth,102,NavigationRailWidth,658);
    text(regular18_,"UNBOUND HQ",24,125,Muted,178);
    const std::array<std::string,6> labels{{"HQ","Install / Update","Workshop","Mod Manager","Support","Network"}};
    const int chosen=navigationFocused_?navigationSelected_:activeSection;
    for(int i=0;i<6;++i){
        const int y=160+i*62;const bool selected=chosen==i;
        touchTarget({14,y,198,48},TouchActionKind::Sidebar,i);
        if(selected){
            if(navigationFocused_)quietPanel({14,y,198,48},true);
            else roundRect({14,y,198,48},6,raisedColor());
        }
        const SDL_Color color=selected&&navigationFocused_?accentColor():(selected?LightText:Muted);
        text(regular18_,labels[i],61,y+10,color,142);
        SDL_Texture* icon=navigationIcons_[static_cast<std::size_t>(i==5?10:i)];
        SDL_SetTextureColorMod(icon,color.r,color.g,color.b);
        const SDL_Rect iconRect{25,y+10,28,28};
        SDL_RenderCopy(renderer_,icon,nullptr,&iconRect);
    }
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,24,560,202,560);
    text(regular18_,navigationFocused_?"Choose a section":gameName(config::activeGame),24,578,Muted,180);
    text(regular18_,navigationFocused_?"A opens section":"Unbound HQ",24,604,Muted,180);
}
void Ui::header(const std::string& section,bool gameSwitchEnabled,int gameChoice){
    SDL_Rect black{0,0,W,102};SDL_SetRenderDrawColor(renderer_,darkColor().r,darkColor().g,darkColor().b,255);SDL_RenderFillRect(renderer_,&black);
    SDL_Rect orange{0,100,W,2};SDL_SetRenderDrawColor(renderer_,accentColor().r,accentColor().g,accentColor().b,255);SDL_RenderFillRect(renderer_,&orange);
    int logoWidth=0,logoHeight=0;
    if(SDL_QueryTexture(logo_,nullptr,nullptr,&logoWidth,&logoHeight)==0&&logoWidth>0&&logoHeight>0){
        const float scale=std::min(190.0f/logoWidth,78.0f/logoHeight);
        const SDL_FRect mark{(NavigationRailWidth-logoWidth*scale)/2, (102-logoHeight*scale)/2,
                             logoWidth*scale,logoHeight*scale};
        SDL_RenderCopyF(renderer_,logo_,nullptr,&mark);
    }
    const std::string build=std::string(config::AppVersion)+" • r"+
        std::to_string(config::AppReleaseRevision);
    const int buildX=W-28-measure(regular18_,build).first;
    text(regular18_,build,buildX,39,accentColor());
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,NavigationRailWidth,18,NavigationRailWidth,84);
    text(regular22_,section,ContentLeft,36,LightText,260);
    const SDL_Rect control{582,28,346,46};
    text(regular18_,"GAME",control.x-65,37,Muted);
    roundRect(control,8,PanelBorder);
    roundRect({control.x+1,control.y+1,control.w-2,control.h-2},7,darkColor());
    const int segmentWidth=control.w/2;
    const int active=config::ghostsMode()?1:0;
    const char* labels[]={"Black Ops II","Ghosts"};
    const SDL_Color activeFill=config::ghostsMode()?SDL_Color{52,73,86,255}:SDL_Color{56,33,20,255};
    for(int i=0;i<2;++i){
        const SDL_Rect segment{control.x+i*segmentWidth,control.y,segmentWidth,control.h};
        if(i==active){
            roundRect({segment.x+3,segment.y+3,segment.w-6,segment.h-6},6,activeFill);
            const auto color=accentColor();
            SDL_SetRenderDrawColor(renderer_,color.r,color.g,color.b,color.a);
            const int x=segment.x+16,y=segment.y+23;
            SDL_RenderDrawLine(renderer_,x,y,x+5,y+5);
            SDL_RenderDrawLine(renderer_,x+5,y+5,x+13,y-4);
        }
        text(bold20_,labels[i],segment.x+segment.w/2+(i==active?9:0),segment.y+9,
             i==active?accentColor():LightText,segment.w-30,true);
        if(i==gameChoice){
            SDL_SetRenderDrawColor(renderer_,Pale.r,Pale.g,Pale.b,Pale.a);
            const SDL_Rect focus{segment.x+2,segment.y+2,segment.w-4,segment.h-4};
            SDL_RenderDrawRect(renderer_,&focus);
        }
        if(gameSwitchEnabled)touchTarget(segment,TouchActionKind::GameChoice,i);
    }
}
void Ui::pill(const std::string& value,int x,int y,SDL_Color fill){const auto size=measure(regular18_,value);SDL_Rect r{x,y,size.first+22,30};roundRect(r,6,raisedColor());const SDL_Color color=fill.r==Blue.r&&fill.g==Blue.g?LightText:(fill.r==PanelBorder.r&&fill.g==PanelBorder.g?Muted:fill);text(regular18_,value,x+11,y+2,color);}
SDL_Texture* Ui::pluginArtwork(const WorkshopPlugin& plugin) const{
    const auto found=pluginArtwork_.find(plugin.key);
    return found!=pluginArtwork_.end()&&found->second?found->second:squareLogo_;
}
void Ui::footer(const std::vector<std::pair<std::string,std::string>>& actions,bool subdued){
    SDL_Rect bar{0,658,W,62};SDL_SetRenderDrawColor(renderer_,15,18,20,248);SDL_RenderFillRect(renderer_,&bar);
    if(subdued){SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,28,658,W-28,658);}
    if(actions.empty())return;
    const int columnWidth=W/static_cast<int>(actions.size());
    for(std::size_t i=0;i<actions.size();++i){
        const auto& action=actions[i];
        const bool horizontalDpad=action.first=="DPAD-LR";
        const bool verticalDpad=action.first=="UP/DN";
        const bool splitDpad=horizontalDpad||verticalDpad;
        int keyWidth=0,labelWidth=0;
        if(!splitDpad)keyWidth=measure(bold20_,action.first).first;
        labelWidth=measure(regular18_,action.second).first;
        const int badgeWidth=splitDpad?94:std::max(38,keyWidth+20);
        const int labelRoom=std::max(0,columnWidth-badgeWidth-34);
        const int contentWidth=badgeWidth+12+std::min(labelRoom,labelWidth);
        const int start=static_cast<int>(i)*columnWidth+std::max(14,(columnWidth-contentWidth)/2);
        SDL_Rect badge{start,672,badgeWidth,34};
        if(splitDpad){
            for(int direction=0;direction<2;++direction){
                SDL_Rect arrow{badge.x+direction*50,badge.y,44,badge.h};
                roundRect(arrow,7,PanelBorder);roundRect({arrow.x+1,arrow.y+1,42,arrow.h-2},6,raisedColor());
                directionIcon({arrow.x+11,arrow.y+6,22,22},
                    horizontalDpad?(direction==0?button::Left:button::Right):
                                   (direction==0?button::Up:button::Down),White);
                touchTarget({arrow.x,663,44,52},TouchActionKind::Button,
                    static_cast<int>(horizontalDpad?(direction==0?button::Left:button::Right):
                                     (direction==0?button::Up:button::Down)));
            }
        }else{
            if(subdued){roundRect(badge,7,PanelBorder);roundRect({badge.x+1,badge.y+1,badge.w-2,badge.h-2},6,raisedColor());}
            else roundRect(badge,10,accentColor());
            text(bold20_,action.first,badge.x+badge.w/2,badge.y+3,White,0,true);
            std::uint32_t bits=0;
            if(action.first=="A")bits=button::A;
            else if(action.first=="B")bits=button::B;
            else if(action.first=="X/1"||action.first=="X")bits=button::X;
            else if(action.first=="Y/2"||action.first=="Y")bits=button::Y;
            else if(action.first=="+")bits=button::Plus;
            else if(action.first=="-")bits=button::Minus;
            else if(action.first=="L")bits=button::L;
            if(bits&&!(suppressedTouchButtons_&bits))
                touchTarget({static_cast<int>(i)*columnWidth+8,663,columnWidth-16,52},
                            TouchActionKind::Button,static_cast<int>(bits));
        }
        text(regular18_,action.second,badge.x+badge.w+12,676,subdued?LightText:White,columnWidth-badgeWidth-34);
    }
}
void Ui::bootScreen(){pump();beginTouch("boot");SDL_RenderCopy(renderer_,boot_,nullptr,nullptr);SDL_Rect shade{0,640,W,80};SDL_SetRenderDrawColor(renderer_,0,0,0,175);SDL_RenderFillRect(renderer_,&shade);text(regular22_,"Loading Unbound HQ...",W/2,663,White,0,true);present();}
void Ui::loadDownloadAnimations(){
    // Decode only the retained center clip once. Its small sheet plus the
    // 720p redraw cache use under 4 MiB instead of loading all three clips.
    // Missing assets or unsupported render targets never prevent HQ starting.
    if(!SDL_RenderTargetSupported(renderer_))return;
    downloadScreenCache_=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET,W,H);
    if(!downloadScreenCache_)return;
    SDL_SetTextureBlendMode(downloadScreenCache_,SDL_BLENDMODE_NONE);
    for(std::size_t i=0;i<downloadAtlases_.size();++i){
        const auto& selected=download_animation::assets::Clips[DownloadClip];
        bool needed=false;
        for(std::size_t frame=0;frame<selected.count;++frame)
            needed=needed||selected.frames[frame].atlas==i;
        if(!needed)continue;
        const auto& asset=download_animation::assets::Atlases[i];
        SDL_Texture* texture=IMG_LoadTexture(renderer_,asset.path);
        int width=0,height=0;
        if(texture&&(SDL_QueryTexture(texture,nullptr,nullptr,&width,&height)!=0||
                     width!=asset.width||height!=asset.height)){
            SDL_DestroyTexture(texture);texture=nullptr;
        }
        if(texture)SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
        downloadAtlases_[i]=texture;
    }
    for(std::size_t i=0;i<downloadClipsReady_.size();++i){
        if(i!=DownloadClip){downloadClipsReady_[i]=false;continue;}
        const auto& clip=download_animation::assets::Clips[i];
        bool ready=clip.count>0&&clip.durationMs>0;
        for(std::size_t f=0;ready&&f<clip.count;++f)
            ready=clip.frames[f].atlas<downloadAtlases_.size()&&downloadAtlases_[clip.frames[f].atlas];
        downloadClipsReady_[i]=ready;
    }
}
void Ui::drawDownloadAnimations(std::uint32_t now){
    if(!downloadClipsReady_[DownloadClip])return;
    const auto& clip=download_animation::assets::Clips[DownloadClip];
    const auto& frame=clip.frames[download_animation::frameIndex(clip,now,downloadAnimationStarted_)];
    const SDL_Rect source{frame.x,frame.y,frame.width,frame.height};
    const SDL_Rect target{(W-frame.width)/2,downloadAnimationTop_+(128-frame.height)/2,
                          frame.width,frame.height};
    SDL_RenderCopy(renderer_,downloadAtlases_[frame.atlas],&source,&target);
}
void Ui::animateDownloadWaiting(){
    if(!downloadAnimationActive_||!downloadScreenCache_||touchIdentity_!="loading-cancel")return;
    const auto now=SDL_GetTicks();
    if(!download_animation::redrawDue(now,downloadAnimationLastDraw_))return;
    downloadAnimationLastDraw_=now;
    pump();
    // Publish the same already-reserved touch map. Animation frames must not
    // cancel a held press on the download's Cancel control.
    pendingTouchIdentity_=touchIdentity_;pendingTouchTargets_=touchTargets_;
    SDL_RenderCopy(renderer_,downloadScreenCache_,nullptr,nullptr);
    drawDownloadAnimations(now);
    present();
}
void Ui::loading(const std::string& rawTitle,const std::string& rawDetail,int progress,bool cancellable,const char* cancelLabel){
    const auto friendly=installProgressFeedback(rawTitle,rawDetail);
    const auto& title=friendly.title;
    const auto& detail=friendly.message;
    pump();beginTouch(cancellable?"loading-cancel":"loading-safe");
    constexpr int contentX=274,contentWidth=922;
    const auto firstBreak=detail.find('\n');
    const std::string lead=detail.substr(0,firstBreak);
    const std::string notes=firstBreak==std::string::npos?std::string{}:detail.substr(firstBreak+1);
    const CachedText* titleText=wrappedText(bold28_,title,contentWidth,White);
    const int titleHeight=titleText?titleText->height:TTF_FontHeight(bold28_);
    const CachedText* leadText=lead.empty()?nullptr:wrappedText(bold20_,lead,contentWidth,LightText);
    const CachedText* noteText=notes.empty()?nullptr:wrappedText(regular18_,notes,contentWidth,Muted);
    const int leadHeight=leadText?leadText->height:0;
    const int noteHeight=noteText?noteText->height:0;
    const int detailHeight=leadHeight+(!lead.empty()&&!notes.empty()?8:0)+noteHeight;
    // One measured column keeps the full version, bytes, ETA and real stage
    // visible. The lead line is easier to scan; subordinate details stay quiet.
    const int cardHeight=std::clamp(30+titleHeight+22+detailHeight+30+8+22+26+30,274,490);
    const bool assetsReady=std::any_of(downloadClipsReady_.begin(),downloadClipsReady_.end(),[](bool ready){return ready;});
    const auto arrangement=download_animation::layout(cardHeight,assetsReady&&downloadScreenCache_&&
                                                       download_animation::eligible(title,cancellable));
    const bool animated=arrangement.visible&&SDL_SetRenderTarget(renderer_,downloadScreenCache_)==0;
    const int top=animated?arrangement.cardTop:102+(556-cardHeight)/2;
    const bool downloading=download_animation::eligible(title,cancellable)||
                           title.find("download")!=std::string::npos;
    const bool verifying=title.find("Verif")!=std::string::npos||title.find("verif")!=std::string::npos||
                         title.find("Checking saved package")!=std::string::npos;
    const bool installing=title.find("npack")!=std::string::npos||title.find("nstall")!=std::string::npos||
                          title.find("destination")!=std::string::npos||title.starts_with("Checking Unbound");
    backdrop();header(downloading?"Download":(verifying?"Package verification":
                      (installing?"Install / Update":"Unbound HQ")));
    quietPanel({36,top,1208,cardHeight});
    SDL_Rect art{88,top+(cardHeight-144)/2,144,144};SDL_RenderCopy(renderer_,squareLogo_,nullptr,&art);
    wrapped(bold28_,title,{contentX,top+30,contentWidth,titleHeight},White);
    const int detailTop=top+30+titleHeight+22;
    const int progressY=top+cardHeight-82;
    const int detailBottom=progressY-24;
    if(leadText)wrapped(bold20_,lead,{contentX,detailTop,contentWidth,std::max(0,std::min(leadHeight,detailBottom-detailTop))},LightText);
    const int notesTop=detailTop+leadHeight+(!lead.empty()&&!notes.empty()?8:0);
    if(noteText)wrapped(regular18_,notes,{contentX,notesTop,contentWidth,std::max(0,detailBottom-notesTop)},Muted);
    SDL_Rect track{contentX,progressY,contentWidth,8};roundRect(track,4,{43,49,53,255});
    const int shown=progress<0?0:std::clamp(progress,0,100);
    if(shown>0){SDL_Rect active{contentX,progressY,std::max(8,contentWidth*shown/100),8};roundRect(active,4,accentColor());}
    text(regular18_,progress<0?"Working...":std::to_string(shown)+"% complete",contentX,progressY+24,Muted);
    if(cancellable)footer({{"B",cancelLabel}});
    else {footer({});text(regular18_,"Operation in progress. Keep the console on.",W/2,679,Muted,1160,true);}
    const auto now=SDL_GetTicks();
    if(animated){
        if(!downloadAnimationActive_)downloadAnimationStarted_=now;
        downloadAnimationActive_=true;downloadAnimationLastDraw_=now;
        downloadAnimationTop_=arrangement.rowTop;
        SDL_SetRenderTarget(renderer_,nullptr);
        SDL_RenderCopy(renderer_,downloadScreenCache_,nullptr,nullptr);
        drawDownloadAnimations(now);
    }else downloadAnimationActive_=false;
    present();
}
int Ui::modal(const std::string& title,const std::string& body,bool isError,
              const std::string& actionLabel,int scrollOffset,bool confirm){
    pump();beginTouch("modal|"+title+"|"+body+"|"+actionLabel+(confirm?"|confirm":""));
    backdrop();header(isError?"Attention required":"Unbound HQ");
    quietPanel({148,156,984,454});
    roundRect({148,174,3,418},1,isError?Red:accentColor());
    const SDL_Rect art{180,172,50,50};
    SDL_RenderCopy(renderer_,squareLogo_,nullptr,&art);
    text(bold28_,title,246,180,isError?Red:White,850);
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,182,233,1098,233);
    const SDL_Rect bodyBox{182,257,890,316};
    const int maximum=wrappedScrollable(regular18_,body,bodyBox,LightText,scrollOffset);
    const int offset=std::clamp(scrollOffset,0,maximum);
    scrollBar(bodyBox,offset,maximum);
    std::vector<std::pair<std::string,std::string>> actions{{"A",actionLabel}};
    if(maximum>0)actions.emplace_back("UP/DN","Scroll");
    if(confirm)actions.emplace_back("B","Cancel");
    footer(actions);
    present();return maximum;
}
void Ui::actionCard(SDL_Rect r,const std::string& eyebrow,const std::string& title,const std::string& detail,bool selected,SDL_Color accent,bool enabled){quietPanel(r,selected);text(regular18_,eyebrow,r.x+20,r.y+18,selected&&enabled?accentColor():Muted,r.w-40);text(bold20_,title,r.x+20,r.y+55,enabled?White:Muted,r.w-50);wrapped(regular18_,detail,{r.x+20,r.y+94,r.w-40,r.h-112},enabled?LightText:Muted);if(selected)directionIcon({r.x+r.w-34,r.y+58,20,20},button::Right,enabled?accentColor():Muted);(void)accent;}
void Ui::home(const GameInstall& install,const Release* release,const std::string& changes,int selected,const std::string& updateStatus,bool repairNeeded,bool restartRequired,int gameChoice){
    pump();beginTouch(config::ghostsMode()?"home-ghosts":"home");backdrop();header("Unbound HQ",true,gameChoice);
    const bool contentFocused=!navigationFocused_;
    quietPanel({244,121,620,330},contentFocused&&selected==0);
    touchTarget({244,121,620,330},TouchActionKind::HomeAction,0);
    text(regular18_,"LATEST RELEASE",264,140,Muted,580);
    text(bold28_,"What's new",264,173,White,580);
    SDL_Texture* hero=newsHero_?newsHero_:boot_;
    int artworkWidth=0,artworkHeight=0;SDL_QueryTexture(hero,nullptr,nullptr,&artworkWidth,&artworkHeight);
    const SDL_Rect art{264,210,580,148};
    if(artworkWidth>0&&artworkHeight>0){
        const double scale=std::min(double(art.w)/artworkWidth,double(art.h)/artworkHeight);
        const int width=int(artworkWidth*scale),height=int(artworkHeight*scale);
        const SDL_Rect fitted{art.x+(art.w-width)/2,art.y+(art.h-height)/2,width,height};
        SDL_RenderCopy(renderer_,hero,nullptr,&fitted);
    }
    const std::string& preview=preparedNewsSource_==changes?preparedNewsPreview_:changes;
    const auto headlineEnd=preview.find('\n');
    const std::string headline=preview.substr(0,headlineEnd);

    text(bold20_,headline,264,363,White,580);
    text(regular18_,updateStatus,264,397,Muted,580);
    text(regular18_,"Read full update  >",264,425,contentFocused&&selected==0?accentColor():LightText);

    struct StatusRow {std::string label;std::string value;SDL_Color color;};
    std::vector<StatusRow> rows;
    if(install.sku){
        rows={{"Game",install.base.present?deviceName(install.base.device):"Not found",install.base.present?Green:Red},
              {"Update",!install.destinationReady?"Unavailable":(install.update.present?deviceName(install.update.device):"Not installed"),install.update.present&&install.destinationReady?LightText:Red},
              {"Official",officialUpdateSummary(install),officialUpdateStatus(install)==OfficialUpdateStatus::Current?Green:accentColor()},
              {"Region",install.sku->label,LightText},{"Language",install.languageFolder,LightText},
              {"Unbound",install.modVersion.empty()?"Not installed":install.modVersion,accentColor()}};
    }else rows={{"Game","Not found",Red}};
    // One baseline and one rhythm for all six rows. Values use the full
    // right-hand column; long regional labels/versions wrap without squeezing
    // the label or changing the spacing of the neighboring rows.
    int statusHeight=78+18;
    for(const auto& row:rows)statusHeight+=std::max(26,flowDocument(regular18_,row.value,216,row.color).height);
    statusHeight+=static_cast<int>(rows.size()>0?rows.size()-1:0)*8;
    statusHeight=330;
    quietPanel({886,121,366,statusHeight});
    text(bold20_,std::string("Your ")+gameName(config::activeGame),906,142,White,326);
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,906,181,1232,181);
    int y=199;
    const SDL_Rect statusClip{906,195,326,statusHeight-86};SDL_RenderSetClipRect(renderer_,&statusClip);
    for(const auto& row:rows){
        text(regular18_,row.label,906,y,Muted,100);
        y+=std::max(26,flowText(regular18_,row.value,1016,y,216,row.color))+8;
    }
    SDL_RenderSetClipRect(renderer_,nullptr);
    if(!install.sku)wrapped(regular18_,"Insert a supported disc or reconnect game storage. HQ stays available while the game is missing.",{906,246,326,126},Muted);
    const bool canUpdate=install.update.present;const bool updateAvailable=release&&canUpdate&&(install.modVersion=="Not installed"||compareVersions(install.modVersion,release->version)<0);
    const bool checkFailed=canUpdate&&!release;
    networkSummary(contentFocused&&selected==5);
    const bool launchFocused=contentFocused&&selected==4;
    if(launchFocused)roundRect({882,580,374,68},10,White);
    roundRect({886,584,366,60},8,accentColor());
    touchTarget({886,584,366,60},TouchActionKind::HomeAction,4);
    text(bold20_,repairNeeded?"Repair before playing":restartRequired?"Restart Aroma before playing":"Launch Game",1069,601,config::ghostsMode()?Ink:White,326,true);
    quietPanel({244,584,620,60},contentFocused&&selected==1);
    touchTarget({244,584,620,60},TouchActionKind::HomeAction,1);
    const std::string installNote=repairNeeded?"Resume installation":checkFailed?"Check for updates":updateAvailable?"Install available update":"Install / Update";
    text(bold20_,installNote,264,601,White,580);
    navigationRail(0);
    if(gameChoice>=0)footer({{"A","Select game"},{"LEFT/RIGHT","Choose game"},{"B","Back"},{"+","Exit"}});
    else if(navigationFocused_)footer({{"A","Open section"},{"UP/DN","Choose section"},{"L","Switch game"},{"+","Exit"}});
    else footer({{"A",selected==4?"Launch game":(selected==1?"Install / Update":selected==5?"Open Network":"Read update")},{"D-PAD","Navigate"},{"L","Switch game"},{"B","Sidebar"},{"+","Exit"}});
    present();
}
void Ui::installSteps(int current){
    SDL_SetRenderDrawColor(renderer_,10,12,14,255);SDL_Rect rail{0,102,226,556};SDL_RenderFillRect(renderer_,&rail);
    text(regular18_,"INSTALLATION",24,134,Muted,180);
    const std::array<std::string,3> steps{{"Choose content","Review","Install"}};
    for(int i=0;i<3;++i){const int y=204+i*74;
        if(i==current)quietPanel({14,y-8,198,58},true);
        text(bold20_,std::to_string(i+1),28,y+4,i==current?accentColor():Muted,24);
        text(regular18_,steps[i],58,y+6,i==current?White:Muted,152);
    }
    wrapped(regular18_,"Your selection is saved for next time.",{24,508,178,100},Muted);
}


void Ui::contentQueue(const std::vector<std::pair<std::string,std::string>>& rows,
                      const GameInstall& install,const std::string& total,int scroll){
    pump();beginTouch("content-queue");backdrop();header("Install / Update");installSteps(1);
    text(bold28_,"Ready to install",244,126,White,1008);
    text(regular18_,"Core first, then your selected map packs. Existing downloads can be reused.",244,169,Muted,1008);
    quietPanel({244,212,606,342});quietPanel({868,212,384,342});
    const SDL_Rect clip{258,228,576,310};SDL_RenderSetClipRect(renderer_,&clip);
    for(std::size_t i=0;i<rows.size();++i){
        const int y=238+int(i)*80-scroll;
        if(y+70<clip.y||y>clip.y+clip.h)continue;
        text(bold20_,rows[i].first,270,y,White,554);
        text(regular18_,rows[i].second,270,y+32,Muted,554);
    }
    SDL_RenderSetClipRect(renderer_,nullptr);
    text(regular18_,"INSTALL DESTINATION",892,237,Muted,336);
    text(bold20_,deviceName(install.update.device),892,270,White,336);
    text(regular18_,"LANGUAGE",892,317,Muted,336);
    text(bold20_,install.languageFolder,892,350,White,336);
    text(regular18_,"PACKAGE TOTAL",892,404,Muted,336);
    text(bold28_,total,892,437,accentColor(),336);
    text(regular18_,"Local ZIPs are verified first.",892,494,Muted,336);
    roundRect({244,587,606,58},8,accentColor());
    text(bold20_,"Start installation",270,605,White,550);
    touchTarget({244,587,606,58},TouchActionKind::Button,int(button::A));
    footer({{"A","Start installation"},{"UP/DN","Scroll queue"},{"B","Change selection"}});present();
}

void Ui::mapPacks(const std::vector<MapPack>& packs,const std::vector<bool>& selected,
                  const std::vector<std::string>& versions,int focused,
                  const GameInstall& install,const std::string& notice,bool reinstall){
    pump();beginTouch("map-packs|"+install.languageFolder);backdrop();header("Install / Update");installSteps(0);
    text(bold28_,"Choose your content",244,121,White,690);
    text(regular18_,"Select the optional map packs you want to install alongside Core.",244,162,Muted,1008);

    quietPanel({244,199,1008,62});
    text(bold20_,"Unbound Core",264,218,White,198);
    pill("REQUIRED",462,215,Muted);
    text(regular18_,"Install destination",750,219,Muted,220);
    const std::string destination=install.update.device==Device::USB?"USB storage":"Internal storage (NAND)";
    pill(destination,951,215,accentColor());

    const int count=int(packs.size());
    const int selectedCount=int(std::count(selected.begin(),selected.end(),true));
    const int start=std::clamp(focused-3,0,std::max(0,count-4));
    text(bold20_,"Optional map packs",244,279,White,286);
    text(regular18_,std::to_string(selectedCount)+" selected",530,281,Muted,120);
    if(count>4){
        directionIcon({677,281,18,18},button::Up,Muted);
        directionIcon({712,281,18,18},button::Down,Muted);
        touchTarget({668,272,36,36},TouchActionKind::Button,int(button::Up));
        touchTarget({704,272,36,36},TouchActionKind::Button,int(button::Down));
    }
    for(int i=start;i<count&&i<start+4;++i){
        const int y=316+(i-start)*61;
        const SDL_Rect row{244,y,496,55};
        quietPanel(row,i==focused);
        const SDL_Rect checkbox{263,y+16,22,22};
        roundRect(checkbox,4,selected[i]?accentColor():Muted);
        if(selected[i]){
            SDL_SetRenderDrawColor(renderer_,Ink.r,Ink.g,Ink.b,255);
            SDL_RenderDrawLine(renderer_,268,y+27,273,y+32);
            SDL_RenderDrawLine(renderer_,273,y+32,281,y+22);
            SDL_RenderDrawLine(renderer_,268,y+28,273,y+33);
            SDL_RenderDrawLine(renderer_,273,y+33,281,y+23);
        }else roundRect({265,y+18,18,18},3,panelColor());
        text(bold20_,packs[i].name,300,y+4,White,422);
        const auto label=versions[i].empty()?"Available to install":"Installed v"+versions[i];
        text(regular18_,label,300,y+30,versions[i].empty()?Muted:Green,422);
        touchTarget(row,TouchActionKind::MapPackRow,i);
    }

    quietPanel({759,278,493,276});
    text(regular18_,"PACK DETAILS",780,295,Muted,450);
    if(!packs.empty()){
        const int i=std::clamp(focused,0,count-1);
        const auto& pack=packs[i];
        text(bold28_,pack.name,780,326,White,450);
        wrapped(regular18_,pack.description,{780,372,450,85},LightText);
        const auto* release=mapPackRelease(pack,versions[i],reinstall);
        const auto size=release?std::to_string((release->compressedBytes+1048575)/1048576)+" MiB package":"Installed version is current";
        text(regular18_,size,780,469,Muted,450);
        const SDL_Rect details{780,501,449,36};
        quietPanel(details);
        text(regular18_,"Full description  >",794,507,accentColor(),413);
        touchTarget(details,TouchActionKind::MapPackDetails,i);
    }else{
        quietPanel({244,316,496,55});
        text(regular18_,"No optional packs available",264,332,Muted,450);
        wrapped(regular18_,"Continue to review your Core installation.",{780,340,449,92},LightText);
    }

    text(regular18_,notice,244,567,Muted,1008);
    const SDL_Rect review{244,602,496,51};
    quietPanel(review,focused==count);
    text(bold20_,"Review installation  >",264,616,accentColor(),450);
    touchTarget(review,TouchActionKind::MapPackRow,count);
    if(count){
        const SDL_Rect repair{759,602,493,51};
        quietPanel(repair,reinstall);
        text(regular18_,reinstall?"Reinstall selected packs: On":"Reinstall selected packs: Off",
             780,617,reinstall?accentColor():LightText,450);
        touchTarget(repair,TouchActionKind::Button,int(button::X));
    }
    footer({{"A","Select"},{"D-PAD","Navigate"},{"RIGHT","Details"},{"X","Reinstall"},{"B","Back"}});present();
}

void Ui::installReview(const GameInstall& active,const std::string& latest,
                       const std::string& notice,int selected,bool enabled,
                       bool sdAvailable,bool repair,bool update){
    pump();beginTouch("install-review|"+active.update.root+"|"+active.languageFolder);
    SDL_SetRenderDrawColor(renderer_,12,15,17,255);SDL_RenderClear(renderer_);
    header("Install / Update");
    text(bold38_,repair?"Resume your installation":update?"Ready to update":"Your installation",80,130,White,1100);
    wrapped(regular18_,notice.empty()?"Review your destination, then install.":notice,{80,185,1120,48},Muted);
    roundRect({80,246,1120,268},10,PanelBorder);roundRect({81,247,1118,266},9,panelColor());
    text(regular18_,"UNBOUND RELEASE",112,268,Muted,590);
    text(bold28_,latest.empty()?"Release unavailable":latest,112,302,White,590);
    text(regular18_,"Installed: "+(active.modVersion.empty()?std::string("Not installed"):active.modVersion),112,350,Muted,590);
    text(regular18_,sdAvailable?"Update found on your SD card":"Uses your saved download or the internet",112,390,LightText,590);
    text(regular18_,"Every package is verified before installation.",112,428,Muted,590);
    text(regular18_,"INSTALL TO",760,268,Muted,390);
    text(bold20_,active.update.present?deviceName(active.update.device):"No update storage found",760,302,White,390);
    text(regular18_,std::string(active.sku?active.sku->key:"Unknown region")+" / "+active.languageFolder,760,342,Muted,390);
    text(regular18_,"Game: "+std::string(active.base.present?deviceName(active.base.device):"Not found"),760,378,Muted,390);
    if(selected==0)roundRect({748,422,432,54},6,raisedColor());
    text(regular18_,"Change storage or language  >",760,438,selected==0?accentColor():LightText,400);
    touchTarget({748,422,432,54},TouchActionKind::InstallRow,0);
    roundRect({80,546,480,56},8,enabled?(selected==3?accentColor():raisedColor()):raisedColor());
    text(bold20_,repair?"Resume installation":sdAvailable?"Verify & install from SD":update?"Choose content & update":"Manage installed content",102,562,enabled?White:Muted,436);
    if(enabled)touchTarget({80,546,480,56},TouchActionKind::InstallRow,3);
    if(sdAvailable&&enabled){
        if(selected==4)roundRect({590,546,400,56},8,raisedColor());
        text(regular18_,"Use internet instead",610,564,selected==4?accentColor():Muted,360);
        touchTarget({590,546,400,56},TouchActionKind::InstallRow,4);
    }
    if(!enabled&&selected!=0)suppressedTouchButtons_|=button::A;
    footer({{"A","Select"},{"D-PAD","Navigate"},{"X/1","Settings"},{"B","Back"}});present();
}
void Ui::installMenu(const GameInstall& active,const std::vector<InstallOption>& options,
                     const InstallPreferences& draft,int selectedRow,
                     const std::string& latest,const std::string& notice,
                     bool contentAllowed,bool hasUpdatePlan,bool reinstallAvailable,bool inheritedRepairNeeded){
    pump();beginTouch("install|"+active.update.root+"|"+draft.languageFolder);
    // A quiet backdrop keeps storage details readable without changing other screens.
    SDL_SetRenderDrawColor(renderer_,12,15,17,255);SDL_RenderClear(renderer_);
    header("Install / Update");
    constexpr SDL_Color secondary{151,161,167,255},divider{43,49,53,255};
    text(bold28_,"Install settings",244,121,White,950);
    const bool manualTarget=draft.updateDevice!=Device::Missing||draft.updateTitleId!=0;
    const bool manualLanguage=!draft.languageFolder.empty();
    const bool ready=active.sku&&active.update.present&&active.destinationReady;
    const bool installEnabled=ready&&contentAllowed&&(hasUpdatePlan||reinstallAvailable||inheritedRepairNeeded);
    const std::string message=!ready&&!active.destinationWarning.empty()?active.destinationWarning:
        (!notice.empty()?notice:(!contentAllowed?"Restart HQ and complete its app update before changing game content.":
         (draft.languageInstallPending?"Language change pending. Install the full release and its patches for this folder.":
          "Choose storage and language, then save your selection before installing.")));
    wrapped(regular18_,message,{244,164,1008,50},!ready?Red:secondary);

    const auto panel=[&](SDL_Rect rect){
        roundRect(rect,9,divider);
        roundRect({rect.x+1,rect.y+1,rect.w-2,rect.h-2},8,panelColor());
    };
    panel({244,226,400,404});
    panel({662,226,590,404});
    std::size_t languageCount=0;
    bool manualTargetAvailable=false;
    for(const auto& option:options){
        if(option.install.sku&&option.install.sku->updateTitleId==draft.updateTitleId&&
           option.install.update.device==draft.updateDevice)manualTargetAvailable=true;
        if(option.install.sku&&active.sku&&
           option.install.sku->updateTitleId==active.sku->updateTitleId&&
           option.install.update.device==active.update.device){
            languageCount=option.languages.size();
        }
    }
    const auto row=[&](int index,int y,int height,const std::string& title,
                       const std::string& value,const std::string& detail,
                       bool editable,bool enabled){
        const bool selected=selectedRow==index&&!navigationFocused_;
        const SDL_Color mainColor=enabled?LightText:secondary;
        if(enabled){
            touchTarget({254,y,editable?272:380,height},TouchActionKind::InstallRow,index);
            if(editable){
                touchTarget({526,y+20,48,height-20},TouchActionKind::InstallChoice,index*2);
                touchTarget({582,y+20,48,height-20},TouchActionKind::InstallChoice,index*2+1);
            }
        }
        if(selected){
            roundRect({254,y,380,height},6,enabled?accentColor():PanelBorder);
            roundRect({255,y+1,378,height-2},5,raisedColor());
            roundRect({254,y+9,3,height-18},1,enabled?accentColor():secondary);
        }
        if(editable){
            text(regular18_,title,268,y+7,secondary,320);
            text(bold20_,value,268,y+32,enabled?White:secondary,238);
            text(regular18_,detail,268,y+60,secondary,344);
            directionIcon({539,y+35,22,22},button::Left,selected&&enabled?accentColor():secondary);
            directionIcon({595,y+35,22,22},button::Right,selected&&enabled?accentColor():secondary);
        }else{
            text(bold20_,title,268,y+(detail.empty()?7:5),selected&&enabled?White:mainColor,320);
            if(!detail.empty())text(regular18_,detail,268,y+31,secondary,344);
            directionIcon({595,y+(detail.empty()?10:8),22,22},button::Right,selected&&enabled?accentColor():secondary);
        }
    };
    std::string target,targetRegion;
    if(manualTarget){
        const GameSku* savedSku=findSkuByTitle(draft.updateTitleId);
        target=deviceName(draft.updateDevice);
        targetRegion=savedSku?savedSku->label:"Saved title";
    }else if(active.sku&&active.update.present){
        target=deviceName(active.update.device);
        targetRegion=active.sku->label;
    }else{
        target="No game update found";
    }
    if(!manualTarget)target="Auto: "+target;
    const std::string targetDetail=(targetRegion.empty()?std::string{}:targetRegion+" • ")+
        (manualTarget&&!manualTargetAvailable?"Unavailable":
         std::to_string(options.size())+" location"+(options.size()==1?"":"s"));
    row(0,237,88,"Install destination",target,
        targetDetail,true,!options.empty());
    const std::string language=manualLanguage?draft.languageFolder:
        "Auto: "+(active.languageFolder.empty()?std::string("Not identified"):active.languageFolder);
    row(1,333,88,"Content language",language,
        std::to_string(languageCount)+" available language folder"+(languageCount==1?"":"s"),true,languageCount>0);
    SDL_SetRenderDrawColor(renderer_,divider.r,divider.g,divider.b,255);
    SDL_RenderDrawLine(renderer_,268,432,620,432);
    row(2,444,42,"Save selection","","",false,ready&&contentAllowed);
    const std::string installLabel=inheritedRepairNeeded?"Restore original game files":hasUpdatePlan?(draft.languageInstallPending?
        "Install selected language":"Download & install"):
        (reinstallAvailable?"Reinstall current release":"No download available");
    std::string installDetail;
    if(!contentAllowed)installDetail="Unavailable until the HQ app update is complete";
    else if(!ready)installDetail="Choose an available destination first";
    else if(inheritedRepairNeeded)installDetail="Uses your installed game files";
    else if(draft.languageInstallPending)installDetail="Full release + current patches";
    else if(hasUpdatePlan)installDetail="Newest files for this destination";
    else if(reinstallAvailable)installDetail="Full release + current patches";
    else installDetail="Check the release feed or rescan";
    row(3,501,58,installLabel,"",installDetail,false,installEnabled);
    row(4,575,38,"Reset to Auto","","",false,true);

    text(bold20_,"Selected installation",684,240,White,380);
    text(regular18_,manualTarget||manualLanguage?"Custom":"Automatic",1178,242,secondary,125,true);
    const auto statusRow=[&](const std::string& label,const std::string& value,int y,
                            SDL_Color color=LightText){
        text(regular18_,label,684,y,secondary,156);
        text(regular18_,value,848,y,color,382);
    };
    statusRow("Game storage",active.base.present?deviceName(active.base.device):"Not found",274);
    statusRow("Update storage",active.update.present?deviceName(active.update.device):"Not found",302,
              ready?LightText:Red);
    statusRow("Region",active.sku?active.sku->label:"Not identified",330);
    statusRow("Language",active.languageFolder.empty()?"Not identified":active.languageFolder,358);
    SDL_SetRenderDrawColor(renderer_,divider.r,divider.g,divider.b,255);
    SDL_RenderDrawLine(renderer_,684,396,1230,396);
    statusRow("Official update",officialUpdateSummary(active),407,
              officialUpdateStatus(active)==OfficialUpdateStatus::Current?Green:accentColor());
    statusRow("Installed mod",active.modVersion.empty()?"Not installed":active.modVersion,435);
    statusRow("Latest release",latest.empty()?"Unavailable":latest,463,
              latest.empty()?LightText:White);
    std::ostringstream titleId;
    if(active.sku){titleId<<std::hex<<std::uppercase;titleId.width(16);titleId.fill('0');
        titleId<<active.sku->updateTitleId;}
    SDL_SetRenderDrawColor(renderer_,divider.r,divider.g,divider.b,255);
    SDL_RenderDrawLine(renderer_,684,500,1230,500);
    statusRow("Update title",active.sku?titleId.str():"Not identified",512);
    text(regular18_,"Content folder",684,543,secondary,546);
    // Keep the volume and title path on separate lines. Wrapping a single
    // slash-delimited path otherwise makes the destination hard to read.
    std::string contentPath=active.update.root;
    if(!contentPath.empty()){
        const auto volumeEnd=contentPath.find(":/");
        if(volumeEnd!=std::string::npos)contentPath.insert(volumeEnd+2,"\n");
        if(contentPath.back()!='/')contentPath+='/';
        contentPath+="content";
    }else contentPath="No writable destination selected";
    wrapped(regular18_,contentPath,{684,569,546,51},ready?LightText:Red);
    text(regular18_,"Settings do not move your game or change which copy the Wii U Menu launches.",
         244,633,secondary,1008);
    const std::string action=selectedRow<2?"Change":
        (selectedRow==2?"Save":(selectedRow==3?(inheritedRepairNeeded?"Restore":"Install"):"Reset Auto"));
    const std::array<bool,5> enabledRows{{!options.empty(),languageCount>0,ready&&contentAllowed,installEnabled,true}};
    if(selectedRow>=0&&selectedRow<5&&!enabledRows[static_cast<std::size_t>(selectedRow)])suppressedTouchButtons_|=button::A;
    footer({{"A",action},{"D-PAD","Navigate / adjust"},
            {"X/1","Rescan"},{"B","Back"}},true);
    navigationRail(1);
    present();
}
void Ui::news(const std::string& changes,int scrollOffset){
    if(newsBlocks_.empty())prepareNews(changes);
    pump();beginTouch("news");backdrop();SDL_Rect clip{244,102,1008,556};SDL_RenderSetClipRect(renderer_,&clip);
    const int top=112-scrollOffset,bottom=newsPageBottom_-scrollOffset;
    SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_Rect sheet{244,std::max(102,top),1008,std::max(0,std::min(658,bottom)-std::max(102,top))};if(sheet.h>0)SDL_RenderFillRect(renderer_,&sheet);
    SDL_SetRenderDrawColor(renderer_,panelColor().r,panelColor().g,panelColor().b,panelColor().a);SDL_Rect inner{245,std::max(102,top+1),1006,std::max(0,std::min(658,bottom-1)-std::max(102,top+1))};if(inner.h>0)SDL_RenderFillRect(renderer_,&inner);
    const int documentTop=scrollOffset+NewsClipTop-4096;
    auto block=std::lower_bound(newsBlocks_.begin(),newsBlocks_.end(),documentTop,[](const NewsBlock& item,int y){return item.rect.y<y;});
    for(;block!=newsBlocks_.end();++block){if(block->rect.y-scrollOffset>=NewsClipBottom)break;SDL_Rect target=block->rect;target.y-=scrollOffset;if(!newsVisible(target.y,target.h))continue;if(block->kind==0&&block->texture)SDL_RenderCopy(renderer_,block->texture,nullptr,&target);else if(block->kind==1){SDL_SetRenderDrawColor(renderer_,43,49,53,255);SDL_RenderDrawLine(renderer_,target.x,target.y,target.x+target.w,target.y);}else if(block->kind==2)roundRect(target,8,raisedColor());else if(block->kind==3)roundRect(target,6,raisedColor());}
    SDL_RenderSetClipRect(renderer_,nullptr);scrollBar({244,118,996,524},scrollOffset,newsMaximumScroll_);header("What's new",true);navigationRail(0);footer({{"UP/DN","Scroll / swipe page"},{"B","Back"}});present();
}

void Ui::prepareSupport(const std::string& report){
    for(auto& block:supportBlocks_)if(block.text.texture)SDL_DestroyTexture(block.text.texture);
    supportBlocks_.clear();preparedSupportSource_=report;
    constexpr std::size_t ChunkBytes=1536;
    constexpr unsigned ChunkLines=32;
    constexpr int Width=960;
    int cursor=0;std::string chunk;unsigned chunkLines=0;
    const auto flush=[&](){
        if(chunk.empty())return;
        SDL_Surface* surface=TTF_RenderUTF8_Blended_Wrapped(regular18_,chunk.c_str(),LightText,Width);
        if(surface){
            SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer_,surface);
            if(texture){
                SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
                supportBlocks_.push_back({{texture,surface->w,surface->h},cursor});
                cursor+=surface->h;
            }
            SDL_FreeSurface(surface);
        }
        chunk.clear();chunkLines=0;
    };
    const auto appendLine=[&](const std::string& line){
        if(chunk.size()+line.size()+1>ChunkBytes||chunkLines>=ChunkLines)flush();
        chunk+=line;chunk.push_back('\n');++chunkLines;
    };
    std::istringstream lines(report);std::string line;
    while(std::getline(lines,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()){appendLine({});continue;}
        // Wrap even an unbroken URL/hash/path, which SDL_ttf's word wrapping
        // alone can leave wider than the viewport. This work occurs only when
        // the report changes, never while scrolling or downloading.
        std::size_t start=0;
        while(start<line.size()){
            std::vector<std::size_t> boundaries;
            const std::size_t limit=std::min(line.size(),start+ChunkBytes-1);
            for(std::size_t end=start+1;end<=limit;++end)
                if(end==line.size()||(static_cast<unsigned char>(line[end])&0xc0U)!=0x80U)
                    boundaries.push_back(end);
            if(boundaries.empty())boundaries.push_back(limit);
            std::size_t lower=0,upper=boundaries.size();
            while(lower<upper){
                const std::size_t middle=lower+(upper-lower)/2;
                int width=0,height=0;
                const std::string prefix=line.substr(start,boundaries[middle]-start);
                TTF_SizeUTF8(regular18_,prefix.c_str(),&width,&height);
                if(width<=Width)lower=middle+1;else upper=middle;
            }
            std::size_t end=boundaries[lower?lower-1:0];
            if(end<line.size()){
                const std::size_t space=line.find_last_of(" \t",end-1);
                if(space!=std::string::npos&&space>start)end=space;
            }
            appendLine(line.substr(start,end-start));start=end;
            while(start<line.size()&&(line[start]==' '||line[start]=='\t'))++start;
        }
    }
    flush();supportMaximumScroll_=std::max(0,cursor-430);
}

int Ui::support(const std::string& report,int scrollOffset,const std::string& status){
    if(preparedSupportSource_!=report||(supportBlocks_.empty()&&!report.empty()))prepareSupport(report);
    pump();beginTouch("support");backdrop();header("Support & diagnostics",true);
    quietPanel({244,121,1008,524});
    text(bold28_,"System information",268,136,White,940);
    const std::string message=status.empty()?"A exports this report to your SD card. Y / 2 refreshes local information.":status;
    text(regular18_,message,268,176,status.empty()?LightText:accentColor(),960);
    const SDL_Rect clip{268,207,960,430};SDL_RenderSetClipRect(renderer_,&clip);
    const int offset=std::clamp(scrollOffset,0,supportMaximumScroll_);
    const auto first=std::lower_bound(supportBlocks_.begin(),supportBlocks_.end(),offset,
        [](const SupportBlock& block,int top){return block.y+block.text.height<=top;});
    for(auto block=first;block!=supportBlocks_.end();++block){
        SDL_Rect target{clip.x,clip.y+block->y-offset,block->text.width,block->text.height};
        if(target.y>=clip.y+clip.h)break;
        SDL_RenderCopy(renderer_,block->text.texture,nullptr,&target);
    }
    SDL_RenderSetClipRect(renderer_,nullptr);
    scrollBar(clip,offset,supportMaximumScroll_);
    navigationRail(4);
    footer({{"A","Export"},{"UP/DN","Scroll"},{"Y/2","Refresh"},{"B","Back"}});
    present();return supportMaximumScroll_;
}

void Ui::localModManager(const std::vector<LocalModInventoryItem>& mods,
                         std::size_t selected,int scrollOffset,
                         const std::string& notice,bool nativeSd,
                         Device consoleDevice,int detailsScroll){
    std::string touchIdentity="local-mods";
    for(const auto& mod:mods)touchIdentity+='|'+mod.folder;
    pump();beginTouch(touchIdentity);backdrop();header("Local Mod Manager",true);

    const auto duplicateCount=static_cast<unsigned>(std::count_if(
        mods.begin(),mods.end(),[](const LocalModInventoryItem& item){
            return item.hasSd&&item.hasConsole;
        }));
    const std::string consoleStorage=consoleDevice==Device::MLC?"INTERNAL":
        (consoleDevice==Device::USB?"USB":"UNKNOWN");
    const auto compactPill=[this](const std::string& label,int right,int y,
                                  SDL_Color fill){
        const auto measured=measure(regular18_,label);
        const int width=measured.first+20;
        SDL_Rect badge{right-width,y,width,28};
        roundRect(badge,5,panelColor());
        text(regular18_,label,badge.x+10,badge.y+1,fill);
        return badge.x-7;
    };

    text(bold28_,"Your mods",244,121,White,290);
    text(regular18_,"Up / Down selects a mod. Left / Right scrolls its details.",244,165,Muted,660);
    if(duplicateCount>0){
        const std::string summary=std::to_string(duplicateCount)+
            (duplicateCount==1?" shared folder needs attention":" shared folders need attention");
        text(regular18_,summary,454,131,accentColor(),455);
    }
    quietPanel({244,205,374,425});
    SDL_Rect clip{256,218,350,399};SDL_RenderSetClipRect(renderer_,&clip);
    if(mods.empty()){
        wrapped(regular22_,
            "No manifested mods were found on the SD card or in the installed game update.",
            {268,250,326,180},LightText);
    }
    for(std::size_t i=0;i<mods.size();++i){
        const auto& item=mods[i];
        const auto& display=item.hasSd&&(nativeSd||!item.hasConsole)
            ?item.sd:item.console;
        SDL_Rect row{258,225+int(i)*82-scrollOffset,346,70};
        if(row.y+row.h<218||row.y>617)continue;
        const bool rowFocused=i==selected;
        const bool duplicate=item.hasSd&&item.hasConsole;
        quietPanel(row,rowFocused);
        touchTarget(row,TouchActionKind::LocalModRow,static_cast<int>(i),&clip);
        int badgeRight=row.x+row.w-10;
        if(item.hasConsole)badgeRight=compactPill("CONSOLE",badgeRight,row.y+8,LightText);
        if(item.hasSd)badgeRight=compactPill("SD",badgeRight,row.y+8,accentColor());
        const int textWidth=std::max(120,badgeRight-(row.x+18));
        text(bold20_,display.name,row.x+18,row.y+8,White,textWidth);
        text(regular18_,"by "+display.author,row.x+18,row.y+38,
             Muted,textWidth);
        if(duplicate)text(regular18_,"!",row.x+row.w-20,row.y+39,accentColor());
    }
    SDL_RenderSetClipRect(renderer_,nullptr);

    quietPanel({636,205,616,425});
    localModDetailsMaximumScroll_=0;
    const SDL_Rect detailsClip{660,231,504,375};
    SDL_RenderSetClipRect(renderer_,&detailsClip);
    if(!mods.empty()){
        const auto& item=mods[std::min(selected,mods.size()-1)];
        const auto& display=item.hasSd&&(nativeSd||!item.hasConsole)
            ?item.sd:item.console;
        int badgeX=665;
        const int offset=std::max(0,detailsScroll);
        if(item.hasSd){
            pill("SD MOD",badgeX,231-offset,accentColor());
            badgeX+=measure(regular18_,"SD MOD").first+37;
        }
        if(item.hasConsole)pill("ON CONSOLE",badgeX,231-offset,LightText);
        int y=277;
        y+=flowText(bold28_,display.name,665,y-offset,488,White)+12;
        y+=flowText(regular18_,"Created by "+display.author,665,y-offset,488,Muted)+23;
        y+=flowText(regular18_,display.description,665,y-offset,488,LightText)+24;
        if(nativeSd&&item.hasSd&&item.hasConsole){
            text(bold20_,"SD copy takes priority",665,y-offset,accentColor(),488);
            y+=34;
            const std::string warningText="SD files override matching console files. Remove the console copy to avoid mixing versions.";
            y+=flowText(regular18_,warningText,665,y-offset,488,LightText)+24;
        }
        y+=flowText(regular18_,"Folder: "+item.folder,665,y-offset,488,Muted)+8;
        if(item.hasConsole)
            y+=flowText(regular18_,"Console storage: "+consoleStorage,665,y-offset,488,Muted)+8;
        if(!notice.empty())y+=flowText(regular18_,notice,665,y+12-offset,488,Green)+12;
        localModDetailsMaximumScroll_=std::max(0,y-(detailsClip.y+detailsClip.h));
    }else{
        wrapped(regular18_,std::string("Put each mod folder with its modload.txt manifest under ")+config::gameSdModsDisplayPath()+". Installed console mods are also listed here.",{665,247,548,150},Muted);
        if(!notice.empty())wrapped(regular18_,notice,{665,417,548,160},LightText);
    }
    SDL_RenderSetClipRect(renderer_,nullptr);
    scrollBar(detailsClip,detailsScroll,localModDetailsMaximumScroll_);
    if(localModDetailsMaximumScroll_>0){
        const SDL_Rect up{1196,231,44,44},down{1196,562,44,44};
        quietPanel(up);quietPanel(down);
        directionIcon({1207,242,22,22},button::Up,LightText);
        directionIcon({1207,573,22,22},button::Down,LightText);
        touchTarget(up,TouchActionKind::Button,static_cast<int>(button::Left));
        touchTarget(down,TouchActionKind::Button,static_cast<int>(button::Right));
    }
    std::string primaryAction="Actions",deleteAction="Delete";
    if(!mods.empty()){
        const auto& item=mods[std::min(selected,mods.size()-1)];
        if(nativeSd&&item.hasSd&&item.hasConsole){
            primaryAction="Resolve";
            deleteAction="Delete console";
        }else if(item.hasSd){
            primaryAction="Copy to console";
            deleteAction=item.hasConsole?"Delete console":"Delete SD";
        }else if(item.hasConsole){
            primaryAction=nativeSd?"Move to SD":"Back up to SD";
            deleteAction="Delete console";
        }
    }
    navigationRail(3);
    if(mods.empty())suppressedTouchButtons_|=button::A|button::X;
    footer({{"A",primaryAction},{"D-PAD","Browse / read"},{"X/1",deleteAction},
            {"B","Back"}});
    present();
}

void Ui::pluginCatalog(const std::vector<WorkshopPlugin>& plugins,
                       const std::vector<PluginInstallStatus>& statuses,
                       const std::vector<bool>& checked,
                       std::size_t selected,int scrollOffset,
                       const std::string& catalogNotice,const std::vector<bool>& releaseReady){
    std::string touchIdentity="plugin-catalog";
    for(const auto& plugin:plugins)touchIdentity+='|'+plugin.key;
    pump();beginTouch(touchIdentity);backdrop();
    SDL_Rect clip{244,102,1008,556};SDL_RenderSetClipRect(renderer_,&clip);
    const int top=122-scrollOffset;text(bold28_,"Plugin Workshop",244,top,White,760);
    text(regular18_,"Curated plugins with verified updates",244,top+44,Muted,1008);
    if(!catalogNotice.empty())text(regular18_,catalogNotice,244,top+82,
                                   catalogNotice.find("Could not")==std::string::npos?LightText:accentColor(),1008);
    if(plugins.empty())wrapped(regular22_,"No plugin listings are currently available. HQ will keep working normally while the Workshop feed is offline.",{268,top+130,960,130},LightText);
    for(std::size_t i=0;i<plugins.size();++i){
        SDL_Rect card{244,top+112+int(i)*142,1008,126};if(card.y+card.h<102||card.y>658)continue;
        quietPanel(card,i==selected);
        touchTarget(card,TouchActionKind::PluginRow,static_cast<int>(i),&clip);
        SDL_Rect art{card.x+10,card.y+10,106,106};
        SDL_Texture* artwork=pluginArtwork(plugins[i]);
        SDL_RenderCopy(renderer_,artwork,nullptr,&art);
        text(bold20_,plugins[i].name,card.x+138,card.y+13,White,590);
        text(regular18_,"by "+plugins[i].creator,card.x+138,card.y+45,Muted,590);
        text(regular18_,plugins[i].description,card.x+138,card.y+78,Muted,590);
        std::string state="OPEN TO CHECK";SDL_Color stateColor=LightText;
        if(i<statuses.size()&&i<checked.size()&&checked[i]){
            const auto& status=statuses[i];
            state="NOT INSTALLED";stateColor=Muted;
            if(status.presence==PluginPresence::Managed){const bool updateKnown=i<releaseReady.size()&&releaseReady[i]&&status.updateAvailable;state=updateKnown?"UPDATE AVAILABLE":"INSTALLED";stateColor=updateKnown?accentColor():LightText;}
            else if(status.presence==PluginPresence::Unmanaged){state="MANUAL INSTALL";stateColor=accentColor();}
            else if(status.presence==PluginPresence::Damaged){state=status.repairable?"REPAIR NEEDED":"MANUAL RECOVERY";stateColor=Red;}
        }
        text(regular18_,state,card.x+754,card.y+24,stateColor,228);
        if(i<statuses.size()&&i<checked.size()&&checked[i]&&
           statuses[i].presence==PluginPresence::Managed&&
           (i>=releaseReady.size()||!releaseReady[i]))
            wrapped(regular18_,"Latest check unavailable",{card.x+754,card.y+58,228,54},Muted);
        else text(regular18_,"Aroma plugin",card.x+754,card.y+67,Muted,228);
    }
    pluginCatalogMaximumScroll_=std::max(0,122+112+int(plugins.size())*142-16-642);
    SDL_RenderSetClipRect(renderer_,nullptr);header("Plugin Workshop");
    scrollBar({244,118,996,524},scrollOffset,pluginCatalogMaximumScroll_);
    navigationRail(2);
    footer({{"A","Open plugin"},
            {"UP/DN","Navigate"},{"X/1","Refresh"},{"B","HQ"},{"+","Exit"}});present();
}

void Ui::pluginDetails(const WorkshopPlugin& plugin,const PluginRelease* release,
                       const PluginInstallStatus& status,bool autoUpdateEnabled,
                       int selected,int scrollOffset,const std::string& notice,
                       const std::string& feedStatus){
    pump();beginTouch("plugin|"+plugin.key+"|"+(release?release->version:"unavailable")+
        "|"+std::to_string(static_cast<int>(status.presence)));backdrop();
    SDL_Rect clip{244,118,996,456};SDL_RenderSetClipRect(renderer_,&clip);
    const int top=122-scrollOffset;
    const int titleHeight=flowDocument(bold28_,plugin.name,512,White).height;
    const int authorHeight=flowDocument(regular18_,"Published by "+plugin.creator,512,Muted).height;
    const int descriptionHeight=flowDocument(regular18_,plugin.description,512,LightText).height;
    const int descriptionY=top+22+titleHeight+10+authorHeight+22;
    const int heroHeight=std::max(240,descriptionY-top+descriptionHeight+24);
    quietPanel({244,top,1008,heroHeight});
    SDL_Rect art{264,top+22,148,148};SDL_RenderCopy(renderer_,pluginArtwork(plugin),nullptr,&art);
    flowText(bold28_,plugin.name,432,top+22,512,White);
    flowText(regular18_,"Published by "+plugin.creator,432,top+22+titleHeight+10,512,Muted);
    flowText(regular18_,plugin.description,432,descriptionY,512,LightText);
    const bool cachedRelease=feedStatus.rfind("Using cached",0)==0||
        feedStatus.find("retained cached")!=std::string::npos||
        feedStatus.find("retained the cached")!=std::string::npos;
    std::string installed=status.installedVersion.empty()?"Not installed":status.installedVersion;
    std::string state="Not installed";SDL_Color stateColor=Muted;
    if(status.presence==PluginPresence::Managed){
        if(release&&release->revision>status.installedRevision){state="Update available";stateColor=accentColor();}
        else if(release&&status.installedRevision==release->revision&&status.installedVersion==release->version){state=cachedRelease?"Matches cached release":"Current";stateColor=cachedRelease?LightText:Green;}
        else if(!release){state="Installed";stateColor=LightText;}
        else {state="Installed (different release)";stateColor=LightText;}
    }
    else if(status.presence==PluginPresence::Unmanaged){state="Manual install detected";stateColor=accentColor();}
    else if(status.presence==PluginPresence::Damaged){state=status.repairable?"Repair needed":"Manual recovery required";stateColor=Red;}
    text(regular18_,"Status",984,top+22,Muted);flowText(regular18_,state,984,top+48,244,stateColor);
    text(regular18_,"Installed",984,top+105,Muted);text(bold20_,installed,984,top+129,White,244);
    text(regular18_,cachedRelease?"Cached release":"Latest",984,top+167,Muted);text(bold20_,release?release->version:"Check unavailable",984,top+191,release?LightText:Muted,244);
    int y=top+heroHeight+28;text(bold20_,"Managed Aroma files",268,y,White);y+=37;
    for(const auto& path:plugin.managedPaths)y+=flowText(regular18_,"sd:/"+path,268,y,960,LightText)+8;
    y+=16;text(bold20_,"Restart after installing",268,y,accentColor());y+=35;
    std::string restartNote="A full console/Aroma restart is required after installing or updating this plugin.";
    if(plugin.key=="PROTARIUM")
        restartNote+=" Protarium/Inkay also changes network and SSL behavior while active; review the publisher's safety notes before installing.";
    else if(plugin.key=="UNBOUND_MOD_LINK")
        restartNote+=" Unbound Mod Link performs no downloads while a game is running and modifies no installed game files.";
    else if(plugin.key=="GAMEPAD_MIC_REDIRECT")
        restartNote+=" GamePad Mic Redirect changes microphone routing for compatible games; review its publisher page for current controls and testing notes.";
    y+=flowText(regular18_,restartNote,268,y,960,LightText)+28;
    if(!feedStatus.empty()){text(bold20_,"Release check",268,y,White);y+=35;y+=flowText(regular18_,feedStatus,268,y,960,release&&!cachedRelease?Muted:accentColor())+28;}
    if(!notice.empty()){text(bold20_,"Status message",268,y,White);y+=35;y+=flowText(regular18_,notice,268,y,960,LightText)+28;}
    pluginDetailsMaximumScroll_=std::max(0,y+scrollOffset-574);
    SDL_RenderSetClipRect(renderer_,nullptr);header("Workshop plugin");SDL_Rect actionBar{244,588,1008,70};SDL_SetRenderDrawColor(renderer_,13,18,22,255);SDL_RenderFillRect(renderer_,&actionBar);
    std::string installLabel="INSTALL";if(status.presence==PluginPresence::Managed)installLabel=release&&release->revision>status.installedRevision?"UPDATE":"REINSTALL";else if(status.presence==PluginPresence::Unmanaged)installLabel="REPLACE & MANAGE";else if(status.presence==PluginPresence::Damaged)installLabel=status.repairable?"REPAIR":"RECOVERY INFO";
    const bool startupAutoUpdateSupported=supportsStartupPluginAutoUpdate(plugin);
    const std::string updateLabel=!startupAutoUpdateSupported
        ? "AUTO-UPDATE UNAVAILABLE"
        : plugin.key=="UNBOUND_MOD_LINK"?"AUTO-UPDATE MANAGED":
          std::string("AUTO-UPDATE ")+(autoUpdateEnabled?"ON":"OFF");
    const std::vector<std::pair<std::string,SDL_Color>> actions={{installLabel,accentColor()},
        {updateLabel,!startupAutoUpdateSupported?PanelBorder:(autoUpdateEnabled?Green:Blue)}};
    for(int i=0;i<2;++i){
        SDL_Rect buttonRect{254+i*502,599,476,47};quietPanel(buttonRect,i==selected);
        const bool enabled=i==0?(release||status.presence==PluginPresence::Damaged):startupAutoUpdateSupported;
        text(bold20_,actions[i].first,buttonRect.x+buttonRect.w/2,buttonRect.y+9,
             enabled?(i==selected?accentColor():LightText):Muted,440,true);
        if(enabled)touchTarget(buttonRect,TouchActionKind::PluginAction,i);
    }
    if((selected==0&&!release&&status.presence!=PluginPresence::Damaged)||
       (selected==1&&!startupAutoUpdateSupported))suppressedTouchButtons_|=button::A;
    scrollBar(clip,scrollOffset,pluginDetailsMaximumScroll_);
    navigationRail(2);
    footer({{"A","Choose"},{"UP/DN","Scroll"},{"DPAD-LR","Action"},{"B","Back"}});present();
}
void Ui::workshopCatalog(const std::vector<WorkshopMod>& mods,std::size_t featured,std::size_t selected,int scrollOffset,bool featuredFocus){
    pump();SDL_SetRenderDrawColor(renderer_,Steam.r,Steam.g,Steam.b,255);SDL_RenderClear(renderer_);SDL_Rect clip{0,102,W,556};SDL_RenderSetClipRect(renderer_,&clip);const int top=118-scrollOffset;text(bold28_,"Featured & Trending",30,top,White);text(regular18_,"Community picks - use Left and Right to browse",30,top+37,{166,183,196,255});
    if(!mods.empty()){const std::size_t count=std::min<std::size_t>(4,mods.size());std::size_t start=featured>1?featured-1:0;if(start+count>mods.size())start=mods.size()-count;for(std::size_t slot=0;slot<count;++slot){const std::size_t index=start+slot;SDL_Rect card{30+int(slot)*306,top+72,286,152};roundRect(card,10,index==featured&&featuredFocus?accentColor():SteamCard);SDL_Rect art{card.x+8,card.y+8,104,104};SDL_RenderCopy(renderer_,squareLogo_,nullptr,&art);text(bold20_,mods[index].name,card.x+124,card.y+14,White,150);text(regular18_,"by "+mods[index].creator,card.x+124,card.y+45,{166,183,196,255},150);text(bold20_,stars(mods[index]),card.x+124,card.y+81,Blue,150);text(regular18_,std::to_string(mods[index].commentCount)+" comments",card.x+124,card.y+114,{166,183,196,255},150);}}
    const int listTop=top+258;text(bold28_,"Browse all mods",30,listTop,White);
    for(std::size_t i=0;i<mods.size();++i){const int col=int(i%2),row=int(i/2);SDL_Rect card{30+col*615,listTop+48+row*116,595,104};if(card.y+card.h<102||card.y>658)continue;roundRect(card,8,i==selected&&!featuredFocus?Blue:SteamCard);SDL_Rect art{card.x+7,card.y+7,90,90};SDL_RenderCopy(renderer_,squareLogo_,nullptr,&art);text(bold20_,mods[i].name,card.x+112,card.y+9,White,300);text(regular18_,"by "+mods[i].creator,card.x+112,card.y+36,{166,183,196,255},300);text(regular18_,std::to_string(mods[i].commentCount)+" comments",card.x+112,card.y+66,{166,183,196,255},180);text(bold20_,stars(mods[i]),card.x+420,card.y+10,Blue,150);text(regular18_,std::to_string(mods[i].upvotes+mods[i].downvotes)+" ratings",card.x+420,card.y+44,{166,183,196,255},150);}
    SDL_RenderSetClipRect(renderer_,nullptr);header("Community Workshop");footer({{"A","Open item"},{"B","HQ"},{"UP/DN","Scroll"},{"L/R","Move"}});present();
}
void Ui::modDetails(const WorkshopMod& mod,const std::vector<Review>& reviews,int selected,int scrollOffset,const std::string& notice,const std::string& userName){
    pump();SDL_SetRenderDrawColor(renderer_,Steam.r,Steam.g,Steam.b,255);SDL_RenderClear(renderer_);SDL_Rect clip{0,102,W,486};SDL_RenderSetClipRect(renderer_,&clip);const int top=122-scrollOffset;SDL_Rect hero{30,top,1220,224};roundRect(hero,12,SteamCard);SDL_Rect art{48,top+18,188,188};SDL_RenderCopy(renderer_,squareLogo_,nullptr,&art);text(bold38_,mod.name,265,top+18,White,620);text(regular22_,"Created by "+mod.creator,265,top+69,{166,183,196,255},620);text(bold20_,stars(mod),945,top+22,Blue,240);text(regular18_,std::to_string(mod.upvotes)+" positive / "+std::to_string(mod.downvotes)+" negative",945,top+58,{166,183,196,255},260);if(!notice.empty())text(regular18_,notice,945,top+101,Green,260);wrapped(regular22_,mod.description,{265,top+120,650,85},White);
    const int commentsTop=top+260;text(bold28_,"Comments",40,commentsTop,White);text(regular18_,"Posting as "+userName,190,commentsTop+7,{166,183,196,255});if(reviews.empty())text(regular22_,"No comments yet. Be the first to leave one.",40,commentsTop+54,{166,183,196,255});
    for(std::size_t i=0;i<reviews.size();++i){SDL_Rect card{40,commentsTop+50+int(i)*112,1200,98};if(card.y+card.h<102||card.y>588)continue;roundRect(card,8,SteamCard);circle(card.x+35,card.y+35,22,i%2?accentColor():Blue);const std::string initial=reviews[i].author.empty()?"?":reviews[i].author.substr(0,1);text(bold20_,initial,card.x+35,card.y+19,White,0,true);text(bold20_,reviews[i].author,card.x+72,card.y+13,White,240);wrapped(regular18_,reviews[i].body,{card.x+72,card.y+43,1085,43},{205,215,222,255});}
    SDL_RenderSetClipRect(renderer_,nullptr);header("Workshop item");SDL_Rect actionBar{0,588,W,70};SDL_SetRenderDrawColor(renderer_,13,18,22,255);SDL_RenderFillRect(renderer_,&actionBar);const std::vector<std::pair<std::string,SDL_Color>> actions={{"DOWNLOAD",accentColor()},{"THUMBS UP",Green},{"THUMBS DOWN",Red},{"COMMENT",Blue}};for(int i=0;i<4;++i){SDL_Rect button{24+i*313,599,291,47};roundRect(button,8,i==selected?actions[i].second:SteamCard);text(bold20_,actions[i].first,button.x+button.w/2,button.y+9,White,0,true);}footer({{"A","Choose"},{"B","Back"},{"UP/DN","Scroll"},{"L/R","Action"}});present();
}
void Ui::commentEditor(const WorkshopMod& mod,const std::string& draft,int selectedRow,int selectedColumn){
    pump();backdrop();header("Write a comment");text(bold28_,"Review "+mod.name,35,119,White,900);text(regular18_,"Use the D-Pad and A. Comments can be up to 240 characters.",35,156,LightText);SDL_Rect entry{35,195,1210,88};roundRect(entry,12,PanelBorder);SDL_Rect entryInner{38,198,1204,82};roundRect(entryInner,10,panelColor());wrapped(regular22_,draft.empty()?"Your comment will appear here...":draft,{58,216,1160,50},draft.empty()?Muted:White);text(regular18_,std::to_string(draft.size())+" / 240",1135,255,Muted);
    for(int row=0;row<int(KeyboardRows.size());++row){const auto& keys=KeyboardRows[row];const int keyWidth=row==3?132:72,gap=9,total=int(keys.size())*keyWidth+(int(keys.size())-1)*gap,x=(W-total)/2,y=310+row*72;for(int col=0;col<int(keys.size());++col){SDL_Rect key{x+col*(keyWidth+gap),y,keyWidth,54};const bool active=row==selectedRow&&col==selectedColumn;if(active){SDL_Rect halo{key.x-4,key.y-4,key.w+8,key.h+8};roundRect(halo,12,accentColor());}roundRect(key,8,active?SDL_Color{74,34,15,255}:raisedColor());text(row==3?regular18_:bold20_,keys[col],key.x+key.w/2,key.y+12,active?White:LightText,0,true);}}
    footer({{"A","Type / choose"},{"B","Cancel"},{"D-PAD","Move"},{"+","Exit"}});present();
}
} // namespace unbound

namespace unbound {
void Ui::networkSummary(bool focused){
 quietPanel({244,466,1008,100},focused);touchTarget({244,466,1008,100},TouchActionKind::HomeAction,5);
 networkGlobe_.draw(renderer_,310,516,40,globeYaw_,globePitch_,networkStats_.cells,
                    config::ghostsMode()?accentColor():SDL_Color{255,105,18,255});
 text(bold20_,"UNC NETWORK",378,480,accentColor(),250);
 text(regular18_,networkStats_.stale?"Last known activity  >":"Explore the globe  >",378,522,Muted,265);
 const auto stat=[&](unsigned n,const char* label,int x){text(bold28_,networkStats_.available?std::to_string(n):"--",x,480,White,200);text(regular18_,label,x,522,Muted,210);};
 if(config::ghostsMode()){
     stat(networkStats_.players,"Reported players",660);
     stat(networkStats_.servers,"Advertised lobbies",940);
 }else{
     stat(networkStats_.players,"Players online",660);
     stat(networkStats_.servers,"Servers",846);
     stat(networkStats_.unique,"Unique today (UTC)",1008);
 }
}
void Ui::network(){
 pump();beginTouch("network");backdrop();header("Network",true);
 text(bold28_,config::ghostsMode()?"Ghosts network":"The Unbound community",244,125,White,1008);
 const unsigned age=(SDL_GetTicks()-networkStats_.received)/1000;
 text(regular18_,!networkStats_.available?(networkStats_.stale?"Network activity is unavailable. HQ remains ready to use.":"Connecting to the Unbound network..."):networkStats_.stale?"Showing last known activity. Reconnecting automatically.":"Updated "+std::to_string(age)+"s ago",244,168,Muted,1008);
 quietPanel({244,210,1008,94});
 const auto stat=[&](unsigned n,const char* label,int x){text(bold28_,networkStats_.available?std::to_string(n):"--",x,220,White,270);text(regular18_,label,x,263,Muted,270);};
 stat(networkStats_.players,config::ghostsMode()?"Host-reported players":"Players online",274);
 stat(networkStats_.servers,config::ghostsMode()?"Advertised lobbies":"Active servers",610);
 if(!config::ghostsMode())stat(networkStats_.unique,"Unique today (UTC)",940);
 quietPanel({244,324,620,322});networkGlobe_.draw(renderer_,554,477,137,globeYaw_,globePitch_,networkStats_.cells,
                    config::ghostsMode()?accentColor():SDL_Color{255,105,18,255});
 text(regular18_,networkStats_.heatAvailable?(networkStats_.heatStale?"Last known regional activity":(config::ghostsMode()?"Approximate lobby regions":"Approximate player regions")):"Regional activity unavailable",264,613,Muted,580);
 quietPanel({886,324,366,322});
 if(config::ghostsMode()){
     text(bold20_,"Lobby activity",906,345,White,320);
     wrapped(regular18_,"Player counts are reported by lobby hosts. The globe shows approximate lobby activity by country.",
             {906,388,320,160},Muted);
 }else{
     text(bold20_,"Across the network",906,345,White,320);
     const char* names[]={"PC","Wii U","Switch","Android","Other"};
     for(int i=0;i<5;++i){text(regular18_,names[i],906,388+i*33,Muted,220);text(regular18_,networkStats_.available?std::to_string(networkStats_.platforms[i]):"--",1170,388+i*33,LightText,60);}
     text(regular18_,"Menus, lobbies and matches",906,556,Muted,320);
 }
 navigationRail(5);footer({{"D-PAD","Rotate / drag globe"},{"B","Back"}});present();
}
}
