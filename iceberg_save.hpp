#pragma once
#include "iceberg_types.hpp"
#include "progress.hpp"
#include <iomanip>
#include <sstream>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace iceberg {
class SaveStore {
  std::filesystem::path path_;
  static bool sync_file(const std::filesystem::path &file) {
#ifdef _WIN32
    HANDLE handle=CreateFileW(file.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE)return false;
    bool okay=FlushFileBuffers(handle)!=0;CloseHandle(handle);return okay;
#else
    int fd=::open(file.c_str(),O_RDONLY);if(fd<0)return false;
    bool okay=::fsync(fd)==0;::close(fd);return okay;
#endif
  }
  static bool sync_parent(const std::filesystem::path &file) {
#ifdef _WIN32
    // Windows replacements below request a write-through move.
    (void)file;
    return true;
#else
    auto parent=file.parent_path();if(parent.empty())parent=".";
    int fd=::open(parent.c_str(),O_RDONLY|O_DIRECTORY);if(fd<0)return false;
    bool okay=::fsync(fd)==0;::close(fd);return okay;
#endif
  }
  static bool replace_file(const std::filesystem::path &from,const std::filesystem::path &to) {
#ifdef _WIN32
    return MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
#else
    std::error_code ec;std::filesystem::rename(from,to,ec);
    return !ec&&sync_parent(to);
#endif
  }
  static bool preserve_unreadable_file(const std::filesystem::path &file) {
    std::error_code ec;
    if(!std::filesystem::exists(file,ec))return !ec;

    auto base=file;base+=".unreadable";
    for(unsigned index=0;;++index) {
      auto copy=base;
      if(index)copy+="."+std::to_string(index);
      if(std::filesystem::copy_file(file,copy,std::filesystem::copy_options::none,ec)) {
        if(sync_file(copy)&&sync_parent(copy))return true;
        std::filesystem::remove(copy,ec);
        return false;
      }
      if(ec!=std::errc::file_exists)return false;
    }
  }
  static uint64_t checksum(const std::string &s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
  }
  static bool record_valid(const Record &r) {
    return std::isfinite(r.best_seconds) && r.best_seconds >= 0 &&
      r.best_seconds < 1e9 && r.best_deaths >= -1 && r.best_deaths < 10000000 &&
      r.best_treasures >= 0 && r.best_treasures < 10000000;
  }
  // Store the preference in a reserved flag in the existing TVICE6 record.
  // Older clients ignore and preserve it when rewriting their saves.
  static constexpr uint32_t focus_pause_disabled = uint32_t(1)<<31;
  static constexpr uint32_t smooth_camera_enabled = uint32_t(1)<<30;
  static bool read(const std::filesystem::path &file, Profile &p, const std::string *matching_payload=nullptr) {
    std::error_code size_error;auto bytes=std::filesystem::file_size(file,size_error);
    if(size_error||bytes>65536)return false;
    std::ifstream in(file, std::ios::binary);
    std::string magic;
    uint64_t hash = 0;
    if (!(in >> magic >> hash) || (magic != "TVICE2" && magic != "TVICE3" && magic != "TVICE4" && magic != "TVICE5" && magic != "TVICE6")) return false;
    in.get();
    std::string payload((std::istreambuf_iterator<char>(in)), {});
    if (matching_payload && (magic != "TVICE6" || payload != *matching_payload)) return false;
    if (payload.size() > 65536 || checksum(payload) != hash) return false;
    std::istringstream data(payload);
    Profile q;
    int language = 0;
    size_t count = 0;
    if (!(data >> q.content_version >> language >> q.started >> q.finished >>
      q.seed >> q.district >> q.stage >> q.room >> q.unlocked >> q.story >>
      q.discoveries >> q.cosmetics >> q.outfit >> q.campaign_seconds >>
      q.campaign_deaths >> q.endless_best >> q.checkpoint >> q.collected >>
      q.switches >> q.room_seconds >> q.room_deaths >> q.music_volume >>
      q.effects_volume >> q.ambience_volume)) return false;
    if (q.content_version == 0 || q.content_version > ContentVersion || language < 0 || language > 1 ||
      q.district < 0 || q.district >= DistrictCount || q.stage < 0 || q.stage > 7 ||
      q.room < 0 || q.room >= RoomsPerLeg || q.unlocked < 0 || q.unlocked >= DistrictCount ||
      q.checkpoint < 0 || q.checkpoint > 64 || q.outfit < 0 || q.outfit > 7 ||
      q.campaign_deaths < 0 || q.campaign_deaths > 10000000 ||
      q.room_deaths < 0 || q.room_deaths > 10000000 ||
      q.endless_best < 0 || q.endless_best > 10000000 ||
      !std::isfinite(q.campaign_seconds) || q.campaign_seconds < 0 || q.campaign_seconds > 1e9 ||
      !std::isfinite(q.room_seconds) || q.room_seconds < 0 || q.room_seconds > 1e9)
      return false;
    for (float v : {q.music_volume, q.effects_volume, q.ambience_volume})
      if (!std::isfinite(v) || v < 0 || v > 1) return false;
    for (auto &quest : q.quests) if (!(data >> quest) || quest > 3) return false;
    for (auto &n : q.quest_counts) if (!(data >> n) || n > 10000) return false;
    for (auto &n : q.task_items) if (!(data >> n)) return false;
    if (!(data >> q.district_seconds >> q.district_deaths >> q.district_treasures) ||
      !std::isfinite(q.district_seconds) || q.district_seconds < 0 || q.district_seconds > 1e9 ||
      q.district_deaths < 0 || q.district_treasures < 0) return false;
    for (auto *records : {&q.records, &q.trials})
      for (auto &r : *records)
        if (!(data >> r.best_seconds >> r.best_deaths >> r.best_treasures >> r.complete) ||
          !record_valid(r)) return false;
    if (!(data >> count) || count > 512) return false;
    q.itinerary.resize(count);
    for (int &id : q.itinerary) if (!(data >> id) || !((id>=0&&id<72)||(id>=400&&id<=405)||(id>=420&&id<=425)||(id>=500&&id<=511))) return false;
    if (magic!="TVICE2" && !(data >> q.at_content_boundary))return false;
    if (!(data >> q.hidden_room >> q.visit_district) || q.hidden_room < -1 || q.hidden_room > 2 || q.visit_district < -1 || q.visit_district >= DistrictCount) return false;
    for (auto &e : q.expeditions) {
      if (!(data >> e.active >> e.seed >> e.district >> e.room >> e.depth >> e.checkpoint >>
        e.collected >> e.switches >> e.room_seconds >> e.seconds >> e.room_deaths >> e.deaths >> e.treasures >> count)) return false;
      if (e.district < 0 || e.district >= DistrictCount || e.room < 0 || e.room >= RoomsPerLeg ||
        e.depth < 0 || e.depth > 10000000 || e.checkpoint < 0 || e.checkpoint > 64 ||
        e.room_deaths < 0 || e.deaths < 0 || e.treasures < 0 ||
        !std::isfinite(e.room_seconds) || e.room_seconds < 0 || e.room_seconds > 1e9 ||
        !std::isfinite(e.seconds) || e.seconds < 0 || e.seconds > 1e9 || count > 512) return false;
      e.itinerary.resize(count);
      for (int &id : e.itinerary) if (!(data >> id) || !((id>=0&&id<72)||(id>=400&&id<=405)||(id>=420&&id<=425)||(id>=500&&id<=511))) return false;
    }
    if(magic!="TVICE2") {
      if(!(data>>count)||count>128)return false;
      q.challenges.resize(count);
      for(auto &challenge:q.challenges) {
        auto &r=challenge.record;
        if(!(data>>challenge.seed>>challenge.district>>challenge.version>>r.best_seconds>>r.best_deaths>>r.best_treasures>>r.complete)||
          challenge.district<0||challenge.district>=DistrictCount||challenge.version==0||challenge.version>65535||!record_valid(r))return false;
      }
    }
    if(magic=="TVICE4"||magic=="TVICE5"||magic=="TVICE6") {
      auto &r=q.revisit;
      if(!(data>>r.active>>r.district>>r.scene>>r.checkpoint>>r.deaths>>r.collected>>r.switches>>r.seconds)||
        r.district<0||r.district>=DistrictCount||r.scene<0||r.scene>2||(r.scene==2&&r.district!=1)||r.checkpoint<0||r.checkpoint>64||
        r.deaths<0||r.deaths>10000000||!std::isfinite(r.seconds)||r.seconds<0||r.seconds>1e9)return false;
      for(auto &e:q.expeditions)if(!(data>>e.origin_seed))return false;
      if(!(data>>q.interlude))return false;
    } else for(auto &e:q.expeditions)e.origin_seed=e.seed;
    q.layout_version=q.content_version;
    for(auto &e:q.expeditions)e.layout_version=q.content_version;
    if(magic=="TVICE5"||magic=="TVICE6") {
      if(!(data>>q.layout_version)||q.layout_version==0||q.layout_version>ContentVersion)return false;
      for(auto &e:q.expeditions)if(!(data>>e.layout_version)||e.layout_version==0||e.layout_version>ContentVersion)return false;
    }
    if(magic=="TVICE6") {
      if(!(data>>q.quest_focus)||q.quest_focus<-1||q.quest_focus>7)return false;
      for(int d=0;d<DistrictCount;++d) {
        auto i=size_t(d);
        if(!(data>>q.route_visited[i]>>q.route_choices[i]>>q.route_decisions[i])||
            q.route_visited[i]>0xffffffu||q.route_choices[i]>7||q.route_decisions[i]>7||
            (q.route_choices[i]&~q.route_decisions[i]))return false;
      }
    }
    data >> std::ws;
    if (!data.eof()) return false;
    q.pause_on_focus_loss=(q.cosmetics&focus_pause_disabled)==0;
    q.smooth_camera=(q.cosmetics&smooth_camera_enabled)!=0;
    q.cosmetics&=~(focus_pause_disabled|smooth_camera_enabled);
    q.language = Language(language);
    p = std::move(q);
    return true;
  }
public:
  // Importers validate the exact file, without consulting recovery siblings.
  static bool validate_file(const std::filesystem::path &file,Profile &profile) {return read(file,profile);}
  bool recovered = false, protected_future = false;
  explicit SaveStore(std::filesystem::path path = default_file()) : path_(std::move(path)) {}
  static constexpr int SlotCount=3;
  static std::filesystem::path slot_file(std::filesystem::path base,int slot) {
    if(!base.empty()&&slot>1)base+=(".slot"+std::to_string(slot));
    return base;
  }
  static std::filesystem::path default_file() {
    if (const char *v = std::getenv("TV_ICEBERG_FILE")) return v;
    // Keep legacy progress.dat untouched, including its TV_PROGRESS_FILE override.
    auto p = ProgressStore::default_file();
    if (p.empty()) return {};
    p += ".iceberg";
    return p;
  }
  bool load(Profile &p) {
    recovered = false;protected_future=false;
    {std::error_code ec;auto bytes=std::filesystem::file_size(path_,ec);
      std::ifstream header; if(!ec&&bytes<=65536)header.open(path_,std::ios::binary);
      std::string magic;uint64_t hash;unsigned version=0;
      if(header>>magic>>hash) {
        header.get();std::string payload((std::istreambuf_iterator<char>(header)),{});std::istringstream body(payload);
        if(checksum(payload)==hash&&(body>>version))protected_future=(magic.rfind("TVICE",0)==0&&magic!="TVICE2"&&magic!="TVICE3"&&magic!="TVICE4"&&magic!="TVICE5"&&magic!="TVICE6")||version>ContentVersion;
      }
    }
    if (read(path_, p)) return true;
    auto backup = path_; backup += ".bak";
    if (read(backup, p)) { recovered = true; return true; }
    return false;
  }
  bool save(const Profile &p) {
    if (path_.empty()||protected_future) return false;
    std::ostringstream body;
    body << std::setprecision(17) << p.content_version << ' ' << int(p.language) << ' '
      << p.started << ' ' << p.finished << ' ' << p.seed << ' ' << p.district << ' '
      << p.stage << ' ' << p.room << ' ' << p.unlocked << ' ' << p.story << ' '
      << p.discoveries << ' ' << (p.cosmetics|(p.pause_on_focus_loss?0u:focus_pause_disabled)|
        (p.smooth_camera?smooth_camera_enabled:0u)) << ' ' << p.outfit << ' '
      << p.campaign_seconds << ' ' << p.campaign_deaths << ' ' << p.endless_best << ' '
      << p.checkpoint << ' ' << p.collected << ' ' << p.switches << ' '
      << p.room_seconds << ' ' << p.room_deaths << ' ' << p.music_volume << ' '
      << p.effects_volume << ' ' << p.ambience_volume << '\n';
    for (auto q : p.quests) body << q << ' ';
    body << '\n';
    for (auto n : p.quest_counts) body << n << ' ';
    for (auto n : p.task_items) body << n << ' ';
    body << p.district_seconds << ' ' << p.district_deaths << ' ' << p.district_treasures << '\n';
    for (const auto *records : {&p.records, &p.trials})
      for (auto r : *records)
        body << r.best_seconds << ' ' << r.best_deaths << ' ' << r.best_treasures << ' ' << r.complete << '\n';
    body << p.itinerary.size() << '\n';
    for (int id : p.itinerary) body << id << ' ';
    body << '\n';
    body << p.at_content_boundary << ' ' << p.hidden_room << ' ' << p.visit_district << '\n';
    for (const auto &e : p.expeditions) {
      body << e.active << ' ' << e.seed << ' ' << e.district << ' ' << e.room << ' ' << e.depth << ' ' << e.checkpoint << ' '
        << e.collected << ' ' << e.switches << ' ' << e.room_seconds << ' ' << e.seconds << ' ' << e.room_deaths << ' '
        << e.deaths << ' ' << e.treasures << ' ' << e.itinerary.size() << '\n';
      for (int id : e.itinerary) body << id << ' ';
      body << '\n';
    }
    body<<p.challenges.size()<<'\n';
    for(const auto &challenge:p.challenges){auto r=challenge.record;
      body<<challenge.seed<<' '<<challenge.district<<' '<<challenge.version<<' '<<r.best_seconds<<' '<<r.best_deaths<<' '<<r.best_treasures<<' '<<r.complete<<'\n';}
    const auto &r=p.revisit;
    body<<r.active<<' '<<r.district<<' '<<r.scene<<' '<<r.checkpoint<<' '<<r.deaths<<' '<<r.collected<<' '<<r.switches<<' '<<r.seconds<<'\n';
    for(const auto &e:p.expeditions)body<<e.origin_seed<<' ';
    body<<p.interlude<<'\n';
    body<<p.layout_version<<' ';
    for(const auto &e:p.expeditions)body<<e.layout_version<<' ';
    body<<'\n';
    body<<p.quest_focus<<'\n';
    for(int d=0;d<DistrictCount;++d)
      body<<p.route_visited[size_t(d)]<<' '<<p.route_choices[size_t(d)]<<' '<<p.route_decisions[size_t(d)]<<'\n';
    auto payload = body.str();
    Profile verified;
    // Idle autosaves must leave the previous distinct progress in the recovery file.
    if (read(path_, verified, &payload)) return true;

    std::error_code ec;
    if (!path_.parent_path().empty()) std::filesystem::create_directories(path_.parent_path(), ec);
    if (ec) return false;
    auto temp = path_;
#ifdef _WIN32
    temp += "." + std::to_string(GetCurrentProcessId()) + ".tmp";
#else
    temp += "." + std::to_string(::getpid()) + ".tmp";
#endif
    {
      std::ofstream out(temp, std::ios::binary | std::ios::trunc);
      out << "TVICE6 " << checksum(payload) << '\n' << payload;
      out.close();
      if (!out) return false;
    }
    if (!sync_file(temp) || !read(temp, verified)) { std::filesystem::remove(temp, ec); return false; }
    Profile current;
    // Never replace a healthy recovery copy with a damaged primary save.
    if (read(path_, current)) {
      auto backup = path_; backup += ".bak";
      if(!read(backup,verified)&&!preserve_unreadable_file(backup)) {
        std::filesystem::remove(temp,ec);
        return false;
      }
      auto backup_temp=temp;backup_temp+=".backup";
      std::filesystem::copy_file(path_, backup_temp, std::filesystem::copy_options::overwrite_existing, ec);
      if (ec || !sync_file(backup_temp) || !replace_file(backup_temp,backup)) { std::filesystem::remove(temp, ec); return false; }
    } else if(!preserve_unreadable_file(path_)) {
      std::filesystem::remove(temp,ec);
      return false;
    }
    bool ok=replace_file(temp,path_);
    if (!ok) std::filesystem::remove(temp, ec);
    return ok;
  }
};
} // namespace iceberg
