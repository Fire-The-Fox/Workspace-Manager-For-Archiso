/*
 * Workspace Manager for Archiso - Create and manage your archiso workspace folders with ease
 * Copyright (C) 2022 Ján Gajdoš
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include "FoxString.h"
#include "toml.h"

FoxString *projectInfo();
void build();
void clean();

int main(int argc, char **args) {

    FoxString pathStart = FoxString_New("/");
    FoxString WorkspaceToml = FoxString_New(NULL);
    FoxString *WToml = &WorkspaceToml;

    FILE *shellOut;
    char path[256];

    DIR* bedrockDir = opendir("/bedrock/strata");

    if (bedrockDir) {

        printf("Bedrock linux detected! Searching for arch stratas, which contain archiso installed.\n");

        shellOut = popen("brl which mkarchiso 2>&1", "r");
        FoxString_Connect(&pathStart, FoxString_New("bedrock/strata/"));

        while (fgets(path, 256, shellOut) != NULL) {
            if (FoxString_Contains(FoxString_New(path), FoxString_New("ERROR")) == EQUAL) {
                printf("No arch strata with archiso detected! Stopping execution.\n");
                exit(1);
            }
            FoxString_Connect(&pathStart, FoxString_New(path));
        }

        pathStart = FoxString_Replace(pathStart, '\n', '\0', 1);

        FoxString_Connect(&pathStart, FoxString_New("/usr/share/archiso/configs/"));

        closedir(bedrockDir);

    } else if (ENOENT == errno) {
        FoxString_Connect(&pathStart,
                          FoxString_New("usr/share/archiso/configs/"));
    }

    printf("Path found: %s\n", pathStart.data);

    for (int i = 1; i < argc; i++) {
        if (!strcmp(args[i], "--create")) {
            FoxString *newProjectData = projectInfo();

            FoxString temp = FoxString_New("Name = \"");

            FoxString_Connect(WToml, FoxString_New("[iso-info]\n"));

            FoxString_Connect(&temp, newProjectData[1]);
            FoxString_Connect(&temp, FoxString_New("\"\n"));

            FoxString_Connect(WToml, temp);

            temp = FoxString_ReCreate(&temp, "Description = \"");

            FoxString_Connect(&temp, newProjectData[2]);
            FoxString_Connect(&temp, FoxString_New("\"\n"));
            FoxString_Connect(WToml, temp);

            temp = FoxString_ReCreate(&temp, "Publisher = \"");

            FoxString_Connect(&temp, newProjectData[3]);
            FoxString_Connect(&temp, FoxString_New("\"\n"));
            FoxString_Connect(WToml, temp);

            temp = FoxString_ReCreate(&temp, "Version = \"");

            FoxString_Connect(&temp, newProjectData[4]);
            FoxString_Connect(&temp, FoxString_New("\"\n"));
            FoxString_Connect(WToml, temp);

            FoxString_Connect(WToml, FoxString_New("\n[workspace-settings]\n"));

            temp = FoxString_ReCreate(&temp, "Compression = \"");

            FoxString_Connect(&temp, newProjectData[5]);
            FoxString_Connect(&temp, FoxString_New("\"\n"));
            FoxString_Connect(WToml, temp);

            temp = FoxString_ReCreate(&temp, "User = \"");

            char *username = malloc(32);

            getlogin_r(username, 32);

            if (username) {
                FoxString_Connect(&temp, FoxString_New(username));
                FoxString_Connect(&temp, FoxString_New("\""));
                FoxString_Connect(WToml, temp);
                free(username);
            }

            FoxString here = FoxString_New(getenv("PWD"));

            FoxString_Connect(&here, FoxString_New("/"));

            FoxString_Connect(&here, newProjectData[1]);

            mkdir(here.data, 0751);

            chdir(here.data);

            FoxString_Clean(&here);

            FoxString cmd = FoxString_New("cp -r ");

            FoxString_Connect(&cmd, pathStart);
            FoxString_Connect(&cmd, newProjectData[0]);

            FoxString_Connect(&cmd, FoxString_New("/* ./"));

            system(cmd.data);

            FILE *config;

            config = fopen("workspace-config.toml", "w");

            if (config != NULL) {
                fwrite(WToml->data, 1, WToml->size, config);
                fclose(config);

                system("sed -i '/iso_name=.*/c\\source ./build-source.sh' ./profiledef.sh");
                system("sed -i '/iso_label/d' ./profiledef.sh");
                system("sed -i '/iso_publisher/d' ./profiledef.sh");
                system("sed -i '/iso_application/d' ./profiledef.sh");
                system("sed -i '/iso_version/d' ./profiledef.sh");

                system("sed -i '/airootfs_image_type/d' ./profiledef.sh");
                system("sed -i '/airootfs_image_tool_options/d' ./profiledef.sh");

            } else {
                printf("ERROR: Can't create config file");
            }

            FoxString_Connect(&here, FoxString_New("/iso/"));

            mkdir(here.data, 0751);

            for (int j = 0; j < 6; j++) {
                FoxString_Clean(&newProjectData[j]);
            }

        } else if (!strcmp(args[i], "--project-dir")) {
            chdir(args[i + 1]);
        } else if (!strcmp(args[i], "--build")) {

            build();

        } else if (!strcmp(args[i], "--clean")) {

            clean();

        }
    }

    return 0;
}

