#!/usr/bin/env bash
#
# Shell script for unit tests of SEM simulator
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

SCRIPT_DIR=$(realpath $(dirname $0))
BUILD_DIR=${BUILD_DIR:="$SCRIPT_DIR"/build/test}

mkdir -p "$BUILD_DIR"/results

echo "Running test_cfgparser"
"$BUILD_DIR"/test_cfgparser "$SCRIPT_DIR"/test/input/test_config.toml "$SCRIPT_DIR"/test/input/test_update.csv > "$BUILD_DIR"/results/test_cfgparser.txt 2>&1
status=$?
if [[ $status != 0 ]]
then
  echo "ERROR!: test_cfgparser exitted with $status"
fi

echo "Running test_calculator"
"$BUILD_DIR"/test_calculator > "$BUILD_DIR"/results/test_calculator.txt 2>&1
status=$?
if [[ $status != 0 ]]
then
  echo "ERROR!: test_calculator exitted with $status"
fi

echo "Running test_metersim"
"$BUILD_DIR"/test_metersim "$SCRIPT_DIR"/test/input/sc00 "$SCRIPT_DIR"/test/input/sc02 > "$BUILD_DIR"/results/test_metersim.txt 2>&1
status=$?
if [[ $status != 0 ]]
then
  echo "ERROR!: test_metersim exitted with $status"
fi

echo "Running test_devices"
"$BUILD_DIR"/test_devices "$SCRIPT_DIR"/test/input/sc00 > "$BUILD_DIR"/results/test_devices.txt 2>&1
status=$?
if [[ $status != 0 ]]
then
  echo "ERROR!: test_devices exitted with $status"
fi

echo "Running test_mm_api"
"$BUILD_DIR"/test_mm_api "$SCRIPT_DIR"/test/input/sc00 "$SCRIPT_DIR"/test/input/sc01 > "$BUILD_DIR"/results/test_mm_api.txt 2>&1
status=$?
if [[ $status != 0 ]]
then
  echo "ERROR!: test_mm_api exitted with $status"
fi


echo "-----------------------"
echo "IGNORED:"
echo ""

grep -s IGNORE "$BUILD_DIR"/results/*.txt
echo ""

echo "-----------------------"
echo "FAILED:"
echo ""

grep -s FAIL "$BUILD_DIR"/results/*.txt
echo ""

echo "-----------------------"
echo "PASSED:"
echo ""

grep -s PASS "$BUILD_DIR"/results/*.txt
echo ""
