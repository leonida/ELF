/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "game_env.h"
#include "cmd.h"

GameEnv::GameEnv() {
    // Load the map.
    // std::cout<<"-------GameEnv_map-----------"<<std::endl;
    _map = unique_ptr<RTSMap>(new RTSMap());
     
    _game_counter = -1;
    Reset();
    //std::cout<<"-------GameEnv Finish-----------"<<std::endl;
}

void GameEnv::Visualize() const {
    for (const auto &player : _players) {
        std::cout << player.PrintInfo() << std::endl;
    }
    // No FoW, everything.
    GameEnvAspect aspect(*this, INVALID);
    UnitIterator unit_iter(aspect, UnitIterator::ALL);
    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        std::cout << u.PrintInfo(*_map) << std::endl;
        ++ unit_iter;
    }
}

void GameEnv::ClearAllPlayers() {
    _players.clear();
}

void GameEnv::Reset() {
    //std::cout<<"-------Reset-----------"<<std::endl;
    _map->ClearMap();
    _next_unit_id = 0;
    _winner_id = INVALID;
    _terminated = false;
    _game_counter ++;
    _units.clear();
    _bullets.clear();
    _trace.clear();
    
    for (auto& player : _players) {
        player.ClearCache();
    }
}

void GameEnv::AddPlayer(const std::string &name, PlayerPrivilege pv) {
    _players.emplace_back(*_map, name, _players.size());
    _players.back().SetPrivilege(pv);
}

void GameEnv::RemovePlayer() {
    _players.pop_back();
}

void GameEnv::SaveSnapshot(serializer::saver &saver) const {
    serializer::Save(saver, _next_unit_id);

    saver << _map;
    saver << _units;
    saver << _bullets;
    saver << _players;
    saver << _winner_id;
    saver << _terminated;
}

void GameEnv::LoadSnapshot(serializer::loader &loader) {
    serializer::Load(loader, _next_unit_id);

    loader >> _map;
    loader >> _units;
    loader >> _bullets;
    loader >> _players;
    loader >> _winner_id;
    loader >> _terminated;

    for (auto &player : _players) {
        player.ResetMap(_map.get());
    }
}

// Compute the hash code.
uint64_t GameEnv::CurrentHashCode() const {
    uint64_t code = 0;
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        serializer::hash_combine(code, it->first);
        serializer::hash_combine(code, *it->second);
        // cout << "Unit: " << it->first << ": #hash = " << this_code << ", " << it->second->GetProperty().CD(CD_ATTACK).PrintInfo() << endl;
        // code ^= this_code;
    }
    // Players.
    for (const auto &player : _players) {
        serializer::hash_combine(code, player);
    }
    return code;
}

bool GameEnv::AddUnit(Tick tick, UnitType type, const PointF &p, PlayerId player_id, UnitId& u_id) {
    // Check if there is any space.
    
    if (!_gamedef.CheckAddUnit(_map.get(), type, p)) return false;
    // cout << "Actual adding unit." << endl;
    UnitId new_id = Player::CombinePlayerId(_next_unit_id, player_id);
    Unit *new_unit = new Unit(tick, new_id, type, p, _gamedef.unit(type)._property);
    _units.insert(make_pair(new_id, unique_ptr<Unit>(new_unit)));
    _map->AddUnit(new_id, p);
    
    u_id = new_id;
    //cout<<"AddUnit u_id: "<<u_id<<endl;

    _next_unit_id ++;
    return true;
}

bool GameEnv::RemoveUnit(const UnitId &id) {
    auto it = _units.find(id);
    if (it == _units.end()) return false;
    _units.erase(it);

    _map->RemoveUnit(id);
    return true;
}

// 找到最近的基地
UnitId GameEnv::FindClosestBase(PlayerId player_id) const {
    // Find closest base. [TODO]: Not efficient here.
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        const Unit *u = it->second.get();
        if ((u->GetUnitType() == BASE || u->GetUnitType() == FLAG_BASE) && u->GetPlayerId() == player_id) {
            return u->GetId();
        }
    }
    return INVALID;
}

