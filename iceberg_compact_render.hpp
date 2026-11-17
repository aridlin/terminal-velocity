// Compact cell layouts are shared by all frontends. No browser dialogue UI.
  void compact_paragraph(int &row,const std::string &value,int bottom,uint32_t color=0xeee7d6) {
    for(const auto &line:wrap(value,cols_-4))if(row<bottom)paint_text(row++,2,line,color);
  }
  void draw_iceberg_compact() {
    const auto &v=state_.adventure;
    const auto &d=iceberg::Districts[size_t(grid_.district)];
    fill(0,0,cols_,4,Color::Default);
    paint_text(0,1,d.name.get(v.language),d.accent,true);
    int y=1;compact_paragraph(y,v.room_name,3,d.light);
    for(int x=0;x<cols_;++x)paint_text(3,x,"─",d.stone);
    fill(rows_-3,0,cols_,3,Color::Default);
    paint_text(rows_-3,1,tr("E użyj · B obejrzyj · P pauza","E use · B inspect · P pause"),d.light);
    y=rows_-2;compact_paragraph(y,v.notice_left>0?v.notice:v.prompt,rows_,d.accent);
    auto panel=[&](const std::string &title){frame(0,0,cols_,rows_,d.stone);paint_text(1,2,title,d.accent,true);};
    if(state_.menu) {
      panel("TERMINAL VELOCITY");
      if(v.confirm_new) {
        y=5;compact_paragraph(y,tr("ZACZĄĆ OD NOWA?","START OVER?"),rows_-3,d.accent);++y;
        compact_paragraph(y,tr("Zastąpi to zapis obecnej podróży. Rekordy i stroje zostaną.","This replaces the current journey. Records and outfits are kept."),rows_-3);
        paint_text(rows_-2,2,tr("ENTER tak · P anuluj","ENTER yes · P cancel"),d.light);return;
      }
      if(v.settings) {
        paint_text(4,2,tr("DŹWIĘK I JĘZYK","SOUND AND LANGUAGE"),d.accent);
        paint_text(7,2,tr("1 Muzyka: ","1 Music: ")+std::to_string(int(v.music_volume*100))+"%",d.light);
        paint_text(9,2,tr("2 Efekty: ","2 Effects: ")+std::to_string(int(v.effects_volume*100))+"%",d.light);
        paint_text(11,2,tr("3 Otoczenie: ","3 Ambience: ")+std::to_string(int(v.ambience_volume*100))+"%",d.light);
        paint_text(13,2,tr("4 Strój: ","4 Outfit: ")+std::to_string(v.outfit+1),d.light);
        paint_text(15,2,tr("L Język: POLSKI","L Language: ENGLISH"),d.light);
        paint_text(rows_-2,2,tr("ENTER wróć","ENTER return"),d.accent);return;
      }
      paint_text(3,2,tr("POD POWIERZCHNIĄ","BENEATH THE SURFACE"),d.light);
      const std::array<std::string,6> labels{
        v.can_continue?tr("Kontynuuj podróż","Continue journey"):tr("Rozpocznij podróż","Begin journey"),
        tr("Nowa historia","New story"),tr("Wyprawa bez końca","Endless expedition"),tr("Próba mistrzowska","Mastery trial"),
        tr("Dźwięk i język","Sound and language"),tr("Trasy klasyczne","Classic courses")};
      for(int i=0;i<6;++i)paint_text((rows_<24?4+i:5+i*2),2,(v.selected==i?"▸ ":"  ")+std::to_string(i+1)+" "+labels[size_t(i)],v.selected==i?d.accent:0xc8bbb8);
      if(v.selected==2||v.selected==3){y=rows_<24?12:18;compact_paragraph(y,"J: "+iceberg::Districts[size_t(v.run_district)].name.get(v.language),rows_-3,d.light);}
      if(v.notice_left>0){y=rows_<24?14:20;compact_paragraph(y,v.notice,rows_-3,d.accent);}
      paint_text(rows_-3,2,tr("↑↓ wybór · ENTER graj","↑↓ choose · ENTER play"),d.light);
      paint_text(rows_-2,2,tr("R nowa wyprawa · L język","R new expedition · L language"),d.light);return;
    }
    if(state_.journal||v.map) {
      panel(v.map?tr("DZIELNICE","DISTRICTS"):tr("DZIENNIK KURIERA","COURIER'S JOURNAL"));
      if(v.map) {
        for(int i=5;i>=0;--i)paint_text(4+(5-i)*(rows_<24?1:2),2,(v.selected==i?"▸ ":"  ")+iceberg::Districts[size_t(i)].name.get(v.language),i==v.selected?d.accent:i<=v.unlocked?d.light:d.stone);
        y=rows_<24?11:17;compact_paragraph(y,iceberg::Districts[size_t(std::clamp(v.selected,0,5))].objective.get(v.language),rows_-3);
      } else {
        int q=std::clamp(v.selected,0,7);const auto &quest=iceberg::Quests[size_t(q)];
        y=4;compact_paragraph(y,std::to_string(q+1)+" / 8",rows_-3,d.stone);++y;
        compact_paragraph(y,v.quests[size_t(q)]?quest.title.get(v.language):tr("Niepoznana historia","An untold story"),rows_-3,d.accent);++y;
        if(v.quests[size_t(q)]) {
          compact_paragraph(y,(v.quests[size_t(q)]==3?quest.reward:quest.objective).get(v.language),rows_-5);++y;
          compact_paragraph(y,v.quests[size_t(q)]==3?tr("UKOŃCZONE","COMPLETE"):std::to_string(v.quest_counts[size_t(q)])+" / "+std::to_string(quest.required),rows_-3,d.accent);
        }
      }
      paint_text(rows_-3,2,v.map?tr("↑↓ wybór · ENTER podróż","↑↓ select · ENTER travel"):tr("↑↓ wybór · E czytaj","↑↓ select · E read"),d.light);
      paint_text(rows_-2,2,tr("P wróć","P return"),d.light);
    }
    if(state_.paused) {
      panel(state_.paused?tr("CHWILA ODDECHU","A MOMENT TO BREATHE"):tr("DROGA OTWARTA","THE WAY IS OPEN"));
      y=6;compact_paragraph(y,state_.paused?tr("Postęp zapisuje się przy lampach i wyjściu.","Progress saves at lamps and when you leave."):tr("R wróć do lampy, jeśli coś zostało z tyłu.","R returns to the lamp if you left something behind."),rows_-3);
      paint_text(rows_-2,2,tr("ENTER dalej · M menu","ENTER continue · M menu"),d.accent);
    }
    if(state_.won&&!state_.paused&&!v.dialogue.active()) {
      paint_text(rows_-3,2,tr("E / ENTER — przejdź dalej","E / ENTER — pass through"),d.accent);
      paint_text(rows_-2,2,tr("R — wróć do lampy","R — return to the lamp"),d.light);
    }
    if(v.dialogue.active()) {
      const auto &line=v.dialogue.lines[v.dialogue.page];const auto &npc=iceberg::Npcs[size_t(std::clamp(line.speaker,0,7))];
      panel(line.speaker<0?tr("DOKUMENT","DOCUMENT"):npc.name);
      for(int i=0;i<8;++i) {
        std::string portrait=line.speaker<0?std::array<const char*,8>{" ╭────────╮"," │ ≋≋≋≋≋  │"," │ ≋≋≋≋≋≋ │"," │ ≋≋≋≋   │"," │        │"," │ ≋≋≋≋≋  │"," │    ▣   │"," ╰────────╯"}[size_t(i)]:npc.portrait[size_t(i)];
        if(grid_.room_role==iceberg::RoomRole::Secret&&line.speaker==7&&v.dialogue.page>=2&&i==2)portrait="   │ │    │ │   ";
        paint_text(2+i,2,portrait,npc.color);
      }
      auto title=wrap(line.speaker<0?tr("Odnaleziony zapis","Recovered record"):npc.title.get(v.language),cols_-22);
      for(size_t i=0;i<title.size()&&i<7;++i)paint_text(3+int(i),20,title[i],d.light);
      y=11;size_t remaining=v.dialogue.visible_characters;
      for(const auto &part:wrap(line.text.get(v.language),cols_-4))if(y<rows_-2)paint_text(y++,2,dialogue_fragment(part,remaining),0xeee7d6);
      paint_text(rows_-2,2,state_.paused?tr("E / ENTER wznów","E / ENTER resume"):v.dialogue.revealing?tr("E / ENTER pokaż całość","E / ENTER reveal line"):tr("E / ENTER dalej","E / ENTER continue"),npc.color);
    }
    if(v.inspecting)draw_iceberg_inspection();
  }
