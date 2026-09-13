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

## Build status

**Compile-check only** (2026-09-12): No CAMAC/Wiener hardware was exercised.

### Build attempt 1: Wiener USB library (`camac/wiener/src/`)

**Command:** `cd camac/wiener/src && make clean && make`

**Result:** FAILED

**Error:** `fatal error: 'malloc.h' file not found`

The build encountered a missing header file (`malloc.h`) in `libxxusb.c:49`. This header was part of older C standards but has been removed in modern C implementations; the allocation functions are now in `<stdlib.h>`.

### Build attempt 2: Top-level camac program (`camac/`)

**Command:** `cd camac && make clean && make`

**Result:** FAILED (cascading from Build attempt 1)

**Error:** `fatal error: 'libxxusb.h' file not found`

The build could not locate the libxxusb header because Build attempt 1 did not produce the required library. The linker search path expected the library at `./../../lib/libxx_usb.so`, but that file was never generated due to the failure in Build attempt 1.

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

