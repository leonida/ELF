/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "cmd.h"
#include "game_env.h"
#include "cmd_receiver.h"

// Derived class. Note that the definition is automatically generated by a python file.
#include "cmd.h"
#include "cmd.gen.h"
#include "cmd_specific.gen.h"

#define PI  3.14159



static const int kMoveToRes = 0;
static const int kGathering = 1;
static const int kMoveToBase = 2;

static const int kMoveToDest = 0;
static const int kBuilding = 1;

static constexpr float kGatherDist = 1;
static constexpr float kBuildDist = 0.05;

static constexpr float kGatherDistSqr = kGatherDist * kGatherDist;
static constexpr float kBuildDistSqr = kBuildDist * kBuildDist;

static bool find_nearby_empty_place(const RTSMap &m, const PointF &curr, PointF *p_nearby);



/*
CMD_DURATIVE(Attack, UnitId, target);
    Durative attack. Will first move to target until in attack range. Then it issues melee attack if melee, and emits a bullet if ranged.
    This will resut in a default counterattack by target.
CMD_DURATIVE(Move, PointF, p);
    Durative move to a point p.
CMD_DURATIVE(Build, UnitType, build_type, PointF, p = PointF(), int, state = 0);
    Move to point p to build a unit of build_type. Also changes resource if necessary.
CMD_DURATIVE(Gather, UnitId, base, UnitId, resource, int, state = 0);
    Move between base and resources to gather resources.
*/

//map<UnitId,pair<float ,PointF>> Points;  //每个飞机进入圆周运动后，存储围绕的圆心和当前角度 <角度、圆心位置>

//////////// Durative Action ///////////////////////
// 飞机移动

//map<UnitId,pair<float,PointF>> Points; 
// 根据 单位位置、 圆心位置 、 以及夹角余弦，求向量与 y = target.y 的 夹角
float Count_Radians(const PointF& u,const PointF target,float cosTheta){
    // 首先判断单位在圆心的上半区[0 - 180] 还是 下半区 (180 , 360)
    float theta = acos(cosTheta);  // 计算 夹角（弧度 0 - PI）
    if(u.y >=  target.y){  //下半区
       theta = 2* PI - theta;  // 计算下半区的theta
    }
   // return theta/PI * 180.0f; // 返回角度
   return theta; // 返回弧度
}



// 计算飞机圆周运动的位置
float Circular_X(float x, float r, float radians){return (x + r * cosf(radians));}
float Circular_Y(float y, float r, float radians){return (y - r * sinf(radians));}

// 给定单位，目标和半径，计算圆心
PointF CountPoint(const Unit& u,const PointF& target,float r){
   PointF point = PointF();
   PointF u_t = PointF(target.x - u.GetPointF().x, target.y - u.GetPointF().y);
   float distance_to_target = sqrt(PointF::L2Sqr(u.GetPointF(),target));
   float u_t_dot_x = u_t.x * 1.0f ;
   float cosTheta = u_t_dot_x / r;
   float theta = Count_Radians(target,u.GetPointF(),u_t_dot_x / distance_to_target); //计算u的角度
   
   float rate = r/distance_to_target; //半径和距离的比值
   point.x = u.GetPointF().x + rate * (target.x - u.GetPointF().x);
   point.y = u.GetPointF().y + rate * (target.y - u.GetPointF().y); 
  
   // 计算圆心位置
   theta += PI/2; //旋转90度
   if(theta >= 2*PI){
               // 控制radians的范围
               theta = theta - 2*PI;
    }
    point.x = Circular_X(u.GetPointF().x,r,theta);
    point.y = Circular_Y(u.GetPointF().y,r,theta);
    return point;
}



