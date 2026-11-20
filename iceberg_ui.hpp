#pragma once
#include "iceberg_inspection.hpp"
#include "iceberg_story.hpp"

namespace iceberg {
inline bool browsing_expedition_room(const View &view) {
  return view.mode!=Mode::Campaign&&view.tour_scene<0&&view.route.available&&view.route.current>=0&&
    view.route_room_selection>=0&&
    view.route_room_selection!=view.route.completed_legs*RoomsPerLeg+view.route.current;
}
struct DialogueLayout {
  int top=4, left=1, width=0, height=0, text_x=3, text_y=6, text_rows=1, pages=1;
  bool portrait=false;
  std::vector<std::string> body;
};
inline int dialogue_panel_top(int rows,int height,int player_row) {
  // Prefer the half opposite the courier.
  return player_row<rows/2?std::max(4,rows-3-height):4;
}
inline constexpr int ReplyPanelHeight=9;
inline DialogueLayout reply_layout(int rows,int player_row) {
  DialogueLayout out;
  out.height=ReplyPanelHeight;
  out.top=dialogue_panel_top(rows,out.height,player_row);
  return out;
}
inline DialogueLayout dialogue_layout(const std::string &text,int columns,int rows,int player_row=1000,int segment=0) {
  DialogueLayout out;out.width=std::max(10,columns-2);
  out.portrait=columns>=100&&rows>=30;
  out.text_x=out.portrait?22:3;
  out.body=inspection_wrap(text,std::max(8,columns-out.text_x-3));
  int capacity=std::max(2,rows-16);
  if(out.portrait)capacity=std::min(8,capacity);
  else capacity=std::min(9,capacity);
  out.text_rows=std::min(capacity,std::max(1,int(out.body.size())));
  out.pages=std::max(1,(int(out.body.size())+out.text_rows-1)/out.text_rows);
  int part=std::clamp(segment,0,out.pages-1);
  int lines=std::min(out.text_rows,std::max(1,int(out.body.size())-part*out.text_rows));
  out.height=std::max(out.portrait?11:0,lines+4);
  out.top=dialogue_panel_top(rows,out.height,player_row);
  out.text_y=out.top+2;
  return out;
}
inline std::string dialogue_segment(const DialogueLayout &layout,int segment) {
  std::string text;
  int first=std::clamp(segment,0,layout.pages-1)*layout.text_rows;
  for(int row=first;row<std::min(first+layout.text_rows,int(layout.body.size()));++row){if(!text.empty())text+=' ';text+=layout.body[size_t(row)];}
  return text;
}
inline std::string dialogue_segment(const DialogueLine &line,Language language,int columns,int rows,int segment) {
  return dialogue_segment(dialogue_layout(line.text.get(language),columns,rows),segment);
}
inline size_t dialogue_character_count(const std::string &wrapped) {
  return size_t(cell_length(wrapped)-std::count(wrapped.begin(),wrapped.end(),' '));
}
inline std::vector<int> journal_entries(const View &v) {
  std::vector<int> entries;
  if(v.journal_tab==0)entries.push_back(-1);
  for(int q=0;q<8;++q)if(v.journal_tab==0?(v.quests[size_t(q)]==1||v.quests[size_t(q)]==2):v.journal_tab==1&&v.quests[size_t(q)]==3)entries.push_back(q);
  if(v.journal_tab==2){if(v.story&(uint64_t(1)<<41))entries.push_back(200);for(int n=0;n<3;++n)if(v.discoveries&(1u<<n))entries.push_back(100+n);}
  return entries;
}
inline std::string journey_next_step(const View &v) {
  auto t=[&](const char *pl,const char *en){return tr(v.language,pl,en);};
  if(!v.current_objective.empty())return v.current_objective;
  if(v.tour_scene>=0)return DebugTour[size_t(v.tour_scene)].change.get(v.language);
  if(v.mode!=Mode::Campaign) {
    const auto &route=v.route;
    const int current=route.current,count=int(route.rooms.size());
    if(route.available&&current>=0&&current<count) {
      if(current+1<count)
        return t("Oświetlone drzwi prowadzą do pokoju ","The lit door leads to room ")+
          std::to_string(current+2)+": "+route.rooms[size_t(current+1)].get(v.language)+".";
      if(v.mode==Mode::Trial)
        return t("Oświetlone drzwi kończą próbę mistrzowską.","The lit door completes the Mastery Trial.");
      return t("Oświetlone drzwi rozpoczynają odcinek ","The lit door starts leg ")+
        std::to_string(v.depth/RoomsPerLeg+2)+".";
    }
    return t("Idź do oświetlonych drzwi.","Head to the lit door.");
  }
  if(v.revisit)return t("Brama przy wejściu wraca do osady. Postęp głównej podróży czeka.","The entrance door returns to the settlement. Your journey is waiting.");
  if(v.visit_district>=0)return t("Tablica wypraw w osadzie wraca do twojej bieżącej trasy.","The settlement's route board returns to your current journey.");
  if(v.finished)return t("Wybierz wyprawę na tablicy albo odwiedź mieszkańców.","Choose an expedition at the board or visit the residents.");
  if(v.story&(uint64_t(1)<<42))return t("Posadź nasiona i dostarcz publiczny rejestr. Potem zakończ obchód przy tablicy.","Plant the seeds and deliver the public register. Then finish the round at the board.");
  if(v.stage==0&&!(v.story&(uint64_t(1)<<v.district)))return t("Porozmawiaj z mieszkańcem przy wejściu, a potem podejdź do tablicy wypraw.","Speak to the resident by the entrance, then approach the route board.");
  if(v.stage==0||v.stage==3||v.stage==5)return t("Podejdź do tablicy wypraw i naciśnij E: ","Go to the route board and press E: ")+route_title(v.district,v.stage).get(v.language)+".";
  if(v.stage==7)return Districts[size_t(v.district)].objective.get(v.language)+" "+t("Uruchom miejscowe urządzenia; wyjście otworzy dalszą drogę.","Operate the local machinery; the exit opens the onward route.");
  return t("Podążaj przez oświetlone przejścia: ","Follow the lit passages: ")+route_title(v.district,v.stage).get(v.language)+". "+t("J pokazuje bieżący odcinek.","J shows your current route.");
}
inline std::string quest_next_step(const View &v,int q) {
  const auto &quest=Quests[size_t(q)];
  auto t=[&](const char *pl,const char *en){return tr(v.language,pl,en);};
  if(v.quests[size_t(q)]==3)return quest.reward.get(v.language);
  if(v.quests[size_t(q)]==2)return t("Wróć do: ","Return to: ")+std::string(Npcs[size_t(quest.owner)].name)+" — "+Districts[size_t(quest.district)].name.get(v.language)+".";
  if(q==6)return t("Na bezpiecznym pomoście: zryw po ziemi, zaraz potem skok. Pokaż Kubie trzy takie skoki.","On a safe landing: dash along the ground, then jump immediately. Show Kuba three dash jumps.");
  return quest.objective.get(v.language)+" "+t("Szukaj na trasach: ","Look along the routes of: ")+Districts[size_t(quest.district)].name.get(v.language)+".";
}
inline bool district_travel_available(const View &v,int target) {
  return v.mode==Mode::Campaign&&v.can_travel&&v.hidden_room<0&&!v.revisit&&
    target>=0&&target<DistrictCount&&target<=v.unlocked&&
    (target!=v.district||(target==v.journey_district&&v.visit_district>=0));
}
inline std::string district_travel_status(const View &v,int target,RoomRole role) {
  auto t=[&](const char *pl,const char *en){return tr(v.language,pl,en);};
  if(v.mode!=Mode::Campaign)return t("Odwiedziny działają w podróży fabularnej. Wyprawę wybierzesz w menu.","Settlement travel is available during the story. Choose an expedition in the menu.");
  if(v.hidden_room>=0)return t("Użyj drzwi powrotnych, aby opuścić odnogę.","Use the return door to leave this detour.");
  if(v.revisit)return t("Wróć drzwiami do osady, aby skorzystać z połączeń.","Return through the door to the settlement to use the connections.");
  if(target>v.unlocked)return target==1?t("Zamknięte. Ukończ trzy trasy Dolnej Przystani i uruchom pompę na końcu rurociągu.","Closed. Finish Lower Harbor's three routes and start the pump at the pipeline's end."):t("Zamknięte. Ukończ główną trasę dzielnicy: ","Closed. Complete the main route through: ")+Districts[size_t(target-1)].name.get(v.language)+".";
  if(target==v.district&&!district_travel_available(v,target)) {
    if(role!=RoomRole::Settlement)return t("Tu jesteś. Wyjdź z tego pokoju, aby ruszyć dalej.","You are here. Use this room's exit to continue.");
    if(v.visit_district>=0)return t("Tu jesteś. Użyj tablicy, by wrócić na swoją trasę.","You are here. Use the board to return to your route.");
    if(v.finished||v.at_content_boundary||(v.story&(uint64_t(1)<<42)))return t("Tu jesteś.","You are here.");
    return t("Tu jesteś. Użyj tablicy wypraw, aby ruszyć dalej.","You are here. Use the route board to continue.");
  }
  if(!v.can_travel)return t("Ukończ trasę Dolnej Przystani, aby odblokować podróż między dzielnicami.","Complete the Lower Harbor route to unlock travel between districts.");
  return target==v.journey_district?t("ENTER: wróć do swojej trasy i ostatniej lampy.","ENTER: return to your route and last lamp."):t("ENTER: odwiedź osadę. Twój postęp na trasie zostanie zachowany.","ENTER: visit the settlement. Your route progress will be kept.");
}
} // namespace iceberg
