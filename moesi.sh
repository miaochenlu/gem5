build/MOESI_hammer/gem5.opt \
              --debug-flags=ProtocolTrace\
              --outdir=results configs/example/se.py    \
              --num-cpus=1 --mem-size=4GB \
              --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 \
              --cpu-type=DerivO3CPU \
              --num-dirs=1 --ruby  \
              --network=simple --topology=Mesh_XY --mesh-rows=1 -c ./code/a.out | grep "0x7ad40" | tee > test.log