PointF circle_move(Tick tick, const Unit& u, const GameEnv &env,  PointF& target, CmdReceiver *receiver) {
     GameEnv& env_temp = const_cast<GameEnv&>(env); // 需要用到GameEnv的方法
    //cout << "circle_move: Current: " << curr << " Target: " << target << endl;
    map<UnitId,pair<float,pair<PointF,PointF>> >& Points = env_temp.GetTrace();
      //float r = sqrt(PointF::L2Sqr(u.GetPointF(),target));
        //    if(Points.find(u.GetId()) != Points.end()){
        //        cout<<"trace target: "<<Points[u.GetId()].second<<" trace radians: "<<Points[u.GetId()].first<<endl;
        //    }
        const float r = 0.1835f; //飞机圆周运动半径
           
           if(Points.find(u.GetId()) == Points.end() || Points[u.GetId()].second.first != target ){ //开始绕新的圆心进行半圆周运动
             //计算初始角度 进入角度为180度 360度出
             
             PointF point = CountPoint(u,target,r);

             //float theta = PI; 
            //  cout<<"Points[u->GetId()].second: "<<Points[u.GetId()].second<<" _p: "<<target<<endl;
             PointF p_u = PointF(u.GetPointF().x-point.x, u.GetPointF().y-point.y);  // p指向U的向量
             float p_u_dot_x = p_u.x * 1.0f ;
             
             float cosTheta = p_u_dot_x / r;
             float theta = Count_Radians(u.GetPointF(),point,p_u_dot_x / r); //计算u的角度
             //cout<<"现在开始绕 "<<point<<" 做圆周运动"<<" 进入角度为 "<<theta/PI * 180.0f<<endl;
             
             Points[u.GetId()] = make_pair(theta,make_pair(target,point) );
             
           }
           
           float radians = Points[u.GetId()].first; //当前角度
           //cout<<"current theta: "<<radians/PI * 180.0f<<" "<<radians<<endl;
           radians += u.GetProperty()._speed / r;  // 飞机移动角度
           //cout<<"new theta: "<<radians/PI * 180.0f<<" " <<radians<<endl;
           if(radians >= 2*PI){
               // 控制radians的范围
               radians = radians - 2*PI;
          }
         // cout<<"New theta: "<< radians/PI * 180.0f<<endl;
          //cout<<"Move: "<<Circular_X(target)
         // cout<<"圆心: "<<Points[u.GetId()].second.second<<endl;
         // cout<<"move.x"<<Circular_X(Points[u.GetId()].second.second.x,r,radians)<<"  move.y: "<<Circular_Y(Points[u.GetId()].second.second.y,r,radians)<<endl;
          //cout<<"Points[u.GetId()].second.second: "<<Points[u.GetId()].second.second<<" r: "<<r<<" radians: "<<radians<<endl;
          PointF towards = PointF(u.GetPointF().x,u.GetPointF().y);
          PointF move = PointF(Circular_X(Points[u.GetId()].second.second.x,r,radians),Circular_Y(Points[u.GetId()].second.second.y,r,radians)); //计算更新的位置
          towards.x = move.x - towards.x;
          towards.y = move.y - towards.y;
          // cout<<"move to: "<<move<<" 角度"<<radians/PI * 180.0f<<" 弧度： "<<radians<<endl;
          receiver->SendCmd(CmdIPtr(new CmdTacticalMove(u.GetId(), move))); //移动飞机
          Points[u.GetId()].first = radians; //更新角度
         
          // return u.GetProperty()._speed / r;  //返回移动的弧度
          return towards;
}


bool Return(Tick tick, const Unit& u, const GameEnv &env,  PointF& towards, CmdReceiver *receiver){
   //GameEnv& env_temp = const_cast<GameEnv&>(env); // 需要用到GameEnv的方法
   //找到下一步的位置
   
   PointF move = PointF();
   move.x = u.GetPointF().x;
   move.y = u.GetPointF().y;
   move.x += towards.x;
   move.y += towards.y;
   int x = env.GetMap().GetXSize();
   int y = env.GetMap().GetYSize();
   if(move.x < 0 || move.x > x || move.y <0 || move.y > y){
       return false;  //超出边界
   }else{
       receiver->SendCmd(CmdIPtr(new CmdTacticalMove(u.GetId(), move))); //移动飞机
       return true;
   }
   
}

