#!/bin/bash

KERNEL_DIR=$(pwd) # Set the kernel directory to the current working directory
ANYKERNEL3_DIR=$KERNEL_DIR/anykernel # Define the AnyKernel3 directory
IMAGE=$KERNEL_DIR/arch/arm64/boot/Image.gz-dtb # Define the kernel image path

function zipping() {
    # Delete old zip files that match the naming pattern
    rm -f $KERNEL_DIR/Perf-kernel-*.zip

    # Copying kernel essentials
    cp $IMAGE $ANYKERNEL3_DIR

    # Change directory to AnyKernel3 directory
    cd $ANYKERNEL3_DIR

    # Define the final zip filename
    FINAL_ZIP=Perf-kernel-$(date +%y%m%d-%H%M).zip

    # Zipping the contents of the AnyKernel3 directory
    zip -r9 $FINAL_ZIP ./*

    # Move the zip file back to the kernel directory or any desired location
    mv $FINAL_ZIP $KERNEL_DIR/

    # Clean AnyKernel3 directory if needed (optional)
    cd ..; rm -rf $ANYKERNEL3_DIR/Image.gz-dtb*

    # Return to the original kernel directory
    cd $KERNEL_DIR
}

# Execute the zipping function
zipping
echo done!

