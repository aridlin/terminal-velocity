  void draw_jubilee_monument() {
    const auto &d=iceberg::Districts[1];
    world_art(89,14,{
      "             ▄██▄              ",
      "            ▐█┼██▌             ",
      "            ██████             ",
      "          ▄█▀▀▀▀▀▀█▄           ",
      "          █ ▄    ▄ █       ┼   ",
      "          █   ▵    █       │   ",
      "          ▀█▄ ── ▄█▀       │   ",
      "      ▄▄▄███▀████▀███▄▄▄   │   ",
      "    ▄██▀    ╲  ╱    ▀██▄  ▄█▄  ",
      "   ██        ╲╱        ██ ███  ",
      "  ██          ┼         ████   ",
      "  ██          │          █▀│   ",
      "  ██   ░      │      ░   █ │   ",
      "  ██   ░      │      ░   █ │   ",
      "  ██   ░      │      ░   █ │   ",
      "  ██   ░      │      ░   █ │   ",
      "  ██▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█ │   ",
      "  ╔══════════════════════╗     ",
      "  ║     JAN PAWEŁ II      ║     ",
      "  ║       21,37 MW       ║     ",
      "  ╚══════════════════════╝     "},0xded8bd);
    world_art(77,33,{"╔═══════════════════════════════════════════════════════╗",
      "║   POMPA IM. ŚW. JANA PAWŁA II  ·  DAR MIESZKAŃCÓW       ║",
      "╠═══════════════════════════════════════════════════════╣",
      "║            ◉    ┌───────────────┐    ◉                ║",
      "║═════════════════╡    21,37 MW   ╞═════════════════════║",
      "║            ◉    └───────────────┘    ◉                ║",
      "╚═══════════════════════════════════════════════════════╝"},d.stone);
    world_art(25,43,{"       ╭──────────────────╮","       │       ◉          │","       ╰────────┬─────────╯",
      "       ╔════════╧═════════╗","       ║                 ║","═══════╩═════════════════╩═══════"},0x789aa3);
    if(!(grid_.switch_flags&1)) {
      for(int jet=0;jet<3;++jet) {
        double x=29.+jet*2.6;
        world_text(x,40.8-std::sin(state_.elapsed*4+jet)*.4,"⠇",0x83b8c8);
        world_text(x-.4,41.7,"╱│╲",0x608c9f);
      }
      world_art(30,46,{tr("TRYB UROCZYSTY","CEREMONIAL MODE")},d.accent);
    } else world_art(30,46,{tr("FONTANNA: STOP","FOUNTAIN: OFF")},d.light);
    world_art(143,23,{"╭──────────────╮",tr("│ OBIEG ZWROTNY│","│ RETURN LOOP  │"),"╰──────┬───────╯","       ↓"},d.accent);
    world_art(104,14,{"╭─────────────────╮",tr("│   DO MIESZKAŃ    │","│   TO THE HOMES   │"),"╰────────┬────────╯","         ↓"},d.accent);
    if((grid_.switch_flags&7)==7) {
      world_art(8,24,{tr("CIEPŁO: MIESZKANIA","HEAT: HOMES"),"═══════════════════"},0xf1c66f);
      world_art(92,32,{tr("NIEZATWIERDZONY CUD","UNAUTHORISED MIRACLE")},d.accent);
    }
    for(const auto &p:grid_.platforms) {
      auto r=p.at(state_.elapsed);
      world_art(r.x+4,r.y+2,{"│   │","│   │","│   │","╧═══╧"},d.shadow);
    }
  }