FoxString *projectInfo() {
    printf("\nStarting project creation.\n");

    FoxString *data = malloc(sizeof(FoxString) * 6);

    printf("Please select profile of your iso: \nreleng - used for monthly iso release\n"
           "baseline - iso where are only required components to boot\nPlease select: ");

    do {
        data[0] = FoxStringInput();
        for (int i = 0; i < data[0].size; i++) {
            data[0].data[i] = (char) tolower(data[0].data[i]);
        }
        if (FoxString_Contains(data[0], FoxString_New("releng")) != EQUAL &&
            FoxString_Contains(data[0], FoxString_New("baseline")) != EQUAL) {
            printf("Profile '%s' was not found!\nPlease select: ", data[0].data);
        }
    } while(FoxString_Contains(data[0], FoxString_New("releng")) != EQUAL &&
            FoxString_Contains(data[0], FoxString_New("baseline")) != EQUAL);

    printf("\nPlease type name of your iso: ");

    data[1] = FoxStringInput();

    printf("Please type description of your iso: ");

    data[2] = FoxStringInput();

    printf("Please type publisher name of your iso: ");

    data[3] = FoxStringInput();

    printf("Please type version of your project: ");

    data[4] = FoxStringInput();

    printf("\nPlease select iso compression type:\nerofs - takes short time, iso image has larger size\n"
           "squashfs - takes long time, iso image has small size\nPlease select: ");

    data[5] = FoxStringInput();

    for (int i = 0; i < data[5].size; i++) {
        data[5].data[i] = (char) tolower(data[5].data[i]);
    }

    return data;
}

