/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _GAMEDEF_H_
#define _GAMEDEF_H_

#include "cmd.h"
#include "common.h"





// [TODO]: Make it more modular.
// 修改 使用结构体继承关系，将单位的基础属性和特有属性分开
// Define unit property.
struct UnitProperty {
    int _hp, _max_hp;  // 当前hp 最大hp

    int _att, _def;    // 攻击力 防御力(不需要)
    //int _att_r;        // 射程
    float _att_r;        // 射程
    float _speed;      // 速度

    // Visualization range. 视野
    int _vis_r;  

    int _changed_hp;
    UnitId _damage_from;

    // Attributes
    UnitAttr _attr;   // 单位类型 （普通/不可攻击）

    // All CDs.
    vector<Cooldown> _cds;  // CD  MOVE ATTACK GATHER BUILD

    // Used for capturing the flag game.
    int _has_flag = 0;

    int round; //载弹量

    PointF towards = PointF(); //朝向，用于计算FOW 和(0,0)组成一组向量

    // 描述飞机的特性 飞行状态、攻击类型、是否佯攻、出发时间
    FlightState flight_state; // 飞行状态 IDLE -- 空闲  MOVE -- 飞向目标 FINISH_ATTACK -- 完成攻击 RETURN -- 返航
    FlightType flight_type = INVALID_FLIGHTTYPE;   // 飞机类型
    Tick start_tick = -1 ;   // 飞机入场时间

    // 统计单位被攻击的次数
    int times_attack = 0;   
    

    


    inline bool IsDead() const { return _hp <= 0; }
    inline Cooldown &CD(CDType t) { return _cds[t]; }
    inline const Cooldown &CD(CDType t) const { return _cds[t]; }
    inline UnitId GetLastDamageFrom() const { return _damage_from; }

    string Draw(Tick tick) const {
        string s = make_string(_hp, _max_hp);
        for (int i = 0; i < NUM_COOLDOWN; ++i) {
            CDType t = (CDType)i;
            int cd_val = CD(t)._cd;
            int diff = std::min(tick - CD(t)._last, cd_val);
            s += make_string(t, diff, cd_val) + " ";
        }
        return s;
    }
    inline string PrintInfo() const { return std::to_string(_hp) + "/" + std::to_string(_max_hp); }

    UnitProperty()
        : _hp(0), _max_hp(0), _att(0), _def(0), _att_r(0),
        _speed(0.0), _vis_r(0), _changed_hp(0), _damage_from(INVALID), _attr(ATTR_NORMAL),  _cds(NUM_COOLDOWN),round(0),towards(PointF()),flight_state(INVALID_FLIGHTSTATE),flight_type(INVALID_FLIGHTTYPE),start_tick(-1){ }

    SERIALIZER(UnitProperty, _hp, _max_hp, _att, _def, _att_r, _speed, _vis_r, _changed_hp, _damage_from, _attr, _cds);
    HASH(UnitProperty, _hp, _max_hp, _att, _def, _att_r, _speed, _vis_r, _changed_hp, _damage_from, _attr, _cds);
};
STD_HASH(UnitProperty);

struct UnitTemplate {
    UnitProperty _property;
    set<CmdType> _allowed_cmds;
    int _build_cost;

    int GetUnitCost() const { return _build_cost; }
    bool CmdAllowed(CmdType cmd) const {
        return _allowed_cmds.find(cmd) != _allowed_cmds.end();
    }
};

UnitTemplate _C(int cost, int hp, int defense, float speed, int att, float att_r, int vis_r,int round, FlightState flight_state,
        const vector<int> &cds, const vector<CmdType> &l, UnitAttr attr = ATTR_NORMAL);

class GameEnv;
class RTSMap;
struct RTSGameOptions;
class RuleActor;

// GameDef class. Have different implementations for different games.
class GameDef {
public:
    std::vector<UnitTemplate> _units;

    // Initialize everything.
    void Init();

    static void GlobalInit();

    // Get the number of unit type in this game
    static int GetNumUnitType();

    // Get the number of game action in this game
    static int GetNumAction();

    bool IsUnitTypeBuilding(UnitType t) const;
    bool HasBase() const;

    // Check if the unit can be added at current location p.
    bool CheckAddUnit(RTSMap* _map, UnitType type, const PointF& p) const;

    const UnitTemplate &unit(UnitType t) const {
        if (t < 0 || t >= (int)_units.size()) {
            cout << "UnitType " << t << " is not found!" << endl;
            throw std::range_error("Unit type is not found!");
        }
        return _units[t];
    }

    // Get game initialization commands.
    vector<pair<CmdBPtr, int> > GetInitCmds(const RTSGameOptions& options) const;

    // Check winner for the game.
    PlayerId CheckWinner(const GameEnv& env, bool exceeds_max_tick = false) const;

    // Get implementation for different befaviors on dead units.
    void CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId _id, UnitId _target) const;
};
#endif


