gcc --version
if [ $? -ne 0 ]; then
    echo "GCC not found. Please install MinGW-w64."
    exit 1
fi

# Build all object files
for src in *.c; do
    if [ "$src" != "Makefile" ] && [ "$src" != "run.sh" ]; then
        echo "Building $src..."
        gcc -c -std=c11 -Wall -Wextra -g -O0 -DEMULATOR_BUILD -DSTM32_HAS_FSMC -I. -I../firmware/btt-custom/src/User -I../firmware/btt-custom/src/User/Hal/stm32f2_f4xx -I../firmware/btt-custom/src/User/SD_Logger -I../firmware/btt-custom/src/User/Variants -I../firmware/btt-custom/src/Libraries/cmsis/Core-CM3 -I../firmware/btt-custom/src/Libraries/cmsis/stm32f2xx "$src" -o "${src%.c}.o"
    fi
done

# Build the final executable
echo "Linking gauge_sim.exe..."
gcc -o gauge_sim.exe *.o -lSDL2 -lm

if [ -f gauge_sim.exe ]; then
    echo "Build successful! Executable created: gauge_sim.exe"
    ./gauge_sim.exe
else
    echo "Build failed!"
fi