PlayerId GameEnv::CheckBase(UnitType base_type) const{
    PlayerId last_player_has_base = INVALID;
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        const Unit *u = it->second.get();
        if (u->GetUnitType() == base_type) {
            if (last_player_has_base == INVALID) {
                last_player_has_base = u->GetPlayerId();
            } else if (last_player_has_base != u->GetPlayerId()) {
                // No winning.
                last_player_has_base = INVALID;
                break;
            }
        }
    }
    return last_player_has_base;
}
 PlayerId GameEnv::CheckWinner(UnitType base_type) const {
     PlayerId player_id  = 0;
     PlayerId enemy_id = 1;
     PlayerId base_player_id = INVALID;
     bool hasEnemyUnit = false;
     if(_units.size()== 0){
        return INVALID;
     }
     //std::cout<<"_units.size() : "<<_units.size()<<std::endl;
     for (auto it = _units.begin(); it != _units.end(); ++it){
          const Unit *u = it->second.get();
           if (u->GetUnitType() == base_type){
               base_player_id = u->GetPlayerId();
           }
           if(!hasEnemyUnit){  //如果还没有检测到敌方单位，持续判断
               if(((u->GetUnitType() == WORKER || u->GetUnitType() == BARRACKS || u->GetUnitType() == BASE) && u->GetPlayerId() == enemy_id))
                    hasEnemyUnit = true;
           }        
     }
     if(base_player_id == INVALID){  //只有玩家有基地，基地被摧毁则认为玩家失败，游戏结束
        printf("玩家基地被摧毁\n");
        GetPlayer(0).PrintLoss();
        // if(hasEnemyUnit)  printf("剩余敌人  \n");
        // else  printf("不剩余敌人  \n");
        
        return enemy_id;  
     }else if(!hasEnemyUnit){  //玩家基地存在，不存在敌方单位，玩家胜利
        // if(GetPlayer(enemy_id).GetCurrentRound()>=GetPlayer(enemy_id).GetEnemyRound()){
        //     printf("玩家胜利\n");
        //     return player_id;  
        // }
        printf("玩家胜利\n");
        GetPlayer(0).PrintLoss();
        return player_id;
       // printf("场上不存在敌方单位，但还有敌人未产生\n");
         
     } 
     return INVALID;    // 游戏还未结束
 }




bool GameEnv::FindEmptyPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const {
    // Find an empty place by simple local grid search.
    const int margin = 2;
    const int cx = _map->GetXSize() / 2;
    const int cy = _map->GetYSize() / 2;
    int sx = p.x < cx ? -1 : 1;
    int sy = p.y < cy ? -1 : 1;

    for (int dx = -sx * l1_radius; dx != sx * l1_radius + sx; dx += sx) {
        for (int dy = -sy * l1_radius; dy != sy * l1_radius + sy; dy += sy) {
            PointF new_p(p.x + dx, p.y + dy);
            if (_map->CanPass(new_p, INVALID) && _map->IsIn(new_p, margin)) {
                // It may not be a good strategy, though.
                *res_p = new_p;
                return true;
            }
        }
    }
    return false;
}

bool GameEnv::FindBuildPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const {
    // Find an empty place by simple local grid search.
    for (int dx = -l1_radius; dx <= l1_radius; dx ++) {
        for (int dy = -l1_radius; dy <= l1_radius; dy ++) {
            PointF new_p(p.x + dx, p.y + dy);
            if (_map->CanBuildTower(new_p, INVALID)) {
                *res_p = new_p;
                return true;
            }
        }
    }
    return false;
}

