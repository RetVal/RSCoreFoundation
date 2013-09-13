#!/bin/sh
cd ~/Desktop
rm -rf SC_BASE
rm -rf DK_BASE
cp -a /SourceCache/RSCoreFoundation-1206/RtlCoreService/RSBase ~/Desktop/SC_BASE
cp -a ~/Desktop/RSCoreFoundation/RtlCoreService/RSBase ~/Desktop/DK_BASE
cd ~/Desktop/SC_BASE
find ./ -name "*" -exec ls -ld {} \; |grep -v ^d|sort -k9,9 > ../__com.retval.tmp.sc_base1.ll
cd ~/Desktop/DK_BASE
find ./ -name "*" -exec ls -ld {} \; |grep -v ^d|sort -k9,9 > ../__com.retval.tmp.sc_base2.ll
cd ..
cat __com.retval.tmp.sc_base1.ll |awk '{print $NF}' > __com.retval.tmp.sc_base1.ls
cat __com.retval.tmp.sc_base2.ll |awk '{print $NF}' > __com.retval.tmp.sc_base2.ls

echo "#!/bin/sh" > __com.retval.tmp.sc_base.sh
cat __com.retval.tmp.sc_base2.ls |awk '{print "echo "$0"; diff SC_BASE/"$0 " DK_BASE/"$0}' >> __com.retval.tmp.sc_base.sh
chmod 755 __com.retval.tmp.sc_base.sh
rm __com.retval.tmp.sc_base1.ll
rm __com.retval.tmp.sc_base2.ll
rm __com.retval.tmp.sc_base1.ls
rm __com.retval.tmp.sc_base2.ls

./__com.retval.tmp.sc_base.sh 1> RSCoreFoundation.diff.rlt.txt 2>&1
rm __com.retval.tmp.sc_base.sh
rm -rf SC_BASE
rm -rf DK_BASE
