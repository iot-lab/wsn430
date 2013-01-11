#! /bin/sh

# Generate export variables for .bashrc.
# It exports all the variable required to compile programs for senslab
#
#
# Run the `install.sh` script and copy its output to your `~/.bashrc` file
#
#         ~$ ./install.sh | less
#         ~$ ./install.sh >> ~/.bashrc
#
# And reload the configuration
#
#         ~$ source ~/.bashrc
#


cd $(dirname $0)

IOT_LAB=$(git rev-parse --show-toplevel)

ENV_VAR="
# add the following lines to your ~/.bashrc file

# IoT-LAB variables
export IOT_LAB="${IOT_LAB}"
export WSN430_DRIVERS_PATH="\${IOT_LAB}/wsn430/drivers"
export WSN430_LIB_PATH="\${IOT_LAB}/wsn430/lib"
export FREERTOS_PATH="\${IOT_LAB}/wsn430/OS/FreeRTOS"
export CONTIKI_PATH="\${IOT_LAB}/wsn430/OS/Contiki"
"

error=0
eval "$ENV_VAR"
for var in "$IOT_LAB" "$WSN430_DRIVERS_PATH" "$WSN430_LIB_PATH" "$FREERTOS_PATH" "$CONTIKI_PATH"
do
        if [ ! -d "$var" ]; then
                echo "Folder $var does not exist"
                error=1
        fi
done

if [ "$error" -eq "1" ]
then
        echo
        echo "Please check your repository and the configuration in the 'install.sh' script"
        echo "and report the issue to the users mailing list"
        exit 1
else
        echo "$ENV_VAR"
fi

cd - > /dev/null