void build() {

    printf("Executing build\n");

    if (geteuid() != 0) {
        fprintf(stderr, "Please run this app as root: %s\n", strerror(EACCES));
        exit(1);
    }

    FILE *config;
    char errBuf[200];

    config = fopen("workspace-config.toml", "r");

    if (!config) {
        fprintf(stderr, "Can't open workspace-config.toml: %s", strerror(errno));
        exit(1);
    }

    toml_table_t const *conf = toml_parse_file(config, errBuf, sizeof(errBuf));

    if (!conf) {
        fprintf(stderr, "Can't parse workspace-config.toml: %s\n", errBuf);
        exit(1);
    }

    toml_table_t const *isoInfo = toml_table_in(conf, "iso-info");

    if (!isoInfo) {
        fprintf(stderr, "[iso-info] wasn't found in config file\n");
        exit(1);
    }

    toml_table_t const *workspaceSettings = toml_table_in(conf, "workspace-settings");

    if (!workspaceSettings) {
        fprintf(stderr, "[workspace-settings] wasn't found in config file\n");
        exit(1);
    }

    toml_datum_t isoInfoArr[] = {toml_string_in(isoInfo, "Name"),
                                 toml_string_in(isoInfo, "Description"),
                                 toml_string_in(isoInfo, "Publisher"),
                                 toml_string_in(isoInfo, "Version")};

    for (int j = 0; j < 4; j++) {
        if(!isoInfoArr[j].u.s) {
            fprintf(stderr, "Name, Description, Publisher of Version is missing in your config file\n");
            exit(1);
        }
    }

    toml_datum_t compressionType = toml_string_in(workspaceSettings, "Compression");
    toml_datum_t defaultOwner = toml_string_in(workspaceSettings, "User");

    if (!compressionType.u.s) {
        fprintf(stderr, "Compression is missing in your config file\n");
        exit(1);
    }

    if (!defaultOwner.u.s) {
        fprintf(stderr, "User is missing in your config file\n");
        exit(1);
    }

    FoxString data = FoxString_New("iso_name");
    FoxString_Connect(&data, FoxString_New("=\""));
    FoxString_Connect(&data, FoxString_New(isoInfoArr[0].u.s));
    FoxString_Connect(&data, FoxString_New("\"\n"));

    FoxString_Connect(&data, FoxString_New("iso_label=\""));
    FoxString_Connect(&data, FoxString_New(isoInfoArr[0].u.s));
    FoxString_Connect(&data, FoxString_New("\"\n"));

    FoxString_Connect(&data, FoxString_New("iso_publisher=\""));
    FoxString_Connect(&data, FoxString_New(isoInfoArr[2].u.s));
    FoxString_Connect(&data, FoxString_New("\"\n"));

    FoxString_Connect(&data, FoxString_New("iso_application=\""));
    FoxString_Connect(&data, FoxString_New(isoInfoArr[1].u.s));
    FoxString_Connect(&data, FoxString_New("\"\n"));

    FoxString_Connect(&data, FoxString_New("iso_version=\""));
    FoxString_Connect(&data, FoxString_New(isoInfoArr[3].u.s));
    FoxString_Connect(&data, FoxString_New("\"\n"));

    if (!strcmp(compressionType.u.s, "erofs")) {
        FoxString_Connect(&data, FoxString_New("airootfs_image_type=\"erofs\"\n"));
        FoxString_Connect(&data,
                          FoxString_New("airootfs_image_tool_options=('-zlz4hc,12')\n")
                          );
    } else if (!strcmp(compressionType.u.s, "squashfs")) {
        FoxString_Connect(&data, FoxString_New("airootfs_image_type=\"squashfs\"\n"));
        FoxString_Connect(&data,
                          FoxString_New("airootfs_image_tool_options=('-comp' 'xz' '-Xbcj' 'x86' '-b' '1M' '-Xdict-size' '1M')")
                          );
    }

    FILE *source;

    source = fopen("build-source.sh", "w");

    if(source != NULL) {
        fwrite(data.data, 1, data.size, source);
        fclose(source);
    }

    FoxString cmd = FoxString_New("chown ");
    FoxString_Connect(&cmd, FoxString_New(defaultOwner.u.s));
    FoxString cmd1 = FoxString_New(cmd.data);
    FoxString cmd2 = FoxString_New(cmd.data);
    FoxString_Connect(&cmd1, FoxString_New(" *.iso"));
    FoxString_Connect(&cmd2, FoxString_New(" build-source.sh"));

    system("mkarchiso -v -w ./ -o ./iso ./");
    system("cp -f iso/*.iso ./");
    system(cmd1.data);
    system(cmd2.data);

    fclose(config);
}

void clean() {
    printf("Executing clean\n");

    if (geteuid() != 0) {
        fprintf(stderr, "Please run this app as root: %s\n", strerror(EACCES));
        exit(1);
    }

    system("rm -rf x86_64/");
    system("rm -rf iso*");
    system("rm -rf base.*");
    system("rm -rf build.*");
    system("rm -rf build_date");
    system("rm -rf efiboot.img");

    printf("Cleaning is done!\n");
}