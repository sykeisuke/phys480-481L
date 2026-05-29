// started 2024-12-13 by mza and Keisuke-san
// based on GPL2 code Copyright (C) 2005-2014 WIENER, Plein & Baus, Corp
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, version 2.
// updated 2024-12 by mza, Cody and Keisuke-san
// updated 2025-01 by mza, Cody and Keisuke-san
// last updated 2025-01-09 by mza and Cody

// notes on camac:
// CamN is the slot number on the camac crate
// CamA is the address of the register you're reading or writing (which frequently corresponds to the channel number for simeple modules)
// CamF is the function you are requesting (read, write, set, etc)
// CamD is the data returned from simple camac modules
// CamQ is whether the data returned is new/stale
// CamX is ???

#include <libxxusb.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <unistd.h>   // usleep
using namespace std;

static usb_dev_handle *udev = nullptr;   // CAMAC CC-USB Handle
static FILE *f = nullptr;

static int  TDC_slot = 7;    // crate slot of Philips 7186 (TDC)
static int  QDC_slot = 12;   // crate slot of Philips 7166 (QDC)
static int  scaler_slot = 6; // crate slot of Lecroy 2551 (Scaler)
static int  scaler_gate_a = 2; // gate signal input channel
static char datestamp[32];

// ---- util ----
static void update_datestamp() {
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    strftime(datestamp, sizeof(datestamp), "%Y-%m-%d.%H%M%S", timenow);
}

// ---- setup / teardown ----
static int setup_camac_crate_controller() {
    xxusb_device_type devices[100];
    xxusb_devices_find(devices);
    struct usb_device *dev = devices[0].usbdev;
    udev = xxusb_device_open(dev);
    if (!udev) {
        printf("\n\nFailed to open CC_USB\n\n");
        return 0;
    }
    return 1;
}

// ---- module clears ----
static void clear_7186_TDC() {
    long D=0; int Q=0, X=0;
    CAMAC_read(udev, TDC_slot, 3, 11, &D, &Q, &X);
    //CAMAC_read(udev, TDC_slot, 0, 9, &D, &Q, &X);
}

static inline void clear_2228A_TDC() {
    long D=0; int Q=0, X=0;
    CAMAC_read(udev, TDC_slot, /*A=*/0, /*F=*/9, &D, &Q, &X);   // F(9)
}

static void clear_7166_QDC() {
    long D=0; int Q=0, X=0;
    CAMAC_read(udev, QDC_slot, 3, 11, &D, &Q, &X);
}

static inline void reset_scaler_2551() {
    long D=0; int Q=0, X=0;
    CAMAC_read(udev, scaler_slot, /*A=*/0, /*F=*/9, &D, &Q, &X);
}

// ---- module reads ----
static int read_tdc7186_ch1(int ch1, int retries=5) {
    const int A = ch1 - 1;             // 1→0
    const int F = 0;                   // read (use 2 for read&clear if desired)
    for (int i=0; i<retries; ++i) {
        long D=0; int Q=0, X=0;
        int ret = CAMAC_read(udev, TDC_slot, A, F, &D, &Q, &X);
        if (ret >= 0 && Q) {
            unsigned short raw = (unsigned short)(D & 0xFFFF);
            int time12 = (int)(raw & 0x0FFF);     // lower 12 bits are data
            // int chid = (raw >> 12) & 0xF;      // debug if needed
            return time12;
            //return chid;
        }
        usleep(100);
    }
    return -2;
}

static int read_tdc2228a(int ch1, int timeout_us=300, int poll_us=10) {
    const int A = ch1 - 1;                  // front-panel ch1 → A=0
    const int F = 0;                        // per-channel read
    long D=0; int Q=0, X=0;
    int waited=0;
    while (waited <= timeout_us) {
        int ret = CAMAC_read(udev, TDC_slot, A, F, &D, &Q, &X);
        if (ret >= 0 && Q) {
            return (int)((unsigned)D & 0x0FFF);
        }
        usleep(poll_us);
        waited += poll_us;
    }
    return -2;
}

static int read_qdc7166_ch1(int ch1, int retries=5) {
    const int A = ch1 - 1;             // 1→0
    const int F = 0;                   // read (use 2 for read&clear if desired)
    for (int i=0; i<retries; ++i) {
        long D=0; int Q=0, X=0;
        int ret = CAMAC_read(udev, QDC_slot, A, F, &D, &Q, &X);
        if (ret >= 0 && Q) {
            unsigned short raw = (unsigned short)(D & 0xFFFF);
            return (int)(raw & 0x0FFF);
        }
        usleep(100);
    }
    return -2;
}

int read_scaler_raw(int slot, int a, int retries=5) {
    const int F = 0;
    for (int i=0; i<retries; ++i) {
        long D=0; int Q=0, X=0;
        int ret = CAMAC_read(udev, slot, a, F, &D, &Q, &X);
        if (ret >= 0 && Q) return (int)D;   // no masking
        usleep(100);
    }
    return -2;
}

// ---- trigger (gate signal) ----
int wait_for_next_coincidence() {
    int prev = read_scaler_raw(scaler_slot, scaler_gate_a);
    if (prev < 0) return prev;

    while (true) {
        usleep(1000); 
        int cur = read_scaler_raw(scaler_slot, scaler_gate_a);
        if (cur < 0) continue;
        if (cur != prev) return cur;
    }
}

// ---- DAQ loop (single channel) ----
static void acquire_qdc_tdc_ch1(int ch1, int N) {
    // open file
    update_datestamp();
    char filename[256];
    sprintf(filename, "%s.datafile", datestamp);
    f = fopen(filename, "w");
    if (!f) { perror("fopen"); return; }
    printf("Writing to %s\n", filename);

    for (int i=0; i<N; ++i) {
	int gatecnt = wait_for_next_coincidence();
	//usleep(50);
        int qdc = read_qdc7166_ch1(ch1);
        int tdc = read_tdc7186_ch1(ch1);
    	clear_7186_TDC();  
        clear_7166_QDC();  
        //int tdc = read_tdc2228a(ch1);

	fprintf(f, "%d,%d,%d\n", gatecnt, qdc, tdc);
        printf("[%6d] gate=%d  QDC=%5d  TDC=%5d\n", i, gatecnt, qdc, tdc);

        fflush(f);
    }

    fclose(f);
    f = nullptr;
}

int main(int argc, char *argv[]) {
    if (!setup_camac_crate_controller()) return 1;

    CAMAC_Z(udev);             // crate initialize (Z)
    CAMAC_C(udev);             // crate clear (C)
    CAMAC_I(udev, false);      // Inhibit = OFF

    const int ch1 = 1;    // physical channel-1 (A=0)
    const int N   = 100000; 
    reset_scaler_2551();
    clear_7166_QDC();  
    clear_7186_TDC();  
    //clear_2228A_TDC();
    acquire_qdc_tdc_ch1(ch1, N);

    xxusb_device_close(udev);
    return 0;
}
