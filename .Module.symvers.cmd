cmd_/home/lww/project/12_FPGA/pzk_driver/src/Module.symvers := sed 's/\.ko$$/\.o/' /home/lww/project/12_FPGA/pzk_driver/src/modules.order | scripts/mod/modpost -m -a  -o /home/lww/project/12_FPGA/pzk_driver/src/Module.symvers -e -i Module.symvers   -T -
