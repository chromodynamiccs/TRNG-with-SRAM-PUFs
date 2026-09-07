# SRAM-based TRNG — decay-to-natural-state experiment

**Context:** Undergrad hardware research (Prof. Ray, collaborator Abishek). Platform: CY7C1041 SRAM chip + Arduino Mega + IRF4905 P-channel MOSFET for software power switching (inverted logic: HIGH=off, LOW=on). Project root: `arduino/`, `data/`, `scripts/`.

**Goal of this experiment:** Measure how long the SRAM must stay powered off before written data (all 1s) decays away and the chip returns to its natural power-up state. Physics: when power is cut, VDD doesn't drop instantly — rail capacitance discharges via RC decay, V(t)=V₀·e^(−t/RC). Cells hold their written value while VDD stays above the retention floor (~2V); below it, they lose state and come up in their natural fingerprint on next power-up.

## Method — two-phase Arduino sketch (`decay_vs_natural.ino`)

- Phase 1 (gold reference): power off 3s (fully decayed cold start), power on, read, NO write. Rows marked `# ref N`. `REF_OFF_DELAY_US=3000000` stays FIXED.
- Phase 2 (test): write all 1s (0xFFFF) across 1000 words, power off for `OFF_DELAY_US` (the swept variable), power on, read. Rows marked `# cycle N`.
- Run as single-cycle: `REF_CYCLES=1`, `NUM_CYCLES=1`.
- Built on the original read-only sketch — reused `sramPinsLow`, `sramDefaultBus`, `sramRead`, `setAddress`, pin map verbatim; added `sramWrite`. `sramPinsLow()` before power-off is critical (prevents sneak-path back-powering through I/O protection diodes).

## Metric — `decay_hamming_calculator.py`

Majority-votes `# ref` rows into a reference, computes Hamming distance (popcount(a^b)) between each `# cycle` read and that reference, reports as dist_% = differing bits / 16000 total. High dist% = written 1s still surviving; low dist% (floor) = decayed to natural. Usage: `python decay_hamming_calculator.py logfile.txt` (no extra args).

NOTE: an older `decay_from_gold.py`-style version printing `flip_%` measures distance-from-all-1s instead — wrong metric; the correct one prints `dist_%` and "majority vote of N cold-start reads".

## Serial capture — `capture.py` (pyserial)

Reads the COM port directly and logs to a text file, auto-stops at `=== DONE ===`. Usage: `python capture.py COM4 filename.txt`. Close the Arduino IDE Serial Monitor first (COM port is single-access). Mega auto-resets on port open so the run starts on its own. Log filenames: `<delay>ms_ref3000ms_comparison.txt`.

## Results (7-point sweep, single cycle, 16000 bits each)

| Off-delay (ms) | Hamming dist (bits) | dist_% |
|---|---|---|
| 500 | 9044 | 56.5 |
| 750 | 4688 | 29.3 |
| 1000 | 1530 | 9.6 |
| 1250 | 858 | 5.4 |
| 1500 | 635 | 4.0 |
| 1750 | 583 | 3.6 |
| 3000 | 453 | 2.8 |

**Interpretation:** Steep drop 500→1000ms (56%→10%), knee ~1000–1250ms, flattens to a ~2–3% floor. **Time to natural state ≈ 1–1.25s.** This corroborates the earlier unstable-bit sweep (knee 750–1500ms). The floor isn't 0% because genuinely unstable cells vote randomly each power-up (~9.5% baseline; single-read comparison lands a bit under that). Floor bounces 2–4% because single-cycle reads are noisy.

## Plotting

`plot_decay.py` (matplotlib) — plots dist_% vs off-delay, data hardcoded in two lists at top, y-axis ticked every 5% (0–60), saves `decay_plot.png`. Also `sweep_to_csv.py` batches all logs into a sorted `sweep_results.csv`.

## Mistakes caught / lessons

- Accidentally varied `REF_OFF_DELAY_US` — must stay fixed at 3s or the comparison baseline changes.
- The filename delay number is just a label; it does NOT set the Arduino delay — sketch value and filename must be kept in sync manually.
- Sub-500ms delays add nothing for the timing question (only extend the top of the curve where 1s survive).
- Capture can grab leftover buffered bytes from the prior run (junk lines above the header) — harmless (parser skips pre-`#ref` lines), but `ser.reset_input_buffer()` after opening the port prevents it.
