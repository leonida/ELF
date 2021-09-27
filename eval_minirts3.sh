MODEL=$1
FRAMESKIP=$2
NUM_EVAL=500

game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py --gpu 0 --save_replay_prefix ./res --num_games 128 --batchsize 32 --tqdm --load save-955.bin --players "fs=20,type=AI_NN;fs=20,type=AI_SIMPLE" --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal --arch "ccpccp;-,-,-,-,-" --greedy --keys_in_reply V  --save_replay_prefix ./res
#--omit_keys Wt,Wt2,Wt3 #

