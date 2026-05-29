# PHYS480/481L CAMAC DAQ

This repository contains simple CAMAC DAQ code for the PHYS480/481L laboratory experiments, including the muon lifetime setup and related radiation-detection experiments.

The code is intended to work with a Wiener/CC-USB CAMAC controller using the `libxxusb` interface.

## Repository structure

```text
.
├── Makefile              # default Makefile
├── include
│   └── libxxusb.h
├── src
│   └── camac.cpp
└── wiener
    ├── README.txt
    ├── examples
    │   ├── CC-USB_demo.cpp
    │   └── Makefile
    ├── include
    │   └── libxxusb.h
    └── src
        ├── FNAL.TXT
        ├── Makefile
        ├── README.txt
        └── libxxusb.c
```

## Purpose

The DAQ program is being developed for CAMAC-based readout in PHYS480/481L. The initial target is the muon lifetime experiment, where the TAC output can be read with a CAMAC peak-sensing ADC. The same DAQ infrastructure may also be used for related experiments such as nuclear spectroscopy, gamma-gamma correlations, and timing measurements.

## Building 

```bash
make clean
make 
```

## Running

After building, the executable should be:

```bash
./camac
```

### GitHub push asks for password

GitHub no longer accepts normal account passwords for Git operations over HTTPS. Use SSH instead:

```bash
git remote set-url origin git@github.com:sykeisuke/phys480-481L.git
git push -u origin main
```

## Notes for PHYS480/481L

The CAMAC DAQ will be used as part of a broader radiation detection and fast-electronics training platform. The related experiments include:

- Speed of Light
- Nuclear Spectroscopy
- Gamma-Gamma Correlations
- Muon Decay

The muon decay setup is expected to use:

```text
Plastic scintillators / PMTs
→ NIM discriminators
→ coincidence / anticoincidence logic
→ TAC
→ CAMAC peak ADC or MCA
→ ROOT/Python analysis
```

For student use, the MCA path may be kept as a stable teaching-lab readout, while the CAMAC path can be used for event-by-event DAQ training.

