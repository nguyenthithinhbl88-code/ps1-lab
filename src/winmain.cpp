#include <windows.h>
#include <commdlg.h>
#include "core.hpp"
#include "display.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <memory>
#include <chrono>

namespace {
std::unique_ptr<ps1::Bus> bus;
std::unique_ptr<ps1::Cpu> cpu;
HWND output,screen;
std::array<HWND,18> buttons{};
bool large_view=false;
void layout(HWND w){
 RECT r{};GetClientRect(w,&r);
 const int width=r.right,height=r.bottom;
 const int columns=std::clamp((width-24)/120,1,18);
 for(int i=0;i<18;i++)MoveWindow(buttons[i],16+(i%columns)*120,16+(i/columns)*42,110,36,TRUE);
 const int y=16+((18+columns-1)/columns)*42+8;
 const int panel=large_view?0:std::min(560,std::max(220,width*36/100));
 ShowWindow(output,large_view?SW_HIDE:SW_SHOW);
 if(!large_view)MoveWindow(output,16,y,panel,std::max(1,height-y-16),TRUE);
 const int x=large_view?16:panel+32;
 MoveWindow(screen,x,y,std::max(1,width-x-16),std::max(1,height-y-16),TRUE);
 InvalidateRect(screen,nullptr,FALSE);
}
HFONT font;
uint32_t entry=0;
bool running=false;
ULONGLONG last_refresh=0;
bool exe_loaded=false,has_bios=false;
ULONGLONG perf_time=0;
uint64_t perf_frames=0,perf_instructions=0;
double video_hz=0,core_mips=0;
void start_perf(){perf_time=GetTickCount64();perf_frames=bus->gpu.video_frames;perf_instructions=cpu->instructions;video_hz=core_mips=0;}
void sample_perf(){
 const auto now=GetTickCount64();if(now-perf_time<1000)return;
 const double seconds=double(now-perf_time)/1000;
 if(bus->gpu.video_frames>=perf_frames&&cpu->instructions>=perf_instructions){video_hz=double(bus->gpu.video_frames-perf_frames)/seconds;core_mips=double(cpu->instructions-perf_instructions)/seconds/1000000;}
 perf_time=now;perf_frames=bus->gpu.video_frames;perf_instructions=cpu->instructions;
}
void update_pad(HWND w){
 auto& pad=bus->sio[0];pad.connected=true;pad.buttons=0xffff;
 if(GetAncestor(GetForegroundWindow(),GA_ROOT)!=w)return;
 const int keys[]={VK_SHIFT,0,0,VK_RETURN,VK_UP,VK_RIGHT,VK_DOWN,VK_LEFT,'E','R','Q','W','S','X','Z','A'};
 for(unsigned i=0;i<16;i++)if(keys[i]&&(GetAsyncKeyState(keys[i])&0x8000))pad.buttons&=uint16_t(~(1u<<i));
}
std::array<uint32_t,32> initial_regs{};
std::wstring last_fault;
std::wstring status=L"San sang. Demo MIPS; chua ho tro game PS1.";
void refresh(){
 std::wostringstream s;
 s<<L"PS1 LAB | CPU DEBUGGER\r\n"<<status<<L"\r\n\r\n";
 s<<L"Video Hz (model): "<<std::fixed<<std::setprecision(2)<<video_hz<<L" | CPU MIPS: "<<core_mips<<L"\r\n";
 s<<L"PC: "<<std::hex<<std::setfill(L'0')<<std::setw(8)<<cpu->pc<<L"    Next: "<<std::setw(8)<<cpu->next_pc<<L"\r\n\r\n";
 s<<L"IRQ exceptions: "<<std::dec<<cpu->interrupts<<L"\r\n";
 s<<std::hex;
 s<<L"HI: "<<std::setw(8)<<cpu->hi<<L" LO: "<<std::setw(8)<<cpu->lo<<L"\r\n";
 s<<L"SR: "<<std::setw(8)<<cpu->cop0[12]<<L" Cause: "<<std::setw(8)<<cpu->cop0[13]<<L" EPC: "<<std::setw(8)<<cpu->cop0[14]<<L"\r\n\r\n";
 for(unsigned i=0;i<32;i++){
  s<<L"R"<<std::dec<<std::setfill(L'0')<<std::setw(2)<<i<<L"  "<<std::hex<<std::setw(8)<<cpu->r[i]<<((i%2==1)?L"\r\n":L"    ");
 }
 s<<L"\r\nInstructions: "<<std::dec<<cpu->instructions<<L" POST: "<<std::hex<<unsigned(bus->post)<<L"\r\n";
 s<<L"\r\nDMA words: "<<std::dec<<bus->dma_words<<L"\r\n";
 const auto disc_info=bus->cd.disc.description();s<<L"Disc: "<<std::wstring(disc_info.begin(),disc_info.end())<<L"\r\n";
 s<<L"CD sectors: "<<std::dec<<bus->cd.sectors<<L" Read: "<<bus->cd.sectors_read<<L" Commands: "<<bus->cd.commands<<L"\r\n";
 s<<L"Video frames: "<<bus->gpu.video_frames<<L" Line: "<<bus->gpu.video_line<<L" VBlank: "<<bus->gpu.vblank_events<<L"\r\n";
 s<<L"GPU polygons: "<<bus->gpu.polygons<<L" Sprites: "<<bus->gpu.rectangles<<L"\r\n";
 s<<L"SPU writes: "<<bus->spu.writes<<L"\r\n";
 s<<L"\r\nRAM[0x100]: "<<std::dec<<bus->read32(256)<<L"\r\n\r\n";
 const auto view=ps1::display_view(bus->gpu);
 s<<L"Display source: "<<view.x<<L","<<view.y<<L" "<<view.width<<L"x"<<view.height<<L" | Fit 4:3\r\n";
 s<<L"GPU: partial. SPU: init/manual transfer. CD-ROM: ISO/BIN/CUE subset.\r\nBIOS co the dung o lenh hoac MMIO chua ho tro.\r\nPad: mui ten, Z/X/A/S, Enter/Shift. Homebrew direct boot; retail chua xac nhan.";
 std::string tty=bus->tty;s<<L"\r\nTTY: "<<std::wstring(tty.begin(),tty.end());
 SetWindowTextW(output,s.str().c_str());
 if(screen)InvalidateRect(screen,nullptr,FALSE);
}
void stop(HWND w){running=false;KillTimer(w,1);}
void advance(HWND w,unsigned n,bool automatic=false){
 const auto started=std::chrono::steady_clock::now();
 try {
  for(unsigned i=0;i<n;i++){
   cpu->step();
   if(automatic && (i&127)==127 && std::chrono::steady_clock::now()-started>=std::chrono::milliseconds(8))break;
  }
  status=automatic?L"Dang chay (loi CPU khong dong bo toc do PS1).":L"Da thuc thi lenh.";
 }
 catch(const std::exception& e){stop(w);std::string message=e.what();status=L"Dung: "+std::wstring(message.begin(),message.end());last_fault=status;}
 if(!automatic||!running||GetTickCount64()-last_refresh>=100){refresh();last_refresh=GetTickCount64();}
}
void loadBios(HWND w){
 wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=file;o.nMaxFile=32768;
 o.lpstrFilter=L"BIOS (*.bin)\0*.bin\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;
 stop(w);
 try {
  std::ifstream f(std::filesystem::path(file),std::ios::binary|std::ios::ate);
  if(!f||f.tellg()!=std::streamoff(512*1024))throw std::runtime_error("BIOS must be exactly 512 KiB");
  auto fresh=std::make_unique<ps1::Bus>();f.seekg(0);
  if(!f.read(reinterpret_cast<char*>(fresh->bios.data()),fresh->bios.size()))throw std::runtime_error("Read failed");
  cpu.reset();bus=std::move(fresh);cpu=std::make_unique<ps1::Cpu>(*bus);exe_loaded=false;has_bios=true;entry=0xbfc00000;cpu->reset(entry);last_fault.clear();status=L"BIOS loaded. Bam Step de kiem tra.";
 }catch(const std::exception&){status=L"Khong nap duoc BIOS. Can tep 512 KiB hop le.";}
 refresh();
}
void openExe(HWND w){
 wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=file;o.nMaxFile=32768;
 o.lpstrFilter=L"PS-X EXE (*.exe;*.ps-exe)\0*.exe;*.ps-exe\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;
 stop(w);
 try{
  std::ifstream f(std::filesystem::path(file),std::ios::binary|std::ios::ate);
  auto size=f.tellg();if(!f||size<0x800||size>4*1024*1024)throw std::runtime_error("EXE file size invalid (max 4 MiB)");
  std::vector<uint8_t> bytes(static_cast<size_t>(size));f.seekg(0);
  if(!f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))throw std::runtime_error("EXE read failed");
  auto fresh=std::make_unique<ps1::Bus>();auto freshCpu=std::make_unique<ps1::Cpu>(*fresh);
  ps1::load_exe(*fresh,*freshCpu,bytes);
  cpu.reset();bus=std::move(fresh);cpu=std::move(freshCpu);entry=cpu->pc;initial_regs=cpu->r;exe_loaded=true;has_bios=false;
  status=L"PS-X EXE loaded. Bam Run. Khong co BIOS services; chi homebrew tu khoi tao.";
 }catch(const std::exception& e){std::string text=e.what();status=L"EXE: "+std::wstring(text.begin(),text.end());}
 refresh();
}
void mountIso(HWND w){
 wchar_t path[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"PS1 disc (*.cue;*.bin;*.iso)\0*.cue;*.bin;*.iso\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;stop(w);
 try{bus->cd.mount(std::filesystem::path(path));status=L"Disc mounted. Boot BIOS CD de chay BIOS voi dia; Boot EXE CD cho homebrew truc tiep.";}
 catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();
}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 switch(m){
 case WM_CREATE:{
  const wchar_t* labels[]={L"Chay demo",L"Nap BIOS",L"Reset CPU",L"Step",L"Run",L"Pause",L"GPU demo",L"Polygon",L"Open EXE",L"Run 10k",L"Save log",L"Mount CD",L"Boot EXE CD",L"Phong hinh",L"Texture demo",L"Boot BIOS CD",L"Region CD",L"Eject CD"};
  for(int i=0;i<18;i++){auto b=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,16+(i%8)*120,16+(i/8)*42,110,36,w,reinterpret_cast<HMENU>(INT_PTR(101+i)),nullptr,nullptr);buttons[i]=b;SendMessageW(b,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);}
  output=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY,16,108,740,400,w,nullptr,nullptr,nullptr);
  screen=CreateWindowW(L"PS1Vram",L"",WS_CHILD|WS_VISIBLE,780,108,640,480,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH,L"Consolas");
  SendMessageW(output,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);layout(w);refresh();return 0;}
 case WM_SIZE:if(output&&screen)layout(w);return 0;
 case WM_GETMINMAXINFO:{auto p=reinterpret_cast<MINMAXINFO*>(lp);p->ptMinTrackSize={640,480};return 0;}
 case WM_COMMAND:
  switch(LOWORD(wp)){
   case 115:{stop(w);exe_loaded=false;entry=0;const auto n=ps1::install_texture_demo(*bus);cpu->reset(0);advance(w,n);status=L"Texture demo: CPU upload -> textured quads, modulation, sprite va blend.";refresh();break;}
   case 114:large_view=!large_view;SetWindowTextW(buttons[13],large_view?L"Hien debug":L"Phong hinh");layout(w);break;
   case 101:stop(w);exe_loaded=false;entry=0;ps1::install_demo(*bus);cpu->reset(entry);advance(w,4);status=L"Demo hoan tat: 7 + 5 = 12. Xem RAM[0x100].";refresh();break;
   case 112:mountIso(w);break;
   case 113:{stop(w);try{ps1::boot_disc(*bus,*cpu,true);entry=cpu->pc;initial_regs=cpu->r;exe_loaded=true;last_fault.clear();start_perf();status=L"Disc EXE loaded directly via SYSTEM.CNF. Bam Run; BIOS kernel services chua khoi tao.";}catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();break;}
   case 116:{stop(w);try{if(!has_bios)throw std::runtime_error("Nap BIOS truoc, sau do Mount CD");ps1::boot_disc(*bus,*cpu,false);exe_loaded=false;entry=cpu->pc;last_fault.clear();start_perf();status=L"BIOS reset voi dia da mount. Bam Run. Retail boot chua xac nhan.";}catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();break;}
   case 117:{stop(w);bus->cd.region=(bus->cd.region+1)%5;const wchar_t* names[]={L"Auto (BOOT prefix)",L"USA",L"Europe",L"Japan",L"Unlicensed"};status=L"GetID region emulated: "+std::wstring(names[bus->cd.region]);refresh();break;}
   case 118:stop(w);bus->cd.eject();status=L"Disc ejected.";refresh();break;
   case 111:{
    stop(w);wchar_t path[32768]=L"ps1_bios_log.txt";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"Text log\0*.txt\0";o.lpstrDefExt=L"txt";o.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
    if(GetSaveFileNameW(&o)){std::ofstream f(std::filesystem::path(path),std::ios::binary);std::string fault;for(wchar_t ch:last_fault)fault.push_back(ch<=127?static_cast<char>(ch):'?');f<<fault<<"\n"<<cpu->diagnostic();status=f?L"Da luu diagnostic log.":L"Khong luu duoc log.";refresh();}break;}
   case 109:openExe(w);break;
   case 110:stop(w);advance(w,10000);break;
   case 108:{stop(w);exe_loaded=false;entry=0;unsigned n=ps1::install_polygon_demo(*bus);cpu->reset(0);advance(w,n);status=L"Polygon demo: tam giac va tu giac mau phang qua CPU -> GP0.";refresh();break;}
   case 107:{stop(w);exe_loaded=false;entry=0;unsigned n=ps1::install_gpu_demo(*bus);cpu->reset(0);advance(w,n);status=L"GPU demo: CPU -> SW -> GP0 -> VRAM. Khung xem 320x240.";refresh();break;}
   case 102:loadBios(w);break;
   case 103:stop(w);cpu->reset(entry);if(exe_loaded)cpu->r=initial_regs;status=L"CPU reset; RAM duoc giu lai.";refresh();break;
   case 104:stop(w);advance(w,1);break;
   case 105:if(!running){start_perf();running=SetTimer(w,1,16,nullptr)!=0;if(running)SetFocus(screen);status=running?L"Dang chay theo lo lenh; khong dong bo toc do PS1.":L"Khong tao duoc timer.";refresh();}break;
   case 106:stop(w);status=L"Da tam dung.";refresh();break;
  }return 0;
 case WM_TIMER:if(wp==1&&running){update_pad(w);advance(w,100000,true);sample_perf();}return 0;
 case WM_DESTROY:stop(w);if(font)DeleteObject(font);PostQuitMessage(0);return 0;
 }return DefWindowProcW(w,m,wp,lp);
}
}
LRESULT CALLBACK vramProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 if(m==WM_LBUTTONDOWN){SetFocus(w);return 0;}
 if(m==WM_KEYDOWN&&wp==VK_ESCAPE){SendMessageW(GetParent(w),WM_COMMAND,106,0);return 0;}
 if(m==WM_PAINT){
  PAINTSTRUCT paint{};HDC dc=BeginPaint(w,&paint);
  const auto view=ps1::display_view(bus->gpu);
  std::vector<uint32_t> pixels(view.width*view.height);
  for(unsigned y=0;y<view.height;y++)for(unsigned x=0;x<view.width;x++)pixels[y*view.width+x]=ps1::display_pixel(bus->gpu,view,x,y);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=static_cast<LONG>(view.width);info.bmiHeader.biHeight=-static_cast<LONG>(view.height);
  info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  RECT r{};GetClientRect(w,&r);SetStretchBltMode(dc,COLORONCOLOR);
  FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  const auto dest=ps1::fit_display(r.right,r.bottom);
  if(dest.width>0&&dest.height>0)StretchDIBits(dc,dest.x,dest.y,dest.width,dest.height,0,0,static_cast<int>(view.width),static_cast<int>(view.height),pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
  EndPaint(w,&paint);return 0;
 }
 return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
 bus=std::make_unique<ps1::Bus>();cpu=std::make_unique<ps1::Cpu>(*bus);ps1::install_demo(*bus);cpu->reset(0);
 WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=h;c.lpszClassName=L"PS1LabWindow";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
 WNDCLASSW v{};v.lpfnWndProc=vramProc;v.hInstance=h;v.lpszClassName=L"PS1Vram";v.hCursor=LoadCursor(nullptr,IDC_ARROW);
 if(!RegisterClassW(&v))return 1;
 if(!RegisterClassW(&c))return 1;
 HWND w=CreateWindowW(c.lpszClassName,L"PS1 Lab v16 - BIN CUE and CD Boot",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1450,710,nullptr,nullptr,h,nullptr);
 if(!w)return 1;ShowWindow(w,show);MSG msg{};int result;
 while((result=GetMessageW(&msg,nullptr,0,0))>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return result==-1?1:static_cast<int>(msg.wParam);
}
