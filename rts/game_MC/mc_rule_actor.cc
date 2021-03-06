/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "mc_rule_actor.h"

bool MCRuleActor::ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
  //  cout<<"ActByState2"<<endl;
    assigned_cmds->clear();
    *state_string = "";

    RegionHist hist;

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        act_per_unit(env, u, &state[0], &hist, state_string, assigned_cmds);
    }

    return true;
}


//判断飞机是否应该投弹
/**
 * far   --   最远距离  5
 * close --   最近距离  12
 * curr  --   当前距离  
 * 距离越近，投弹概率越大
 * **/
bool IsAttack(GameEnv& env,float close,float far,float curr){
    //std::cout<<"curr: "<<curr<<"far"<<far<<std::endl;
   // printf("IsAttack close:%f far:%f curr:%f\n",close,far,curr);
    if(curr > far*far) return false;
    if(curr < close*close) return true;
    auto f = env.GetRandomFunc();
    int between = far * far - close * close;
    int select = f(between);
    float dist = far*far - curr;
    //std::cout<<"select: "<<select<<" dist: "<<dist<<" curr: "<<curr<<std::endl;
    //printf("IsAttack finish select: %d, dist: %f\n",select,dist);
    return select <= dist;
}


// 创建飞机
// 飞机位置、攻击类型、出发时刻(tick)
bool CreateFlight(Tick _tick,GameEnv& env,PlayerId player_id,PointF flight_p,FlightType flight_t,Tick _start_tick){
    UnitId flight_id = -1;
    if(! env.AddUnit(_tick, WORKER, flight_p, player_id,flight_id)){
        //std::cout<<"create flighter failed at "<<flight_p<<std::endl;
        return false;
    }
    Unit* flight = env.GetUnit(flight_id);
    if(flight == nullptr) return false; //飞机创建失败
    // 设置飞机属性
    flight->GetProperty().flight_state = FLIGHT_IDLE; // 设置飞行状态为待机
    flight->GetProperty().flight_type = flight_t; // 设置飞机类型
    flight->GetProperty().start_tick = _start_tick; // 设置入场时间
    //std::cout<<"Create Flighter start at "<<flight_p<<" tick_: "<<_start_tick<<std::endl; 
    return true;
}

// 随机生成飞机的座标
/**
 * left up right down 飞机生成区域边界 
 * 4 66 2 10
 * **/
PointF GetStartPoint(GameEnv& env,int up,int right){
        auto f = env.GetRandomFunc();
        int l= up,r = right - up;
        int _x = 0,_y = up;
        _x = f(r - l+1) + l;       
        return PointF(_x,_y);
    }


