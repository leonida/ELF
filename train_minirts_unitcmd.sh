game=./rts/game_MC/game model=actor_critic model_file=./rts/game_MC/model_unit_cmd python3 train.py --batchsize 128 --freq_update 50 --players "type=AI_NN,fs=5;type=AI_SIMPLE,fs=10" --num_games 1024 --tqdm --T 20 --additional_labels id,last_terminal --trainer_stats winrate --use_unit_action --keys_in_reply V --num_episode 200 "$@" 
# --save_replay_prefix replay


