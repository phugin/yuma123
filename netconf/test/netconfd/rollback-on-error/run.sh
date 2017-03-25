#!/bin/bash -e
rm -rf tmp || true
mkdir tmp
cp startup-cfg.xml tmp
rm /tmp/ncxserver.sock || true
/usr/sbin/netconfd --module=test-rollback-on-error --startup=tmp/startup-cfg.xml --superuser=$USER 1>tmp/netconfd.stdout 2>tmp/netconfd.stderr &
NETCONFD_PID=$!
sleep 1
python session.py
kill $NETCONFD_PID
cat tmp/netconfd.stdout
sleep 1
