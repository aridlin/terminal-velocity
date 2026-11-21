#pragma once
// Length-prefixed cell frames; gameplay authority stays on the match server.
namespace { namespace party {
class Remote {
  ghosts::Socket fd=ghosts::invalid_socket;
  std::string pending,endpoint_,mode_;
  double retry_at=0;
public:
  CellFrame frame;
  std::string status="Connecting…",code,token;
  unsigned cues=0;int district=0;
  Remote(const std::string &endpoint,const std::string &room,const std::string &mode,const std::string &resume="-"):endpoint_(endpoint),mode_(mode) {
    auto split=endpoint.rfind(':');const auto host=endpoint.substr(0,split);
    const auto port=split==std::string::npos?"27890":endpoint.substr(split+1);
    if(host.empty()||(mode!="race"&&mode!="coop"))throw std::runtime_error("Use --online HOST:PORT --mode race|coop --room new|CODE");
    ghosts::init_sockets();addrinfo hints{},*addresses=nullptr;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(host.c_str(),port.c_str(),&hints,&addresses))throw std::runtime_error("Server address unavailable");
    for(auto a=addresses;a;a=a->ai_next) {
      fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
      if(fd==ghosts::invalid_socket)continue;
#ifdef _WIN32
      u_long one=1;ioctlsocket(fd,FIONBIO,&one);
#else
      fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
#endif
      const int result=connect(fd,a->ai_addr,int(a->ai_addrlen));
      fd_set write;FD_ZERO(&write);FD_SET(fd,&write);timeval timeout{5,0};
      if(result==0||select(int(fd)+1,nullptr,&write,nullptr,&timeout)>0) {
        int error=0;ghosts::SocketLength length=sizeof(error);
        getsockopt(fd,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&length);
        if(!error)break;
      }
      ghosts::close_socket(fd);fd=ghosts::invalid_socket;
    }
    freeaddrinfo(addresses);
    if(fd==ghosts::invalid_socket)throw std::runtime_error("Cannot reach multiplayer server");
    send_line("JOIN "+room+" "+mode+" 1 "+resume);
  }
  ~Remote(){if(fd!=ghosts::invalid_socket)ghosts::close_socket(fd);}
  Remote(const Remote&)=delete;Remote&operator=(const Remote&)=delete;
  void send_line(const std::string &line) {
    const auto bytes=line+'\n';
    #ifdef _WIN32
    constexpr int send_flags=0;
#else
    constexpr int send_flags=MSG_NOSIGNAL;
#endif
    const int n=int(send(fd,bytes.data(),int(bytes.size()),send_flags));
    if(n!=int(bytes.size())) {
      if(!token.empty()){ghosts::close_socket(fd);fd=ghosts::invalid_socket;retry_at=ghosts::seconds()+1;return;}
      throw std::runtime_error("Multiplayer connection stalled");
    }
  }
  void input(unsigned mask,int cols,int rows,bool english) {
    if(fd==ghosts::invalid_socket)return;
    send_line("I "+std::to_string(mask)+" "+std::to_string(std::clamp(cols,34,160))+" "+
      std::to_string(std::clamp(rows,24,60))+" "+std::to_string(int(english)));
  }
  static std::string glyph(uint32_t cp) {
    std::string out;
    if(cp<32||(cp>=127&&cp<160)||(cp>=0xd800&&cp<=0xdfff))cp=32;
    if(cp<128)out+=char(cp);
    else if(cp<2048){out+=char(192|(cp>>6));out+=char(128|(cp&63));}
    else if(cp<65536){out+=char(224|(cp>>12));out+=char(128|((cp>>6)&63));out+=char(128|(cp&63));}
    else if(cp<=0x10ffff){out+=char(240|(cp>>18));out+=char(128|((cp>>12)&63));out+=char(128|((cp>>6)&63));out+=char(128|(cp&63));}
    else out="?";
    return out;
  }
  bool poll() {
    if(fd==ghosts::invalid_socket) {
      if(ghosts::seconds()<retry_at)return false;
      retry_at=ghosts::seconds()+2;
      try {Remote replacement(endpoint_,code,mode_,token);std::swap(fd,replacement.fd);pending.clear();status="Reconnecting…";}
      catch(const std::exception &){return false;}
    }
    char block[65536];
    for(int i=0;i<32;++i) {
      int n=int(recv(fd,block,sizeof(block),0));
      if(n<0) {
#ifdef _WIN32
        const int error=WSAGetLastError();if(error!=WSAEWOULDBLOCK&&error!=WSAEINTR)n=0;
#else
        if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)n=0;
#endif
      }
      if(n==0) {
        ghosts::close_socket(fd);fd=ghosts::invalid_socket;pending.clear();status="Reconnecting…";retry_at=ghosts::seconds()+1;
        if(!frame.empty()){for(auto &cell:frame[1])cell.glyph=" ";const std::string text="Connection lost - reconnecting. Match is paused.";for(size_t x=0;x<text.size()&&x+1<frame[1].size();++x)frame[1][x+1].glyph=text.substr(x,1);}
        return true;
      }
      if(n<0)break;
      pending.append(block,size_t(n));
      if(pending.size()>2000000)throw std::runtime_error("Invalid server frame");
    }
    auto get=[](const char *p){uint32_t v=0;for(int i=0;i<4;++i)v|=uint32_t(static_cast<unsigned char>(p[i]))<<(i*8);return v;};
    bool changed=false;cues=0;
    while(pending.size()>=4) {
      const uint32_t size=get(pending.data());
      if(size>160*60*16+8)throw std::runtime_error("Invalid frame size");
      if(pending.size()<size+4)break;
      auto payload=pending.substr(4,size);pending.erase(0,size+4);
      if(payload.rfind("WELCOME ",0)==0){std::istringstream in(payload);std::string type;int slot;in>>type>>code>>slot>>token;status="Room "+code;continue;}
      if(payload.rfind("ERROR ",0)==0)throw std::runtime_error(payload.substr(6));
      if(payload.size()<8)throw std::runtime_error("Truncated frame");
      const auto cols=get(payload.data());auto rows=get(payload.data()+4);
      if(rows&0x80000000u) {
        rows&=0x7fffffffu;
        if(cols<34||cols>160||rows<24||rows>60)throw std::runtime_error("Invalid packed dimensions");
        std::string expanded=payload.substr(0,8);for(int j=0;j<4;++j)expanded[4+j]=char((rows>>(j*8))&255);
        size_t at=8,cells=0;
        while(at<payload.size()) {
          if(at+18>payload.size())throw std::runtime_error("Truncated cell run");
          const unsigned run=static_cast<unsigned char>(payload[at])|unsigned(static_cast<unsigned char>(payload[at+1]))<<8;
          if(!run||cells+run>cols*rows)throw std::runtime_error("Invalid cell run");
          for(unsigned j=0;j<run;++j)expanded.append(payload,at+2,16);
          at+=18;cells+=run;
        }
        payload=std::move(expanded);
      }
      if(cols<34||cols>160||rows<24||rows>60||payload.size()!=8+cols*rows*16)throw std::runtime_error("Invalid cell dimensions");
      cues|=(get(payload.data()+20)>>1)&511;district=int(get(payload.data()+20)>>24);
      frame.assign(rows,std::vector<Cell>(cols));size_t at=8;
      for(auto &row:frame)for(auto &cell:row){cell.glyph=glyph(get(payload.data()+at));cell.rgb=get(payload.data()+at+4);cell.background=get(payload.data()+at+8);cell.bold=get(payload.data()+at+12)&1;at+=16;}
      changed=true;
    }
    return changed;
  }
};
inline void play_cues(GameAudio &audio,unsigned cues) {
  static constexpr AudioCue kinds[]={AudioCue::Jump,AudioCue::Dash,AudioCue::Pickup,AudioCue::Death,AudioCue::Checkpoint,AudioCue::Land,AudioCue::Door,AudioCue::Step1,AudioCue::Valve};
  for(int bit=0;bit<9;++bit)if(cues&(1u<<bit))audio.play(kinds[bit]);
}
inline unsigned input_mask(const Controls &c) {
  return (c.move<0?1:0)|(c.move>0?2:0)|(c.aim_y<0?4:0)|(c.fast_fall?8:0)|(c.jump_held||c.jump_pressed?16:0)|
    (c.dash_pressed?32:0)|(c.interact?64:0)|(c.inspect?128:0)|(c.confirm?256:0)|(c.pause?512:0)|
    (c.journal?1024:0)|(c.map?2048:0)|(c.restart?8192:0)|(c.mute_audio?65536:0);
}
struct ConnectionArgs {
  std::string endpoint,room="new",mode="race";
  bool english=false;
  explicit ConnectionArgs(int argc,char **argv){iceberg::Profile profile;iceberg::SaveStore().load(profile);english=profile.language==iceberg::Language::English;for(int i=1;i<argc;++i){std::string a=argv[i];if(i+1<argc){if(a=="--language")english=std::string(argv[++i])=="en";else if(a=="--online")endpoint=argv[++i];else if(a=="--room")room=argv[++i];else if(a=="--mode")mode=argv[++i];}}}
};
struct Lobby {
  int selected=0,cursor=0;bool editing=false,submitted=false;
  unsigned previous=0;
  std::string room="100000",error;
  ConnectionArgs args{0,nullptr};
  void tick(unsigned mask) {
    unsigned pressed=mask&~previous;previous=mask;
    int direction=int(bool(pressed&8))-int(bool(pressed&4));
    if(editing) {
      if(pressed&1)cursor=(cursor+5)%6;
      if(pressed&2)cursor=(cursor+1)%6;
      if(direction)room[size_t(cursor)]=char('0'+(room[size_t(cursor)]-'0'+direction+10)%10);
    } else if(direction)selected=(selected+direction+4)%4;
    if(pressed&128){editing=false;return;}
    if(pressed&256) {
      if(selected>=2&&!editing){editing=true;return;}
      args.endpoint="het.aridlin.pl:27890";
      std::ifstream endpoint_file(audio_directory().parent_path().parent_path()/"multiplayer-endpoint");
      std::string configured;if(endpoint_file>>configured&&configured.size()<128&&configured.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:-")==std::string::npos)args.endpoint=configured;
      args.mode=selected%2?"coop":"race";
      args.room=selected<2?"new":room;submitted=true;
    }
  }
  CellFrame draw(int cols=76,int rows=34) const {
    cols=std::clamp(cols,34,160);rows=std::clamp(rows,24,60);
    CellFrame frame(rows,std::vector<Cell>(cols));
    for(auto &row:frame)for(auto &cell:row){cell.background=0x101923;cell.rgb=0xd5e8ed;}
    auto line=[&](int y,const std::string &text){int x=2;for(size_t i=0;i<text.size()&&x<cols-2;){const unsigned char c=text[i];const size_t n=c<128?1:c<224?2:c<240?3:4;frame[y][x++].glyph=text.substr(i,n);i+=n;}};
    line(2,"MULTIPLAYER");
    const char *choices[]={"Create race / Utwórz wyścig","Create co-op / Utwórz współpracę","Join race / Dołącz do wyścigu","Join co-op / Dołącz do współpracy"};
    for(int i=0;i<4;++i)line(5+i*2,std::string(selected==i?"> ":"  ")+choices[i]);
    line(14,"Room / Pokój: "+room);
    if(editing)line(15,std::string(14+cursor,' ')+"^");
    line(18,editing?"W/S: digit  A/D: cursor  ENTER: join  B: back":"W/S: choose  ENTER: confirm  Q/M: leave");
    line(21,error);
    return frame;
  }
};
inline void ansi_frame(const CellFrame &frame) {
  std::string out="\x1b[H";uint32_t fg=~0u,bg=~0u;
  for(size_t y=0;y<frame.size();++y){out+="\x1b["+std::to_string(y+1)+";1H";for(const auto &cell:frame[y]){
    if(cell.rgb!=fg||cell.background!=bg){fg=cell.rgb;bg=cell.background;out+="\x1b[0;38;2;"+std::to_string(fg>>16)+";"+std::to_string((fg>>8)&255)+";"+std::to_string(fg&255)+";48;2;"+std::to_string(bg>>16)+";"+std::to_string((bg>>8)&255)+";"+std::to_string(bg&255)+"m";}
    out+=cell.glyph;
  }}std::cout<<out<<std::flush;
}
inline std::optional<ConnectionArgs> terminal_lobby() {
  TerminalGuard terminal(true);TuiInput input(InputMode::Ssh);Lobby lobby;
  if(!terminal.active())return {};
  while(!stop_requested) {
    auto controls=input.poll(.016);if(controls.quit||controls.menu_request)return {};
    lobby.tick(input_mask(controls));const auto size=term_size();ansi_frame(lobby.draw(size.cols,size.rows));
    if(lobby.submitted)return lobby.args;
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  return {};
}
#ifndef __EMSCRIPTEN__
inline int terminal_client(const ConnectionArgs &args) {
  Remote client(args.endpoint,args.room,args.mode);
  TerminalGuard terminal(true);TuiInput input(InputMode::Ssh);
  if(!terminal.active())throw std::runtime_error("Online play needs a terminal or --terminal-view");
  GameAudio audio;audio.init(audio_directory(),std::getenv("SSH_CONNECTION")!=nullptr||std::getenv("SSH_TTY")!=nullptr);
  auto last=std::chrono::steady_clock::now(),send_at=last;
  unsigned edges=0;
  std::cout<<"Connecting to multiplayer…\r\n"<<std::flush;
  while(!stop_requested) {
    auto now=std::chrono::steady_clock::now();double dt=std::chrono::duration<double>(now-last).count();last=now;
    const Controls controls=input.poll(std::min(dt,.1));if(controls.quit||controls.menu_request)return 0;
    if(controls.mute_audio)audio.muted=!audio.muted;
    const unsigned mask=input_mask(controls);edges|=mask&~31u;
    if(now>=send_at){const auto size=term_size();client.input(input.focused?(mask&31u)|edges:0,size.cols,size.rows,args.english);edges=0;send_at=now+std::chrono::milliseconds(16);}
    if(client.poll()) {
      audio.set_environment(client.district);play_cues(audio,client.cues);
      std::string out="\x1b[H";uint32_t fg=~0u,bg=~0u;
      for(size_t y=0;y<client.frame.size();++y){out+="\x1b["+std::to_string(y+1)+";1H";for(auto &cell:client.frame[y]){
        if(cell.rgb!=fg||cell.background!=bg){fg=cell.rgb;bg=cell.background;out+="\x1b[0;38;2;"+std::to_string(fg>>16)+";"+std::to_string((fg>>8)&255)+";"+std::to_string(fg&255)+";48;2;"+std::to_string(bg>>16)+";"+std::to_string((bg>>8)&255)+";"+std::to_string(bg&255)+"m";}
        out+=cell.glyph;
      }}std::cout<<out<<std::flush;
    }
    audio.update(AudioScene::Play,2,std::min(dt,.1),input.focused);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return 0;
}
#endif
} }