// given a set of units and a target point, a distance, find closest place to go to to maintain the distance.
// can be used by hit and run or scout.
bool GameEnv::FindClosestPlaceWithDistance(const PointF &p, int dist,
  const vector<const Unit *>& units, PointF *res_p) const {
  const RTSMap &m = *_map;
  vector<Loc> distances(m.GetXSize() * m.GetYSize());
  vector<Loc> current;
  vector<Loc> nextloc;
  for (auto unit : units) {
      Loc loc = m.GetLoc(unit->GetPointF().ToCoord());
      distances[loc] = 0;
      current.push_back(loc);
  }
  const int dx[] = { 1, 0, -1, 0 };
  const int dy[] = { 0, 1, 0, -1 };
  for (int d = 1; d <= dist; d++) {
      for (Loc loc : current) {
          Coord c_curr = m.GetCoord(loc);
          for (size_t i = 0; i < sizeof(dx) / sizeof(int); ++i) {
              Coord next(c_curr.x + dx[i], c_curr.y + dy[i]);
              if (_map->CanPass(next, INVALID) && _map->IsIn(next)) {
                  Loc l_next = m.GetLoc(next);
                  if (distances[l_next] == 0 || distances[l_next] > d) {
                    nextloc.push_back(l_next);
                    distances[l_next] = d;
                  }
              }
          }
      }
      current = nextloc;
      nextloc.clear();
  }

  float closest = m.GetXSize() * m.GetYSize();
  bool found = false;
  int k = 0;
  for (Loc loc : current) {
      k++;
      PointF pf = PointF(m.GetCoord(loc));
      float dist_sqr = PointF::L2Sqr(pf, p);
      if (closest > dist_sqr) {
          *res_p = pf;
          closest = dist_sqr;
          found = true;
      }
  }
  return found;
}

void GameEnv::Forward(CmdReceiver *receiver) {
    // Compute all bullets.
    set<int> done_bullets;
    for (size_t i = 0; i < _bullets.size(); ++i) {
        CmdBPtr cmd = _bullets[i].Forward(*_map, _units);
        if (cmd.get() != nullptr) {
            // Note that this command is special. It should not be recorded in
            // the cmd_history.
            receiver->SetSaveToHistory(false);
            receiver->SendCmd(std::move(cmd));
            receiver->SetSaveToHistory(true);
        }
        if (_bullets[i].IsDead()) done_bullets.insert(i);
    }

    // Remove bullets that are done.
    // Need to traverse in the reverse order.
    for (set<int>::reverse_iterator it = done_bullets.rbegin(); it != done_bullets.rend(); ++it) {
        unsigned int idx = *it;
        if (idx < _bullets.size() - 1) {
            swap(_bullets[idx], _bullets.back());
        }
        _bullets.pop_back();
    }
}

void GameEnv::ComputeFOW() {
    // Compute FoW.
    for (Player &p : _players) {
        p.ComputeFOW(_units);
    }
}

int GameEnv::GetPrevSeenCount(PlayerId player_id) const {
    GameEnvAspect aspect(*this, player_id);
    const Player &player = aspect.GetPlayer();

    set<UnitId> unit_ids;

    // cout << "GetPrevSeenCount: " << endl;

    for (int x = 0; x < _map->GetXSize(); ++x) {
        for (int y = 0; y < _map->GetYSize(); ++y) {
            Loc loc = _map->GetLoc(x, y);
            const Fog &f = player.GetFog(loc);
            if (f.CanSeeTerrain()) continue;
            for (const Unit &u : f.seen_units()) {
                // cout << u.PrintInfo(*_map) << endl;
                unit_ids.insert(u.GetId());
            }
        }
    }
    return unit_ids.size();
}

bool GameEnv::GenerateMap(int num_obstacles, int init_resource) {
    return _map->GenerateMap(GetRandomFunc(), num_obstacles, _players.size(), init_resource);
}

bool GameEnv::GenerateImpassable(int num_obstacles) {
    return _map->GenerateImpassable(GetRandomFunc(), num_obstacles);
}

bool GameEnv::GenerateTDMaze() {
    return _map->GenerateTDMaze(GetRandomFunc());
}

const Unit *GameEnv::PickFirstIdle(const vector<const Unit *> units, const CmdReceiver &receiver) {
    return PickFirst(units, receiver, INVALID_CMD);
}

const Unit *GameEnv::PickFirst(const vector<const Unit *> units, const CmdReceiver &receiver, CmdType t) {
    for (const auto *u : units) {
        const CmdDurative *cmd = receiver.GetUnitDurativeCmd(u->GetId());
        if ( (t == INVALID_CMD && cmd == nullptr) || (cmd != nullptr && cmd->type() == t)) return u;
    }
    return nullptr;
}

string GameEnv::PrintDebugInfo() const {
    stringstream ss;
    ss << "Game #" << _game_counter << endl;
    for (const auto& player : _players) {
        ss << "Player " << player.GetId() << endl;
        ss << player.PrintHeuristicsCache() << endl;
    }

    ss << _map->Draw() << endl;
    ss << _map->PrintDebugInfo() << endl;
    return ss.str();
}