//

bool CmdMove::run(const GameEnv &env, CmdReceiver *receiver) {
    //std::cout<<this->PrintInfo()<<std::endl;
    const Unit *u = env.GetUnit(_id);
   //const float r = 0.1835; // 半径
    if (u == nullptr) return false;
    if (micro_move(_tick, *u, env, _p, receiver) < kDistEps){
        _done = true;
    } 
    
    // cout << "id: " << u.GetId() << " from " << u.GetPointF() << " to " << u.GetLastCmd().p << endl;
    return true;
}



// 目标做返航运动
bool CmdCircleMove::run(const GameEnv &env, CmdReceiver *receiver) {
    //std::cout<<this->PrintInfo()<<std::endl;
    const Unit *u = env.GetUnit(_id);
   //const float r = 0.1835; // 半径
    if (u == nullptr) return false;
    if(u->GetUnitType() == WORKER){
       float distance_to_target = PointF::L2Sqr(u->GetPointF(),_p);
       float r = sqrt(distance_to_target);
       //cout<<"distance_to_target: "<<distance_to_target<<endl;
       if(_isReturn){
        if(!Return(_tick, *u, env, _towards, receiver)){
            // 飞离战场
            receiver->SendCmd(CmdIPtr(new CmdOnDeadUnit(_id, _id))); //销毁飞机
            _done = true;
        }    
       }else{
           // Test 绕目标做圆周运动
           if(!_isInCircle){
              circle_move(_tick, *u, env, _p, receiver);
              _radians = 0.0f;
              _isInCircle = true;
               }else{
               _towards = circle_move(_tick, *u, env, _p, receiver);
               _radians += u->GetProperty()._speed / 0.1835f;
              // cout<<"curr radians: "<<radians<<endl;
               //cout<<fabs(radians - PI);
               if(_radians > PI){
                  //cout<<"finish towards: "<<towards<<endl;
                  // cout<<"radians: "<<radians<<endl;
                  // cout<<"length: "<< sqrt(towards.x * towards.x + towards.y * towards.y)<<endl;
                  //设置新的Target
                  _isReturn = true;  
                  // _done = true;       
                 }
              }
        // _done = true;  

      }
    }
    // cout << "id: " << u.GetId() << " from " << u.GetPointF() << " to " << u.GetLastCmd().p << endl;
    return true;
}



/**
 *    检查有没有合适的雷达锁定目标，有的话锁定，返回结果
 * 
 * 
 * **/
bool LockUnit(GameEnv &env,PlayerId _player_id,UnitId _target){
    return false;
}

