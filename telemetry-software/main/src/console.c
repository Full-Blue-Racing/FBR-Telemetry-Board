#include "console.h"
#include "sd_log.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "console";

// Builds MOUNT_POINT "/" <name> into out, rejecting names that would escape the mount point.
static bool build_path(const char *name, char *out, size_t out_sz) {
    if (!name[0] || strchr(name, '/')) {
        printf("bad filename: %s\n", name);
        return false;
    }
    snprintf(out, out_sz, MOUNT_POINT "/%s", name);
    return true;
}

static int cmd_ls(int argc, char **argv) {
    DIR *d = opendir(MOUNT_POINT);
    if (!d) {
        printf("opendir(%s) failed\n", MOUNT_POINT);
        return 1;
    }
    struct dirent *e;
    char path[280];
    while ((e = readdir(d)) != NULL) {
        struct stat st;
        snprintf(path, sizeof(path), MOUNT_POINT "/%s", e->d_name);
        if (stat(path, &st) == 0) {
            printf("%8ld  %s\n", (long)st.st_size, e->d_name);
        } else {
            printf("       ?  %s\n", e->d_name);
        }
    }
    closedir(d);
    return 0;
}

static int cmd_cat(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: cat <file>\n");
        return 1;
    }
    char path[280];
    if (!build_path(argv[1], path, sizeof(path))) return 1;
    sd_dump_file(path);
    return 0;
}

static int cmd_rm(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: rm <file>\n");
        return 1;
    }
    char path[280];
    if (!build_path(argv[1], path, sizeof(path))) return 1;
    if (unlink(path) != 0) {
        printf("rm %s failed\n", path);
        return 1;
    }
    printf("removed %s\n", path);
    return 0;
}

// Streams a file's raw bytes between sentinel markers for a host-side script to capture.
static int cmd_get(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: get <file>\n");
        return 1;
    }
    char path[280];
    if (!build_path(argv[1], path, sizeof(path))) return 1;

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("get: fopen(%s) failed\n", path);
        return 1;
    }
    printf("-----BEGIN-FILE-----\n");
    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        fwrite(buf, 1, n, stdout);
    }
    fclose(f);
    printf("\n-----END-FILE-----\n");
    return 0;
}

// Toggles the per-sample GPS/IMU/logger debug output (ESP_LOGD) on or off at runtime.
static int cmd_verbose(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "on") != 0 && strcmp(argv[1], "off") != 0)) {
        printf("usage: verbose <on|off>\n");
        return 1;
    }
    esp_log_level_t level = (strcmp(argv[1], "on") == 0) ? ESP_LOG_DEBUG : ESP_LOG_INFO;
    esp_log_level_set("GPS", level);
    esp_log_level_set("IMU", level);
    esp_log_level_set("sd", level);
    printf("verbose %s\n", argv[1]);
    return 0;
}

void console_start(void) {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "flash>";
    repl_config.max_cmdline_length = 256;

    esp_console_dev_usb_serial_jtag_config_t usb_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usb_config, &repl_config, &repl));

    esp_console_register_help_command();

    const esp_console_cmd_t ls_cmd = {
        .command = "ls",
        .help = "List files on the flash log partition",
        .hint = NULL,
        .func = &cmd_ls,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&ls_cmd));

    const esp_console_cmd_t cat_cmd = {
        .command = "cat",
        .help = "Print a log file's contents",
        .hint = "<file>",
        .func = &cmd_cat,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cat_cmd));

    const esp_console_cmd_t rm_cmd = {
        .command = "rm",
        .help = "Delete a log file",
        .hint = "<file>",
        .func = &cmd_rm,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&rm_cmd));

    const esp_console_cmd_t get_cmd = {
        .command = "get",
        .help = "Stream a file for a host-side script to capture (see tools/pull_log.py)",
        .hint = "<file>",
        .func = &cmd_get,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&get_cmd));

    const esp_console_cmd_t verbose_cmd = {
        .command = "verbose",
        .help = "Toggle per-sample GPS/IMU/logger debug output",
        .hint = "<on|off>",
        .func = &cmd_verbose,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&verbose_cmd));

    ESP_LOGI(TAG, "console ready: ls / cat <file> / get <file> / rm <file> / verbose <on|off>");
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
