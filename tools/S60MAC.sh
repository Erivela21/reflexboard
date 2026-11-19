#!/bin/sh
### BEGIN INIT INFO
# Provides:          S60MAC
# Required-Start:    $local_fs
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Bring up eth0 on the DE10-Standard
### END INIT INFO
#
# Brings up eth0 at boot (the Terasic image has no valid MAC address).
# Copy to /etc/init.d/S60MAC.sh on the target, then:
#   chmod +x /etc/init.d/S60MAC.sh
#   systemctl enable S60MAC.sh
#
# MODE=static -> scenario 2 (board connected straight to the PC through the NIC)
# MODE=dhcp   -> scenario 1 (board connected to a router)
MODE=static
IP=192.168.10.13
NETMASK=255.255.255.0
# MAC given by the professor (leave empty to keep the board's own MAC)
MAC=02:01:02:03:04:08

start() {
        printf "Setting up eth0 ($MODE): "
        ifconfig eth0 down
        [ -n "$MAC" ] && ifconfig eth0 hw ether "$MAC"
        ifconfig eth0 up
        if [ "$MODE" = "dhcp" ]; then
                udhcpc -i eth0 -n -q -t 10
        else
                ifconfig eth0 "$IP" netmask "$NETMASK" up
        fi
        [ $? = 0 ] && echo "OK" || echo "FAIL"
}

stop() {
        ifconfig eth0 down
}

case "$1" in
start)
        start
        ;;
stop)
        stop
        ;;
restart|reload)
        stop
        start
        ;;
*)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac
exit 0