bool MCRuleActor::ActByState(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    // Each unit can only have one command. So we have this map.
    // cout << "Enter ActByState" << endl << flush;
    //cout<<"ActByState"<<endl;
    assigned_cmds->clear();
    *state_string = "NOOP";
    
    
    // Build workers.
    if (state[STATE_BUILD_WORKER]) {
        *state_string = "Build worker..NOOP";
        const Unit *base = _preload.Base();
        if (IsIdle(*_receiver, *base)) {
            if (_preload.BuildIfAffordable(WORKER)) {
                *state_string = "Build worker..Success";
                store_cmd(base, _B(WORKER), assigned_cmds);
            }
        }
    }

    const auto& my_troops = _preload.MyTroops();

    // Ask workers to gather.
    for (const Unit *u : my_troops[WORKER]) {
        if (IsIdle(*_receiver, *u)) {
            // Gather!
            store_cmd(u, _preload.GetGatherCmd(), assigned_cmds);
        }
    }

    // 进攻
    if (state[STATE_BUILD_BARRACK]) {

       // printf("MC Ruler Act\n");
        GameEnv& env_temp = const_cast<GameEnv&>(env); // 需要用到GameEnv的方法
        Player &_player = env_temp.GetPlayer(_player_id);
        *state_string = "Build barracks..NOOP";

        // int cur_round = _player.GetCurrentRound();
        // GamePlan &plan = _player.GetPlan();
        Tick cur_tick = _receiver->GetTick();
        
        // if(cur_round < _player.GetEnemyRound() && cur_tick>= plan.start_tick[cur_round]){
        //     //printf("cur_tick: %d\n",cur_tick);
        //     for(int i=0;i<plan.plan["A"][cur_round];++i){
        //         while(true){
        //             if(CreateFlight(cur_tick,env_temp,_player_id, GetStartPoint(env_temp,5,70),FLIGHT_BASE,cur_tick+3)) break;
        //         }
        //     }
        //     for(int i=0;i<plan.plan["B"][cur_round];++i){
        //         while(true){
        //             if(CreateFlight(cur_tick,env_temp,_player_id, GetStartPoint(env_temp,5,70),FLIGHT_NORMAL,cur_tick+2)) break;
        //         }
        //     }
        //     for(int i=0;i<plan.plan["C"][cur_round];++i){
        //         while(true){
        //             if(CreateFlight(cur_tick,env_temp,_player_id, GetStartPoint(env_temp,5,70),FLIGHT_FAKE,cur_tick+1)) break;
        //         }
        //     }
        //     _player.ChangeCurrentRound(1);
        // }
       

        
       
         if(_preload.HavePlane()){
            // printf("轮询飞机\n");
            for(const Unit* u: my_troops[WORKER]){
                
                if(u!=nullptr){
                    float distance_to_target_test = PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()); 
                    if(distance_to_target_test < 5*5){
                        printf("Wrong Plane: dist_to_target: %f, flight_type: %d\n",sqrt(distance_to_target_test),u->GetProperty().flight_type);
                    }

                    if(u->GetProperty().flight_state != FLIGHT_RETURN){
                          // 向符合要求的飞机下达攻击开始目标，飞向目标
                         if(u->GetProperty().flight_state == FLIGHT_IDLE){  // 飞机处于空闲状态

                            if(cur_tick >= u->GetProperty().start_tick){
                                CmdBPtr cmd = _preload.GetMOVECmd(); // 飞机飞向目标
                                if (cmd != nullptr){
                                   store_cmd(u, std::move(cmd), assigned_cmds); // 向飞机发布移动命令
                                   //std::cout<<"flight start at "<<cur_tick<<std::endl;
                                   env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_MOVE; // 修改飞行状态
                                }
                            }
                        }else if(u->GetProperty().flight_state == FLIGHT_MOVE){  // 飞机处于飞行状态
                             //printf("FLIGHT_MOVE\n");
                           // 判断飞机是否执行攻击  70 - 120 km随机投弹
                           float distance_to_target = PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()); // 计算飞机与目标的距离
                            
                           // 测试攻击命令
                           
                           //if(IsAttack(env_temp,7,12,distance_to_target)){ 
                             // printf("投弹距离 %f\n",sqrt(distance_to_target)) ;
                               int round = u->GetProperty().round; // 载弹量
                               const auto& enemyTroops = _preload.EnemyTroops();
                               auto f = env_temp.GetRandomFunc();
                               

                            //INVALID_FLIGHTTYPE = -1, FLIGHT_NORMAL = 0, FLIGHT_BASE, FLIGHT_FAKE, NUM_FLIGHT); 
                               switch(u->GetProperty().flight_type){
                                   case FLIGHT_BASE:  // 仅攻击基地
                                      // 打光所有弹药
                                      //printf("FLIGHT_BASE\n");
                                      if(IsAttack(env_temp,7,12,distance_to_target)){
                                          //printf("投弹距离 %f\n",sqrt(distance_to_target)) ;
                                          while(round>0){
                                          CmdBPtr cmd = _preload.GetAttackEnemyBaseCmd();
                                          if(cmd != nullptr){
                                               store_cmd(u, std::move(cmd), assigned_cmds);
                                          }
                                          --round;
                                        }
                                         env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_FINISH_ATTACK; 
                                      }
                                     // printf("FLIGHT_BASE Finish\n");
                                      break;
                                   case FLIGHT_NORMAL:    // 攻击防御目标  火炮 雷达
                                      {  
                                         //printf("FLIGHT_NORMAL\n");
                                        if(IsAttack(env_temp,7,12,distance_to_target)){
                                        
                                         // printf("投弹距离 %f\n",sqrt(distance_to_target)) ;
                                            // 随机选择round个目标
                                          int num_melee = 0,num_range = 0;

                                          if(!enemyTroops[MELEE_ATTACKER].empty()){
                                           num_melee = enemyTroops[MELEE_ATTACKER].size();
                                          }
                                          //printf("num_melee: %d\n",num_melee);
                                          if(!enemyTroops[RANGE_ATTACKER].empty()){
                                           num_range = enemyTroops[RANGE_ATTACKER].size();
                                          }
                                          // printf("num_range: %d\n",num_range);
                                           if(num_melee == 0 && num_range == 0){
                                               // 没有目标，返航
                                               env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_FINISH_ATTACK; 
                                               break;
                                           }
                                           
                                          while(round>0){
                                              int select = 0;
                                              int target_id = INVALID;
                                              if(num_melee + num_range > 1){
                                                   select = f(num_melee+num_range-1); // 随机选择一个目标 
                                              }
                                              
                                              
                                              //std::cout<<"select: "<<select<<std::endl;
                                             
                                            
                                            if(select < num_melee){
                                                    //printf("select Meele %d\n",select);
                                                    target_id = enemyTroops[MELEE_ATTACKER][select]->GetId();
                                                }
                                                else{
                                                   // printf("select RANGE %d\n",select-num_melee);
                                                    target_id = enemyTroops[RANGE_ATTACKER][select-num_melee]->GetId();
                                                }
                                              if(target_id != INVALID){
                                                  CmdBPtr cmd = _preload.GetAccackEnemyUnitCmd(target_id); 
                                                   if(cmd != nullptr){
                                                       store_cmd(u, std::move(cmd), assigned_cmds);
                                                    }
                                                    --round;
                                                }   
                                          }
                                          env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_FINISH_ATTACK; 
                                        } 
                                        //printf("FLIGHT_NORMAL Finish\n");
                                      }
                                      
                                      break;
                                   case FLIGHT_FAKE :  // 佯攻飞机不进行投弹
                                      //  printf("FLIGHT_FAKE\n");
                                        if(IsAttack(env_temp,15,18,distance_to_target)){
                                            
                                             //printf("佯攻返航 %f\n",sqrt(distance_to_target)) ;
                                            env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_FINISH_ATTACK; 
                                        }    
                                       // printf("FLIGHT_FAKE Finish\n"); 
                                      break;
                                   default:
                                      break;
                                }
                             //printf("3\n");
                                 //env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_FINISH_ATTACK; // 修改飞行状态
                            //}
                        }else if(u->GetProperty().flight_state == FLIGHT_FINISH_ATTACK){   
                               // 飞机请求返航
                               //*state_string = "Move..Success";
                               CmdBPtr cmd = _preload.GetCIRCLEMOVECmd(); // 飞机返航
                               if (cmd != nullptr){
                                  //*state_string = "Move..Success";
                                  store_cmd(u, std::move(cmd), assigned_cmds); // 向飞机发布移动命令
                                  env_temp.GetUnit(u->GetId())->GetProperty().flight_state = FLIGHT_RETURN; // 修改飞行状态
                                  //std::cout<<" Give Order to "<<u->GetId()<<" State is "<<u->GetProperty().flight_state<<std::endl;
                                }
                            }

                    }
                  
                }
            }
           // printf("轮询飞机 结束\n");
           
        }
        // 根据飞机离目标的距离判定是否攻击
        
      // printf("MC Ruler Act Finish\n");

        
    }

    // If we have barracks with resource, build troops.
    if (state[STATE_BUILD_MELEE_TROOP]) {
        *state_string = "Build Melee Troop..NOOP";
        if (_preload.Affordable(MELEE_ATTACKER)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACKS], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Melee Troop..Success";
                store_cmd(u, _B(MELEE_ATTACKER), assigned_cmds);
                _preload.Build(MELEE_ATTACKER);
            }
        }
    }

    if (state[STATE_BUILD_RANGE_TROOP]) {
        *state_string = "Build Range Troop..NOOP";
        if (_preload.Affordable(RANGE_ATTACKER)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACKS], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Range Troop..Success";
                store_cmd(u, _B(RANGE_ATTACKER), assigned_cmds);
                _preload.Build(RANGE_ATTACKER);
            }
        }
    }

    if (state[STATE_ATTACK]) {
        *state_string = "Attack..Normal";
        // 飞机攻击基地
         vector<const Unit*>  units;
        for (const Unit* u : my_troops[WORKER]){
             // 距离和弹药满足
            if(PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()) < u->GetProperty()._att_r *  u->GetProperty()._att_r && u->GetProperty().round > 0){
                units.push_back(u);
            }
        }
        
        if(units.size() > 0){
            auto cmd = _preload.GetAttackEnemyBaseCmd();
            batch_store_cmds(units, cmd, false, assigned_cmds);
        }
        
        
        // batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
        // batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
    }

    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& all_my_troops = _preload.AllMyTroops();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (state[STATE_HIT_AND_RUN]) {
        *state_string = "Hit and run";
        // cout << "Enter hit and run procedure" << endl << flush;
        if (enemy_troops[MELEE_ATTACKER].empty() && enemy_troops[RANGE_ATTACKER].empty() && ! enemy_troops[WORKER].empty()) {
            // cout << "Enemy only have worker" << endl << flush;
            for (const Unit *u : my_troops[RANGE_ATTACKER]) {
                hit_and_run(env, u, enemy_troops[WORKER], assigned_cmds);
            }
        }
        if (! enemy_troops[MELEE_ATTACKER].empty()) {
            // cout << "Enemy only have malee attacker" << endl << flush;
            for (const Unit *u : my_troops[RANGE_ATTACKER]) {
                hit_and_run(env, u, enemy_troops[MELEE_ATTACKER], assigned_cmds);
            }
        }
        if (! enemy_troops[RANGE_ATTACKER].empty()) {
            auto cmd = _A(enemy_troops[RANGE_ATTACKER][0]->GetId());
            batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
            batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
        }
    }

    if (state[STATE_ATTACK_IN_RANGE]) {  // 向目标发射导弹
         *state_string = "Attack enemy in range..NOOP";
    //   if (! enemy_troops_in_range.empty()) {
    //     *state_string = "Attack enemy in range..Success";
    //     auto cmd = _A(enemy_troops_in_range[0]->GetId());
    //     batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
    //     batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
    //   }
        if(! enemy_troops_in_range.empty()){
            
        }
    }

    if (state[STATE_DEFEND]) {
      // Group Retaliation. All troops attack.
      *state_string = "Defend enemy attack..NOOP";

      const Unit *enemy_at_resource = _preload.EnemyAtResource();
      if (enemy_at_resource != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_resource->GetId()), true, assigned_cmds);
      }

      const Unit *enemy_at_base = _preload.EnemyAtBase();
      if (enemy_at_base != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_base->GetId()), true, assigned_cmds);
      }

      if (! enemy_attacking_economy.empty()) {
        *state_string = "Defend enemy attack..Success";
        auto it = enemy_attacking_economy.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds(all_my_troops, cmd, true, assigned_cmds);
      }
    }
    return true;
}

