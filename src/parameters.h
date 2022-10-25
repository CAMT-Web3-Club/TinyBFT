#ifndef _parameters_h
#define _parameters_h 1

#ifndef WINDOW_SIZE
#define WINDOW_SIZE 256
#endif  // WINDOW_SIZE

namespace libbyzea {

const int Max_num_replicas = 32;

// Interval in sequence space between "checkpoint" states, i.e.,
// states that are checkpointed and for which Checkpoint messages are
// sent.
constexpr int checkpoint_interval = (WINDOW_SIZE / 2);
static_assert(checkpoint_interval > 0,
               "Invalid window size causes checkpoint interval to be 0");

// Maximum number of messages for which protocol can be
// simultaneously in progress, i.e., messages with sequence number
// higher than last_stable+max_out are ignored. It is required that
// max_out > checkpoint_interval. Otherwise, the algorithm will be
// unable to make progress.
constexpr int max_out = WINDOW_SIZE;

}  // namespace libbyzea

#endif  // _parameters_h
