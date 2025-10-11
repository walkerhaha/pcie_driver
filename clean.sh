#set -x
export EMU_DRV_DIR=`pwd`    
cd ${EMU_DRV_DIR}/driver
make clean
rm -rf ${EMU_DRV_DIR}/test/build
cd ${EMU_DRV_DIR}