bool MCRuleActor::GetActSimpleState(vector<int>* state) {
    //std::cout<<"Player "<< _player_id << " GetActSimpleState"<<std::cout;
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    //const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    //const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();
    
    
    _state[STATE_BUILD_BARRACK] = 1;

    // 最远的飞机距离 目标 90km时 发射 第一枚导弹
    // 最远的飞机距离 目标 70km时，发射第二枚导弹
    
    // float max_distance  = 0; 
    // int round = 0;
    // for(const Unit* u : my_troops[WORKER]){
    //     float distance = PointF::L2Sqr(u->GetPointF(),enemy_troops[BASE][0]->GetPointF()); //测量目标与飞机的距离
    //     round = u->GetProperty().round;
    //     if (distance > max_distance)
    //       max_distance = distance;
    // }

    // if(max_distance > 81.0f){ 
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }else if(max_distance< 81.0f && round > 1){
    //     _state[STATE_ATTACK] = 1;
    // }else if(max_distance < 49.0f && round > 0){
    //     _state[STATE_ATTACK] = 1;
    // }else{
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }
    //_state[STATE_BUILD_BARRACK] =1 ;
    // if (my_troops[WORKER].size() < 3 && _preload.Affordable(WORKER)) {
    //     _state[STATE_BUILD_WORKER] = 1;
    // }

    // if (my_troops[WORKER].size() >= 3 && my_troops[BARRACKS].size() + cnt_under_construction[BARRACKS] < 1 && _preload.Affordable(BARRACKS)) {
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }

    // if (my_troops[BARRACKS].size() >= 1 && _preload.Affordable(MELEE_ATTACKER)) {
    //     _state[STATE_BUILD_MELEE_TROOP] = 1;
    // }

    // if (my_troops[MELEE_ATTACKER].size() >= 5 && ! enemy_troops[BASE].empty()) {
    //     _state[STATE_ATTACK] = 1;
    // }
    // if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
    //     _state[STATE_ATTACK_IN_RANGE] = 1;
    //     _state[STATE_DEFEND] = 1;
    // }
    
    return true;
}

