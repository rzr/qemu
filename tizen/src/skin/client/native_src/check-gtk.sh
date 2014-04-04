#!/bin/bash

GTK3="gtk+-3.0"
GTK2="gtk+-2.0"

pkg-config --exists $GTK3
if [ "$?" = "0" ]
then
    echo $GTK3
else
    echo $GTK2
fi
