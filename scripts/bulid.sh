#set -x
# 定义日志文件路径
LOG_FILE="script.log"

export EMU_DRV_DIR=`pwd`    
sudo apt install -y build-essential cmake
sudo apt install -y linux-source
sudo apt install -y linux-headers-$(uname -r) 
cd ${EMU_DRV_DIR}/driver
#make clean && make VF_NUM=120 DMA_RESV_MEM=1 DDR_SZ_GB=80
#make clean && make VF_NUM=8 DMA_RESV_MEM=1 DDR_SZ_GB=32
make clean && make VF_NUM=0 DMA_RESV_MEM=1 DDR_SZ_GB=32 ROM_ENABLE=0
rm -rf ${EMU_DRV_DIR}/test/build
mkdir -p ${EMU_DRV_DIR}/test/build
cd ${EMU_DRV_DIR}/test/build
cmake .. 
#输出build信息到 /test/build/script
make
#make > $LOG_FILE 2>&1
cd ${EMU_DRV_DIR}
