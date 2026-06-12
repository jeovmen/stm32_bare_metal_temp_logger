
#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "ff.h"
#include "dht22.hpp"
#include "systick.hpp"

class Logger {
public:

    // init()
    // Mounts the FatFs volume and opens (or creates) the log file.
    // Returns true on success. Call once during system init.
    bool init()
    {
        // Mount the volume (immediate mount, not deferred)
        FRESULT res = f_mount(&fatfs, "", 1);
        if (res != FR_OK) return false;

        // Open file in append mode — creates if not present,
        // appends if the file already exists from a previous session
        res = f_open(&file, FILENAME,
                     FA_OPEN_APPEND | FA_WRITE | FA_READ);
        if (res != FR_OK) return false;

        mounted = true;

        // Write header only if the file is brand new (size == 0)
        if (f_size(&file) == 0) {
            writeHeader();
        }

        return true;
    }

    // log()
    // Appends one CSV row. Call this every time a valid sensor reading
    // is taken in SAMPLE state.
    void log(const Dht22::Reading& reading)
    {
        if (!mounted) return;

        // Format: elapsed_ms,temperature,humidity\n
        // f_printf is available when FF_USE_STRFUNC >= 1 in ffconf.h
        f_printf(&file, "%lu,%.1f,%.1f\n",
                 Tick::getTick(),
                 static_cast<double>(reading.temperatureC()),
                 static_cast<double>(reading.humidity()));

        // Flush to SD card immediately so data isn't lost on power-off
        f_sync(&file);
    }

    // deinit()
    // Closes the file and unmounts. Call before removing power or SD card.
    void deinit()
    {
        if (mounted) {
            f_close(&file);
            f_unmount("");
            mounted = false;
        }
    }

private:
    static constexpr const char* FILENAME = "templog.csv";

    FATFS fatfs{};
    FIL   file{};
    bool  mounted = false;

    void writeHeader()
    {
        f_printf(&file, "elapsed_ms,temperature_c,humidity_pct\n");
        f_sync(&file);
    }
};

#endif /* LOGGER_HPP_ */
