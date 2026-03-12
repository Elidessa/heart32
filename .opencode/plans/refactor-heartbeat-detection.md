# Refactor Heartbeat Detection in sampler.c

## Goal
Replace the implicit dual-flag beat detection logic with an explicit state machine, while keeping the same Schmitt trigger / dual-threshold technique.

## Changes

### File: `main/sampler.c`

#### 1. Replace globals and old `calculate_bpm` (lines 11-79) with:

**Remove these variables** (no longer needed):
- `bpm_previous`
- `prev_sample`
- `bpm_str`
- `beat[]` array (was declared but never used)
- `is_rising` and `over_upper_threshold` static locals inside calculate_bpm

**Make these static** (no reason to be globally visible):
- `beat_times[]`
- `time_since_last_beat`

**Add constants:**
```c
#define THRESHOLD_FLOOR 200
#define THRESHOLD_DECAY 0.99f
#define UPPER_RATIO 0.75f
#define LOWER_RATIO 0.50f
#define BEAT_TIMEOUT_MS 2000
#define SAMPLE_PERIOD_MS 20
```

**Add state enum:**
```c
typedef enum { WAITING_FOR_BEAT, IN_BEAT } beat_state_t;
```

**Add `register_beat()` helper** that:
- Shifts the circular `beat_times` buffer
- Stores the current `time_since_last_beat`
- Recalculates BPM (only called on actual beats, not every cycle)

**Rewrite `calculate_bpm()`** as explicit state machine:
- `WAITING_FOR_BEAT`: transition to `IN_BEAT` when sample >= upper_threshold
- `IN_BEAT`: track peak, transition back when sample <= lower_threshold (registers beat, updates thresholds from actual peak)
- Threshold decay and timeout logic remain the same

**Remove debug printf statements.**

#### Full replacement for lines 11-79:

```c
#define BEATS_FOR_BPM 10
#define THRESHOLD_FLOOR 200
#define THRESHOLD_DECAY 0.99f
#define UPPER_RATIO 0.75f
#define LOWER_RATIO 0.50f
#define BEAT_TIMEOUT_MS 2000
#define SAMPLE_PERIOD_MS 20

typedef enum { WAITING_FOR_BEAT, IN_BEAT } beat_state_t;

void calculate_bpm(int sample);
volatile int bpm;

static unsigned int beat_times[BEATS_FOR_BPM] = {0};
static int time_since_last_beat = 0;

static adc_oneshot_unit_handle_t adc_handle;
QueueHandle_t sample_queue = NULL;
float highpass_filter(float value);
float lowpass_filter(float value);

static void register_beat(void){
    for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
        beat_times[i] = beat_times[i - 1];
    }
    beat_times[0] = time_since_last_beat;
    time_since_last_beat = 0;

    int t_sum = 0;
    int count = 0;
    for(int i = 0; i < BEATS_FOR_BPM; i++){
        if(beat_times[i] > 0){
            t_sum += beat_times[i];
            count++;
        }
    }
    if(count > 5){
        bpm = (count * 60) / ((float)t_sum / 1000);
    }
}

void calculate_bpm(int sample){
    static beat_state_t state = WAITING_FOR_BEAT;
    static int upper_threshold = 1000;
    static int lower_threshold = 800;
    static int peak_value = 0;

    time_since_last_beat += SAMPLE_PERIOD_MS;

    switch(state){
    case WAITING_FOR_BEAT:
        if(sample >= upper_threshold){
            state = IN_BEAT;
            peak_value = sample;
        }
        break;

    case IN_BEAT:
        if(sample > peak_value){
            peak_value = sample;
        }
        if(sample <= lower_threshold){
            register_beat();
            upper_threshold = peak_value * UPPER_RATIO;
            lower_threshold = peak_value * LOWER_RATIO;
            state = WAITING_FOR_BEAT;
        }
        break;
    }

    // Decay thresholds toward floor
    if(upper_threshold > THRESHOLD_FLOOR)
        upper_threshold *= THRESHOLD_DECAY;
    else
        upper_threshold = THRESHOLD_FLOOR;

    if(lower_threshold > THRESHOLD_FLOOR)
        lower_threshold *= THRESHOLD_DECAY;
    else
        lower_threshold = THRESHOLD_FLOOR;

    // No beat for 2 seconds: clear oldest beat, reset BPM
    if(time_since_last_beat > BEAT_TIMEOUT_MS){
        for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
            beat_times[i] = beat_times[i - 1];
        }
        beat_times[0] = 0;
        bpm = 0;
    }
}
```

### No changes needed to:
- `sampler.h` (the public interface `bpm`, `sample_queue`, `sampler_init()` remain the same)
- `periodic_timer_callback()` (calls `calculate_bpm` the same way)
- `highpass_filter()` / `lowpass_filter()` (unchanged)
- `sampler_init()` (unchanged)

## Key improvements
| Aspect | Before | After |
|---|---|---|
| State tracking | Implicit via `is_rising` + `over_upper_threshold` | Explicit `enum` state machine |
| Peak tracking | None (used `prev_sample` at threshold crossing) | Tracks actual peak while `IN_BEAT` |
| BPM recalculation | Every call (50 Hz) | Only when a beat is registered |
| Threshold accuracy | Based on `prev_sample` at upper crossing | Based on actual tracked peak |
| Dead variables | `bpm_previous`, `bpm_str`, `beat[]` unused | Removed |
| Magic numbers | Hardcoded `200`, `0.99`, `0.75`, etc. | Named constants |
| Debug prints | Left in production code | Removed |