// ----- Attack
// Attack cmd.target_id
// 持续攻击应该改成单发攻击？
bool CmdAttack::run(const GameEnv &env, CmdReceiver *receiver) {
    //std::cout<<"Attack round = "<<_round<<"Target: "<< _target<<std::endl; 
    GameEnv& env_temp = const_cast<GameEnv&>(env); // 需要用到GameEnv的方法
    const Unit *u = env.GetUnit(_id); // 执行命令的单位
    if (u == nullptr) {
        _done = true;
        return false;
     }
    // 判断弹药量
    if(!(u->GetProperty().round == -1 || u->GetProperty().round >0)){
        //std::cout<<"Out of ammunition"<<std::endl;  
        _done = true;  // 弹药耗尽，无法继续攻击
        return true;
    }
    //clock_t startTime,endTime;
    

    const Unit *target = env.GetUnit(_target);  // 攻击目标单位
    const Player &player = env.GetPlayer(u->GetPlayerId()); // 执行命令单位的玩家

    if (target == nullptr || (player.GetPrivilege() == PV_NORMAL && ! player.FilterWithFOW(*target)))  {
        // The goal is destroyed or is out of FOW, back to idle.
        // FilterWithFOW is checked if the agent is not a KNOW_ALL agent.
        // For example, for AI, they could cheat and attack wherever they want. AI无视FOW
        // For normal player you cannot attack a Unit outside the FOW.  玩家无法攻击FOW外的敌方单位
        // 
        // printf("null unit, id: %d\n",_target);
        if(u->GetUnitType() == BARRACKS){
            // 如果是导弹，目标失活后直接销毁？
            receiver->SendCmd(CmdIPtr(new CmdOnDeadUnit(_id, _id)));  
        }
        _done = true;
        return true;
    }

    //const RTSMap &m = env.GetMap();
    const UnitProperty &property = u->GetProperty();  
    const PointF &curr = u->GetPointF();  // 执行单位位置
    const PointF &target_p = target->GetPointF(); // 目标单位位置

    float dist_sqr_to_enemy = PointF::L2Sqr(curr, target_p);  // 距离
    bool in_attack_range = (dist_sqr_to_enemy <= property._att_r * property._att_r);  // 判断是否在攻击范围内
    // cout << "[" << _id << "] dist_sqr_to_enemy[" << _last_cmd.target_id << "] = " << dist_sqr_to_enemy << endl;
    // if(!in_attack_range){
    //     // cout << "[" << _id << "] dist_sqr_to_enemy[" << dist_sqr_to_enemy  << "att_r : "<<property._att_r * property._att_r<<endl;
    // }
    // Otherwise attack.  
    // if(!property.CD(CD_ATTACK).Passed(_tick) ){
    //     std::cout<<"CD not ready  "<<this->PrintInfo()<<std::endl;
    // }
    if ((property.CD(CD_ATTACK).Passed(_tick)|| _tick == property.CD(CD_ATTACK)._last)&& in_attack_range) {   // 如果目标在攻击范围内且攻击就绪

        /**
         *  飞机攻击
         *  锁定目标
         *  发射导弹
         * */
        
        
        if(u->GetUnitType() == WORKER){  // 如果是飞机
              // 制造一个导弹
              //cout<<u->GetUnitType()<<" 正在攻击 "<<target->GetUnitType()<<endl;
              PointF build_p;  //创造导弹的地点
              const RTSMap &m = env.GetMap();
              build_p.SetInvalid();
              find_nearby_empty_place(m, curr, &build_p);
              if (! build_p.IsInvalid()) {
                   // 创造导弹 
                   //cout<<"创造导弹"<<endl;
                   UnitId rocket_id;
                    
                   if (! env_temp.AddUnit(_tick, BARRACKS, build_p, u->GetPlayerId(),rocket_id)) {
                        std::cout<<"emit rocket failed"<<std::endl;
                        return false;
                    }
                    //载弹量 -1
                    env_temp.GetUnit(_id)->GetProperty().round -= 1;
                    // 发射导弹(让导弹去攻击目标)
                    receiver->SendCmd(CmdBPtr(new CmdAttack(rocket_id, _target)));
                    _done = true;
                }
            
                return true;
        }else if(u->GetUnitType() == RANGE_ATTACKER){  // 如果是雷达
            // 攻击命令改为锁定目标
            env_temp.UpdateTargets(player.GetId());   // 先更新锁定目标列表
            
            if(!player.isUnitLocked(_target)){
                //锁定目标
                env_temp.Lock(u->GetPlayerId(),u->GetId(),_target);
            }
             std::cout<<env_temp.PrintTargetsInfo(u->GetPlayerId())<<std::endl;
             //printf("Radar Round: %d  Round Left: %d\n",u->GetProperty().round,env_temp.GerRadarRound(u->GetId()));
            // std::cout<<"剩余索敌数量： "<<env_temp.GerRadarRound(u->GetId())<<std::endl;
            _done = true;
            return true;
        } else if(u->GetUnitType() == MELEE_ATTACKER){  // 炮塔
            
            //std::cout<<"Start_Tick "<<_start_tick<<" curr_tick "<< _tick<<" Durative: "<<_tick - _start_tick<<endl;
            
            
            // 先判断目标是不是锁定目标
            // env_temp.UpdateTargets(player.GetId());
            // if(!player.isUnitLocked(_target)){
            //     //std::cout<<"Target not Lock"<<std::endl;
            //     _done = true;
            //     return true;
            // }

            // 攻击前先判断目标有没有锁定
            env_temp.UpdateTargets(player.GetId());   // 先更新锁定目标列表
            if(!player.isUnitLocked(_target)){
                //printf("target: %d is not Locked!\n",_target);
                if(!env_temp.Lock(player.GetId(),_target)){
                   // printf("索敌失败，没有合适的雷达!\n");
                    _done = true;
                    return true;
                }
               // printf("分配雷达发动攻击\n");
               // std::cout<<env_temp.PrintTargetsInfo(u->GetPlayerId())<<std::endl;
            }
            
            //载弹量 -n
            int round =  env_temp.GetUnit(_id)->GetProperty().round;
            if(round<_round) _round = round;  // 实际发射的导弹数量
            Player& player= env_temp.GetPlayer(u->GetPlayerId());  // 获得玩家
            
            switch(_round){
                case 1:
                  player.ChangeReward(-0.002);  // 单发 -0.02
                  break;
                case 2:
                  player.ChangeReward(-0.005);  // 双发 -0.05
                  break;
                default:
                  break;
            }
            //std::cout<<"奖励： "<<player.GetPlayerReward()<<std::endl;
            env_temp.GetUnit(_id)->GetProperty().round  -= _round;
            //printf("防御塔  [%f , %f]  发射了 %d 枚导弹  目标位置 [%f , %f]  剩余载弹量: %d\n",u->GetPointF().x,u->GetPointF().y,_round,target_p.x,target_p.y,u->GetProperty().round);
            for(int i=0;i<_round;++i){
                 receiver->SendCmd(CmdIPtr(new CmdEmitBullet(_id, _target, curr, -property._att, property._att_r,0.1,_round)));
                 // Test 玩家发射子弹
                 env_temp.GetPlayer(u->GetPlayerId()).ChangeAmmoLoss(1);
            }
             //printf("防御塔: %d 攻击： %d, 发射了 %d 发导弹 载弹量 %d\n",_id,_target,_round,env_temp.GetUnit(_id)->GetProperty().round);
            
            receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_ATTACK)));  // 重置攻击CD
            _done = true;
            return true;
        }
        receiver->SendCmd(CmdIPtr(new CmdMeleeAttack(_id, _target, -property._att)));  // 实际地减少目标的生命值
        
        // if (property._att_r <= 1.0) {
        //     receiver->SendCmd(CmdIPtr(new CmdMeleeAttack(_id, _target, -property._att)));
        // } else {
        //     receiver->SendCmd(CmdIPtr(new CmdEmitBullet(_id, _target, curr, -property._att, 0.1)));
        // }
        receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_ATTACK)));  // 重置攻击CD
        if(u->GetUnitType() == BARRACKS){ // 导弹执行攻击后销毁
            receiver->SendCmd(CmdIPtr(new CmdOnDeadUnit(_id, _id)));  
        }
    } else if (! in_attack_range) {  
       if(u->GetUnitType() == BARRACKS){
           micro_move(_tick, *u, env, target_p, receiver);   // 如果是导弹不在攻击范围内时移动单位接近攻击目标
       }else{
           _done = true;
       }
    }
    

    // In both case, continue this action.
    return true;
}

