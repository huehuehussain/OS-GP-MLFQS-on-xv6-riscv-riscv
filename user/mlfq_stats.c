#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Week 3: MLFQ Statistics Reporting Tool - Simplified for xv6 printf

int
main(int argc, char *argv[])
{
  struct mlfq_stats stats;
  
  if(getschedulerstats(&stats) < 0) {
    printf("Error: Failed to get scheduler statistics\n");
    exit(1);
  }
  
  printf("\n===== MLFQ Scheduler Statistics =====\n\n");
  
  printf("Global Statistics:\n");
  printf("  Total Scheduler Cycles: %lu\n", stats.total_schedules);
  printf("  Total Priority Boosts:  %lu\n", stats.total_boosts);
  printf("  Total Demotions:        %lu\n", stats.total_demotions);
  printf("\n");
  
  printf("Per-Queue Statistics:\n");
  printf("  Queue  Processes  Total Schedules  Schedule %%\n");
  
  uint64 total_sched = stats.total_schedules;
  if(total_sched == 0) total_sched = 1;
  
  for(int i = 0; i < 4; i++) {
    uint64 pct = (stats.level_schedules[i] * 100) / total_sched;
    printf("    %d      %lu         %lu          %lu%%\n",
           i,
           stats.level_queue_count[i],
           stats.level_schedules[i],
           pct);
  }
  printf("\n");
  
  uint64 demotion_rate = (stats.total_demotions * 1000) / total_sched;
  printf("Demotion Rate: %lu per 1000 schedules\n", demotion_rate);
  
  uint64 boost_freq = 0;
  if(stats.total_boosts > 0) {
    boost_freq = total_sched / stats.total_boosts;
  }
  printf("Boost Frequency: 1 per %lu schedules\n", boost_freq);
  printf("\n");
  
  printf("Fairness Analysis:\n");
  
  if(total_sched < 100) {
    printf("  Need more data (%lu schedules, require 100+)\n", total_sched);
  } else {
    uint64 avg_per_queue = total_sched / 4;
    uint64 max_dev = 0;
    
    for(int i = 0; i < 4; i++) {
      uint64 dev = 0;
      if(stats.level_schedules[i] > avg_per_queue) {
        dev = stats.level_schedules[i] - avg_per_queue;
      } else {
        dev = avg_per_queue - stats.level_schedules[i];
      }
      if(dev > max_dev) max_dev = dev;
    }
    
    printf("  Expected per queue: %lu\n", avg_per_queue);
    printf("  Max deviation: %lu\n", max_dev);
    
    if(max_dev < avg_per_queue / 10) {
      printf("  Status: EXCELLENT fairness\n");
    } else if(max_dev < avg_per_queue / 3) {
      printf("  Status: GOOD fairness\n");
    } else {
      printf("  Status: WARNING - High deviation\n");
    }
  }
  printf("\n");
  
  printf("Recommendations:\n");
  if(total_sched < 100) {
    printf("  - Run more tests to collect data\n");
  } else if(stats.total_demotions == 0) {
    printf("  - WARNING: No demotions detected\n");
  } else if(stats.total_boosts == 0) {
    printf("  - WARNING: No boosts detected\n");
  } else {
    printf("  - Scheduler functioning correctly!\n");
  }
  printf("\n");
  
  exit(0);
}
