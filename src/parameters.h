#ifndef _parameters_h
#define _parameters_h 1

#ifndef WINDOW_SIZE
#define WINDOW_SIZE 256
#endif  // WINDOW_SIZE

#ifndef MAX_NUM_REPLICAS
#define MAX_NUM_REPLICAS 32
#endif  // MAX_NUM_REPLICAS

#ifndef CHECKPOINT_INTERVAL
#define CHECKPOINT_INTERVAL 2
#endif  // CHECKPOINT_INTERVAL

namespace libbyzea {

constexpr int Max_num_replicas = MAX_NUM_REPLICAS;

// Interval in sequence space between "checkpoint" states, i.e.,
// states that are checkpointed and for which Checkpoint messages are
// sent.
constexpr int checkpoint_interval = (WINDOW_SIZE / CHECKPOINT_INTERVAL);
static_assert(checkpoint_interval > 0,
              "Combination of window size and checkpoint interval is invalid");

// Maximum number of messages for which protocol can be
// simultaneously in progress, i.e., messages with sequence number
// higher than last_stable+max_out are ignored. It is required that
// max_out > checkpoint_interval. Otherwise, the algorithm will be
// unable to make progress.
constexpr int max_out = WINDOW_SIZE;

}  // namespace libbyzea

#endif  // _parameters_h
