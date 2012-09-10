#! /bin/sh

# Generate export variables for .bashrc.
# It exports all the variable required to compile programs for senslab



#  FIT_ECO=$(git rev-parse --show-toplevel)
# --show-toplevel is not supported on git versions from senslab VMs :/
FIT_ECO=$(dirname $(git rev-parse --git-dir))

cat << EOF

# add the following lines to your ~/.bashrc file

# Senslab variables
export FIT_ECO="${FIT_ECO}"
export WSN430_DRIVERS_PATH="\${FIT_ECO}/software/drivers/wsn430"
export WSN430_LIB_PATH="\${FIT_ECO}/software/lib"
export FREERTOS_PATH="\${FIT_ECO}/software/OS/FreeRTOS"
export CONTIKI_PATH="\${FIT_ECO}/software/OS/Contiki"
EOF

