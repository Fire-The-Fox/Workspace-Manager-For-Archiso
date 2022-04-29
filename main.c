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

#include "FoxString.h"

/*
iso_name="NAME"
iso_label="NAME"
iso_publisher="PUBLISHER"
iso_application="DESCRIPTION"
iso_version="VERSION"
install_dir="arch"
buildmodes=('iso')
bootmodes=('bios.syslinux.mbr' 'bios.syslinux.eltorito' 'uefi-x64.systemd-boot.esp' 'uefi-x64.systemd-boot.eltorito')
arch="x86_64"
pacman_conf="pacman.conf"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'xz' '-Xbcj' 'x86' '-b' '1M' '-Xdict-size' '1M')

airootfs_image_type="erofs"
airootfs_image_tool_options=('-zlz4hc,12')
file_permissions=(
  ["/etc/shadow"]="0:0:400"
  ["/root"]="0:0:750"
  ["/root/.automated_script.sh"]="0:0:755"
  ["/usr/local/bin/choose-mirror"]="0:0:755"
  ["/usr/local/bin/Installation_guide"]="0:0:755"
  ["/usr/local/bin/livecd-sound"]="0:0:755"
)
 */

FoxString *projectInfo();

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

            printf("\nConfig: \n%s", WToml->data);

            for (int j = 0; j < 6; j++) {
                FoxString_Clean(&newProjectData[j]);
            }
        }
    }

    return 0;
}

FoxString *projectInfo() {
    printf("\nStarting project creation.\n");

    FoxString *data = malloc(sizeof(FoxString) * 6);

    printf("Please select profile of your iso: \nreleng - used for monthly iso release\n"
           "baseline - iso where are only required components to boot\nPlease select: ");

    data[0] = FoxStringInput();
    for (int i = 0; i < data[0].size; i++) {
        data[0].data[i] = (char) tolower(data[0].data[i]);
    }

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