/*
 * BSD LICENSE
 *
 * Copyright(c) 2022 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "monitor_csv.h"

#include "common.h"
#include "monitor.h"
#include "monitor_utils.h"

#include <string.h>

void
monitor_csv_begin(FILE *fp)
{
        enum pqos_mon_event events = monitor_get_events();
        enum monitor_llc_format format = monitor_get_llc_format();

        ASSERT(fp != NULL);

        if (monitor_core_mode()) {
                fprintf(fp, "Time,Core");
#ifdef PQOS_RMID_CUSTOM
                enum pqos_interface iface;

                pqos_inter_get(&iface);
                if (iface == PQOS_INTER_MSR)
                        fprintf(fp, ",RMID");
#endif
        } else if (monitor_process_mode())
                fprintf(fp, "Time,PID,Core");
        else if (monitor_uncore_mode())
                fprintf(fp, "Time,Socket");

        if (events & PQOS_PERF_EVENT_IPC)
                fprintf(fp, ",IPC");
        if (events & PQOS_PERF_EVENT_LLC_MISS)
                fprintf(fp, ",LLC Misses");
        if (events & PQOS_PERF_EVENT_LLC_REF)
                fprintf(fp, ",LLC References");
        if (events & PQOS_MON_EVENT_L3_OCCUP) {
                if (format == LLC_FORMAT_KILOBYTES)
                        fprintf(fp, ",LLC[KB]");
                else
                        fprintf(fp, ",LLC[%%]");
        }
        if (events & PQOS_MON_EVENT_LMEM_BW)
                fprintf(fp, ",MBL[MB/s]");
        if (events & PQOS_MON_EVENT_RMEM_BW)
                fprintf(fp, ",MBR[MB/s]");
        if (events & PQOS_MON_EVENT_TMEM_BW)
                fprintf(fp, ",MBT[MB/s]");

        if (events & PQOS_PERF_EVENT_LLC_MISS_PCIE_READ)
                fprintf(fp, ",%11s", "LLC Misses Read");
        if (events & PQOS_PERF_EVENT_LLC_MISS_PCIE_WRITE)
                fprintf(fp, ",%11s", "LLC Misses Write");
        if (events & PQOS_PERF_EVENT_LLC_REF_PCIE_READ)
                fprintf(fp, ",%11s", "LLC References Read");
        if (events & PQOS_PERF_EVENT_LLC_REF_PCIE_WRITE)
                fprintf(fp, ",%11s", "LLC References Write");

        fputs("\n", fp);
}

void
monitor_csv_header(FILE *fp, const char *timestamp)
{
        UNUSED_ARG(fp);
        UNUSED_ARG(timestamp);
}

/**
 * @brief Fills in single CSV column in the monitoring table
 *
 * @param format numerical value format
 * @param val numerical value to be put into the column
 * @param data place to put formatted column into
 * @param sz_data available size for the column
 * @param is_monitored if true then \a val holds valid data
 * @param is_column_present if true then corresponding event is
 *        selected for display
 * @return Number of characters added to \a data excluding NULL
 */
static size_t
fillin_csv_column(const char *format,
                  const double val,
                  char data[],
                  const size_t sz_data,
                  const int is_monitored,
                  const int is_column_present)
{
        size_t offset = 0;

        if (is_monitored) {
                /**
                 * This is monitored and we have the data
                 */
                snprintf(data, sz_data - 1, format, val);
                offset = strlen(data);
        } else if (is_column_present) {
                /**
                 * The column exists though there's no data
                 */
                snprintf(data, sz_data - 1, ",");
                offset = strlen(data);
        }

        return offset;
}

void
monitor_csv_row(FILE *fp,
                const char *timestamp,
                const struct pqos_mon_data *mon_data)
{
        const size_t sz_data = 128;
        char data[sz_data];
        size_t offset = 0;
        char core_list[16];
        enum pqos_mon_event events = monitor_get_events();
        unsigned i;

        ASSERT(fp != NULL);
        ASSERT(timestamp != NULL);
        ASSERT(mon_data != NULL);

        memset(data, 0, sz_data);

#ifdef PQOS_RMID_CUSTOM
        enum pqos_interface iface;

        pqos_inter_get(&iface);
        if (iface == PQOS_INTER_MSR && monitor_core_mode()) {
                pqos_rmid_t rmid;
                int ret = pqos_mon_assoc_get(mon_data->cores[0], &rmid);

                offset += fillin_csv_column(
                    ",%.0f", rmid, data + offset, sz_data - offset,
                    ret == PQOS_RETVAL_OK, iface == PQOS_INTER_MSR);
        }
#endif

        struct {
                enum pqos_mon_event event;
                const char *format;
        } output[] = {
            {.event = PQOS_PERF_EVENT_IPC, .format = ",%.2f"},
            {.event = PQOS_PERF_EVENT_LLC_MISS, .format = ",%.0f"},
            {.event = PQOS_PERF_EVENT_LLC_REF, .format = ",%.0f"},
            {.event = PQOS_MON_EVENT_L3_OCCUP, .format = ",%.1f"},
            {.event = PQOS_MON_EVENT_LMEM_BW, .format = ",%.1f"},
            {.event = PQOS_MON_EVENT_RMEM_BW, .format = ",%.1f"},
            {.event = PQOS_MON_EVENT_TMEM_BW, .format = ",%.1f"},
            {.event = PQOS_PERF_EVENT_LLC_MISS_PCIE_READ, .format = ",%.0f"},
            {.event = PQOS_PERF_EVENT_LLC_MISS_PCIE_WRITE, .format = ",%.0f"},
            {.event = PQOS_PERF_EVENT_LLC_REF_PCIE_READ, .format = ",%.0f"},
            {.event = PQOS_PERF_EVENT_LLC_REF_PCIE_WRITE, .format = ",%.0f"},
        };

        for (i = 0; i < DIM(output); i++) {
                double value =
                    monitor_utils_get_value(mon_data, output[i].event);

                offset += fillin_csv_column(output[i].format, value,
                                            data + offset, sz_data - offset,
                                            mon_data->event & output[i].event,
                                            events & output[i].event);
        }

        if (monitor_core_mode() || monitor_uncore_mode())
                fprintf(fp, "%s,\"%s\"%s\n", timestamp,
                        (char *)mon_data->context, data);
        else if (monitor_process_mode()) {
                memset(core_list, 0, sizeof(core_list));

                if (monitor_utils_get_pid_cores(mon_data, core_list,
                                                sizeof(core_list)) == -1) {
                        strncpy(core_list, "err", sizeof(core_list) - 1);
                }

                fprintf(fp, "%s,\"%s\",\"%s\"%s\n", timestamp,
                        (char *)mon_data->context, core_list, data);
        }
}

void
monitor_csv_footer(FILE *fp)
{
        UNUSED_ARG(fp);
}

void
monitor_csv_end(FILE *fp)
{
        ASSERT(fp != NULL);

        if (isatty(fileno(fp)))
                fputs("\n\n", fp);
}
