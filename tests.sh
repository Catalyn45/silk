#!/bin/sh
GREEN='\033[0;32m'
NC='\033[0m'

echo ""
echo -e "${GREEN}[classes]${NC}"
./sylk ./examples/classes.slk

echo ""
echo -e "${GREEN}[fibonacci iterative]${NC}"
./sylk ./examples/fibonacci.slk <<< 10 || exit 1

echo ""
echo -e "${GREEN}[2power iterative]${NC}"
./sylk ./examples/2_power.slk <<< 10 || exit 1

echo ""
echo -e "${GREEN}[fibonacci recursive]${NC}"
./sylk ./examples/fibonacci_recursive.slk <<< 10 || exit 1

echo ""
echo -e "${GREEN}[2power recursive]${NC}"
./sylk ./examples/2_power_recursive.slk <<< 10 || exit 1

echo ""
echo -e "${GREEN}[garbage collector]${NC}"
./sylk ./examples/gc.slk || exit 1