bool MCRuleActor::GetActHitAndRunState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (my_troops[WORKER].size() < 3 && _preload.Affordable(WORKER)) {
        _state[STATE_BUILD_WORKER] = 1;
    }
    if (my_troops[WORKER].size() >= 3 && my_troops[BARRACKS].size() + cnt_under_construction[BARRACKS] < 1 && _preload.Affordable(BARRACKS)) {
        _state[STATE_BUILD_BARRACK] = 1;
    }
    if (my_troops[BARRACKS].size() >= 1 && _preload.Affordable(RANGE_ATTACKER)) {
        _state[STATE_BUILD_RANGE_TROOP] = 1;
    }
    int range_troop_size = my_troops[RANGE_ATTACKER].size();
    if (range_troop_size >= 2) {
        if (enemy_troops[MELEE_ATTACKER].empty() && enemy_troops[RANGE_ATTACKER].empty()
          && enemy_troops[WORKER].empty()) {
            _state[STATE_ATTACK] = 1;
        } else {
            _state[STATE_HIT_AND_RUN] = 1;
        }
    }
    if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
        _state[STATE_DEFEND] = 1;
    }
    return true;
}

bool MCRuleActor::ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    vector<vector<RegionHist>> hist(action_map.size());
    for (size_t i = 0; i < hist.size(); ++i) {
        hist[i].resize(action_map[i].size());
    }

    const int x_size = env.GetMap().GetXSize();
    const int y_size = env.GetMap().GetYSize();
    const int rx = action_map.size();
    const int ry = action_map[0].size();

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        const PointF& p = u->GetPointF();
        int x = static_cast<int>(std::round(p.x / x_size * rx));
        int y = static_cast<int>(std::round(p.y / y_size * ry));
        // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
        act_per_unit(env, u, &action_map[x][y][0], &hist[x][y], state_string, assigned_cmds);
    }

    return true;
}
