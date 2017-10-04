#!/bin/sh
#
#    update-motd - update the dynamic MOTD immediately
#
set -e
 
if ! touch /var/run/motd.new 2>/dev/null; then
        echo "ERROR: Permission denied, try:" 1>&2
        echo "  sudo $0" 1>&2
        exit 1
fi
 
if scripts/update-motd/motd-dynamic "$1" > motd.new; then
        if mv -f motd.new /etc/motd; then
                echo "MOTD updated successfully, new MOTD:"
                echo
                cat /etc/motd
                exit 0
        else
                echo "ERROR: could not install new MOTD" 1>&2
                exit 1
        fi
fi
echo "ERROR: could not generate new MOTD" 1>&2
exit 2
