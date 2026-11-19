#!/bin/bash
# Regenerates the screenshots used in the tutorial (docs/figs) from real runs:
#   - the build output of `make`
#   - a scripted game in the PC simulator, and every LCD frame it drew
# Run from the repository root:  bash tools/make_docs_images.sh
set -e
FIGS=docs/figs
OUT=out
mkdir -p $FIGS $OUT
rm -rf $OUT/lcd && mkdir -p $OUT/lcd

# 1. cross-compilation
make clean > /dev/null
{ echo '$ make'; make 2>&1 | sed -E 's#-I[^ ]+ ?##g; s/ +/ /g'; echo '$ file reflexboard';
  file reflexboard | sed 's/, BuildID[^,]*//'; } > $OUT/build.txt
python3 tools/term2svg.py $OUT/build.txt $FIGS/term-build.svg "make (cross-compile for the board)"

# 2. one scripted game in the simulator:
#    start, 235 ms, false start, 198 ms, too slow, 262 ms
make reflexboard-sim > /dev/null
{ echo '$ make sim'; REFLEX_SEED=7 REFLEX_LCD_DUMP=$OUT/lcd REFLEX_SIM_AUTO="S,235,S,198,T,262" ./reflexboard-sim; } \
    > $OUT/sim.txt 2>&1
python3 tools/term2svg.py $OUT/sim.txt $FIGS/term-sim.svg "make sim (PC simulator)"

# 3. LCD frames -> PNG (frame numbers follow the order the game draws them)
lcd() { python3 tools/lcd2png.py $OUT/lcd/lcd-$1.pbm $FIGS/lcd-$2.png 3; }
lcd 001 title
lcd 002 ready
lcd 003 result
lcd 005 false-start
lcd 009 too-slow
lcd 007 world-class
lcd 012 summary

echo "images written to $FIGS"