string GameEnv::PrintPlayerInfo() const {
    stringstream ss;
    for (const auto& player : _players) {
        ss << "Player " << player.GetId() << endl;
        ss << player.PrintInfo()<< endl;
    }

    //ss << _map->Draw() << endl;
    //ss << _map->PrintDebugInfo() << endl;
    return ss.str();
}


// =============追踪目标的方法==============
string GameEnv::PrintUnitInfo() const {
    stringstream ss;
    ss<<"UnitInfo: "<<endl;
    for(auto& i : _units){
        ss<<"UnitId: "<<i.first<<" UnitType: "<<i.second->GetUnitType()<<" Pointf: "<<"Player_id: "<<i.second->GetPlayerId()<<endl;
    }
    return ss.str();
}

string GameEnv::PrintTargetsInfo(PlayerId player_id,UnitId radar_id) {
    // ToDO  查询前要先更新
    UpdateTargets(player_id);
    stringstream ss;
    if(radar_id != -1){
        //_units.find(radar_id) == _units.end() || _units[radar_id]->GetUnitType() != RANGE_ATTACKER
      if( !CheckUnit(radar_id,RANGE_ATTACKER) || _units[radar_id]->GetPlayerId() != player_id){   
            ss<<"---------Failed Invalid RadarId "<<radar_id<<"------------------";
            return ss.str();
        }
    }
    for( auto& player : _players){
        if(player.GetId() == player_id ){
            ss<<player.PrintTargetInfo(radar_id);
            break;
        }
    }
    return ss.str();
}


void GameEnv::UpdateTargets(PlayerId player_id) {
            Player& player = GetPlayer(player_id);
            if(!player.IsRadarInit()) InitRadar(player_id);
            Targets _targets = player.GetTargets();
            for(auto r : _targets){
                if(_units.find(r.first) == _units.end()){  // 雷达失活
                    player.RemoveRadar(r.first);
                    continue;
                }
                for(auto u : r.second){  // 遍历雷达的所有单位
                   // 移除所有失活或者离开FOW的单位
                   if(_units.find(u) == _units.end()){
                       player.RemoveUnit(u);
                       continue;
                   }
                   // 判断单位是否离开玩家FOW
                   const Unit * target = GetUnit(u);
                   if(!player.FilterWithFOW(*target))
                      player.RemoveUnit(u);
                }
            }     
}

bool GameEnv::CheckUnit(UnitId u_id,UnitType type){
    return (_units.find(u_id) != _units.end() && _units[u_id]->GetUnitType() == type);  // 单位存在且类型相符
}
bool GameEnv::Lock(PlayerId player_id,UnitId radar_id,UnitId target_id){
    //UpdateTargets(player_id);
    // 判断目标是否合法
    if(!CheckUnit(target_id,WORKER) && !CheckUnit(target_id,BARRACKS) && !CheckUnit(target_id,BASE)){ //如果目标不存在或不是导弹、飞机
        cout<<"--------Invalid Target------------"<<endl;
        return false;
    }
    // 判断雷达是否合法
    if(!CheckUnit(radar_id,RANGE_ATTACKER) || _units[radar_id]->GetPlayerId()!= player_id){
        cout<<"-----------Invalid Radar--------------"<<endl;
        return false;
    }
    GetPlayer(player_id).AddUnit(radar_id,target_id,GetUnit(radar_id)->GetProperty().round); //锁定目标
    
    return true;
}

// bool GameEnv::IsTargetInRange(UnitId radar_id, UnitId _target){
//     Unit* r = GetUnit(radar_id);
//     if(r == nullptr || r->GetUnitType() != RANGE_ATTACKER) return false;
//     Unit* t = GetUnit(_target);
//     if(t == nullptr) return false;

//     //float distance = PointF::L2Sqr(r->GetPointF(),t->GetPointF());
//     float range = r->GetProperty()._att_r;
//     return PointF::IsPointInCircularSector(r->GetPointF(),r->GetProperty().towards,t->GetPointF(),range*range,0.5));
// }