// ---- Gather
bool CmdGather::run(const GameEnv &env, CmdReceiver *receiver) {
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;

    // Gather resources back and forth.
    const Unit *resource = env.GetUnit(_resource);
    const Unit *base = env.GetUnit(_base);
    if (resource == nullptr || base == nullptr) {
        // Either resource or base is gone. so we stop.
        _done = true;
        return false;
    }
    //const RTSMap &m = env.GetMap();
    //const PointF &curr = u->GetPointF();
    const UnitProperty &property = u->GetProperty();
    const PointF &res_p = resource->GetPointF();
    const PointF &base_p = base->GetPointF();

    //  cout << "[" << tick << "] [" << cmd_state.state << "]" << " CD_MOVE: "
    //       << property.CD(CD_MOVE).PrintInfo(tick) << "  CD_GATHER: "
    //       << property.CD(CD_GATHER).PrintInfo(tick) << endl;
    switch(_state) {
        case kMoveToRes:
            if (micro_move(_tick, *u, env, res_p, receiver) < kGatherDistSqr) {
                // Switch
                receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_GATHER)));
                _state = kGathering;
            }
            break;
        case kGathering:
            // Stay for gathering.
            if (property.CD(CD_GATHER).Passed(_tick)) {
                receiver->SendCmd(CmdIPtr(new CmdHarvest(_id, _resource, -5)));
                _state = kMoveToBase;
            }
            break;
        case kMoveToBase:
            // Get the location of closest base.
            // base_p = m.FindClosestBase(curr, Player::GetPlayerId(u.GetId()));
            if (micro_move(_tick, *u, env, base_p, receiver) < kGatherDistSqr) {
                // Switch
                receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(_id, u->GetPlayerId(), 5)));
                _state = kMoveToRes;
            }
            break;
    }
    return true;
}

