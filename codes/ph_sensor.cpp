#include <iostream>
#include <fstream>
#include <cmath>
#include <wiringPiI2C.h>
#include <unistd.h>

#define ADS1115_ADDR 0x48  // Alamat default ADS1115
#define PH_SENSOR_CHANNEL 0 // Gunakan channel 0 dari ADS1115

// File untuk menyimpan data kalibrasi (pengganti EEPROM di Arduino)
#define CALIBRATION_FILE "ph_calibration.txt"

// Struktur untuk menyimpan data kalibrasi
struct PHCalibrationData {
    float neutralVoltage = 1500.0;  // Tegangan pH 7.0 (default 1500mV)
    float acidVoltage = 2032.44;    // Tegangan pH 4.0 (default 2032.44mV)
};

// Fungsi untuk membaca ADC dari ADS1115
int readADC(int adc, int channel) {
    int config = 0xC183 | (channel << 12); // Konfigurasi default single-ended
    wiringPiI2CWriteReg16(adc, 0x01, config);
    int result = wiringPiI2CReadReg16(adc, 0x00);
    return ((result << 8) & 0xFF00) | ((result >> 8) & 0x00FF); // Swap endianess
}

// Fungsi untuk membaca tegangan dalam mV dari ADS1115
float readVoltage(int adc, int channel) {
    int raw = readADC(adc, channel);
    return (raw * 4.096) / 32767.0 * 1000;  // Konversi ke mV
}

// Fungsi untuk membaca pH dari tegangan
float readPH(float voltage, PHCalibrationData &calData) {
    float slope = (7.0 - 4.0) / ((calData.neutralVoltage - 1500.0) / 3.0 - (calData.acidVoltage - 1500.0) / 3.0);
    float intercept = 7.0 - slope * (calData.neutralVoltage - 1500.0) / 3.0;
    return slope * (voltage - 1500.0) / 3.0 + intercept;
}

// Fungsi untuk membaca data kalibrasi dari file
bool loadCalibrationData(PHCalibrationData &calData) {
    std::ifstream file(CALIBRATION_FILE);
    if (file.is_open()) {
        file >> calData.neutralVoltage >> calData.acidVoltage;
        file.close();
        return true;
    }
    return false;
}

int main() {
    // Inisialisasi ADS1115
    int adc = wiringPiI2CSetup(ADS1115_ADDR);
    if (adc == -1) {
        std::cerr << "Gagal menghubungkan ke ADS1115!\n";
        return 1;
    }

    PHCalibrationData calData;
    
    // Coba load data kalibrasi, jika gagal gunakan default
    if (!loadCalibrationData(calData)) {
        std::cout << "Tidak ada data kalibrasi, menggunakan nilai default.\n";
    }

    while (true) {
        float voltage = readVoltage(adc, PH_SENSOR_CHANNEL);
        float pH = readPH(voltage, calData);

        std::cout << "Tegangan: " << voltage << " mV | pH: " << pH << std::endl;
	usleep(1000000);
    }

    return 0;
}
