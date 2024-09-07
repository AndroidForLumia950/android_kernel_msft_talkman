#!/bin/bash

KERNEL_DIR=$(pwd) # Set the kernel directory to the current working directory
COMPILER_DIR=$KERNEL_DIR/aarch64-linux-android-4.9/bin/aarch64-linux-android- #specify compiler directory

# Ask the user if they want to recompile the defconfig
read -p "Do you want to recompile the defconfig? (Y/N) " answer

# Convert the answer to lowercase
answer=$(echo "$answer" | tr '[:upper:]' '[:lower:]')

# Check the user's response
if [ "$answer" == "y" ]; then
    echo "Recompiling the defconfig..."
    make ARCH=arm64 lineageos_bullhead_defconfig
else
    echo "Skipping defconfig recompilation..."
fi

echo "Compiling the kernel..."
make ARCH=arm64 CROSS_COMPILE=$COMPILER_DIR -j$(nproc) #Compile kernel
echo "Done!"
echo "Time to zip up!"
./anykernel.sh