// ------ Build
// Move to nearby location at cmd.p and build at cmd.p
// For fixed building, we just set cmd.p as its current location.
static bool find_nearby_empty_place(const RTSMap &m, const PointF &curr, PointF *p_nearby) {
    PointF nn;
    nn = curr.Left(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.Right(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.Up(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.Down(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }

    nn = curr.LT(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.LB(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.RT(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.RB(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }

    nn = curr.LL(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.RR(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.TT(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }
    nn = curr.BB(); if (m.CanPass(nn, INVALID)) { *p_nearby = nn; return true; }

    return false;
}

bool CmdBuild::run(const GameEnv &env, CmdReceiver *receiver) {
    //cout << "CmdBuild cmd = " << PrintInfo() << endl;
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;

    const RTSMap &m = env.GetMap();
    const PointF& curr = u->GetPointF();
    const UnitProperty &p = u->GetProperty();
    //const Player &player = env.GetPlayer(u->GetPlayerId());
    int cost = env.GetGameDef().unit(_build_type).GetUnitCost();

    // When cmd.p = INVALID, build nearby.
    // Otherwise move to nearby location of cmd.p and build at cmd.p
    // cout << "Build cd: " << property.CD(CD_BUILD).PrintInfo(tick) << endl;
    switch(_state) {
        case kMoveToDest:  
             //cout << "build_act stage 0 cmd = " << PrintInfo() << endl;
            if (_p.IsInvalid() || PointF::L2Sqr(curr, _p) < kBuildDistSqr) {
                // Note that when we are out of money, the command CmdChangePlayerResource will terminate this command.
                // cout << "Build cost = " << cost << endl;
                receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(_id, u->GetPlayerId(), -cost)));
                receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_BUILD)));
                _state = kBuilding;
            } else {
                // Move to nearby location.
                PointF nearby_p;
                if (find_nearby_empty_place(m, _p, &nearby_p)) {
                    micro_move(_tick, *u, env, _p, receiver);
                }
            }
            break;
        case kBuilding:
            // Stay for building.
            if (p.CD(CD_BUILD).Passed(_tick)) {
                PointF build_p;
                if (_p.IsInvalid()) {
                    build_p.SetInvalid();
                    find_nearby_empty_place(m, curr, &build_p);
                } else {
                    build_p = _p;
                }
                if (! build_p.IsInvalid()) {
                    receiver->SendCmd(CmdIPtr(new CmdCreate(_id, _build_type, build_p, u->GetPlayerId(), cost)));
                    _done = true;
                }
            }
            break;
    }
    return true;
}

/**
 * round 单发/多发  P(d)= d>200km？ 0：P  单发P＝0.64，双发P=0.87
 * **/
bool IsHit(GameEnv *env,float dist_tower_to_enemy,float range,int round) {
    //printf("att_r: %f\n",range);
    if(dist_tower_to_enemy>range*range) return false;
    auto f = env->GetRandomFunc();
    int select = f(99)+1;
    // int p = (round == 1? 64:87);
    int p = 64;
    //std::cout<<"select: "<<select<<" p: "<<p<<std::endl;
    
    return select<=p; 
}


bool CmdMeleeAttack::run(GameEnv *env, CmdReceiver *receiver) {
    Unit *target = env->GetUnit(_target);
    //Unit *u = env->GetUnit(_id);
    // if(u==nullptr){
    //     printf("wrong nullptr!\n");
    // }
    


    if (target == nullptr) return true;

    UnitProperty &p_target = target->GetProperty();  // 获取目标属性
    if (p_target.IsDead()) return true;  // 目标已经被销毁，取消攻击命令

    const auto &target_tp = env->GetGameDef().unit(target->GetUnitType());  // 获得target 单位UnitTemplate
    //计算伤害
    int changed_hp = _att;
    // if (changed_hp < 0) {  //单位不设防御力
    //     changed_hp += p_target._def;  // 考虑目标单位的防御力
    //     if (changed_hp > 0) changed_hp = 0;  // 目标收到的伤害值最小不得低于0
    // }
    float dist_tower_to_enemy = PointF::L2Sqr(_p_tower, target->GetPointF());  // 距离
    //std::cout<<"_p_tower: "<<_p_tower<<" _p_target: "<<target->GetPointF()<<" distance: "<< dist_tower_to_enemy<<std::endl;
    if(_isRandom){  // 是否考虑命中概率
        // 只有敌方单位考虑命中概率
        // printf("目标: %d  进行命中判定，当前被 %d 发子弹攻击\n",_target,p_target.times_attack);
        if(!IsHit(env,dist_tower_to_enemy,_att_r,_round) ){   //判定是否击中目标
            //std::cout<<"======攻击无效========="<<std::endl;
            --p_target.times_attack;  // 减少目标受到攻击的次数
           // printf("没有命中!,当前目标被 %d 发子弹攻击\n",p_target.times_attack);
            return true;  //判定这次攻击无效
        }
       // printf("子弹击中目标: %d\n",_target);
    }
    

    p_target._hp += changed_hp;  // 修改目标hp 击中目标
    //解除目标的锁定
    // if(u==nullptr){
    //     printf("wrong nullptr!\n");
    // }
    //Player& player = env->GetPlayer(u->GetPlayerId());
    Player& player = env->GetPlayer(0); // 需要修改，默认玩家id为0
    env->UpdateTargets(player.GetId());
    if(player.isUnitLocked(_target)){
        env->UnLock(player.GetId(),_target);
    }

    //根据击中目标为玩家修改奖励
    Player& player_target = env->GetPlayer(target->GetPlayerId());  // 获得玩家
    if(player_target.GetId() == 0){
        // 炮塔、雷达 -2 基地 -5
        //std::cout<<"被击中的是玩家单位 uid: "<<target->GetId()<<std::endl;
        if(target->GetUnitType() == MELEE_ATTACKER){  // 炮塔
            player_target.ChangeReward(-0.1f);
            player_target.ChangeTowerLoss(1);
            //printf("攻击时间： %d,防御塔被击中 \n",_tick);
        }else if(target->GetUnitType() == RANGE_ATTACKER){ // 雷达
            player_target.ChangeReward(-0.5f);
            player_target.ChangeRadarLoss(1);
            //printf("攻击时间： %d,雷达被击中 \n",_tick);
        }else if(target->GetUnitType() == BASE){ // 基地  
            player_target.ChangeReward(-1.0f);
            player_target.ChangeBaseHit(1);
            // std::cout<<"被击中的是基地: reward: "<<player_target.GetReward()<<" last_reward: "<<player_target.GetLastReward()<<std::endl;
          // printf("攻击时间： %d,基地被击中 \n",_tick);
        }
        //std::cout<<"奖励： "<<player_target.GetPlayerReward()<<std::endl;
    }else {   // 击中敌方单位，那么发出攻击的是玩家
        // 飞机 1 导弹 0.5
        //std::cout<<"被击中的是敌方单位 uid: "<<target->GetId()<<std::endl;
        if(target->GetUnitType() == WORKER){ // 飞机
            // std::cout<<"被击中的是飞机"<<std::endl;
             if(target->GetProperty().flight_type == FLIGHT_BASE){
                 player.ChangeReward(0.2f); 
                 player.ChangeEnemyA(1);
             }else if(target->GetProperty().flight_type == FLIGHT_NORMAL){
                 player.ChangeReward(0.1f); 
                 player.ChangeEnemyB(1);
             }else if(target->GetProperty().flight_type == FLIGHT_FAKE){
                 player.ChangeReward(0.01f); 
                 player.ChangeEnemyC(1);
             }
             
             
            // printf("攻击时间： %d,在 [%f , %f] 击中飞机 \n",_tick,target->GetPointF().x,target->GetPointF().y);
        }else if(target->GetUnitType() == BARRACKS){ // 导弹
            // std::cout<<"被击中的是导弹"<<std::endl;
             player.ChangeReward(0.05f);
             player.ChangeEnemyBullet(1);
             
            // printf("攻击时间： %d,在  [%f, %f] 击中导弹\n",_tick,target->GetPointF().x,target->GetPointF().y);
        }
       // std::cout<<"奖励： "<<player.GetPlayerReward()<<std::endl;
    }
    
    if (p_target.IsDead()) {
        receiver->SendCmd(CmdIPtr(new CmdOnDeadUnit(_id, _target)));  // 目标死亡，执行环境命令处理目标尸体
    } else if (changed_hp < 0) {
        // 记录目标收到的伤害
        p_target._changed_hp = changed_hp;
        p_target._damage_from = _id;
        // if (receiver->GetUnitDurativeCmd(_target) == nullptr && target_tp.CmdAllowed(ATTACK)) {
        //     // Counter attack.
        //     receiver->SendCmd(CmdDPtr(new CmdAttack(_target, _id))); // 目标反击（不需要）
        // }
    }
    return true;
}

bool CmdChangePlayerResource::run(GameEnv *env, CmdReceiver *receiver) {
    Player &player = env->GetPlayer(_player_id);
    int curr_resource = player.ChangeResource(_delta);
    if (curr_resource < 0) {
        // Change it back and cancel the durative command of _id.
        // cout << "Cancel the build from " << _id << " amount " << _delta << " player_id " << _player_id << endl;
        player.ChangeResource(-_delta);
        receiver->FinishDurativeCmd(_id);
        return false;
    }
    return true;
}

bool CmdOnDeadUnit::run(GameEnv *env, CmdReceiver *receiver) {
    env->GetGameDef().CmdOnDeadUnitImpl(env, receiver, _id, _target);
    return true;
}

// For gathering.
bool CmdHarvest::run(GameEnv *env, CmdReceiver *receiver) {
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return false;

    UnitProperty &p_target = target->GetProperty();
    if (p_target.IsDead()) return false;

    p_target._hp += _delta;
    if (p_target.IsDead()) receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
    return true;
}


// 敌方目标移动指令
// bool CmdEnemyMove::run(const GameEnv &env, CmdReceiver *receiver) {
//     //std::cout<<this->PrintInfo()<<std::endl;
//     // const Unit *u = env.GetUnit(_id);
//     // if (u == nullptr) return false;

//     // // cout << "id: " << u.GetId() << " from " << u.GetPointF() << " to " << u.GetLastCmd().p << endl;
//     // if (micro_move(_tick, *u, env, _p, receiver) < kDistEps) _done = true;
//     return true;
// }