  bool has_side_flag(iceberg::SideFlag flag) const {
    return (profile.task_items[1]&iceberg::side_flag(flag))!=0;
  }
  void remember_side_flag(iceberg::SideFlag flag) {
    profile.task_items[1]|=iceberg::side_flag(flag);
  }
  void place_side_stories() {
    using iceberg::SideStory;
    const auto add=[&](Rect area,SideStory story,iceberg::Text label) {
      grid.interactions.push_back({area,InteractionKind::SideStory,int(story),label});
    };
    if(grid.template_id==100||grid.template_id==600) {
      add({2,44,2,4},SideStory::HarborStub,{"Bloczek pocztowy","Postal counterfoil"});
    } else if(grid.template_id==101) {
      add({29,44,3,4},SideStory::HeatworksMug,{"Sprawdź kubek na rurze","Check the mug on the pipe"});
    } else if(grid.template_id==102) {
      add({34,44,4,4},SideStory::EstatePlate,{"Blaszka pod schodami","Plate under the stairs"});
      add({104,30,4,4},SideStory::Casting,{"Podnieś odlew","Pick up the casting"});
      if(has_side_flag(iceberg::SideFlag::Roster))
        add({60,44,4,4},SideStory::RosterDesk,{"Pokaż Grażynie listę","Show Grażyna the roster"});
      grid.interactions.push_back({{67,44,3,4},InteractionKind::Secret,iceberg::UnnumberedFloor,
        {"Drzwi pod schodami","Door under the stairs"}});
    } else if(grid.template_id==103) {
      add({33,44,4,4},SideStory::ArchiveCopy,{"Odpis z teczki 11","Copy from folder 11"});
      add({84,44,4,4},SideStory::EmergencyBell,{"Dzwonek alarmowy","Emergency bell"});
    } else if(grid.template_id==104) {
      add({104,46,4,4},SideStory::GardensPot,{"Podnieś odwróconą doniczkę","Lift the overturned pot"});
    } else if(grid.template_id==105) {
      add({97,30,4,4},SideStory::CrownPunch,{"Sprawdź kasownik","Test the ticket punch"});
    }
  }
  struct SideDocument {
    iceberg::SideFlag flag;
    iceberg::Text title, body;
  };
  static std::optional<SideDocument> side_document(iceberg::SideStory story) {
    using iceberg::SideFlag;
    using iceberg::SideStory;
    switch(story) {
      case SideStory::HarborStub:
        return SideDocument{SideFlag::HarborStub,{"Bloczek pocztowy","Postal counterfoil"},
          {"ZWROT 11 — dozorczyni: najpierw pociągnąć w dół, potem w lewo. Nie doręczać po 00:07.",
           "RETURN 11 — caretaker: pull down, then left. Do not deliver after 00:07."}};
      case SideStory::EstatePlate:
        return SideDocument{SideFlag::EstatePlate,{"Blaszka pod schodami","Plate under the stairs"},
          {"Pod numerem 11: rysa w dół, druga w lewo. Zawiasy są na ścianie obok.",
           "Under number 11: a scratch down, a second to the left. The hinges are on the wall beside it."}};
      case SideStory::ArchiveCopy:
        return SideDocument{SideFlag::ArchiveCopy,{"Odpis z teczki 11","Copy from folder 11"},
          {"TEKA 11 — zatarte polecenie: w dół, potem w lewo. Zlecenie wpłynęło po zakończeniu akcji.",
           "FOLDER 11 — erased instruction: down, then left. Order received after the operation ended."}};
      case SideStory::Roster:
        return SideDocument{SideFlag::Roster,{"Lista lokatorów","Tenant roster"},
          {"TRZYNAŚCIE NAZWISK. Czternasty wiersz jest wgnieciony od ołówka: «Zosia, zastępcza kołyska w pralni».",
           "THIRTEEN NAMES. Pencil marks dent the fourteenth line: ‘Zosia, spare cot in the laundry.’"}};
      default:return std::nullopt;
    }
  }
  iceberg::Text roster_desk_title() const {
    using iceberg::SideFlag;
    if(has_side_flag(SideFlag::RosterPosted))return {"Sprostowanie","Correction notice"};
    if(has_side_flag(SideFlag::RosterSealed))return {"Zapieczętowany odpis","Sealed copy"};
    return {"Lista lokatorów","Tenant roster"};
  }
  iceberg::Text roster_desk_body() const {
    using iceberg::SideFlag;
    if(has_side_flag(SideFlag::RosterPosted))
      return {"KLATKA 11 · Obecnych: 14. Drzwi zamknięto przed zakończeniem obchodu. Podpisano: Grażyna.",
              "STAIRWELL 11 · Present: 14. The door was shut before the count finished. Signed: Grażyna."};
    if(has_side_flag(SideFlag::RosterSealed))
      return {"Koperta jest zapieczętowana. Na wierzchu: «Do rąk własnych. Chcesz porozmawiać? Będę przy biurku. Grażyna».",
              "The envelope is sealed. On the front: ‘For you alone. If you want to talk, I'll be by my desk. Grażyna.’"};
    return {"E — pokaż Grażynie odpis znaleziony pod schodami.",
            "E — show Grażyna the copy you found below the stairs."};
  }
  bool side_story_interact(const Interaction &node) {
    using iceberg::Achievement;
    using iceberg::SideFlag;
    using iceberg::SideStory;
    if(node.kind==InteractionKind::Npc&&grid.template_id==102&&node.id==3&&
       has_side_flag(SideFlag::Casting)&&!has_side_flag(SideFlag::CastingReturned)) {
      remember_side_flag(SideFlag::CastingReturned);
      unlock_achievement(Achievement::SiliconeReturn);
      talk({{3,{"Tak, to nasz silikonowy odbojnik. Nie. Nie wiem, kto zatwierdził ten kształt.",
                  "Yes, that's our silicone doorstop. No, I don't know who approved that shape."}},
            {3,{"Wpiszę «część gumowa». Jemu też oszczędzimy formularza o zmianę nazwiska.",
                  "I'll put down ‘rubber fitting’. One of us should be spared a name-change form."}}});
      persist();
      return true;
    }
    if(node.kind!=InteractionKind::SideStory)return false;
    const auto story=SideStory(node.id);
    if(const auto document=side_document(story)) {
      if(!has_side_flag(document->flag)) {
        remember_side_flag(document->flag);
        persist();
      }
      inspect_nearby();
      return true;
    }
    switch(story) {
      case SideStory::RosterDesk:
        if(has_side_flag(SideFlag::RosterPosted)||has_side_flag(SideFlag::RosterSealed))inspect_nearby();
        else talk({{3,{"Poznaję ten ślad po ołówku. Pilnowałam Zosi w pralni, kiedy jej mama poszła po wodę.",
                       "I know that pencil mark. I watched Zosia in the laundry while her mother went for water."}},
                   {3,{"Miałam klucz. Kiedy dyżurny powiedział, że wszyscy wyszli, przekręciłam go. Rano nikt nie chciał dopisać dziecka bez numeru mieszkania.",
                       "I had the key. When the dispatcher said everyone was out, I turned it. In the morning nobody would add a child without a flat number."}},
                   {3,{"Zosia przeszła z matką do piekarni przez pralnię. W papierach nigdy nie weszła do budynku. Co robimy z odpisem?",
                       "Zosia and her mother got to the bakery through the laundry. On paper she never entered the building. What should we do with the copy?"}}},
                  0,iceberg::EstateRosterReplyScene);
        break;
      case SideStory::Casting:
        remember_side_flag(SideFlag::Casting);
        animate_pickup(node,0);
        notice(t("Miękki, cielisty odlew. Na spodzie ktoś napisał GRAŻYNA.",
          "A soft, flesh-coloured casting. Somebody wrote GRAŻYNA underneath."),6);
        persist();
        break;
      case SideStory::Intercom:
        audio.play(AudioCue::Spark,.45f);
        talk({{-1,{"Trzy uderzenia od środka. Za trzecim razem słuchawka powtarza pierwsze dwa.",
                    "Three knocks from inside. On the third, the receiver repeats the first two."}},
              {-1,{"Na zawieszce: «Jeśli słyszysz swój głos, odłóż słuchawkę. Prawdziwy dyżurny nie zna twojego imienia».",
                    "A tag reads: ‘If you hear your own voice, hang up. The real dispatcher doesn't know your name.’"}}});
        break;
      case SideStory::Tape:
        audio.play(AudioCue::Rumble,.35f);
        talk({{-1,{"TAŚMA 11 · 00:07. Wentylator zwalnia. Dwa głosy siedzą tuż przy mikrofonie.",
                    "TAPE 11 · 00:07. The fan slows. Two voices are close to the microphone."}},
              {-1,{"DOZORCZYNI: Czternaście. Doliczyłam niemowlę. Proszę wpisać czternaście.",
                    "CARETAKER: Fourteen. I counted the baby. Please write down fourteen."}},
              {-1,{"DYŻURNY: Mam trzynaście formularzy. Zaraz przyniosę czternasty. Jeszcze nie zamykaj.",
                    "DISPATCHER: I have thirteen forms. I'll bring a fourteenth. Don't shut it yet."}},
              {-1,{"GŁOS Z GÓRY: Macie trzynaście. Grzejcie trzynaście. Drzwi o 00:07 same podadzą liczbę.",
                    "VOICE FROM ABOVE: You have thirteen. Heat thirteen. At 00:07 the door will give you the number."}},
              {-1,{"Wentylator staje. Od drugiej strony ktoś dyktuje twoje kroki. Liczy o jeden do przodu.",
                    "The fan stops. Someone on the other side calls out your footsteps. They're one step ahead."}}},FinishBasementTape);
        break;
      case SideStory::EmergencyBell:
        audio.play(AudioCue::Metal2,.8f);
        if(has_side_flag(SideFlag::BellRung)) {
          unlock_achievement(Achievement::RingAgain);
          notice(t("GŁOŚNIK: Drugie wezwanie oznacza, że pierwsze już zostało przyjęte.", 
            "SPEAKER: A second call means your first has already been received."),6);
        } else {
          remember_side_flag(SideFlag::BellRung);
          notice(t("GŁOŚNIK: Dzwonek działa. Prosimy nie sprawdzać.", "SPEAKER: The bell works. Please refrain from checking."),6);
        }
        persist();
        break;
      case SideStory::HeatworksMug:
        audio.play(AudioCue::Cloth,.6f);
        notice(t("Zimna herbata. Pod kubkiem karteczka: «Ciepły obieg odkręcaj po nocnej zmianie. Witek nie śpi od wtorku».",
          "Cold tea. A note under the mug: ‘Open the hot circuit after the night shift. Witek hasn't slept since Tuesday.’"),7);
        break;
      case SideStory::GardensPot:
        audio.play(AudioCue::Stone1,.55f);
        notice(t("Pod doniczką ślimak i tabliczka: «Najemca tymczasowy. Proszę podlewać obok».",
          "Under the pot, a snail and a sign: ‘Temporary tenant. Please water around it.’"),6);
        break;
      case SideStory::CrownPunch:
        audio.play(AudioCue::Metal1,.6f);
        notice(t("Kasownik wybija datę na pustym bilecie. Jutrzejszą, bez roku.",
          "The punch stamps a blank ticket. Tomorrow's date, with the year missing."),6);
        break;
      default:return false;
    }
    return true;
  }