UnitId GameEnv::SelectRadar_DistanceFirst(Targets& radar_info, PlayerId player_id, UnitId _target){
    UnitId radar_id = -1;
    Unit * t = GetUnit(_target);
    Player& player = GetPlayer(player_id);
    if(t == nullptr) return -1;
    float min_distance = 1000000.0f;
    for(auto r : radar_info){
        const Unit* radar = GetUnit(r.first);
        if(radar == nullptr || radar->GetUnitType()!= RANGE_ATTACKER || GerRadarRound(radar->GetId())<=0){ //检查雷达是否合法且能够索敌
            continue;
        }
        // 判断目标是否在雷达锁定范围内
        // printf("判断目标是否在雷达范围内： 目标: %d 位置[%f , %f]  雷达: %d 位置[%f , %f] 朝向[%f , %f] 射程: %f 索敌数量： %d\n",_target, t->GetPointF().x, t->GetPointF().y, 
        // r.first, radar->GetPointF().x, radar->GetPointF().y, radar->GetProperty().towards.x, radar->GetProperty().towards.y, radar->GetProperty()._att_r,GerRadarRound(radar->GetId())
        // );

        // 首先判断目标是否在雷达的扇形索敌区域内 costheta = 0.5
        if(!PointF::IsPointInCircularSector(radar->GetPointF(),radar->GetProperty().towards,t->GetPointF(),radar->GetProperty()._att_r * radar->GetProperty()._att_r,0.5)){
           // printf("不在扇形区域内！\n");
            continue;
        }
        float distance = PointF::L2Sqr(radar->GetPointF(),t->GetPointF());
        if(distance < min_distance){
           // printf("符合要求且更新最优雷达\n");
            min_distance = distance;
            radar_id = r.first;
        }
    }
    return radar_id;
}


bool GameEnv::Lock(PlayerId player_id,UnitId target_id){
    // 判断目标是否合法
    if(!CheckUnit(target_id,WORKER) && !CheckUnit(target_id,BARRACKS) && !CheckUnit(target_id,BASE)){ //如果目标不存在或不是导弹、飞机
        cout<<"--------Invalid Target------------"<<endl;
        return false;
    }
    Player& player = GetPlayer(player_id);
    Targets& radar_info = player.GetTargets();

    //printf("In Lock GetPlayer radar_size: %d\n",radar_info.size());
    UnitId radar_id = SelectRadar_DistanceFirst(radar_info,player_id,target_id);
    if(radar_id == -1) {
       // printf("没有合适的雷达\n");
        return false;
    }else{
       // printf("找到合适的雷达： %d\n",radar_id);
        // 让雷达锁定敌人
        const Unit* radar = GetUnit(radar_id);
        if(radar == nullptr || radar->GetUnitType() != RANGE_ATTACKER) return false;
        player.AddUnit(radar_id,target_id,radar->GetProperty().round);
        return true;
    }
    
}


 
bool GameEnv::UnLock(PlayerId player_id,UnitId target_id){
    //cout<<"Unlock---UnitId:"<<target_id<<endl;
    //UpdateTargets(player_id);
    return GetPlayer(player_id).RemoveUnit(target_id);
}

int  GameEnv::GerRadarRound(UnitId radar_id) {
     const Unit* radar = GetUnit(radar_id);
     if(radar == nullptr) return 0;
     //UpdateTargets(radar->GetPlayerId());
     const Player& player = GetPlayer(radar->GetPlayerId());
     return radar->GetProperty().round - player.GetRadarRound(radar_id);
}


bool GameEnv::isUnitLock(PlayerId player_id,UnitId u_id) const{
    const Player &player = GetPlayer(player_id);
    return player.isUnitLocked(u_id);
}

void GameEnv::InitRadar(PlayerId player_id) {
    //printf("InitRadar\n");
    Player& player = GetPlayer(player_id);
    if(!player.IsRadarInit()){  //雷达没有初始化
        for (auto it = _units.begin(); it != _units.end(); ++it) {
            const Unit * u = it->second.get();
            if(u!=nullptr && u->GetPlayerId() == player_id && u->GetUnitType() == RANGE_ATTACKER){  //玩家的雷达
                player.AddRadar(u->GetId());
            }
        }
        player.FinishInitRadar();
    }
}



 




