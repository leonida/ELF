# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from torch.autograd import Variable
from torch.distributions import Normal

from copy import deepcopy
from collections import Counter
import numpy as np

from rlpytorch import Model, ActorCritic
from actor_critic_changed import ActorCriticChanged
from trunk import MiniRTSNet
from trunk import ProcessSet, DataProcess,TargetAttention,TowerAttention
from td_sampler import TD_Sampler

def flattern(x):
    return x.view(x.size(0), -1)


class Model_ActorCritic(Model):
    def __init__(self, args):
        super(Model_ActorCritic, self).__init__(args)
        self._init(args)

    def _init(self, args):
        params = args.params
        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        #self.net = MiniRTSNet(args, output1d=False)

        self.na = params["num_action"]
        self.num_unit = params["num_unit_type"]
        self.num_planes = params["num_planes"]
        self.num_cmd_type = params["num_cmd_type"]
        self.mapx = params["map_x"]
        self.mapy = params["map_y"]

        out_dim = self.num_planes * self.mapx * self.mapy

        # After trunk, we can predict the unit action and value.
        # Four dimensions. unit loc, target loc,
        #self.unit_locs = nn.Conv2d(self.num_planes, 1, 3, padding=1)
        #self.target_locs = nn.Conv2d(self.num_planes, 1, 3, padding=1)
        
        # test
        #self.unit_select = nn.Linear(self.mapx * self.mapy,20)
        #self.target_select = nn.Linear(self.mapx * self.mapy,20)

        #self.cmd_types = nn.Linear(out_dim, self.num_cmd_type)
        # self.cmd_types = nn.Linear(out_dim, 3)

        #self.build_types = nn.Linear(out_dim, self.num_unit)
        # self.value = nn.Linear(out_dim, 1)
        self.value_test = nn.Linear(256,1)

        # self.relu = nn.LeakyReLU(0.1)
        self.softmax = nn.Softmax()

        # test
        # self.radarProcess = ProcessSet(args,2, 3, 12)
        #self.towerProcess = ProcessSet(args,16, 8, 32)
        # self.enemyProcess = ProcessSet(args,20, 7, 28)
        # self.baseProcess =  ProcessSet(args,1,3,12)
        # self.dataProcess  = DataProcess(args,baseProcess,radarProcess,towerProcess,enemyProcess)
        self.dataProcess  = DataProcess(args,ProcessSet(args,1,3,50),ProcessSet(args,2, 9, 100),ProcessSet(args,20, 8, 100),ProcessSet(args,20, 9, 100))
        self.round_select = nn.Linear(256,3)
        self.enemy_select = TargetAttention(args,256,100)
        self.tower_select = TowerAttention(args,256,100,100)
        self.td_sampler = TD_Sampler(0.1) # epsilon
        

        
    def get_define_args():
        return MiniRTSNet.get_define_args()

    def forward(self, x):
        #s, res = x["s"], x["res"]
        # output = self.net(self._var(x["s"]), self._var(x["res"]))
        
        # s = x["s"]
        # output = self.net(self._var(x["s"])) # 8 10 70 70
 
        #unit_locs = self.softmax(flattern(self.unit_locs(output)))   # 18
        # target_locs = self.softmax(flattern(self.target_locs(output))) # 8
        #unit_locs = self.softmax(self.unit_select(flattern(self.unit_locs(output))) )   # 20
        #target_locs = self.softmax( self.target_select(flattern(self.target_locs(output))) ) # 20

        # flat_output = flattern(output)
        #cmd_types = self.softmax(self.cmd_types(flat_output))
        #build_types = self.softmax(self.build_types(flat_output))
        #value = self.value(flat_output)
        
        # test
        
        self.dataProcess.set_volatile(self.volatile)
        base,radar,tower,tower_embedding,enemy,enemy_embedding,global_stat = self.dataProcess(x)
        self.dataProcess.set_volatile(False)
        
        value_test = self.value_test(global_stat)
        
        
        round_mu = self.round_select(global_stat)
        round_sigma = self.round_select(global_stat) # sac
        round_sigma = torch.clamp(round_sigma, -20, 2) 
        round_std = round_sigma.exp()
        dist = Normal(round_mu, round_std)
        e = dist.sample()
        round_prob = self.softmax(e)
        
        round_choose = self.td_sampler.sample_with_eps(round_prob)  #  ????????????


        enemy_mu = self.enemy_select(global_stat,enemy_embedding,x['s_enemy'],True)# ???????????????
        enemy_sigma = self.enemy_select(global_stat,enemy_embedding,x['s_enemy'],True)
        enemy_sigma = torch.clamp(enemy_sigma, -20, 2)
        enemy_std = enemy_sigma.exp()
        
        dist = Normal(enemy_mu, enemy_std)
        e = dist.sample()
        enemy_prob = self.softmax(e)
        enemy_choose =  self.td_sampler.sample_with_eps(enemy_prob)  #  ????????????
        
        # ?????????????????????????????????
        batch_size = enemy_embedding.data.shape[0]
        enemy_embedding_list = []
        enemy_mask_list = []
        
        for i in range(batch_size):
            enemy_embedding_list.append(enemy_embedding[i][enemy_choose[i].data])
            enemy_mask_list.append(x['s_enemy_mask'][i][enemy_choose[i].data])
        
        
        enemy_embedding_select = Variable(torch.cat(enemy_embedding_list).data,volatile = self.volatile)
        enemy_embedding_select = enemy_embedding_select.view(batch_size,-1)
        # import pdb
        # pdb.set_trace()
        enemy_mask_select = torch.cat(enemy_mask_list)
        enemy_mask_select = enemy_mask_select.view(batch_size,-1)
        
        # ??????????????????????????????????????????????????????mask
        tower_mu = self.tower_select(global_stat,enemy_embedding_select,enemy_mask_select,tower_embedding,x['s_tower'],True)
        tower_sigma = self.tower_select(global_stat,enemy_embedding_select,enemy_mask_select,tower_embedding,x['s_tower'],True)
        tower_sigma = torch.clamp(tower_sigma, -20, 2)
        tower_std = tower_sigma.exp()
        dist = Normal(tower_mu, tower_std)
        e = dist.sample()
        tower_prob = self.softmax(e)
         
        # tower_attention = Variable(tower_attention.data)
        tower_choose =  self.td_sampler.sample_with_eps(tower_prob)  # ?????????
        
        # import pdb
        # pdb.set_trace()
        return dict(V=value_test, uloc_prob=tower_prob, tloc_prob=enemy_prob, ct_prob = round_prob,action_type=1),dict(uloc = tower_choose.data,uloc_prob=tower_prob.data,tloc=enemy_choose.data,tloc_prob=enemy_prob.data,ct=round_choose.data,ct_prob = round_prob.data)
        # return dict(V=value_test, uloc_prob=tower_attention, tloc_prob=enemy_attention, ct_prob = round_prob,action_type=1)
        #return dict(V=value, uloc_prob=unit_locs, tloc_prob=target_locs, ct_prob = cmd_types,action_type=1)

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
}

Defaults = {
    #"sample_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct;bt_prob,bt",
    #"policy_action_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct;bt_prob,bt",
    "sample_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct",
    "policy_action_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct",
    "arch" : "cccc;-,64,64,64,-"
}
