#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Week 3: MLFQ Statistics Reporting Tool
// Displays scheduler performance metrics

struct mlfq_stats {
  uint64 total_schedules;
  uint64 total_boosts;
  uint64 total_demotions;
  uint64 level_queue_count[4];
  uint64 level_schedules[4];
};

void
print_header(void)
{
  printf("\n");
  printf("╔════════════════════════════════════════════════════════════════╗\n");
  printf("║           MLFQ Scheduler Statistics Report                     ║\n");
  printf("╚════════════════════════════════════════════════════════════════╝\n");
  printf("\n");
}

void
print_summary(struct mlfq_stats *stats)
{
  printf("┌─ Global Statistics ──────────────────────────────────────────┐\n");
  printf("│ Total Scheduler Cycles:  %20lu                          │\n", stats->total_schedules);
  printf("│ Total Priority Boosts:   %20lu                          │\n", stats->total_boosts);
  printf("│ Total Demotions:         %20lu                          │\n", stats->total_demotions);
  printf("└──────────────────────────────────────────────────────────────┘\n");
  printf("\n");
}

void
print_queue_stats(struct mlfq_stats *stats)
{
  printf("┌─ Per-Queue Statistics ───────────────────────────────────────┐\n");
  printf("│ Queue │ Processes │ Total Schedules │ Schedule %%           │\n");
  printf("├───────┼───────────┼─────────────────┼──────────────────────┤\n");
  
  uint64 total_sched = stats->total_schedules;
  if(total_sched == 0) total_sched = 1;  // Avoid division by zero
  
  for(int i = 0; i < 4; i++) {
    uint64 pct = (stats->level_schedules[i] * 100) / total_sched;
    printf("│   %d   │     %2lu    │      %10lu    │       %3lu%%           │\n",
           i,
           stats->level_queue_count[i],
           stats->level_schedules[i],
           pct);
  }
  
  printf("└───────┴───────────┴─────────────────┴──────────────────────┘\n");
  printf("\n");
}

void
print_analysis(struct mlfq_stats *stats)
{
  printf("┌─ Performance Analysis ───────────────────────────────────────┐\n");
  
  // Demotion rate
  uint64 total_sched = stats->total_schedules;
  if(total_sched == 0) total_sched = 1;
  uint64 demotion_rate = (stats->total_demotions * 1000) / total_sched;
  
  printf("│ Demotion Rate:           %u.%u demotions per 10 schedules  │\n",
         (unsigned)(demotion_rate / 100),
         (unsigned)(demotion_rate % 100));
  
  // Boost frequency
  uint64 boost_freq = 0;
  if(stats->total_boosts > 0) {
    boost_freq = total_sched / stats->total_boosts;
  }
  printf("│ Boost Frequency:         ~1 boost every %lu schedules      │\n", boost_freq);
  
  // Distribution analysis
  printf("│                                                              │\n");
  printf("│ Queue Distribution:                                          │\n");
  for(int i = 0; i < 4; i++) {
    uint64 pct = (stats->level_schedules[i] * 100) / total_sched;
    
    // Print bar chart
    printf("│   Q%d: ", i);
    for(int j = 0; j < (pct / 5); j++) printf("█");
    for(int j = pct / 5; j < 20; j++) printf("·");
    printf(" %2lu%%                           │\n", pct);
  }
  
  printf("└──────────────────────────────────────────────────────────────┘\n");
  printf("\n");
}

void
print_fairness_analysis(struct mlfq_stats *stats)
{
  printf("┌─ Fairness Analysis ──────────────────────────────────────────┐\n");
  
  uint64 total_sched = stats->total_schedules;
  if(total_sched == 0) {
    printf("│ No data available yet                                        │\n");
    printf("└──────────────────────────────────────────────────────────────┘\n");
    return;
  }
  
  // Calculate average schedules per queue
  uint64 avg_per_queue = total_sched / 4;
  uint64 max_dev = 0;
  
  for(int i = 0; i < 4; i++) {
    uint64 dev = 0;
    if(stats->level_schedules[i] > avg_per_queue) {
      dev = stats->level_schedules[i] - avg_per_queue;
    } else {
      dev = avg_per_queue - stats->level_schedules[i];
    }
    if(dev > max_dev) max_dev = dev;
  }
  
  printf("│ Expected schedules/queue: %lu (total/4)                   │\n", avg_per_queue);
  printf("│ Max deviation:            %lu                              │\n", max_dev);
  printf("│                                                              │\n");
  
  if(max_dev < avg_per_queue / 10) {
    printf("│ ✓ EXCELLENT fairness - Low deviation across queues         │\n");
  } else if(max_dev < avg_per_queue / 3) {
    printf("│ ✓ GOOD fairness - Reasonable distribution                  │\n");
  } else {
    printf("│ ⚠ WARNING: High deviation - Check scheduling logic         │\n");
  }
  
  printf("└──────────────────────────────────────────────────────────────┘\n");
  printf("\n");
}

void
print_recommendations(struct mlfq_stats *stats)
{
  printf("┌─ Recommendations ────────────────────────────────────────────┐\n");
  
  uint64 total_sched = stats->total_schedules;
  
  if(total_sched < 100) {
    printf("│ ⓘ Collect more data (current: %lu schedules)                 │\n", total_sched);
  } else if(stats->total_demotions == 0) {
    printf("│ ⚠ No demotions detected - Check if processes are yielding  │\n");
  } else if(stats->total_boosts == 0) {
    printf("│ ⚠ No boosts detected - Check if 100-tick interval running  │\n");
  } else {
    printf("│ ✓ Scheduler appears to be functioning correctly            │\n");
  }
  
  printf("└──────────────────────────────────────────────────────────────┘\n");
  printf("\n");
}

int
main(int argc, char *argv[])
{
  struct mlfq_stats stats;
  
  // Call the syscall to get real kernel statistics
  if(getschedulerstats(&stats) < 0) {
    printf("Error: Failed to get scheduler statistics\n");
    exit(1);
  }
  
  print_header();
  print_summary(&stats);
  print_queue_stats(&stats);
  print_analysis(&stats);
  print_fairness_analysis(&stats);
  print_recommendations(&stats);
  
  exit(0);
}
