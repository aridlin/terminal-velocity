// AdventureSession members. Every task transition follows an interaction or
// the courier's real collision state; switches already have atomic save and
// checkpoint persistence, so no separate unsaved mission state is needed.
  bool harbor_interact(const Interaction &node) {
    if(node.kind==InteractionKind::DockWorker) {
      unlock_achievement(iceberg::Achievement::DockChat);
      static constexpr iceberg::Text lines[]={
        {"Mewy znów polują na mój obiad.","The gulls are after my lunch again."},
        {"Nie wszystko, co przecieka, jest statkiem.","Some leaks don't even come with a boat."},
        {"Wychodzi na to, że lód też skrzypi.","Even the ice creaks around here."},
        {"Herbata stygnie. Skrzynki poczekają.","My tea's going cold. The crates can wait."},
        {"Dziś wiatr pachnie mokrym drewnem.","The wind smells of wet wood today."},
        {"Jeszcze trzy skrzynki. Chyba.","Three crates left. I think."}
      };
      constexpr int count=int(std::size(lines));
      int line=dock_chatter.below(last_dock_line<0?count:count-1);
      if(last_dock_line>=0&&line>=last_dock_line)++line;
      last_dock_line=line;
      notice(t("Doker: ","Dockworker: ")+lines[line].get(profile.language),4);
      return true;
    }
    auto &v=state.adventure;
    if(iceworld::harbor_v4_room(grid.template_id)&&node.kind==InteractionKind::Task&&node.id==0) {
      const uint64_t item=uint64_t(1)<<node.detail;
      if(profile.task_items[0]&item)notice(t("Te listy już wysłano.","These letters have already been sent."));
      else if(grid.switch_flags&(uint64_t(1)<<20))notice(t("Listy są w torbie. Skrzynka czeka przy wyjściu.","The letters are in your bag. The postbox is by the exit."),5);
      else {
        grid.switch_flags|=uint64_t(1)<<20;animate_pickup(node,0);
        notice(t("LISTY W TORBIE — znajdź drogę do skrzynki przy wyjściu.","LETTERS BAGGED — find your way to the postbox by the exit."),6);
        audio.play(AudioCue::Paper);persist();
      }
      sync_view();return true;
    }
    if(grid.template_id==310&&node.kind==InteractionKind::Rescue) {
      if((grid.switch_flags&5)!=5) {
        notice(t("BOGDAN: Dociśnij oba styki zasilania na górnych pomostach. Stąd widzę, który jeszcze nie świeci.",
          "BOGDAN: Step on both power contacts on the upper gangways. I can see which one is still dark."),7);
      } else if(!(grid.switch_flags&8)) {
        grid.switch_flags|=8;activate_machinery(node,false);v.machinery_kind=1;
        v.reacting_npc=2;v.npc_reaction_left=1.6;
        talk({{2,{"Jest prąd. Jedźmy dolnym torem: lampa A, potem B, potem przystanek. Idź przodem — z kabiny nie widzę krawędzi.",
          "Power's back. Take the lower track: lamp A, then B, then the stop. Lead the way — I can't see the edges from the cab."}}});
        audio.play(AudioCue::Motor);persist();
      } else notice(iceworld::harbor_objective(310,grid.switch_flags).get(profile.language),5);
      sync_view();return true;
    }
    if(grid.template_id==311) {
      if(node.kind==InteractionKind::Valve) {
        if(!(grid.switch_flags&1))notice(t("Najpierw zabierz termometr przy wejściu.","Collect the thermometer by the entrance first."));
        else if(!(grid.switch_flags&4)) {
          grid.switch_flags|=4;activate_machinery(node);animate_pickup(node,9);
          notice(t("ODCZYT: rura 8°, wskaźnik 16°. Zanieś Witkowi termometr i tę różnicę.",
            "READING: pipe 8°, dial 16°. Bring Witek the thermometer and this difference."),8);
          audio.play(AudioCue::Valve);persist();
        } else notice(t("Odczyt zapisany: osiem stopni różnicy.","Reading recorded: an eight-degree difference."));
        sync_view();return true;
      }
      if(node.kind==InteractionKind::Deliver&&(grid.switch_flags&1)&&!(grid.switch_flags&4)) {
        notice(t("WITEK: Sam przyrząd nie wystarczy. Porównaj odczyt z ciepłą rurą pośrodku drogi.",
          "WITEK: I need a reading too. Compare the warm pipe halfway along the route."),7);return true;
      }
    }
    if(grid.template_id==312) {
      if(node.kind==InteractionKind::Note) {
        grid.switch_flags|=16;
        talk({{1,{"Dopływ: 16. Powrót mieszkań: 8. Najpierw otwórz obejście, dopiero potem pompę. Ciepłą wodę skieruj do znaku domu; zrzut z falą prowadzi prosto do morza.",
          "Inlet: 16. Homes' return: 8. Open the bypass before the pump. Send the warm water to the house symbol; the wave outlet drains straight into the sea."}}});
        audio.play(AudioCue::Paper);persist();sync_view();return true;
      }
      if(node.kind==InteractionKind::Valve) {
        if(!(grid.switch_flags&16)) {
          notice(t("Manometry przy wejściu wskazują kolejność. Najpierw sprawdź ciśnienie.",
            "The entry gauges show the order. Check the pressure first."),6);return true;
        }
        if(node.id==0&&!(grid.switch_flags&2)) {
          notice(t("Pompa stawia opór. Otwórz obejście, żeby wyrównać ciśnienie.",
            "The pump resists. Open the bypass to equalise the pressure."),6);
          audio.play(AudioCue::Steam);return true;
        }
        if(node.id>=2&&(grid.switch_flags&3)!=3) {
          notice(t("W rurze nie ma przepływu. Obejście, potem pompa.","There is no flow. Bypass first, then pump."),5);return true;
        }
        if(node.id==3) {
          grid.switch_flags=(grid.switch_flags|8)&~uint64_t(4);
          activate_machinery(node);audio.play(AudioCue::Steam);
          notice(t("Woda ucieka do morza. Wróć do zaworu ze znakiem domu — możesz jeszcze skierować ciepło do mieszkań.",
            "Water is escaping into the sea. Return to the house valve — you can still send the heat to the homes."),8);
        } else {
          const auto bit=uint64_t(1)<<node.id;
          if(grid.switch_flags&bit){notice(t("Ten obieg już pracuje.","This circuit is already running."));return true;}
          grid.switch_flags|=bit;if(node.id==2)grid.switch_flags&=~uint64_t(8);
          activate_machinery(node);audio.play(AudioCue::Valve);
          if(node.id==1)notice(t("Ciśnienie wyrównane. Zasuwa serwisowa otwarta.","Pressure equalised. Service shutter open."),5);
          else if(node.id==0)notice(t("Pompa pracuje. Wybierz, dokąd popłynie ciepło.","Pump running. Choose where the heat goes."),5);
          else {
            for(auto &bridge:grid.growth)bridge.amount=0;
            notice(t("WITEK: Powrót rośnie. Mieszkania dostają ciepło! Most serwisowy się wysuwa.",
              "WITEK: The return is warming. The homes have heat! The service bridge is extending."),7);
          }
        }
        persist();sync_view();return true;
      }
    }
    return false;
  }
  void harbor_step(const Player &,int old_deaths) {
    if(state.deaths!=old_deaths)return;
    bool changed=false;
    if(grid.template_id==310) {
      auto on_deck=[&](double x,double y,double width) {
        const auto &p=state.player;
        return p.grounded&&p.x+p.rect().w>x&&p.x<x+width&&std::abs(p.y+kPlayerHeight-y)<.6;
      };
      auto contact=[&](uint64_t flag,bool touched,const char *pl,const char *en) {
        if(touched&&!(grid.switch_flags&flag)) {
          grid.switch_flags|=flag;changed=true;
          notice(t(pl,en),5);
          audio.play(flag==16||flag==32||flag==2?AudioCue::Talk2:AudioCue::Valve);
          state.adventure.lamp_left=1.;
        }
      };
      contact(1,on_deck(57,30,18),"PIERWSZY STYK — zasilanie wraca do toru.","FIRST CONTACT — power returns to the track.");
      contact(4,on_deck(107,38,18),"DRUGI STYK — tramwaj ma zasilanie. Zawołaj Bogdana.","SECOND CONTACT — the tram has power. Call Bogdan.");
      if(grid.switch_flags&8) {
        contact(16,on_deck(112,47,19),"BOGDAN: Lampa A. Widzę cię. Jedziemy dalej.","BOGDAN: Lamp A. I can see you. Keep going.");
        if(grid.switch_flags&16)contact(32,on_deck(62,55,19),"BOGDAN: Lampa B. Jeszcze zakręt pod zadaszenie.","BOGDAN: Lamp B. One more turn under the shelter.");
        if(grid.switch_flags&32) {
          const auto previous=grid.switch_flags;
          contact(2,on_deck(12,48,19),"BOGDAN: Na miejscu. Teraz ja zawiozę ludzi, kiedy otworzysz dalszą trasę.",
            "BOGDAN: Made it. I'll carry the residents once you open the line ahead.");
          if(previous!=grid.switch_flags){audio.play(AudioCue::Win);state.adventure.reacting_npc=2;state.adventure.npc_reaction_left=2.;}
        }
      }
    }
    const int item=iceworld::harbor_mail_item(grid.template_id);
    if(item>=0&&(grid.switch_flags&(uint64_t(1)<<20))) {
      Rect box=grid.goal;box.x-=3;box.w+=6;
      if(state.player.rect().intersects(box)) {
        const auto mask=uint64_t(1)<<item;
        if(!(profile.task_items[0]&mask)) {
          profile.task_items[0]|=mask;++profile.quest_counts[0];
          if(profile.quests[0]==1&&profile.quest_counts[0]>=unsigned(iceberg::Quests[0].required))profile.quests[0]=2;
          notice(t("LISTY WYSŁANE — ","LETTERS SENT — ")+std::to_string(profile.quest_counts[0])+"/3",5);
          audio.play(AudioCue::Paper);audio.play(AudioCue::Pickup);changed=true;
        }
        grid.switch_flags&=~(uint64_t(1)<<20);
      }
    }
    if(changed)persist();
  }
